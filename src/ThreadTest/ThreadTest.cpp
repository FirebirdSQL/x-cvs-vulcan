#ifdef WIN32
#include <Windows.h>
#endif 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ibase.h"
#include "common.h"
#include "perf.h"

// ------------------------------------------------------------
// Change these to change the way the code operates

/***
// The number of threads to start
#define NUM_THREADS 3

// The number of rows in a table
#define NUM_ROWS    1000

// The number of times all of the rows in a table will get fetched
#define NUM_LOOPS   100

// If set to 0 each thread will access it's own table
// If set to 1 all threads will access the same table
#define SAME_TABLE   1
***/

#define NUM_THREADS 32
#define NUM_ROWS    100 //1000
#define NUM_LOOPS   10 //100
#define SAME_TABLE   0

/***
#define NUM_THREADS 20
#define NUM_ROWS    2048
#define NUM_LOOPS   64
#define SAME_TABLE   1
***/

static int	attaches = 0;
static perf before;


// ------------------------------------------------------------
// There should be no need to change these
#define USERID      "sysdba"
//#define PASSWORD    "whocares"
#define PASSWORD    "masterkey"
#define CHARSET     "NONE"

// Ugly macro wrapper for create thread routine
#ifdef WIN32
#define fbCreateThread(threadfn, arg)   \
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)threadfn, arg, 0, NULL);
#endif

// Ugly macro wrappers for lock routines
// IGNORE these for now as they are not used !!!
//#ifdef WIN32
//#define fbLock                  CRITICAL_SECTION
//#define fbInitLock(plock)       (plock ? InitializeCriticalSection(plock) : 0)
//#define fbDestroyLock(plock)	  (plock ? DeleteCriticalSection(plock) : 0)
//#define fbGetLock(plock)        (plock ? EnterCriticalSection(plock) : 0)
//#define fbReleaseLock(plock)    (plock ? LeaveCriticalSection(plock) : 0)
//#endif

// Stub out the lock calls to do nothing ...
#define fbLock                  int
#define fbInitLock(plock)       
#define fbDestroyLock(plock)	
#define fbGetLock(plock)
#define fbReleaseLock(plock)




// These arguments are passed to each Threads
typedef struct _ThreadArgs {
    ISC_STATUS_ARRAY    status;     // status array passed to FB functions
	int                 threadID;   // The number for this thread
    int                 tableID;    // The table number for this thread to access
    fbLock              *plock;     // pointer to the lock for connection serialization
    int                 doCreates;  // Should this thread create it's table?
    int                 doFetch;    // Should this thread fetch?
	int                 done;       // Indicates the thread is finished
    const char          *database;  // database to be used
} ThreadArgs, *Threadargs;


// -----------------------------
// Wrapper functions for FB calls
void fberror(
    Threadargs     args,
    const char      *msg)
{
    printf("ERROR: rc = %d during \"%s\" on table x%d thread %d.\n\n", 
        (int)*args->status, msg, args->tableID, args->threadID);
    exit(1);
} // error


int fbconnect(
    Threadargs      args,
    const char      *database,
    isc_db_handle   *pdb)
{
    char  dpb_buffer[256];
    char  *dpb = dpb_buffer;
    int   rc;

    // build dbp to connect
    *dpb++ = isc_dpb_version1;

    *dpb++ = isc_dpb_user_name;
    *dpb++ = (char)strlen(USERID);
    memcpy(dpb, USERID, strlen(USERID));
    dpb += strlen(USERID);

    *dpb++ = isc_dpb_password;
    *dpb++ = (char)strlen(PASSWORD);
    memcpy(dpb, PASSWORD, strlen(PASSWORD));
    dpb += strlen(PASSWORD);

    *dpb++ = isc_dpb_lc_ctype;
    *dpb++ = (char)strlen(CHARSET);
    memcpy(dpb, CHARSET, strlen(CHARSET));
    dpb += strlen(CHARSET);
    
    // fbGetLock(args->plock);

    // Should not need semaphore protection
    rc = isc_attach_database(args->status,
                             (short)strlen(database), 
                             (char*)database,          // nice lack of const ...
                             pdb, 
                             (short)(dpb-dpb_buffer), 
                             dpb_buffer);

	if (++attaches == 1)
		perf_get_info(pdb, &before);
		
    // fbReleaseLock(args->plock);

    return rc;
} // connect


int fbdisconnect(
    Threadargs     args,
    isc_db_handle   *pdb)
{
    int rc;

    // fbGetLock(args->plock);

	if (--attaches == 0)
		{
		perf after;
		perf_get_info(pdb, &after);
		char buffer [256];
		short bufferLength = sizeof (buffer);
		perf_report(&before, &after, buffer, NULL);
		printf ("%s\n", buffer);
		}

    // Should not need semaphore protection
    rc = isc_detach_database(args->status, pdb);

    // fbReleaseLock(args->plock);

    return rc;
} // disconnect


int fbbeginTran(
    Threadargs     args,
    isc_db_handle   *pdb,
    isc_tr_handle   *ptrans)
{
    char    tpb[8];
    size_t  tpbL = 0;
    int     rc;

    tpb[tpbL++] = isc_tpb_version3;
    tpb[tpbL++] = isc_tpb_write;
    tpb[tpbL++] = isc_tpb_read_committed;
    tpb[tpbL++] = isc_tpb_rec_version;
    tpb[tpbL++] = isc_tpb_wait;

    fbGetLock(args->plock);

    // Lock db connection for multithreaded access
    rc = isc_start_transaction(args->status,
                                 ptrans,
                                 1, pdb, tpbL, tpb);

    fbReleaseLock(args->plock);

    return rc;
} // beginTran


int fballocStmt(
    Threadargs     args,
    isc_db_handle   *pdb,
    isc_stmt_handle *pstmt)
{
    int rc;

    fbGetLock(args->plock);

    // Lock db connection for multithreaded access
    rc = isc_dsql_allocate_statement(
                    args->status, pdb, pstmt);

    fbReleaseLock(args->plock);

    return rc;
} // allocStmt


int fbfreeStmt(
    Threadargs     args,
    isc_stmt_handle *pstmt,
    unsigned short  option)
{
    int rc;

    // fbGetLock(args->plock);

    // Lock db connection for multithreaded access
    rc = isc_dsql_free_statement(
                    args->status, pstmt, option);

    fbReleaseLock(args->plock);

    return rc;
} // freeStmt


int fbprepexec(
    Threadargs     args,
    isc_tr_handle   *ptrans,
    isc_stmt_handle *pstmt,
    const char      *sql)
{
    int rc;

    fbGetLock(args->plock);

    // Lock db connection for multithreaded access
    rc = isc_dsql_prepare(args->status, ptrans, pstmt,
                          (unsigned short)strlen(sql), 
                          (char*)sql,          // nice lack of const ...
                          SQL_DIALECT_V6, NULL);

    if (!rc)
        rc = isc_dsql_execute(args->status, ptrans, pstmt, 
                              SQL_DIALECT_V6, // wouldn't this be implied by the prepare ?!?!
                              NULL);

    fbReleaseLock(args->plock);

    return rc;
} // prepexec


int fbexeci(
    Threadargs     args,
    isc_db_handle   *pdb,
    isc_tr_handle   *ptrans,
    const char      *sql)
{
    int rc;

    fbGetLock(args->plock);

    // Lock db connection for multithreaded access
    rc = isc_dsql_execute_immediate(args->status, 
                                      pdb,
                                      ptrans,
                                      (unsigned short)strlen(sql), 
                                      (char*)sql,          // nice lack of const ...
                                      SQL_DIALECT_V6,
                                      NULL);

    fbReleaseLock(args->plock);

    return rc;
} // execi


int fbcommit(
    Threadargs     args,
    isc_tr_handle   *ptrans)
{
    int rc;

    fbGetLock(args->plock);

    // Lock db connection for multithreaded access (?!?)
    rc = isc_commit_transaction(args->status, ptrans);

    fbReleaseLock(args->plock);

    return rc;
} // commit


int fbrollback(
    Threadargs     args,
    isc_tr_handle   *ptrans)
{
    int rc;

    fbGetLock(args->plock);

    // Lock db connection for multithreaded access (?!?)
    rc = isc_rollback_transaction(args->status, ptrans);

    fbReleaseLock(args->plock);

    return rc;
} // rollback


// Free memory associated with an sqlda struct
void fbfreeSqlda(
    Threadargs     args,
    XSQLDA          **psqlda)
{
    int     i;
    XSQLVAR *var;

    if (!psqlda || !*psqlda)
        return;

    // Free the sqlvar memory
    for (var = (*psqlda)->sqlvar, i = 0; 
        i < (*psqlda)->sqln; 
        var++, i++)
    {
        if (var->sqldata)
            free(var->sqldata);
        if (var->sqlind)
            free(var->sqlind);
    }

    free(*psqlda);
    *psqlda = NULL;

} // freeSqlda


// Allocate an sqlda struct for the associated prepared statement
int fballocSqlda(
    Threadargs     args,
    isc_stmt_handle *pstmt,
    XSQLDA          **psqlda)
{
    if (!psqlda)
        fberror(args, "invalid psqlda specified");

    // Clean up existing SQLDA
    fbfreeSqlda(args, psqlda);

    // Allocate a new sqlda
    *psqlda = (XSQLDA*) malloc(XSQLDA_LENGTH(1));
    if (!*psqlda)
        fberror(args, "malloc failed");

    memset(*psqlda, 0x00, XSQLDA_LENGTH(1));
    (*psqlda)->version = SQLDA_VERSION1;
    (*psqlda)->sqln = 1;

    if (isc_dsql_describe(args->status, pstmt, SQLDA_VERSION1, *psqlda))
        fberror(args, "isc_dsql_describe failed");

    // Alloc this sqlda larger if there are more cols than allocated ...
    if ((*psqlda)->sqld > (*psqlda)->sqln)
    {
        int cnt = (*psqlda)->sqld;

        *psqlda = (XSQLDA*) realloc(*psqlda, XSQLDA_LENGTH(cnt));
        if (!*psqlda)
            fberror(args, "malloc failed");

        memset(*psqlda, 0x00, XSQLDA_LENGTH(cnt));
        (*psqlda)->version = SQLDA_VERSION1;
        (*psqlda)->sqln = cnt;

        // Now, fill it in completely
        if (isc_dsql_describe(args->status, pstmt, SQLDA_VERSION1, *psqlda))
            fberror(args, "isc_dsql_describe failed");

        // Size must be the same as previously described
        if ((*psqlda)->sqld != cnt)
            fberror(args, "isc_dsql_describe returned 2 different column counts");
    }

    // Allocate buffers for data and indicators ...
    if ((*psqlda)->sqld)
    {
        int     i;
        XSQLVAR *var;

        // Alloc the sqlvar memory
        for (var = (*psqlda)->sqlvar, i = 0; 
            i < (*psqlda)->sqln; 
            var++, i++)
        {
            // Reserve space for VARYING len bytes
            var->sqldata = (char*)malloc(var->sqllen + 2);
            var->sqlind  = (short*)malloc(sizeof(*(var->sqlind)));
            if (!var->sqldata || !var->sqlind)
                fberror(args, "malloc failed");
        }

    } // if result set

    // we made it here so it must be OK ...
    return 0;

} // allocSqlda


int fbfetch(
    Threadargs     args,
    isc_stmt_handle *pstmt,
    XSQLDA          *sqlda)
{
    int rc;

    fbGetLock(args->plock);

    // Lock db connection for multithreaded access (?!?)
    rc = isc_dsql_fetch(args->status, pstmt,
                          SQLDA_VERSION1, sqlda);

    fbReleaseLock(args->plock);

    return rc;
} // fetch


int ThreadFn(Threadargs args)
{
   char              sql[128];
   int               row;
   isc_db_handle     db1 = {0};
   isc_tr_handle     tr1 = {0};
   isc_stmt_handle   stmt1 = {0};
   XSQLDA            *sqlda = NULL;

   // Connect and set up ...
   if (fbconnect(args, args->database, &db1))
       fberror(args, "connect");

   if (args->doCreates)
   {
        // -------------------------------------
        // Drop index and table - ignore error
        if (fbbeginTran(args, &db1, &tr1))
            fberror(args, "beginTran");

        sprintf(sql, "drop index x%didx", args->tableID);
        fbexeci(args, &db1, &tr1, sql);

        sprintf(sql, "drop table x%d", args->tableID);
        fbexeci(args, &db1, &tr1, sql);
        if (fbcommit(args, &tr1))
            fberror(args, "commit after drop index/table");

        // -------------------------------------
        // Create table and index
        if (fbbeginTran(args, &db1, &tr1))
            fberror(args, "beginTran");

        sprintf(sql, "create table x%d(INTCOL INTEGER, DBLCOL DOUBLE PRECISION, CHARCOL CHAR(10), VCHARCOL VARCHAR(10))", args->tableID);
        if (fbexeci(args, &db1, &tr1, sql))
            fberror(args, "create table x#");

        if (fbcommit(args, &tr1))
            fberror(args, "commit");
            
        if (fbbeginTran(args, &db1, &tr1))
            fberror(args, "beginTran");

        sprintf(sql, "create unique desc index x%didx on x%d(INTCOL)", args->tableID, args->tableID);
        if (fbexeci(args, &db1, &tr1, sql))
            fberror(args, "create index x#idx");

        if (fbcommit(args, &tr1))
            fberror(args, "commit");
            
        // -------------------------------------
        // Populate table
        if (fbbeginTran(args, &db1, &tr1))
            fberror(args, "beginTran");

        for (row = 0; row < NUM_ROWS; row++)
        {
            // Should be parameterized, but this is easier ...
	        sprintf((char*)sql, "INSERT INTO x%d VALUES (%d,%d.%d,'thread%d','row%d')", args->tableID, row, args->threadID, row, args->threadID, row);
            if (fbexeci(args, &db1, &tr1, sql))
                fberror(args, "insert into x#");
        } // for row

        if (fbcommit(args, &tr1))
            fberror(args, "commit");

   } // if doCreates

   if (args->doFetch)
   {
        // -------------------------------------
        // Now fetch all the rows ...
        // Do it the requested number of times ...
        if (fballocStmt(args, &db1, &stmt1))
            fberror(args, "alloc statement");

        for (row = 0; row < NUM_LOOPS; row++)
        {
            int rc = 0;

            if (fbbeginTran(args, &db1, &tr1))
                fberror(args, "beginTran");
// Do an order by that doesn't use the index:
//            sprintf(sql, "select INTCOL, DBLCOL, CHARCOL, VCHARCOL from x%d order by INTCOL", args->tableID);

// Do an order by that does (hopefully) use the index:
//            sprintf(sql, "select INTCOL, DBLCOL, CHARCOL, VCHARCOL from x%d order by INTCOL DESC", args->tableID);

// Do a painful self join:
            sprintf(sql, "select x.INTCOL, y.DBLCOL, x.CHARCOL, y.VCHARCOL from x%d x, x%d y where x.intcol=y.intcol order by x.INTCOL DESC, y.DBLCOL", args->tableID, args->tableID);

// Do a painful self join2:
//            sprintf(sql, "select x.INTCOL, y.DBLCOL, x.CHARCOL, y.VCHARCOL from x%d x, x%d y where x.intcol=y.intcol order by x.INTCOL", args->tableID, args->tableID);

// Do a simple select * from table
//            sprintf(sql, "select * from x%d", args->tableID);

            // Execute the query to generate the result set:
            if (fbprepexec(args, &tr1, &stmt1, sql))
                fberror(args, "prepexec");
                
            // Set up the output SQLDA ...
            if (fballocSqlda(args, &stmt1, &sqlda ))
                fberror(args, "alloc sqlda");

            while (rc == 0)
            {
                    rc = fbfetch(args, &stmt1, sqlda);

                    // sqlda should be filled in ...
                    // we could validate, but we'll ignore it for now

            } // while rows to fetch

            // 100 is no data, meaning we have fetched all rows
            if (rc != 100)
                    fberror(args, "fetch");                

            if (fbcommit(args, &tr1))
                fberror(args, "commit");        

        } // for 

        if (fbfreeStmt(args, &stmt1, DSQL_drop))
            fberror(args, "drop statetment");

   } // if we should fetch the rows

   if (fbdisconnect(args, &db1))
       fberror(args, "disconnect");

   args->done = 1;

    return 0;
} // ThreadFn




// pass in the database to connect to ...
int main(int argc, char* argv[])
{
   ThreadArgs        args[NUM_THREADS] = {0};
   fbLock            lock = {0};
   int               thread;
   long              start;
   long              end;

   if (argc < 2)
   {
       printf("Usage:\t%s <full-path-to-database-file.ext>\n\n", argv[0]);
       exit(1);
   }

   fbInitLock(&lock);

   // If all of the threads share the same table,
   // we need to create the table beforehand
   // Don't time this since it is not done in parallel
   if (SAME_TABLE)
   {
       // Don't Fetch, only do the creates
       args->doFetch = 0;
       args->doCreates = 1;

       // Pass a reference to the lock, since this API is not thread safe
       args->plock = &lock;

       // Table ID is always 0 for same table

       // Tell function the database to use
       args->database = argv[1];

       // Do the creates
       if (ThreadFn(args))
            fberror(args, "");        

       // Reset thread state
       args->done = 0;
   }

   // Get time ...
   start = GetTickCount();

   // Start requested number of threads
   for (thread = 0; thread < NUM_THREADS; thread++)
   {   
       // Do the Fetch operation
       args[thread].doFetch = 1;

       // Have the thread create it's own table if it 
       // isn't charing one with the other threads
       args[thread].doCreates = !SAME_TABLE;

       // Pass a reference to the lock, since this API is not thread safe
       args[thread].plock = &lock;

       // Note: Using the same table increases concurrency issues
       if (!SAME_TABLE)
           args[thread].tableID = thread;

       args[thread].threadID = thread;

       // Tell thread the database to use
       // Note: We can use different databases to reduce concurrency issues
       args[thread].database = argv[1];

       // Reset thread state
       args[thread].done = 0;

       // Start the thread
       fbCreateThread(ThreadFn, &args[thread]);

// Debug code:
//     // Wait for this thread to finish for full serialization
//     while (!args[thread].done)
//          Sleep(1);

   } // for thread

   printf("\nStarted %d threads ...\n", thread);

   // Wait for completion ...
   for (thread = 0; thread < NUM_THREADS; thread++)
   {
       while (!args[thread].done)
            Sleep(1);

   } // for thread

   // Print out end time ?!?!?
   end = GetTickCount();

   // Print statistics
   // We only count the creates/inserts if they are done in parallel
   if (!SAME_TABLE)
        printf("\n%d rows inserted and ", NUM_ROWS, NUM_THREADS);

   printf("%d rows selected from each of %d tables in %f seconds.\n\n", 
          NUM_ROWS * NUM_LOOPS, NUM_THREADS, (end-start)/1000.0);

   fbDestroyLock(&lock);

   return 0;
} // main


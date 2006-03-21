
#ifdef WIN32
#include <windows.h>
#endif 

#ifdef SSA_OS_UNIX
#include <sys/resource.h>
#include <pthread.h>
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ibase.h"

// ------------------------------------------------------------
// Change these to change the way the code operates

// The number of threads to start
// #define NUM_THREADS 1
#define MAX_THREADS 64

// The number of rows in a table
#define NUM_ROWS    1000

// The number of times all of the rows in a table will get fetched
#define NUM_LOOPS   1

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
    int                 doPopulate; // Should this thread add to the table?
    int                 doFetch;    // Should this thread fetch?
	int                 done;       // Indicates the thread is finished
    const char          *database;  // database to be used
    int                 rowsFetched;
} ThreadArgs, *Threadargs;



// ------------------------------------------------------------
// There should be no need to change these
#define USERID      "sysdba"
#define PASSWORD    "masterkey"
#define CHARSET     "NONE"

// Ugly macro wrapper for create thread routine
#ifdef WIN32
#define fbCreateThread(threadfn, arg)   \
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)threadfn, arg, 0, NULL);
#endif

#ifdef SSA_OS_UNIX
void fbCreateThread(int (*ThreadFn)(Threadargs args), Threadargs args)
{
    pthread_t thread;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_create(&thread,   &attr, ThreadFn, args);
    
    pthread_attr_destroy(&attr);

}


long GetTickCount()
{
#if 0
     struct rusage r_usage;
     memset(&r_usage, 0, sizeof(r_usage));     
     getrusage(RUSAGE_SELF, &r_usage);
     return r_usage.ru_utime. tv_usec;
#else
     return time(NULL) * 1000;
#endif
}

void Sleep(int sec)
{
    usleep(sec * 1000);
}

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






// -----------------------------
// Wrapper functions for FB calls
void fberror(
    Threadargs     args,
    const char      *msg)
{
    short sqlcode;

    printf("ERROR: rc = %d during \"%s\" on table x%d thread %d.\n\n", 
        (int)*args->status, msg, args->tableID, args->threadID);

    // You'd think sqlcode would be defined the same for both APIs, wouldn't you?
    sqlcode = (short)isc_sqlcode(args->status);
    isc_print_sqlerror(sqlcode, args->status);

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

    // fbReleaseLock(args->plock);

    return rc;
} // connect


int fbdisconnect(
    Threadargs     args,
    isc_db_handle   *pdb)
{
    int rc;

    // fbGetLock(args->plock);

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

    fbGetLock(args->plock);

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


int fbprepare(
    Threadargs      args,
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

    fbReleaseLock(args->plock);

    return rc;
} // prepare


int fbexecute(
    Threadargs      args,
    isc_tr_handle   *ptrans,
    isc_stmt_handle *pstmt,
    XSQLDA          *sqlda)
{
    int rc;

    fbGetLock(args->plock);

    rc = isc_dsql_execute(args->status, ptrans, pstmt, 
                          SQL_DIALECT_V6, // wouldn't this be implied by the prepare ?!?!
                          sqlda);

    fbReleaseLock(args->plock);

    return rc;
} // execute


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
#define FB_COL_DATA       0x00
#define FB_PARAM_DATA     0x01
int fballocSqlda(
    Threadargs     args,
    isc_stmt_handle *pstmt,
    XSQLDA          **psqlda,
    int             type)
{
    if (!psqlda)
        fberror(args, "invalid psqlda specified");

    // Clean up existing SQLDA
    fbfreeSqlda(args, psqlda);

    // Allocate a new sqlda
    *psqlda = malloc(XSQLDA_LENGTH(1));
    if (!*psqlda)
        fberror(args, "malloc failed");

    memset(*psqlda, 0x00, XSQLDA_LENGTH(1));
    (*psqlda)->version = SQLDA_VERSION1;
    (*psqlda)->sqln = 1;

    if (type == FB_COL_DATA)
    {
        if (isc_dsql_describe(args->status, pstmt, SQLDA_VERSION1, *psqlda))
            fberror(args, "isc_dsql_describe failed");
    }
    else
    {
        if (isc_dsql_describe_bind(args->status, pstmt, SQLDA_VERSION1, *psqlda))
            fberror(args, "isc_dsql_describe_bind failed");
    }

    // Alloc this sqlda larger if there are more cols than allocated ...
    if ((*psqlda)->sqld > (*psqlda)->sqln)
    {
        int cnt = (*psqlda)->sqld;

        *psqlda = realloc(*psqlda, XSQLDA_LENGTH(cnt));
        if (!*psqlda)
            fberror(args, "malloc failed");

        memset(*psqlda, 0x00, XSQLDA_LENGTH(cnt));
        (*psqlda)->version = SQLDA_VERSION1;
        (*psqlda)->sqln = cnt;

        // Now, fill it in completely
        if (type == FB_COL_DATA)
        {
            if (isc_dsql_describe(args->status, pstmt, SQLDA_VERSION1, *psqlda))
                fberror(args, "isc_dsql_describe failed");
        }
        else
        {
            if (isc_dsql_describe_bind(args->status, pstmt, SQLDA_VERSION1, *psqlda))
                fberror(args, "isc_dsql_describe_bind failed");
        }

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
    else
    {
        // Nuke the sqlda because it is empty
        fbfreeSqlda(args, psqlda);
    }

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
   XSQLDA            *paramsqlda = NULL;

   args->rowsFetched = 0;

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
            
   } // if doCreates



	if (args->doPopulate)
	{
        // -------------------------------------
        // Populate table
        if (fbbeginTran(args, &db1, &tr1))
            fberror(args, "beginTran");

        for (row = 0; row < NUM_ROWS; row++)
        {
            // Should be parameterized, but this is easier ...
	        sprintf((char*)sql, "INSERT INTO x%d VALUES (%d,%d.%d,'thread%d','row%d')", 
				args->tableID, row + NUM_ROWS *args->threadID, args->threadID, row, args->threadID, row);
            if (fbexeci(args, &db1, &tr1, sql))
                fberror(args, "insert into x#");
        } // for row

        // Should be parameterized, but this is easier ...
	    sprintf((char*)sql, "DELETE from x%d where intcol >= %d and intcol < %d", 
				args->tableID, NUM_ROWS *args->threadID + NUM_ROWS/2, NUM_ROWS * args->threadID + NUM_ROWS);
				 
        if (fbexeci(args, &db1, &tr1, sql))
            fberror(args, "delete from");


        if (fbcommit(args, &tr1))
            fberror(args, "commit");

   }

   if (args->doFetch)
   {

	   	if (fbdisconnect(args, &db1))
    	   fberror(args, "disconnect");

	   	// Connect and set up ...
   		if (fbconnect(args, args->database, &db1))
       		fberror(args, "connect");

        // -------------------------------------
        // Now fetch all the rows ...
        // Do it the requested number of times ...
        if (fballocStmt(args, &db1, &stmt1))
            fberror(args, "alloc statement");

        if (fbbeginTran(args, &db1, &tr1))
            fberror(args, "beginTran");

//            sprintf(sql, "select INTCOL, DBLCOL, CHARCOL, VCHARCOL from x%d", args->tableID);


// Do an order by that doesn't use the index:
//            sprintf(sql, "select INTCOL, DBLCOL, CHARCOL, VCHARCOL from x%d order by INTCOL ASC", args->tableID);

// Do an order by that does (hopefully) use the index:
//            sprintf(sql, "select INTCOL, DBLCOL, CHARCOL, VCHARCOL from x%d order by INTCOL DESC", args->tableID);

// Do a painful self join:
//            sprintf(sql, "select x.INTCOL, y.DBLCOL, x.CHARCOL, y.VCHARCOL from x%d x, x%d y where x.intcol=y.intcol order by x.INTCOL DESC, y.DBLCOL", args->tableID, args->tableID);

// Do a painful self join2:
//            sprintf(sql, "select x.INTCOL, y.DBLCOL, x.CHARCOL, y.VCHARCOL from x%d x, x%d y where x.intcol=y.intcol order by x.INTCOL", args->tableID, args->tableID);

// Do a simple select * from table
//            sprintf(sql, "select * from x%d", args->tableID);

// Do select * from table where intcol > row
            sprintf(sql, "select INTCOL, DBLCOL, CHARCOL, VCHARCOL from x%d where INTCOL >= ? and INTCOL < ?", 
					args->tableID);


        // Prepare the query to generate the result set:
        if (fbprepare(args, &tr1, &stmt1, sql))
            fberror(args, "prepare");
    
        // Set up the output SQLDA ...
        if (fballocSqlda(args, &stmt1, &sqlda, FB_COL_DATA))
            fberror(args, "alloc sqlda");

        // Set up the param SQLDA ...
        if (fballocSqlda(args, &stmt1, &paramsqlda, FB_PARAM_DATA))
            fberror(args, "alloc param sqlda");

        // Point the parameter at the row ...
        if (paramsqlda && paramsqlda->sqld)
        {
            // Two params ...
            if (paramsqlda->sqld != 2)
                fberror(args, "expected 2 parameters");

            if (paramsqlda->sqlvar->sqllen != sizeof(row))
                fberror(args, "expected an int parameter");

            // Initialize the ind ...
            *(paramsqlda->sqlvar[0].sqlind) = 0;
            *(paramsqlda->sqlvar[1].sqlind) = 0;

        }


        for (row = 0; row < NUM_LOOPS; row++)
        {
            int rc = 0;

            // Set the parameter value ...
            if (paramsqlda && paramsqlda->sqld)
			{
                *(int*)(paramsqlda->sqlvar[0].sqldata) = (NUM_ROWS * args->threadID);
                *(int*)(paramsqlda->sqlvar[1].sqldata) = (NUM_ROWS * args->threadID + NUM_ROWS);
			}

            // Execute the query to generate the result set:
            if (fbexecute(args, &tr1, &stmt1, paramsqlda))
                fberror(args, "execute");

            while (rc == 0)
            {
                    rc = fbfetch(args, &stmt1, sqlda);

                    // sqlda should be filled in ...
                    // we could validate, but we'll ignore it for now

                    if (rc == 0)
                        args->rowsFetched++;

            } // while rows to fetch

            // 100 is no data, meaning we have fetched all rows
            if (rc != 100)
                    fberror(args, "fetch");                

            if (fbfreeStmt(args, &stmt1, DSQL_close))
                fberror(args, "close statetment");

        } // for 

        if (fbcommit(args, &tr1))
            fberror(args, "commit");        

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
   ThreadArgs        args[MAX_THREADS] = {0};
   fbLock            lock = {0};
   int               thread;
   long              start;
   long              end;
   int               numThreads = 0;

   if (argc < 3)
   {
       printf("Usage:\t%s <full-path-to-database-file.ext> <num threads>\n\n", argv[0]);
       exit(1);
   }

   numThreads = atoi(argv[2]);
   if ((numThreads < 1) || (numThreads > MAX_THREADS))
   {
       printf("Error:\tInvalid number of threads specified %s\n\n", argv[2]);
       exit(1);
   }

   fbInitLock(&lock);

   // we need to create the table beforehand
   // Don't time this since it is not done in parallel

   // Don't Fetch, only do the creates
   args->doFetch = 0;
   args->doCreates = 1;
   args->doPopulate = 0;

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

   // Get time ...
   start = GetTickCount();

   printf("Starting %d threads to update table\n", numThreads);

   // Start requested number of threads
   for (thread = 0; thread < numThreads; thread++)
   {   
       // Do the Fetch operation
       args[thread].doFetch = 1;
       args[thread].doPopulate = 1;
       args[thread].doCreates = 0;

       // Pass a reference to the lock, since this API is not thread safe
       args[thread].plock = &lock;

       // Note: Using the same table increases concurrency issues
       args[thread].tableID = 0;

       args[thread].threadID = thread;

       // Tell thread the database to use
       // Note: We can use different databases to reduce concurrency issues
       args[thread].database = argv[1];

       // Reset thread state
       args[thread].done = 0;

#if 0
	   ThreadFn(&args[thread]);
#else
       // Start the thread
       fbCreateThread(ThreadFn, &args[thread]);
#endif

// Debug code:
//     // Wait for this thread to finish for full serialization
//     while (!args[thread].done)
//          Sleep(1);

   } // for thread

#if 0
   // Wait for completion ...
   for (thread = 0; thread < numThreads; thread++)
   {
       while (!args[thread].done)
            Sleep(1);
   } // for thread


   printf("\nStarting %d threads to reading table\n", thread);

   // Start requested number of threads
   for (thread = 0; thread < numThreads; thread++)
   {   
       // Do the Fetch operation
       args[thread].doFetch = 1;
       args[thread].doPopulate = 0;
       args[thread].doCreates = 0;

       // Pass a reference to the lock, since this API is not thread safe
       args[thread].plock = &lock;

       // Note: Using the same table increases concurrency issues
       args[thread].tableID = 0;

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
#endif


   // Wait for completion ...
   for (thread = 0; thread < numThreads; thread++)
   {
       while (!args[thread].done)
            Sleep(1);

       // Make sure they all selected the same number of rows!
       if (thread && 
           (args[0].rowsFetched != args[thread].rowsFetched))
          fberror(args, "Threads fetched different numbers of rows");        

   } // for thread

   // Print out end time ?!?!?
   end = GetTickCount();

   // Print statistics
   // We only count the creates/inserts if they are done in parallel
   printf("\n%d rows inserted and ", NUM_ROWS, numThreads);

   printf("%d threads each selected %d rows in %f seconds.\n\n", 
          numThreads, args->rowsFetched, (end-start)/1000.0);

   fbDestroyLock(&lock);

   return 0;
} // main


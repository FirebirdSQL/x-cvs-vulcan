/* $Id$ */
#ifdef WIN32
#include <windows.h>
#endif 

#ifdef SSA_OS_UNIX_S64
#include <sys/resource.h>
#include <pthread.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ibase.h"

#define myassert(a)  { int *_p = NULL; if (!(a)) *_p = 0; }

// pass in the database to connect to ...
int main(int argc, char* argv[])
{
    ISC_STATUS        rc;
    ISC_STATUS_ARRAY  status = {0};

    isc_db_handle     con1 = {0};
    isc_stmt_handle	  stmt1 = {0};
    isc_tr_handle     tr1 = {0};

    isc_db_handle     con2 = {0};
    isc_stmt_handle   stmt2 = {0};
    isc_tr_handle     tr2 = {0};

    XSQLDA	          sqlda = {0};

    char	          *database = argv[1];
    char	          dpb[] = {isc_dpb_version1,isc_dpb_sql_dialect,1,SQL_DIALECT_V6,
                               isc_dpb_user_name,6,'s','y','s','d','b','a',
                               isc_dpb_password,9,'m','a','s','t','e','r','k','e','y',
                               isc_dpb_lc_ctype,4,'N','O','N','E'};
    short 		      dpb_len = sizeof(dpb);

    char		      tpb[] = {isc_tpb_version3,isc_tpb_write,
                               isc_tpb_read_committed,isc_tpb_rec_version,
                               isc_tpb_wait};
    short	          tpbL = sizeof(tpb);

    char 	          sqlRecreate[] = "recreate table foo ( col int ) ";
    short 	          sqlRecreateL = sizeof(sqlRecreate)-1;
    char 	          sqlDrop[] = "drop table foo ";
    short 	          sqlDropL = sizeof(sqlDrop)-1;



     if (argc < 2)
     {
         printf("Usage:\t%s <full-path-to-database-file.ext>\n\n", argv[0]);
         return 1;
     }


// con1
  rc = isc_attach_database(status,
                                        (short)strlen(database), database,
                                        &con1, (short)dpb_len, dpb );
  myassert(rc == 0);                                        
                                        
// stmt1
// con1
  rc = isc_dsql_allocate_statement(
                                   status,
                                   &con1, &stmt1);
  myassert(rc == 0);                                        
                                   
// con2
  rc = isc_attach_database(status,
                                        (short)strlen(database), database,
                                        &con2, (short)dpb_len, dpb );                                   
  myassert(rc == 0);                                        
                                        

// stmt2
// con2
  rc = isc_dsql_allocate_statement(
                                   status,
                                   &con2, &stmt2);                                        
  myassert(rc == 0);                                        
                                        

                                        
// stmt1
// con1
  rc = isc_dsql_free_statement(status,
                                            &stmt1, DSQL_close);                                        
  myassert(rc == 0);                                        
                                        
                                        
// con1
  rc = isc_start_transaction(status, &tr1,
                                          1, &con1, tpbL,
                                          (char*)tpb);  // Cast required by declaration                                        
  myassert(rc == 0);                                        
                                        
                                        
// stmt1
// con1
// "recreate table foo ( col int ) "
  rc = isc_dsql_prepare(
                        status,
                        &tr1,
                        &stmt1, sqlRecreateL,
                        (char *)sqlRecreate, SQL_DIALECT_V6, &sqlda);                                        
  myassert(rc == 0);                                        



// stmt1
// con1
  rc = isc_dsql_describe_bind(
                              status, &stmt1,
                              SQLDA_VERSION1, &sqlda);
  myassert(rc == 0);                                        

// con1
  rc = isc_rollback_transaction(status, &tr1);
  myassert(rc == 0);                                        
  
  
// con1
  rc = isc_start_transaction(status, &tr1,
                                          //1, &stmt1, tpbL,
                                          1, &con1, tpbL,
                                          (char*)tpb);  // Cast required by declaration
  myassert(rc == 0);                                        

// stmt1
// con1
  rc = isc_dsql_execute(
                        status,
                        &tr1,
                        &stmt1, SQL_DIALECT_V6, &sqlda);
  myassert(rc == 0);                                        


// con1
  rc = isc_commit_transaction(status, &tr1);
  myassert(rc == 0);                                        


// stmt1
// con1
  rc = isc_dsql_free_statement(status,
                                            &stmt1, DSQL_close);
  myassert(rc == 0);                                        


// stmt2
// con2
  rc = isc_dsql_free_statement(status,
                                            &stmt2, DSQL_close);
  myassert(rc == 0);                                        

// con2
  rc = isc_start_transaction(status, &tr2,
                                          1, &con2, tpbL,
                                          (char*)tpb);  // Cast required by declaration
  myassert(rc == 0);                                        


// stmt2
// con2
// "drop table foo "
  rc = isc_dsql_prepare(
                        status,
                        &tr2,
                        &stmt2, sqlDropL,
                        (char *)sqlDrop, SQL_DIALECT_V6, &sqlda);
  myassert(rc == 0);                                        



// stmt2
// con2
  rc = isc_dsql_describe_bind(
                              status, &stmt2,
                              SQLDA_VERSION1, &sqlda);
  myassert(rc == 0);                                        



// con2
  rc = isc_rollback_transaction(status, &tr2);
  myassert(rc == 0);                                        


// con2
  rc = isc_start_transaction(status, &tr2,
                                          1, &con2, tpbL,
                                          (char*)tpb);  // Cast required by declaration
  myassert(rc == 0);                                        

// stmt2
// con2
  rc = isc_dsql_execute(
                        status,
                        &tr2,
                        &stmt2, SQL_DIALECT_V6, &sqlda);
  myassert(rc == 0);                                        


// con2
  rc = isc_commit_transaction(status, &tr2);
  myassert(rc == 0);                                        


// stmt2
// con2
  rc = isc_dsql_free_statement(status,
                                            &stmt2, DSQL_close);
  myassert(rc == 0);                                        


// stmt1
// con1
  rc = isc_dsql_free_statement(status,
                                            &stmt1, DSQL_close);
  myassert(rc == 0);                                        


// con1
  rc = isc_start_transaction(status, &tr1,
                                          1, &con1, tpbL,
                                          (char*)tpb);  // Cast required by declaration
  myassert(rc == 0);                                        


// stmt1
// con1
// "recreate table foo ( col int ) "
  rc = isc_dsql_prepare(
                        status,
                        &tr1,
                        &stmt1, sqlRecreateL,
                        (char *)sqlRecreate, SQL_DIALECT_V6, &sqlda);
  myassert(rc == 0);                                        


// stmt1
// con1
  rc = isc_dsql_describe_bind(
                              status, &stmt1,
                              SQLDA_VERSION1, &sqlda);
  myassert(rc == 0);                                        

// con1
  rc = isc_rollback_transaction(status, &tr1);  
  myassert(rc == 0);                                        
  
// con1
  rc = isc_start_transaction(status, &tr1,
                                          1, &con1, tpbL,
                                          (char*)tpb);  // Cast required by declaration
  myassert(rc == 0);                                        

// stmt1
// con1
  rc = isc_dsql_execute(
                        status,
                        &tr1,
                        //&con1, SQL_DIALECT_V6, &sqlda);
                        &stmt1, SQL_DIALECT_V6, &sqlda);
  
  if (rc)
	isc_print_status(status);
	
  myassert(rc == 0);                                        



// con1
  rc = isc_commit_transaction(status, &tr1);
  myassert(rc == 0);                                        


// stmt1
// con1
  rc = isc_dsql_free_statement(status,
                                            &stmt1, DSQL_close);
  myassert(rc == 0);                                        



// con1
  rc = isc_start_transaction(status, &tr1,
                                          1, &con1, tpbL,
                                          (char*)tpb);  // Cast required by declaration
  myassert(rc == 0);                                        


// con2
  rc = isc_start_transaction(status, &tr2,
                                          1, &con2, tpbL,
                                          (char*)tpb);  // Cast required by declaration
  myassert(rc == 0);                                        
                                          
                                          

// stmt2
// con2
  rc = isc_dsql_free_statement(status,
                                            &stmt2, DSQL_close);
  myassert(rc == 0);                                        

// stmt2
// con2
// "drop table foo "
  rc = isc_dsql_prepare(
                        status,
                        &tr2,
                        &stmt2, sqlDropL,
                        (char *)sqlDrop, SQL_DIALECT_V6, &sqlda);
  myassert(rc == 0);                                        



// stmt2
// con2
  rc = isc_dsql_describe_bind(
                              status, &stmt2,
                              SQLDA_VERSION1, &sqlda);
  myassert(rc == 0);                                        

// stmt2
// con2
  rc = isc_dsql_execute(
                        status,
                        &tr2,
                        &stmt2, SQL_DIALECT_V6, &sqlda);
  myassert(rc == 0);                                        


// stmt2
// con2
  rc = isc_dsql_free_statement(status,
                                            &stmt2, DSQL_close);
  myassert(rc == 0);                                        

// stmt1
// con1
  rc = isc_dsql_free_statement(status,
                                            &stmt1, DSQL_close);
  myassert(rc == 0);                                        

// stmt1
// con1
// "recreate table foo ( col int ) "
  rc = isc_dsql_prepare(
                        status,
                        &tr1,
                        &stmt1, sqlRecreateL,
                        (char *)sqlRecreate, SQL_DIALECT_V6, &sqlda);
  myassert(rc == 0);                                        



// stmt1
// con1
  rc = isc_dsql_describe_bind(
                              status, &stmt1,
                              SQLDA_VERSION1, &sqlda);
  myassert(rc == 0);                                        

/* boom */

// stmt1
// con1
  rc = isc_dsql_execute(
                        status,
                        &tr1,
                        &stmt1, SQL_DIALECT_V6, &sqlda);
  myassert(rc == 0);                                        

/* boom */

                                        


   
     return 0;
} // main

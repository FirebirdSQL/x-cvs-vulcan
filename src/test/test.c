/* $Id$ */
/*---------------------------------------------------------------------
 *  Copyright (C), 2000
 *  SAS Institute Inc., Cary, N.C. 27513, U.S.A.  All rights reserved.
 * --------------------------------------------------------------------
 *
 *  SUPPORT:     lihamm - Linda Hamm
 *  LANGUAGE:    C
 *  PURPOSE:     Generic test 
 *
 *
 *  HISTORY:
 *     Defect     Action                                Date          Name
 *     ----------------------------------------------------------------
 *                  Initial implementation          03/08/2004  lihamm
 *
 *  NOTES:
 *--------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/***
#include "firebird.h"
***/
#include "ibase.h"

int main()
{
	typedef struct vary 
		{
		short vary_length;
		char vary_string[1];
		} VARY;

	isc_db_handle    db1 = NULL;
	isc_db_handle    db2 = NULL;
	isc_tr_handle    tr1 = NULL;
	isc_tr_handle    tr2 = NULL;

	char *database = NULL;
	long databaseL = 0;

	char *uid = "sysdba";
	char uidC = 6;
	char *pwd = "masterkey";
	char pwdC = 9;

	int dpb_len = 0;
	char *dpb = NULL;
	char dpb_buffer[512];      /* Space for uid and pwd */

//	char stmt_buffer[512];     /* Space for execute immediate statement */
	long tkrc;

	char *newdb = "newdb2.gdb";
	long newdbL = 13;

	ISC_STATUS status_vector[20];
	isc_stmt_handle stmt = NULL;
	isc_stmt_handle stmt2 = NULL;
	XSQLDA *isqldata = NULL;
	XSQLDA *ivar = NULL;

	int *skus;
	float *prices;
	char **toys;
	int sku_val[] = {1223, 1224, 1225, 1226, 0};
	float price_val[] = {10.00, 11.00, 12.00, 13.00, 0}; 
	char *toy_val[] = {"Barbie", "Skipper", "Midge", "Allan", ""};
	char toy[32];
	char sku[20], price[20];

	short flag0, flag1, flag2;

	long SQLCODE;
	XSQLDA *osqlda;
	XSQLVAR *ovar;
	VARY *vary;

	printf ("Size of XSQLDA is %d\n", sizeof (XSQLDA));

	char *create_stmt = "create database 'newdb2.gdb' page_size 16384 user 'SYSDBA' password 'masterkey';";
	char *table_stmt = "create table test(sku integer, price float, toy char(30))";
	char *insert_stmt = "insert into test (sku, price, toy) values (1222, 2.00, 'ken')";
	char *insert_stmt2 = "insert into test (sku, price, toy) values (?, ?, ?)";
	char *fetch_stmt = "select * from test";


	// Stuff the db params for this connection
	dpb = dpb_buffer;
	*dpb++ = isc_dpb_version1;

	// Stuff UID
	*dpb++ = isc_dpb_user_name;
	*dpb++ = uidC;
	memcpy(dpb, uid, uidC);
	dpb += uidC;

	// Stuff PWD
	*dpb++ = isc_dpb_password;
	*dpb++ = pwdC;
	memcpy(dpb, pwd, pwdC);
	dpb += pwdC;

	// Stuff the lc_ctype
	*dpb++ = isc_dpb_lc_ctype;
	*dpb++ = sizeof("NONE")-1;
	memcpy(dpb, "NONE", sizeof("NONE")-1);
	dpb += sizeof("NONE")-1;

	// Figure out how big the db params are ...
	dpb_len = (dpb - dpb_buffer);

	if (dpb_len < 2)
		{
		dpb = NULL;
		dpb_len = 0;
		}

	printf("Create database, isc call\n");
	if (tkrc = isc_create_database(status_vector, 0, 
					newdb, &db1, (short)dpb_len, dpb_buffer, 0))
		{
		isc_print_status(status_vector);
		printf("Cannot create database - exiting test\n");
		isc_print_status(status_vector);
		return(2); 
		}

	printf ("Start transaction\n");
	if (tkrc = isc_start_transaction (status_vector, &tr1, 1, &db1, 0, NULL))
		isc_print_status(status_vector);

	printf ("Allocate statement\n");
	if (tkrc = isc_dsql_allocate_statement (status_vector, &db1, &stmt))
		isc_print_status(status_vector);

	printf ("Prepare Sstatement\n");
	if (tkrc = isc_dsql_prepare (status_vector, &tr1, &stmt, 0, table_stmt, 3, NULL))
		isc_print_status(status_vector);

	printf ("Execute statement- Create table 1\n");
	if (tkrc = isc_dsql_execute (status_vector, &tr1, &stmt, 1, NULL))
		isc_print_status(status_vector);

	printf ("Commit transaction 1\n");
	if (tr1)
		isc_commit_transaction (status_vector, &tr1);

	printf ("Start transaction 2\n");
	if (tkrc = isc_start_transaction (status_vector, &tr1, 1, &db1, 0, NULL))
		isc_print_status (status_vector);

	printf ("Prepare Statement 2\n");
	if (tkrc = isc_dsql_prepare  (status_vector, &tr1, &stmt, 0, insert_stmt, 3, NULL))
		isc_print_status (status_vector);

	isqldata = (XSQLDA *)malloc(XSQLDA_LENGTH(3));
	memset (isqldata, 0, sizeof (*isqldata));
	isqldata->version = SQLDA_VERSION1;
	isqldata->sqln = 3;
	isqldata->sqld = 0;

	printf ("Execute Statement- insert row 1\n");
	if (tkrc = isc_dsql_execute (status_vector, &tr1, &stmt, 1, isqldata))
		isc_print_status (status_vector);

	printf ("Commit transaction\n");
	if (tr1)
		isc_commit_transaction(status_vector, &tr1);

	printf ("Start transaction 3\n");
	if (tkrc = isc_start_transaction(status_vector, &tr1, 1, &db1, 0, NULL))
		isc_print_status(status_vector);

	printf ("Prepare statement 3\n");
	if (tkrc = isc_dsql_prepare (status_vector, &tr1, &stmt, 0, insert_stmt2, 1, isqldata))
		 isc_print_status(status_vector);

	printf ("Describe/bind statement 3\n");
	if (tkrc = isc_dsql_describe_bind (status_vector, &stmt, 1, isqldata))
		isc_print_status (status_vector);
	
	toys = toy_val;
	skus = sku_val ;
	prices = price_val;
	flag0 = flag1 = flag2 = 0;

	while (*skus)
		{ 
		int i;
		for (i = 0; i < isqldata->sqln; i++)
			{
			ovar = &(isqldata->sqlvar[i]);
			switch (ovar->sqltype)
				{
				case SQL_LONG:
				case (SQL_LONG + 1):
					ovar->sqllen = sizeof (*skus);
					ovar->sqldata = (char *) (skus++);
					ovar->sqlind = &flag0;
					printf ("\tstoring sku %d\n", *(int *)ovar->sqldata);
					break;
					
				case SQL_FLOAT:
				case (SQL_FLOAT + 1):
					ovar->sqllen = sizeof (*prices);
					ovar->sqldata = (char *)(prices++);
					printf ("\tstoring price %f\n", *(float *)ovar->sqldata);
					ovar->sqlind = &flag1;
					break;
					
				case SQL_TEXT:
				case (SQL_TEXT + 1):
					ovar->sqllen = strlen (*toys);
					ovar->sqldata = (*toys++);
					printf ("\tstoring toy %s\n", ovar->sqldata);
					ovar->sqlind = &flag2;
					break;
				
				default:
					printf ("\t unexpected datatype\n");
				}
			}
		printf ("Execute statement- insert row 2-5\n");
		if (tkrc = isc_dsql_execute(status_vector, &tr1, &stmt, 1, isqldata))
			isc_print_status (status_vector);
		}
						
		
	printf ("Commit transaction\n");
	if (tr1)
		 isc_commit_transaction (status_vector, &tr1);

	printf ("Detach database\n");
	if (tkrc = isc_detach_database (status_vector, &db1))
		isc_print_status (status_vector);

     // Now we can try to connect again ...
     printf("Attach Database again\n");
     if (tkrc = isc_attach_database(status_vector, 0, newdb, &db2, (short)dpb_len, dpb_buffer))
		{
		printf("Cannot attach database twice\n");
		isc_print_status(status_vector);
		}
     
     printf ("Allocate statement\n");
     tkrc = isc_dsql_allocate_statement(status_vector, &db2, &stmt2);
     if (status_vector[0] == 1 && status_vector[1])
              isc_print_status (status_vector);

     printf("Start transaction DB2\n");
     tkrc = isc_start_transaction(status_vector, &tr2, 1, &db2, 0, NULL);
     if ((status_vector[0] == 1) && (status_vector[1]))
             {
             printf ("Cannot start transaction\n");
             isc_print_status (status_vector);
             }

    osqlda = (XSQLDA *)malloc (XSQLDA_LENGTH(3));
    osqlda->version = SQLDA_VERSION1;
    osqlda->sqln = 3;

    /* Prepare the statement. */
    printf ("Prepare Statement DB2\n");
	if (tkrc = isc_dsql_prepare(status_vector, &tr2, &stmt2, 0, fetch_stmt, 1, osqlda))
		{
		printf ("Cannot prepare fetch\n");
		isc_print_status (status_vector);
		}

    /* Set up an output XSQLVAR structure to allocate space for each item to be returned. */
    osqlda->sqlvar[0].sqldata = sku;
    osqlda->sqlvar[0].sqltype = SQL_VARYING +1;
    osqlda->sqlvar[0].sqlind = &flag0;
    osqlda->sqlvar[1].sqldata = price;
    osqlda->sqlvar[1].sqltype = SQL_VARYING +1;
    osqlda->sqlvar[1].sqlind = &flag1;
    osqlda->sqlvar[2].sqldata = toy;
    osqlda->sqlvar[2].sqltype = SQL_VARYING +1;
    osqlda->sqlvar[2].sqlind = &flag2;

    /* Execute the statement. */
    printf ("execute the select\n");
    if (tkrc = isc_dsql_execute (status_vector, &tr2, &stmt2, 1, NULL))
        isc_print_status(status_vector);

    printf ("fetch rows\n");
    printf ("\n%-20s %-15s %-10s\n\n", "SKU", "PRICE", "TOY NAME");
 
    /* Fetch and print the records in the select list one by one. */

    SQLCODE = 0;
    while (!SQLCODE)
		{
		if (tkrc = isc_dsql_fetch (status_vector, &stmt2, 1, osqlda))
			{
			isc_print_status (status_vector);
			break;
			}
			
		SQLCODE = isc_sqlcode (status_vector);

		if (flag0)
			printf ("%-20.*s \n", strlen ("<null>"), "<null>");
		else
			{
			vary = (VARY *)sku;
			printf("%-20.*s ", vary->vary_length, vary->vary_string);
			}

		if (flag1)
			printf ("%-15.*s \n", strlen ("<null>"), "<null>");
		else
			{
			vary = (VARY *)price;
			printf("%-15.*s ", vary->vary_length, vary->vary_string);
			}

		if (flag2)
			printf ("%-15.*s \n", strlen ("<null>"), "<null>");
		else
			{
			vary = (VARY *)toy;
			printf("%-4.*s \n", vary->vary_length, vary->vary_string);
			}
		}

	if (SQLCODE && SQLCODE != 100)
		isc_print_sqlerror ((short)SQLCODE, status_vector);

	if (tr2)
		{
		printf ("commit transaction2\n");
		isc_commit_transaction (status_vector, &tr2);
		}

	if (0 && db2)
		{
		printf("Detach database\n");
		if (tkrc = isc_detach_database (status_vector, &db2))
			isc_print_status (status_vector);
		}

	if (db2)
		{
		printf ("Drop database\n");
		if (tkrc = isc_drop_database (status_vector, &db2))
			isc_print_status (status_vector);
		}

	return(0);
}

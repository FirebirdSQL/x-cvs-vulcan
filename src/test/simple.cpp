/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version WI-V2.0.0.4027 Vulcan 1.0 Development **********************/
#include <stdio.h>

/*DATABASE DB = STATIC "yachts.lnk";*/
/**** GDS Preprocessor Definitions ****/
#ifndef JRD_IBASE_H
#include <ibase.h>
#endif

static const ISC_QUAD
   isc_blob_null = {0,0};	/* initializer for blobs */
static isc_db_handle
   DB = 0;		/* database handle */

static isc_tr_handle
   gds_trans = 0;		/* default transaction handle */
static long
   isc_status [20],	/* status vector */
   isc_status2 [20];	/* status vector */
static SLONG
   isc_array_length, 	/* array return size */
   SQLCODE;		/* SQL status code */
static isc_req_handle
   isc_0 = 0;		/* request handle */

static const short
   isc_1l = 168;
static const char
   isc_1 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 2, 1,0, 
	    blr_short, 0, 
	 blr_message, 1, 1,0, 
	    blr_short, 0, 
	 blr_message, 0, 3,0, 
	    blr_cstring, 32,0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_begin, 
	    blr_for, 
	       blr_rse, 1, 
		  blr_relation, 13, 'R','D','B','$','R','E','L','A','T','I','O','N','S', 0, 
		  blr_end, 
	       blr_begin, 
		  blr_send, 0, 
		     blr_begin, 
			blr_assignment, 
			   blr_field, 0, 17, 'R','D','B','$','R','E','L','A','T','I','O','N','_','N','A','M','E', 
			   blr_parameter, 0, 0,0, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 0, 1,0, 
			blr_assignment, 
			   blr_field, 0, 15, 'R','D','B','$','R','E','L','A','T','I','O','N','_','I','D', 
			   blr_parameter, 0, 2,0, 
			blr_end, 
		  blr_label, 0, 
		     blr_loop, 
			blr_select, 
			   blr_receive, 2, 
			      blr_leave, 0, 
			   blr_receive, 1, 
			      blr_modify, 0, 1, 
				 blr_begin, 
				    blr_assignment, 
				       blr_parameter, 1, 0,0, 
				       blr_field, 1, 15, 'R','D','B','$','R','E','L','A','T','I','O','N','_','I','D', 
				    blr_end, 
			   blr_end, 
		  blr_end, 
	    blr_send, 0, 
	       blr_assignment, 
		  blr_literal, blr_long, 0, 0,0,0,0,
		  blr_parameter, 0, 1,0, 
	    blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_1 */


#define gds_blob_null	isc_blob_null	/* compatibility symbols */
#define gds_status	isc_status
#define gds_status2	isc_status2
#define gds_array_length	isc_array_length
#define gds_count	isc_count
#define gds_slack	isc_slack
#define gds_utility	isc_utility	/* end of compatibility symbols */

#ifndef isc_version4
    Generate a compile-time error.
    Picking up a V3 include file after preprocessing with V4 GPRE.
#endif

/**** end of GPRE definitions ****/


int main (int argc, char **argv)
{
   struct {
          short isc_9;	/* isc_utility */
   } isc_8;
   struct {
          short isc_7;	/* RDB$RELATION_ID */
   } isc_6;
   struct {
          char  isc_3 [32];	/* RDB$RELATION_NAME */
          short isc_4;	/* isc_utility */
          short isc_5;	/* RDB$RELATION_ID */
   } isc_2;
	/*READY;*/
	{
	isc_attach_database ((long*) 0L, 0, "yachts.lnk", &DB, 0, (char*) 0);
	}
	/*START_TRANSACTION;*/
	{
	{
	if (!DB)
	   isc_attach_database ((long*) 0L, 0, "yachts.lnk", &DB, 0, (char*) 0);
	if (DB)
	   isc_start_transaction ((long*) 0L, (FRBRD**) &gds_trans, (short) 1, &DB, (short) 0, (char*) 0);
	}
	}

	/*for x in rdb$relations*/
	{
	{
	if (!DB)
	   isc_attach_database ((long*) 0L, 0, "yachts.lnk", &DB, 0, (char*) 0);
	if (DB && !gds_trans)
	   isc_start_transaction ((long*) 0L, (FRBRD**) &gds_trans, (short) 1, &DB, (short) 0, (char*) 0);
	}
        if (!isc_0)
           isc_compile_request2 ((long*) 0L, (FRBRD**) &DB, (FRBRD**) &isc_0, (short) sizeof (isc_1), (char *) isc_1);
        isc_start_request ((long*) 0L, (FRBRD**) &isc_0, (FRBRD**) &gds_trans, (short) 0);
	while (1)
	   {
           isc_receive ((long*) 0L, (FRBRD**) &isc_0, (short) 0, (short) 36, &isc_2, (short) 0);
	   if (!isc_2.isc_4) break;
		printf ("%s\n", /*x.rdb$relation_name*/
				isc_2.isc_3);
		/*modify x using*/
		{
			/*x.rdb$relation_id*/
			isc_2.isc_5 = 123;
		/*end_modify;*/
		isc_6.isc_7 = isc_2.isc_5;
                isc_send ((long*) 0L, (FRBRD**) &isc_0, (short) 1, (short) 2, &isc_6, (short) 0);
		}
	/*end_for;*/
           isc_send ((long*) 0L, (FRBRD**) &isc_0, (short) 2, (short) 2, &isc_8, (short) 0);
	   }
	}

	/*commit_transaction;*/
	{
	isc_commit_transaction ((long*) 0L, (FRBRD**) &gds_trans);
	}
	/*finish*/
	{
        if (gds_trans)
            isc_commit_transaction ((long*) 0L, (FRBRD**) &gds_trans);
	if (DB)
	   isc_detach_database ((long*) 0L, &DB);
	};

	return 0;
}
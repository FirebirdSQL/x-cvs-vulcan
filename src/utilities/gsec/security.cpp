/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version WI-V6.2.972 Firebird 1.0.3 **********************/
/*
 *
 *	PROGRAM:	Security data base manager
 *	MODULE:		security.epp
 *	DESCRIPTION:	Security routines
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../jrd/ib_stdio.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../jrd/common.h"
#include "../jrd/y_ref.h"
#include "../jrd/ibase.h"
#include "../jrd/jrd_pwd.h"
#include "../jrd/enc_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/isc_proto.h"
#include "../utilities/gsec/gsec.h"
#include "../utilities/gsec/secur_proto.h"

/*DATABASE DB = STATIC FILENAME "security.fdb";*/
/**** GDS Preprocessor Definitions ****/
#ifndef _JRD_IBASE_H_
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
static long
   isc_array_length, 	/* array return size */
   SQLCODE;		/* SQL status code */
static isc_req_handle
   isc_0 = 0;		/* request handle */

static const short
   isc_1l = 228;
static const char
   isc_1 [] = {
      blr_version5,
      blr_begin, 
	 blr_message, 1, 9,0, 
	    blr_long, 0, 
	    blr_long, 0, 
	    blr_short, 0, 
	    blr_cstring, 129,0, 
	    blr_cstring, 129,0, 
	    blr_cstring, 33,0, 
	    blr_cstring, 97,0, 
	    blr_cstring, 97,0, 
	    blr_cstring, 97,0, 
	 blr_message, 0, 1,0, 
	    blr_cstring, 129,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 1, 
		     blr_relation, 5, 'U','S','E','R','S', 0, 
		     blr_boolean, 
			blr_eql, 
			   blr_field, 0, 9, 'U','S','E','R','_','N','A','M','E', 
			   blr_parameter, 0, 0,0, 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_field, 0, 3, 'G','I','D', 
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_field, 0, 3, 'U','I','D', 
			   blr_parameter, 1, 1,0, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 2,0, 
			blr_assignment, 
			   blr_field, 0, 9, 'U','S','E','R','_','N','A','M','E', 
			   blr_parameter, 1, 3,0, 
			blr_assignment, 
			   blr_field, 0, 10, 'G','R','O','U','P','_','N','A','M','E', 
			   blr_parameter, 1, 4,0, 
			blr_assignment, 
			   blr_field, 0, 6, 'P','A','S','S','W','D', 
			   blr_parameter, 1, 5,0, 
			blr_assignment, 
			   blr_field, 0, 10, 'F','I','R','S','T','_','N','A','M','E', 
			   blr_parameter, 1, 6,0, 
			blr_assignment, 
			   blr_field, 0, 11, 'M','I','D','D','L','E','_','N','A','M','E', 
			   blr_parameter, 1, 7,0, 
			blr_assignment, 
			   blr_field, 0, 9, 'L','A','S','T','_','N','A','M','E', 
			   blr_parameter, 1, 8,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 2,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_1 */

static isc_req_handle
   isc_14 = 0;		/* request handle */

static const short
   isc_15l = 201;
static const char
   isc_15 [] = {
      blr_version5,
      blr_begin, 
	 blr_message, 0, 9,0, 
	    blr_long, 0, 
	    blr_long, 0, 
	    blr_short, 0, 
	    blr_cstring, 129,0, 
	    blr_cstring, 129,0, 
	    blr_cstring, 33,0, 
	    blr_cstring, 97,0, 
	    blr_cstring, 97,0, 
	    blr_cstring, 97,0, 
	 blr_begin, 
	    blr_for, 
	       blr_rse, 1, 
		  blr_relation, 5, 'U','S','E','R','S', 0, 
		  blr_end, 
	       blr_send, 0, 
		  blr_begin, 
		     blr_assignment, 
			blr_field, 0, 3, 'G','I','D', 
			blr_parameter, 0, 0,0, 
		     blr_assignment, 
			blr_field, 0, 3, 'U','I','D', 
			blr_parameter, 0, 1,0, 
		     blr_assignment, 
			blr_literal, blr_long, 0, 1,0,0,0,
			blr_parameter, 0, 2,0, 
		     blr_assignment, 
			blr_field, 0, 9, 'U','S','E','R','_','N','A','M','E', 
			blr_parameter, 0, 3,0, 
		     blr_assignment, 
			blr_field, 0, 10, 'G','R','O','U','P','_','N','A','M','E', 
			blr_parameter, 0, 4,0, 
		     blr_assignment, 
			blr_field, 0, 6, 'P','A','S','S','W','D', 
			blr_parameter, 0, 5,0, 
		     blr_assignment, 
			blr_field, 0, 10, 'F','I','R','S','T','_','N','A','M','E', 
			blr_parameter, 0, 6,0, 
		     blr_assignment, 
			blr_field, 0, 11, 'M','I','D','D','L','E','_','N','A','M','E', 
			blr_parameter, 0, 7,0, 
		     blr_assignment, 
			blr_field, 0, 9, 'L','A','S','T','_','N','A','M','E', 
			blr_parameter, 0, 8,0, 
		     blr_end, 
	    blr_send, 0, 
	       blr_assignment, 
		  blr_literal, blr_long, 0, 0,0,0,0,
		  blr_parameter, 0, 2,0, 
	    blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_15 */

static isc_req_handle
   isc_26 = 0;		/* request handle */

static const short
   isc_27l = 109;
static const char
   isc_27 [] = {
      blr_version5,
      blr_begin, 
	 blr_message, 3, 1,0, 
	    blr_short, 0, 
	 blr_message, 2, 1,0, 
	    blr_short, 0, 
	 blr_message, 1, 1,0, 
	    blr_short, 0, 
	 blr_message, 0, 1,0, 
	    blr_cstring, 129,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 1, 
		     blr_relation, 5, 'U','S','E','R','S', 0, 
		     blr_boolean, 
			blr_eql, 
			   blr_field, 0, 9, 'U','S','E','R','_','N','A','M','E', 
			   blr_parameter, 0, 0,0, 
		     blr_end, 
		  blr_begin, 
		     blr_send, 1, 
			blr_begin, 
			   blr_assignment, 
			      blr_literal, blr_long, 0, 1,0,0,0,
			      blr_parameter, 1, 0,0, 
			   blr_end, 
		     blr_label, 0, 
			blr_loop, 
			   blr_select, 
			      blr_receive, 3, 
				 blr_leave, 0, 
			      blr_receive, 2, 
				 blr_handler, 
				    blr_erase, 0, 
			      blr_end, 
		     blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 0,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_27 */

static isc_req_handle
   isc_36 = 0;		/* request handle */

static const short
   isc_37l = 420;
static const char
   isc_37 [] = {
      blr_version5,
      blr_begin, 
	 blr_message, 3, 1,0, 
	    blr_short, 0, 
	 blr_message, 2, 14,0, 
	    blr_long, 0, 
	    blr_long, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_cstring, 97,0, 
	    blr_cstring, 97,0, 
	    blr_cstring, 97,0, 
	    blr_cstring, 33,0, 
	    blr_cstring, 129,0, 
	 blr_message, 1, 15,0, 
	    blr_long, 0, 
	    blr_long, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_cstring, 129,0, 
	    blr_cstring, 33,0, 
	    blr_cstring, 97,0, 
	    blr_cstring, 97,0, 
	    blr_cstring, 97,0, 
	 blr_message, 0, 1,0, 
	    blr_cstring, 129,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 1, 
		     blr_relation, 5, 'U','S','E','R','S', 0, 
		     blr_boolean, 
			blr_eql, 
			   blr_field, 0, 9, 'U','S','E','R','_','N','A','M','E', 
			   blr_parameter, 0, 0,0, 
		     blr_end, 
		  blr_begin, 
		     blr_send, 1, 
			blr_begin, 
			   blr_assignment, 
			      blr_field, 0, 3, 'G','I','D', 
			      blr_parameter2, 1, 0,0, 8,0, 
			   blr_assignment, 
			      blr_field, 0, 3, 'U','I','D', 
			      blr_parameter2, 1, 1,0, 9,0, 
			   blr_assignment, 
			      blr_literal, blr_long, 0, 1,0,0,0,
			      blr_parameter, 1, 2,0, 
			   blr_assignment, 
			      blr_field, 0, 10, 'G','R','O','U','P','_','N','A','M','E', 
			      blr_parameter2, 1, 10,0, 7,0, 
			   blr_assignment, 
			      blr_field, 0, 6, 'P','A','S','S','W','D', 
			      blr_parameter2, 1, 11,0, 6,0, 
			   blr_assignment, 
			      blr_field, 0, 10, 'F','I','R','S','T','_','N','A','M','E', 
			      blr_parameter2, 1, 12,0, 5,0, 
			   blr_assignment, 
			      blr_field, 0, 11, 'M','I','D','D','L','E','_','N','A','M','E', 
			      blr_parameter2, 1, 13,0, 4,0, 
			   blr_assignment, 
			      blr_field, 0, 9, 'L','A','S','T','_','N','A','M','E', 
			      blr_parameter2, 1, 14,0, 3,0, 
			   blr_end, 
		     blr_label, 0, 
			blr_loop, 
			   blr_select, 
			      blr_receive, 3, 
				 blr_leave, 0, 
			      blr_receive, 2, 
				 blr_handler, 
				    blr_modify, 0, 1, 
				       blr_begin, 
					  blr_assignment, 
					     blr_parameter2, 2, 9,0, 8,0, 
					     blr_field, 1, 9, 'L','A','S','T','_','N','A','M','E', 
					  blr_assignment, 
					     blr_parameter2, 2, 10,0, 7,0, 
					     blr_field, 1, 11, 'M','I','D','D','L','E','_','N','A','M','E', 
					  blr_assignment, 
					     blr_parameter2, 2, 11,0, 6,0, 
					     blr_field, 1, 10, 'F','I','R','S','T','_','N','A','M','E', 
					  blr_assignment, 
					     blr_parameter2, 2, 12,0, 5,0, 
					     blr_field, 1, 6, 'P','A','S','S','W','D', 
					  blr_assignment, 
					     blr_parameter2, 2, 13,0, 4,0, 
					     blr_field, 1, 10, 'G','R','O','U','P','_','N','A','M','E', 
					  blr_assignment, 
					     blr_parameter2, 2, 1,0, 3,0, 
					     blr_field, 1, 3, 'G','I','D', 
					  blr_assignment, 
					     blr_parameter2, 2, 0,0, 2,0, 
					     blr_field, 1, 3, 'U','I','D', 
					  blr_end, 
			      blr_end, 
		     blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 2,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_37 */

static isc_req_handle
   isc_73 = 0;		/* request handle */

static const short
   isc_74l = 196;
static const char
   isc_74 [] = {
      blr_version5,
      blr_begin, 
	 blr_message, 0, 15,0, 
	    blr_long, 0, 
	    blr_long, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_cstring, 129,0, 
	    blr_cstring, 129,0, 
	    blr_cstring, 33,0, 
	    blr_cstring, 97,0, 
	    blr_cstring, 97,0, 
	    blr_cstring, 97,0, 
	 blr_receive, 0, 
	    blr_store, 
	       blr_relation, 5, 'U','S','E','R','S', 0, 
	       blr_begin, 
		  blr_assignment, 
		     blr_parameter2, 0, 14,0, 2,0, 
		     blr_field, 0, 9, 'L','A','S','T','_','N','A','M','E', 
		  blr_assignment, 
		     blr_parameter2, 0, 13,0, 3,0, 
		     blr_field, 0, 11, 'M','I','D','D','L','E','_','N','A','M','E', 
		  blr_assignment, 
		     blr_parameter2, 0, 12,0, 4,0, 
		     blr_field, 0, 10, 'F','I','R','S','T','_','N','A','M','E', 
		  blr_assignment, 
		     blr_parameter2, 0, 11,0, 5,0, 
		     blr_field, 0, 6, 'P','A','S','S','W','D', 
		  blr_assignment, 
		     blr_parameter2, 0, 10,0, 6,0, 
		     blr_field, 0, 10, 'G','R','O','U','P','_','N','A','M','E', 
		  blr_assignment, 
		     blr_parameter2, 0, 0,0, 7,0, 
		     blr_field, 0, 3, 'G','I','D', 
		  blr_assignment, 
		     blr_parameter2, 0, 1,0, 8,0, 
		     blr_field, 0, 3, 'U','I','D', 
		  blr_assignment, 
		     blr_parameter, 0, 9,0, 
		     blr_field, 0, 9, 'U','S','E','R','_','N','A','M','E', 
		  blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_74 */


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


#define MAX_PASSWORD_LENGTH	31
#define SYSDBA_USER_NAME	"SYSDBA"


SSHORT SECURITY_exec_line(ISC_STATUS* isc_status,
						  FRBRD* DB,
						  internal_user_data* io_user_data,
						  FPTR_SECURITY_CALLBACK display_func,
						  void* callback_arg)
{
   struct {
          long isc_5;	/* GID */
          long isc_6;	/* UID */
          short isc_7;	/* isc_utility */
          char  isc_8 [129];	/* USER_NAME */
          char  isc_9 [129];	/* GROUP_NAME */
          char  isc_10 [33];	/* PASSWD */
          char  isc_11 [97];	/* FIRST_NAME */
          char  isc_12 [97];	/* MIDDLE_NAME */
          char  isc_13 [97];	/* LAST_NAME */
   } isc_4;
   struct {
          char  isc_3 [129];	/* USER_NAME */
   } isc_2;
   struct {
          long isc_17;	/* GID */
          long isc_18;	/* UID */
          short isc_19;	/* isc_utility */
          char  isc_20 [129];	/* USER_NAME */
          char  isc_21 [129];	/* GROUP_NAME */
          char  isc_22 [33];	/* PASSWD */
          char  isc_23 [97];	/* FIRST_NAME */
          char  isc_24 [97];	/* MIDDLE_NAME */
          char  isc_25 [97];	/* LAST_NAME */
   } isc_16;
   struct {
          short isc_35;	/* isc_utility */
   } isc_34;
   struct {
          short isc_33;	/* isc_utility */
   } isc_32;
   struct {
          short isc_31;	/* isc_utility */
   } isc_30;
   struct {
          char  isc_29 [129];	/* USER_NAME */
   } isc_28;
   struct {
          short isc_72;	/* isc_utility */
   } isc_71;
   struct {
          long isc_57;	/* UID */
          long isc_58;	/* GID */
          short isc_59;	/* gds__null_flag */
          short isc_60;	/* gds__null_flag */
          short isc_61;	/* gds__null_flag */
          short isc_62;	/* gds__null_flag */
          short isc_63;	/* gds__null_flag */
          short isc_64;	/* gds__null_flag */
          short isc_65;	/* gds__null_flag */
          char  isc_66 [97];	/* LAST_NAME */
          char  isc_67 [97];	/* MIDDLE_NAME */
          char  isc_68 [97];	/* FIRST_NAME */
          char  isc_69 [33];	/* PASSWD */
          char  isc_70 [129];	/* GROUP_NAME */
   } isc_56;
   struct {
          long isc_41;	/* GID */
          long isc_42;	/* UID */
          short isc_43;	/* isc_utility */
          short isc_44;	/* gds__null_flag */
          short isc_45;	/* gds__null_flag */
          short isc_46;	/* gds__null_flag */
          short isc_47;	/* gds__null_flag */
          short isc_48;	/* gds__null_flag */
          short isc_49;	/* gds__null_flag */
          short isc_50;	/* gds__null_flag */
          char  isc_51 [129];	/* GROUP_NAME */
          char  isc_52 [33];	/* PASSWD */
          char  isc_53 [97];	/* FIRST_NAME */
          char  isc_54 [97];	/* MIDDLE_NAME */
          char  isc_55 [97];	/* LAST_NAME */
   } isc_40;
   struct {
          char  isc_39 [129];	/* USER_NAME */
   } isc_38;
   struct {
          long isc_76;	/* GID */
          long isc_77;	/* UID */
          short isc_78;	/* gds__null_flag */
          short isc_79;	/* gds__null_flag */
          short isc_80;	/* gds__null_flag */
          short isc_81;	/* gds__null_flag */
          short isc_82;	/* gds__null_flag */
          short isc_83;	/* gds__null_flag */
          short isc_84;	/* gds__null_flag */
          char  isc_85 [129];	/* USER_NAME */
          char  isc_86 [129];	/* GROUP_NAME */
          char  isc_87 [33];	/* PASSWD */
          char  isc_88 [97];	/* FIRST_NAME */
          char  isc_89 [97];	/* MIDDLE_NAME */
          char  isc_90 [97];	/* LAST_NAME */
   } isc_75;
/*************************************
 *
 *	S E C U R I T Y _ e x e c _ l i n e
 *
 **************************************
 *
 * Functional description
 *	Process a command line for the security data base manager.
 *	This is used to add and delete users from the user information
 *	database (security.fdb).   It also displays information
 *	about current users and allows modification of current
 *	users' parameters.   
 *	Returns 0 on success, otherwise returns a Gsec message number
 *	and the status vector containing the error info.
 *	The syntax is:
 *
 *	Adding a new user:
 *
 *	    gsec -add <name> [ <parameter> ... ]    -- command line
 *	    add <name> [ <parameter> ... ]          -- interactive
 *
 *	Deleting a current user:
 *
 *	    gsec -delete <name>     -- command line
 *	    delete <name>           -- interactive
 *
 *	Displaying all current users:
 *
 *	    gsec -display           -- command line
 *	    display                 -- interactive
 *
 *	Displaying one user:
 *
 *	    gsec -display <name>    -- command line
 *	    display <name>          -- interactive
 *
 *	Modifying a user's parameters:
 *
 *	    gsec -modify <name> <parameter> [ <parameter> ... ] -- command line
 *	    modify <name> <parameter> [ <parameter> ... ]       -- interactive
 *
 *	Get help:
 *
 *	    gsec -help              -- command line
 *	    ?                       -- interactive
 *	    help                    -- interactive
 *
 *	Quit interactive session:
 *
 *	    quit                    -- interactive
 *
 *	where <parameter> can be one of:
 *
 *	    -uid <uid>
 *	    -gid <gid>
 *	    -fname <firstname>
 *	    -mname <middlename>
 *	    -lname <lastname>
 *
 **************************************/
	SCHAR encrypted1[MAX_PASSWORD_LENGTH + 2];
	SCHAR encrypted2[MAX_PASSWORD_LENGTH + 2];
	ISC_STATUS_ARRAY tmp_status;
	FRBRD *gds_trans = NULL;
	bool found;
	SSHORT ret = 0;

	// check for non-printable characters in user name
	for (const TEXT *p = io_user_data->user_name; *p; p++) {
		if (! isprint(*p)) {
			return GsecMsg75;  // Add special error message for this case ?
		}
	}

	/*START_TRANSACTION*/
	{
	{
	isc_start_transaction (isc_status, &gds_trans, (short) 1, &DB, (short) 0, (char*) 0);
	};
	/*ON_ERROR*/
	if (isc_status [1])
	   {
		return GsecMsg75;		/* gsec error */
	/*END_ERROR;*/
	   }
	}

	switch (io_user_data->operation) {
	case ADD_OPER:
		/* this checks the "entered" flags for each parameter (except the name)
		   and makes all non-entered parameters null valued */

		/*STORE U IN USERS USING*/
		{
		
                if (!isc_73)
                   isc_compile_request2 (isc_status, &DB, &isc_73, (short) sizeof (isc_74), (char ISC_FAR *) isc_74);
		if (isc_73)
		   {
			strcpy(/*U.USER_NAME*/
			       isc_75.isc_85, io_user_data->user_name);
			if (io_user_data->uid_entered) {
				/*U.UID*/
				isc_75.isc_77 = io_user_data->uid;
				/*U.UID.NULL*/
				isc_75.isc_84 = ISC_FALSE;
			}
			else
				/*U.UID.NULL*/
				isc_75.isc_84 = ISC_TRUE;
			if (io_user_data->gid_entered) {
				/*U.GID*/
				isc_75.isc_76 = io_user_data->gid;
				/*U.GID.NULL*/
				isc_75.isc_83 = ISC_FALSE;
			}
			else
				/*U.GID.NULL*/
				isc_75.isc_83 = ISC_TRUE;
			if (io_user_data->group_name_entered) {
				strcpy(/*U.GROUP_NAME*/
				       isc_75.isc_86, io_user_data->group_name);
				/*U.GROUP_NAME.NULL*/
				isc_75.isc_82 = ISC_FALSE;
			}
			else
				/*U.GROUP_NAME.NULL*/
				isc_75.isc_82 = ISC_TRUE;
			if (io_user_data->password_entered) {
				strcpy(encrypted1,
					   ENC_crypt(io_user_data->password, PASSWORD_SALT));
				strcpy(encrypted2, ENC_crypt(&encrypted1[2], PASSWORD_SALT));
				strcpy(/*U.PASSWD*/
				       isc_75.isc_87, &encrypted2[2]);
				/*U.PASSWD.NULL*/
				isc_75.isc_81 = ISC_FALSE;
			}
			else
				/*U.PASSWD.NULL*/
				isc_75.isc_81 = ISC_TRUE;
			if (io_user_data->first_name_entered) {
				strcpy(/*U.FIRST_NAME*/
				       isc_75.isc_88, io_user_data->first_name);
				/*U.FIRST_NAME.NULL*/
				isc_75.isc_80 = ISC_FALSE;
			}
			else
				/*U.FIRST_NAME.NULL*/
				isc_75.isc_80 = ISC_TRUE;
			if (io_user_data->middle_name_entered) {
				strcpy(/*U.MIDDLE_NAME*/
				       isc_75.isc_89, io_user_data->middle_name);
				/*U.MIDDLE_NAME.NULL*/
				isc_75.isc_79 = ISC_FALSE;
			}
			else
				/*U.MIDDLE_NAME.NULL*/
				isc_75.isc_79 = ISC_TRUE;
			if (io_user_data->last_name_entered) {
				strcpy(/*U.LAST_NAME*/
				       isc_75.isc_90, io_user_data->last_name);
				/*U.LAST_NAME.NULL*/
				isc_75.isc_78 = ISC_FALSE;
			}
			else
				/*U.LAST_NAME.NULL*/
				isc_75.isc_78 = ISC_TRUE;
		/*END_STORE*/
		   
                   isc_start_and_send (isc_status, &isc_73, &gds_trans, (short) 0, (short) 604, &isc_75, (short) 0);
		   };
		/*ON_ERROR*/
		if (isc_status [1])
		   {
			ret = GsecMsg19;	/* gsec - add record error */
		/*END_ERROR;*/
		   }
		}
		break;

	case MOD_OPER:
		/* this updates an existing record, replacing all fields that are
		   entered, and for those that were specified but not entered, it
		   changes the current value to the null value */

		found = false;
		/*FOR U IN USERS WITH U.USER_NAME EQ io_user_data->user_name*/
		{
                if (!isc_36)
                   isc_compile_request2 (isc_status, &DB, &isc_36, (short) sizeof (isc_37), (char ISC_FAR *) isc_37);
		isc_vtov (io_user_data->user_name, isc_38.isc_39, 129);
		if (isc_36)
                   isc_start_and_send (isc_status, &isc_36, &gds_trans, (short) 0, (short) 129, &isc_38, (short) 0);
		if (!isc_status [1]) {
		while (1)
		   {
                   isc_receive (isc_status, &isc_36, (short) 1, (short) 477, &isc_40, (short) 0);
		   if (!isc_40.isc_43 || isc_status [1]) break;
			found = true;
			/*MODIFY U USING*/
			{
				if (io_user_data->uid_entered) {
					/*U.UID*/
					isc_40.isc_42 = io_user_data->uid;
					/*U.UID.NULL*/
					isc_40.isc_50 = ISC_FALSE;
				}
				else if (io_user_data->uid_specified)
					/*U.UID.NULL*/
					isc_40.isc_50 = ISC_TRUE;
				if (io_user_data->gid_entered) {
					/*U.GID*/
					isc_40.isc_41 = io_user_data->gid;
					/*U.GID.NULL*/
					isc_40.isc_49 = ISC_FALSE;
				}
				else if (io_user_data->gid_specified)
					/*U.GID.NULL*/
					isc_40.isc_49 = ISC_TRUE;
				if (io_user_data->group_name_entered) {
					strcpy(/*U.GROUP_NAME*/
					       isc_40.isc_51, io_user_data->group_name);
					/*U.GROUP_NAME.NULL*/
					isc_40.isc_48 = ISC_FALSE;
				}
				else if (io_user_data->group_name_specified)
					/*U.GROUP_NAME.NULL*/
					isc_40.isc_48 = ISC_TRUE;
				if (io_user_data->password_entered) {
					strcpy(encrypted1,
						   ENC_crypt(io_user_data->password, PASSWORD_SALT));
					strcpy(encrypted2,
						   ENC_crypt(&encrypted1[2], PASSWORD_SALT));
					strcpy(/*U.PASSWD*/
					       isc_40.isc_52, &encrypted2[2]);
					/*U.PASSWD.NULL*/
					isc_40.isc_47 = ISC_FALSE;
				}
				else if (io_user_data->password_specified)
					/*U.PASSWD.NULL*/
					isc_40.isc_47 = ISC_TRUE;
				if (io_user_data->first_name_entered) {
					strcpy(/*U.FIRST_NAME*/
					       isc_40.isc_53, io_user_data->first_name);
					/*U.FIRST_NAME.NULL*/
					isc_40.isc_46 = ISC_FALSE;
				}
				else if (io_user_data->first_name_specified)
					/*U.FIRST_NAME.NULL*/
					isc_40.isc_46 = ISC_TRUE;
				if (io_user_data->middle_name_entered) {
					strcpy(/*U.MIDDLE_NAME*/
					       isc_40.isc_54, io_user_data->middle_name);
					/*U.MIDDLE_NAME.NULL*/
					isc_40.isc_45 = ISC_FALSE;
				}
				else if (io_user_data->middle_name_specified)
					/*U.MIDDLE_NAME.NULL*/
					isc_40.isc_45 = ISC_TRUE;
				if (io_user_data->last_name_entered) {
					strcpy(/*U.LAST_NAME*/
					       isc_40.isc_55, io_user_data->last_name);
					/*U.LAST_NAME.NULL*/
					isc_40.isc_44 = ISC_FALSE;
				}
				else if (io_user_data->last_name_specified)
					/*U.LAST_NAME.NULL*/
					isc_40.isc_44 = ISC_TRUE;
			/*END_MODIFY*/
			isc_56.isc_57 = isc_40.isc_42;
			isc_56.isc_58 = isc_40.isc_41;
			isc_56.isc_59 = isc_40.isc_50;
			isc_56.isc_60 = isc_40.isc_49;
			isc_56.isc_61 = isc_40.isc_48;
			isc_56.isc_62 = isc_40.isc_47;
			isc_56.isc_63 = isc_40.isc_46;
			isc_56.isc_64 = isc_40.isc_45;
			isc_56.isc_65 = isc_40.isc_44;
			isc_vtov (isc_40.isc_55, isc_56.isc_66, 97);
			isc_vtov (isc_40.isc_54, isc_56.isc_67, 97);
			isc_vtov (isc_40.isc_53, isc_56.isc_68, 97);
			isc_vtov (isc_40.isc_52, isc_56.isc_69, 33);
			isc_vtov (isc_40.isc_51, isc_56.isc_70, 129);
                        isc_send (isc_status, &isc_36, (short) 2, (short) 475, &isc_56, (short) 0);;
			/*ON_ERROR*/
			if (isc_status [1])
			   {
				ret = GsecMsg20;
			/*END_ERROR;*/
			   }
			}
		/*END_FOR*/
                   isc_send (isc_status, &isc_36, (short) 3, (short) 2, &isc_71, (short) 0);
		   }
		   };
		/*ON_ERROR*/
		if (isc_status [1])
		   {
			ret = GsecMsg21;
		/*END_ERROR;*/
		   }
		}
		if (!ret && !found)
			ret = GsecMsg22;
		break;

	case DEL_OPER:
		/* looks up the specified user record and deletes it */

		found = false;
		/* Do not allow SYSDBA user to be deleted */
#ifdef HAVE_STRCASECMP
		if (!strcasecmp(io_user_data->user_name, SYSDBA_USER_NAME))
#else
#ifdef HAVE_STRICMP
		if (!stricmp(io_user_data->user_name, SYSDBA_USER_NAME))
#else
#error dont know how to compare strings case insensitive on this system
#endif /* HAVE_STRICMP */
#endif /* HAVE_STRCASECMP */
			ret = GsecMsg23;
		else {
			/*FOR U IN USERS WITH U.USER_NAME EQ io_user_data->user_name*/
			{
                        if (!isc_26)
                           isc_compile_request2 (isc_status, &DB, &isc_26, (short) sizeof (isc_27), (char ISC_FAR *) isc_27);
			isc_vtov (io_user_data->user_name, isc_28.isc_29, 129);
			if (isc_26)
                           isc_start_and_send (isc_status, &isc_26, &gds_trans, (short) 0, (short) 129, &isc_28, (short) 0);
			if (!isc_status [1]) {
			while (1)
			   {
                           isc_receive (isc_status, &isc_26, (short) 1, (short) 2, &isc_30, (short) 0);
			   if (!isc_30.isc_31 || isc_status [1]) break;
				found = true;
				/*ERASE U*/
				{
                                isc_send (isc_status, &isc_26, (short) 2, (short) 2, &isc_32, (short) 0);
				/*ON_ERROR*/
				if (isc_status [1])
				   {
					ret = GsecMsg23;	/* gsec - delete record error */
				/*END_ERROR;*/
				   }
				}
			/*END_FOR*/
                           isc_send (isc_status, &isc_26, (short) 3, (short) 2, &isc_34, (short) 0);
			   }
			   };
			/*ON_ERROR*/
			if (isc_status [1])
			   {
				ret = GsecMsg24;	/* gsec - find/delete record error */
			/*END_ERROR;*/
			   }
			}
		}

		if (!ret && !found)
			ret = GsecMsg22;	/* gsec - record not found for user: */
		break;

	case DIS_OPER:
		/* gets either the desired record, or all records, and displays them */

		found = false;
		if (!io_user_data->user_name_entered) {
			/*FOR U IN USERS*/
			{
                        if (!isc_14)
                           isc_compile_request2 (isc_status, &DB, &isc_14, (short) sizeof (isc_15), (char ISC_FAR *) isc_15);
			if (isc_14)
                           isc_start_request (isc_status, &isc_14, &gds_trans, (short) 0);
			if (!isc_status [1]) {
			while (1)
			   {
                           isc_receive (isc_status, &isc_14, (short) 0, (short) 592, &isc_16, (short) 0);
			   if (!isc_16.isc_19 || isc_status [1]) break;
				io_user_data->uid = /*U.UID*/
						    isc_16.isc_18;
				io_user_data->gid = /*U.GID*/
						    isc_16.isc_17;
				*(io_user_data->sys_user_name) = '\0';
				strcpy(io_user_data->user_name, /*U.USER_NAME*/
								isc_16.isc_20);
				strcpy(io_user_data->group_name, /*U.GROUP_NAME*/
								 isc_16.isc_21);
				strcpy(io_user_data->password, /*U.PASSWD*/
							       isc_16.isc_22);
				strcpy(io_user_data->first_name, /*U.FIRST_NAME*/
								 isc_16.isc_23);
				strcpy(io_user_data->middle_name, /*U.MIDDLE_NAME*/
								  isc_16.isc_24);
				strcpy(io_user_data->last_name, /*U.LAST_NAME*/
								isc_16.isc_25);
				display_func(callback_arg, io_user_data, !found);

				found = true;
			/*END_FOR*/
			   }
			   };
			/*ON_ERROR*/
			if (isc_status [1])
			   {
				ret = GsecMsg28;	/* gsec - find/display record error */
			/*END_ERROR;*/
			   }
			}
		}
		else {
			/*FOR U IN USERS WITH U.USER_NAME EQ io_user_data->user_name*/
			{
                        if (!isc_0)
                           isc_compile_request2 (isc_status, &DB, &isc_0, (short) sizeof (isc_1), (char ISC_FAR *) isc_1);
			isc_vtov (io_user_data->user_name, isc_2.isc_3, 129);
			if (isc_0)
                           isc_start_and_send (isc_status, &isc_0, &gds_trans, (short) 0, (short) 129, &isc_2, (short) 0);
			if (!isc_status [1]) {
			while (1)
			   {
                           isc_receive (isc_status, &isc_0, (short) 1, (short) 592, &isc_4, (short) 0);
			   if (!isc_4.isc_7 || isc_status [1]) break;
				io_user_data->uid = /*U.UID*/
						    isc_4.isc_6;
				io_user_data->gid = /*U.GID*/
						    isc_4.isc_5;
				*(io_user_data->sys_user_name) = '\0';
				strcpy(io_user_data->user_name, /*U.USER_NAME*/
								isc_4.isc_8);
				strcpy(io_user_data->group_name, /*U.GROUP_NAME*/
								 isc_4.isc_9);
				strcpy(io_user_data->password, /*U.PASSWD*/
							       isc_4.isc_10);
				strcpy(io_user_data->first_name, /*U.FIRST_NAME*/
								 isc_4.isc_11);
				strcpy(io_user_data->middle_name, /*U.MIDDLE_NAME*/
								  isc_4.isc_12);
				strcpy(io_user_data->last_name, /*U.LAST_NAME*/
								isc_4.isc_13);
				display_func(callback_arg, io_user_data, !found);

				found = true;
			/*END_FOR*/
			   }
			   };
			/*ON_ERROR*/
			if (isc_status [1])
			   {
				ret = GsecMsg28;	/* gsec - find/display record error */
			/*END_ERROR;*/
			   }
			}
		}
		break;

	default:
		ret = GsecMsg16;		/* gsec - error in switch specifications */
		break;
	}

/* rollback if we have an error using tmp_status to avoid 
   overwriting the error status which the caller wants to see */

	if (ret)
		isc_rollback_transaction(tmp_status, &gds_trans);
	else {
		/*COMMIT*/
		{
		isc_commit_transaction (isc_status, &gds_trans);;
		/*ON_ERROR*/
		if (isc_status [1])
		   {
			return GsecMsg75;	/* gsec error */
		/*END_ERROR;*/
		   }
		}
	}

	return ret;
}

void SECURITY_get_db_path(const TEXT* server, TEXT* buffer)
{
/**************************************
 *
 *	S E C U R I T Y _ g e t _ d b _ p a t h
 *
 **************************************
 *
 * Functional description
 *	Gets the path to the security database
 *	from server	
 *
 *  NOTE:  Be sure that server specifies the 
 *         protocol being used to connect.
 *
 *         for example:  
 *		server:
 *         would connect via tcpip
 *
 **************************************/
#define RESPBUF	256
	ISC_STATUS_ARRAY status;
	isc_svc_handle svc_handle = NULL;
	TEXT svc_name[256];
	const TEXT sendbuf[] = { isc_info_svc_user_dbpath };
	TEXT respbuf[RESPBUF], *p = respbuf;
	USHORT path_length;

/* Whatever is defined for a given platform as a name for
   the security database is used as a default in case we
   are unable to get the path from server
*/

/* to get rid of these "security.fdb not found" messges add 
   the path to USER_INFO NAME FSG june 30 2001 */
	gds__prefix (buffer, USER_INFO_NAME);

	if (server)
		sprintf(svc_name, "%sanonymous", server);
	else
#ifdef WIN_NT
		sprintf(svc_name, "anonymous");
#else
		sprintf(svc_name, "localhost:anonymous");
#endif

	if (isc_service_attach(status, 0, svc_name, &svc_handle, 0, NULL))
		return;

	if (isc_service_query
		(status, &svc_handle, NULL, 0, NULL, 1, sendbuf, RESPBUF, respbuf)) {
		isc_service_detach(status, &svc_handle);
		return;
	}

	if (*p == isc_info_svc_user_dbpath) {
		p++;
		path_length = (USHORT) isc_vax_integer(p, sizeof(short));
		p += sizeof(USHORT);
		strncpy(buffer, p, path_length);
		buffer[path_length] = '\0';
	}

	isc_service_detach(status, &svc_handle);
}

void SECURITY_msg_get(USHORT number, TEXT* msg)
{
/**************************************
 *
 *	S E C U R I T Y _ m s g _ g e t
 *
 **************************************
 *
 * Functional description
 *	Retrieve a message from the error file
 *
 **************************************/

	gds__msg_format(NULL, GSEC_MSG_FAC, number, MSG_LENGTH, msg,
					NULL, NULL, NULL, NULL, NULL);
}


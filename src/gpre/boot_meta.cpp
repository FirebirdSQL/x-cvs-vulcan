/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-V2.0.0.4027 Vulcan 1.0 Development **********************/
#line 1 "gpre_meta.epp"
/*
 * tab=4
 *____________________________________________________________
 *  
 *		PROGRAM:	C preprocessor
 *		MODULE:		gpre_meta.epp
 *		DESCRIPTION:	Meta data interface to system
 *  
 *  The contents of this file are subject to the Interbase Public
 *  License Version 1.0 (the "License"); you may not use this file
 *  except in compliance with the License. You may obtain a copy
 *  of the License at http://www.Inprise.com/IPL.html
 *  
 *  Software distributed under the License is distributed on an
 *  "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 *  or implied. See the License for the specific language governing
 *  rights and limitations under the License.
 *  
 *  The Original Code was created by Inprise Corporation
 *  and its predecessors. Portions created by Inprise Corporation are
 *  Copyright (C) Inprise Corporation.
 *  
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *  
 *
 *____________________________________________________________
 *
 *	$Id$
 */

#include "fbdev.h"
#include <string.h>
#include "../jrd/gds.h"
#include "../gpre/gpre.h"
#include "../jrd/license.h"
#include "../gpre/parse.h"
#include "../jrd/intl.h"
#include "../gpre/gpre_proto.h"
#include "../gpre/hsh_proto.h"
#include "../gpre/jrdme_proto.h"
#include "../gpre/gpre_meta.h"
#include "../gpre/msc_proto.h"
#include "gpre_par_proto.h"
#include "../jrd/constants.h"

#define MAX_USER_LENGTH		33
#define MAX_PASSWORD_LENGTH	33

/*DATABASE DB = FILENAME "yachts.lnk" RUNTIME dbb->dbb_filename;*/
/**** GDS Preprocessor Definitions ****/
#ifndef JRD_IBASE_H
#include <ibase.h>
#endif

static ISC_QUAD
   isc_blob_null = {0,0};	/* initializer for blobs */
isc_db_handle
   DB = 0;		/* database handle */

isc_tr_handle
   gds_trans = 0;		/* default transaction handle */
ISC_STATUS
   isc_status [20],	/* status vector */
   isc_status2 [20];	/* status vector */
SLONG
   isc_array_length, 	/* array return size */
   SQLCODE;		/* SQL status code */
static short
   isc_0l = 324;
static char
   isc_0 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 2,0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_message, 0, 2,0, 
	    blr_cstring, 32,0, 
	    blr_cstring, 32,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 3, 
		     blr_relation, 18, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','S', 0, 
		     blr_relation, 14, 'R','D','B','$','C','O','L','L','A','T','I','O','N','S', 1, 
		     blr_relation, 9, 'R','D','B','$','T','Y','P','E','S', 2, 
		     blr_first, 
			blr_literal, blr_long, 0, 1,0,0,0,
		     blr_boolean, 
			blr_and, 
			   blr_eql, 
			      blr_field, 1, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','I','D', 
			      blr_field, 0, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','I','D', 
			   blr_and, 
			      blr_eql, 
				 blr_field, 1, 18, 'R','D','B','$','C','O','L','L','A','T','I','O','N','_','N','A','M','E', 
				 blr_parameter, 0, 1,0, 
			      blr_and, 
				 blr_eql, 
				    blr_field, 2, 13, 'R','D','B','$','T','Y','P','E','_','N','A','M','E', 
				    blr_parameter, 0, 0,0, 
				 blr_and, 
				    blr_eql, 
				       blr_field, 2, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
				       blr_literal, blr_text, 22,0, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','N','A','M','E',
				    blr_eql, 
				       blr_field, 2, 8, 'R','D','B','$','T','Y','P','E', 
				       blr_field, 0, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','I','D', 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_field, 0, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','I','D', 
			   blr_parameter, 1, 1,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 0,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_0 */

static short
   isc_7l = 206;
static char
   isc_7 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 2,0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_message, 0, 1,0, 
	    blr_cstring, 32,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 2, 
		     blr_relation, 18, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','S', 0, 
		     blr_relation, 14, 'R','D','B','$','C','O','L','L','A','T','I','O','N','S', 1, 
		     blr_first, 
			blr_literal, blr_long, 0, 1,0,0,0,
		     blr_boolean, 
			blr_and, 
			   blr_eql, 
			      blr_field, 1, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','I','D', 
			      blr_field, 0, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','I','D', 
			   blr_eql, 
			      blr_field, 1, 18, 'R','D','B','$','C','O','L','L','A','T','I','O','N','_','N','A','M','E', 
			      blr_parameter, 0, 0,0, 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_field, 0, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','I','D', 
			   blr_parameter, 1, 1,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 0,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_7 */

static short
   isc_13l = 296;
static char
   isc_13 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 2,0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_message, 0, 1,0, 
	    blr_cstring, 32,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 3, 
		     blr_relation, 18, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','S', 0, 
		     blr_relation, 14, 'R','D','B','$','C','O','L','L','A','T','I','O','N','S', 1, 
		     blr_relation, 9, 'R','D','B','$','T','Y','P','E','S', 2, 
		     blr_first, 
			blr_literal, blr_long, 0, 1,0,0,0,
		     blr_boolean, 
			blr_and, 
			   blr_eql, 
			      blr_field, 2, 13, 'R','D','B','$','T','Y','P','E','_','N','A','M','E', 
			      blr_parameter, 0, 0,0, 
			   blr_and, 
			      blr_eql, 
				 blr_field, 2, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
				 blr_literal, blr_text, 22,0, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','N','A','M','E',
			      blr_and, 
				 blr_eql, 
				    blr_field, 2, 8, 'R','D','B','$','T','Y','P','E', 
				    blr_field, 0, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','I','D', 
				 blr_eql, 
				    blr_field, 0, 24, 'R','D','B','$','D','E','F','A','U','L','T','_','C','O','L','L','A','T','E','_','N','A','M','E', 
				    blr_field, 1, 18, 'R','D','B','$','C','O','L','L','A','T','I','O','N','_','N','A','M','E', 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_field, 0, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','I','D', 
			   blr_parameter, 1, 1,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 0,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_13 */

static short
   isc_19l = 162;
static char
   isc_19 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 0, 2,0, 
	    blr_cstring, 32,0, 
	    blr_short, 0, 
	 blr_begin, 
	    blr_for, 
	       blr_rse, 1, 
		  blr_relation, 12, 'R','D','B','$','D','A','T','A','B','A','S','E', 0, 
		  blr_first, 
		     blr_literal, blr_long, 0, 1,0,0,0,
		  blr_boolean, 
		     blr_and, 
			blr_not, 
			   blr_missing, 
			      blr_field, 0, 22, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','N','A','M','E', 
			blr_neq, 
			   blr_field, 0, 22, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','N','A','M','E', 
			   blr_literal, blr_text, 1,0, 32,
		  blr_end, 
	       blr_send, 0, 
		  blr_begin, 
		     blr_assignment, 
			blr_field, 0, 22, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','N','A','M','E', 
			blr_parameter, 0, 0,0, 
		     blr_assignment, 
			blr_literal, blr_long, 0, 1,0,0,0,
			blr_parameter, 0, 1,0, 
		     blr_end, 
	    blr_send, 0, 
	       blr_assignment, 
		  blr_literal, blr_long, 0, 0,0,0,0,
		  blr_parameter, 0, 1,0, 
	    blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_19 */

static short
   isc_23l = 193;
static char
   isc_23 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 4,0, 
	    blr_long, 0, 
	    blr_long, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_message, 0, 1,0, 
	    blr_cstring, 32,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 1, 
		     blr_relation, 20, 'R','D','B','$','F','I','E','L','D','_','D','I','M','E','N','S','I','O','N','S', 0, 
		     blr_boolean, 
			blr_eql, 
			   blr_field, 0, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
			   blr_parameter, 0, 0,0, 
		     blr_sort, 1, 
			blr_ascending, 
			   blr_field, 0, 13, 'R','D','B','$','D','I','M','E','N','S','I','O','N', 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_field, 0, 15, 'R','D','B','$','U','P','P','E','R','_','B','O','U','N','D', 
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_field, 0, 15, 'R','D','B','$','L','O','W','E','R','_','B','O','U','N','D', 
			   blr_parameter, 1, 1,0, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 2,0, 
			blr_assignment, 
			   blr_field, 0, 13, 'R','D','B','$','D','I','M','E','N','S','I','O','N', 
			   blr_parameter, 1, 3,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 2,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_23 */

static short
   isc_31l = 115;
static char
   isc_31 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 2,0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_message, 0, 1,0, 
	    blr_cstring, 32,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 1, 
		     blr_relation, 10, 'R','D','B','$','F','I','E','L','D','S', 0, 
		     blr_boolean, 
			blr_eql, 
			   blr_field, 0, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
			   blr_parameter, 0, 0,0, 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_field, 0, 14, 'R','D','B','$','D','I','M','E','N','S','I','O','N','S', 
			   blr_parameter, 1, 1,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 0,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_31 */

static short
   isc_37l = 95;
static char
   isc_37 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 1,0, 
	    blr_short, 0, 
	 blr_message, 0, 1,0, 
	    blr_cstring, 32,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 1, 
		     blr_relation, 12, 'R','D','B','$','T','R','I','G','G','E','R','S', 0, 
		     blr_boolean, 
			blr_eql, 
			   blr_field, 0, 16, 'R','D','B','$','T','R','I','G','G','E','R','_','N','A','M','E', 
			   blr_parameter, 0, 0,0, 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 0,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 0,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_37 */

static short
   isc_42l = 133;
static char
   isc_42 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 2,0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_message, 0, 2,0, 
	    blr_cstring, 32,0, 
	    blr_cstring, 32,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 1, 
		     blr_relation, 9, 'R','D','B','$','T','Y','P','E','S', 0, 
		     blr_boolean, 
			blr_and, 
			   blr_eql, 
			      blr_field, 0, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
			      blr_parameter, 0, 1,0, 
			   blr_eql, 
			      blr_field, 0, 13, 'R','D','B','$','T','Y','P','E','_','N','A','M','E', 
			      blr_parameter, 0, 0,0, 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_field, 0, 8, 'R','D','B','$','T','Y','P','E', 
			   blr_parameter, 1, 1,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 0,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_42 */

static short
   isc_49l = 92;
static char
   isc_49 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 0, 2,0, 
	    blr_cstring, 32,0, 
	    blr_short, 0, 
	 blr_begin, 
	    blr_for, 
	       blr_rse, 1, 
		  blr_relation, 14, 'R','D','B','$','G','E','N','E','R','A','T','O','R','S', 0, 
		  blr_end, 
	       blr_send, 0, 
		  blr_begin, 
		     blr_assignment, 
			blr_field, 0, 18, 'R','D','B','$','G','E','N','E','R','A','T','O','R','_','N','A','M','E', 
			blr_parameter, 0, 0,0, 
		     blr_assignment, 
			blr_literal, blr_long, 0, 1,0,0,0,
			blr_parameter, 0, 1,0, 
		     blr_end, 
	    blr_send, 0, 
	       blr_assignment, 
		  blr_literal, blr_long, 0, 0,0,0,0,
		  blr_parameter, 0, 1,0, 
	    blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_49 */

static short
   isc_53l = 130;
static char
   isc_53 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 0, 2,0, 
	    blr_cstring, 32,0, 
	    blr_short, 0, 
	 blr_begin, 
	    blr_for, 
	       blr_rse, 1, 
		  blr_relation, 12, 'R','D','B','$','D','A','T','A','B','A','S','E', 0, 
		  blr_first, 
		     blr_literal, blr_long, 0, 1,0,0,0,
		  blr_boolean, 
		     blr_not, 
			blr_missing, 
			   blr_field, 0, 22, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','N','A','M','E', 
		  blr_end, 
	       blr_send, 0, 
		  blr_begin, 
		     blr_assignment, 
			blr_field, 0, 22, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','N','A','M','E', 
			blr_parameter, 0, 0,0, 
		     blr_assignment, 
			blr_literal, blr_long, 0, 1,0,0,0,
			blr_parameter, 0, 1,0, 
		     blr_end, 
	    blr_send, 0, 
	       blr_assignment, 
		  blr_literal, blr_long, 0, 0,0,0,0,
		  blr_parameter, 0, 1,0, 
	    blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_53 */

static short
   isc_57l = 177;
static char
   isc_57 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 2,0, 
	    blr_cstring, 32,0, 
	    blr_short, 0, 
	 blr_message, 0, 2,0, 
	    blr_cstring, 32,0, 
	    blr_short, 0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 1, 
		     blr_relation, 9, 'R','D','B','$','T','Y','P','E','S', 0, 
		     blr_boolean, 
			blr_and, 
			   blr_eql, 
			      blr_field, 0, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
			      blr_literal, blr_text, 22,0, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','N','A','M','E',
			   blr_and, 
			      blr_eql, 
				 blr_field, 0, 8, 'R','D','B','$','T','Y','P','E', 
				 blr_parameter, 0, 1,0, 
			      blr_neq, 
				 blr_field, 0, 13, 'R','D','B','$','T','Y','P','E','_','N','A','M','E', 
				 blr_parameter, 0, 0,0, 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_field, 0, 13, 'R','D','B','$','T','Y','P','E','_','N','A','M','E', 
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 1,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 1,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_57 */

static short
   isc_64l = 290;
static char
   isc_64 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 0, 7,0, 
	    blr_cstring, 32,0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_begin, 
	    blr_for, 
	       blr_rse, 2, 
		  blr_relation, 18, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','S', 0, 
		  blr_relation, 14, 'R','D','B','$','C','O','L','L','A','T','I','O','N','S', 1, 
		  blr_boolean, 
		     blr_eql, 
			blr_field, 0, 24, 'R','D','B','$','D','E','F','A','U','L','T','_','C','O','L','L','A','T','E','_','N','A','M','E', 
			blr_field, 1, 18, 'R','D','B','$','C','O','L','L','A','T','I','O','N','_','N','A','M','E', 
		  blr_end, 
	       blr_send, 0, 
		  blr_begin, 
		     blr_assignment, 
			blr_field, 0, 22, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','N','A','M','E', 
			blr_parameter, 0, 0,0, 
		     blr_assignment, 
			blr_literal, blr_long, 0, 1,0,0,0,
			blr_parameter, 0, 1,0, 
		     blr_assignment, 
			blr_field, 0, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','I','D', 
			blr_parameter, 0, 2,0, 
		     blr_assignment, 
			blr_field, 0, 23, 'R','D','B','$','B','Y','T','E','S','_','P','E','R','_','C','H','A','R','A','C','T','E','R', 
			blr_parameter2, 0, 4,0, 3,0, 
		     blr_assignment, 
			blr_field, 1, 16, 'R','D','B','$','C','O','L','L','A','T','I','O','N','_','I','D', 
			blr_parameter, 0, 5,0, 
		     blr_assignment, 
			blr_field, 1, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','I','D', 
			blr_parameter, 0, 6,0, 
		     blr_end, 
	    blr_send, 0, 
	       blr_assignment, 
		  blr_literal, blr_long, 0, 0,0,0,0,
		  blr_parameter, 0, 1,0, 
	    blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_64 */

static short
   isc_73l = 173;
static char
   isc_73 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 2,0, 
	    blr_cstring, 32,0, 
	    blr_short, 0, 
	 blr_message, 0, 2,0, 
	    blr_cstring, 32,0, 
	    blr_short, 0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 1, 
		     blr_relation, 9, 'R','D','B','$','T','Y','P','E','S', 0, 
		     blr_boolean, 
			blr_and, 
			   blr_eql, 
			      blr_field, 0, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
			      blr_literal, blr_text, 18,0, 'R','D','B','$','C','O','L','L','A','T','I','O','N','_','N','A','M','E',
			   blr_and, 
			      blr_eql, 
				 blr_field, 0, 8, 'R','D','B','$','T','Y','P','E', 
				 blr_parameter, 0, 1,0, 
			      blr_neq, 
				 blr_field, 0, 13, 'R','D','B','$','T','Y','P','E','_','N','A','M','E', 
				 blr_parameter, 0, 0,0, 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_field, 0, 13, 'R','D','B','$','T','Y','P','E','_','N','A','M','E', 
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 1,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 1,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_73 */

static short
   isc_80l = 254;
static char
   isc_80 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 0, 6,0, 
	    blr_cstring, 32,0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_begin, 
	    blr_for, 
	       blr_rse, 2, 
		  blr_relation, 18, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','S', 0, 
		  blr_relation, 14, 'R','D','B','$','C','O','L','L','A','T','I','O','N','S', 1, 
		  blr_boolean, 
		     blr_eql, 
			blr_field, 1, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','I','D', 
			blr_field, 0, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','I','D', 
		  blr_end, 
	       blr_send, 0, 
		  blr_begin, 
		     blr_assignment, 
			blr_field, 1, 18, 'R','D','B','$','C','O','L','L','A','T','I','O','N','_','N','A','M','E', 
			blr_parameter, 0, 0,0, 
		     blr_assignment, 
			blr_literal, blr_long, 0, 1,0,0,0,
			blr_parameter, 0, 1,0, 
		     blr_assignment, 
			blr_field, 0, 23, 'R','D','B','$','B','Y','T','E','S','_','P','E','R','_','C','H','A','R','A','C','T','E','R', 
			blr_parameter2, 0, 3,0, 2,0, 
		     blr_assignment, 
			blr_field, 1, 16, 'R','D','B','$','C','O','L','L','A','T','I','O','N','_','I','D', 
			blr_parameter, 0, 4,0, 
		     blr_assignment, 
			blr_field, 1, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','I','D', 
			blr_parameter, 0, 5,0, 
		     blr_end, 
	    blr_send, 0, 
	       blr_assignment, 
		  blr_literal, blr_long, 0, 0,0,0,0,
		  blr_parameter, 0, 1,0, 
	    blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_80 */

static short
   isc_88l = 356;
static char
   isc_88 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 3,0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_message, 0, 2,0, 
	    blr_cstring, 32,0, 
	    blr_short, 0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 3, 
		     blr_relation, 22, 'R','D','B','$','F','U','N','C','T','I','O','N','_','A','R','G','U','M','E','N','T','S', 0, 
		     blr_relation, 18, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','S', 1, 
		     blr_relation, 14, 'R','D','B','$','C','O','L','L','A','T','I','O','N','S', 2, 
		     blr_boolean, 
			blr_and, 
			   blr_eql, 
			      blr_field, 0, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','I','D', 
			      blr_field, 1, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','I','D', 
			   blr_and, 
			      blr_eql, 
				 blr_field, 2, 18, 'R','D','B','$','C','O','L','L','A','T','I','O','N','_','N','A','M','E', 
				 blr_field, 1, 24, 'R','D','B','$','D','E','F','A','U','L','T','_','C','O','L','L','A','T','E','_','N','A','M','E', 
			      blr_and, 
				 blr_not, 
				    blr_missing, 
				       blr_field, 0, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','I','D', 
				 blr_and, 
				    blr_eql, 
				       blr_field, 0, 17, 'R','D','B','$','F','U','N','C','T','I','O','N','_','N','A','M','E', 
				       blr_parameter, 0, 0,0, 
				    blr_eql, 
				       blr_field, 0, 21, 'R','D','B','$','A','R','G','U','M','E','N','T','_','P','O','S','I','T','I','O','N', 
				       blr_parameter, 0, 1,0, 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_field, 2, 16, 'R','D','B','$','C','O','L','L','A','T','I','O','N','_','I','D', 
			   blr_parameter, 1, 1,0, 
			blr_assignment, 
			   blr_field, 0, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','I','D', 
			   blr_parameter, 1, 2,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 0,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_88 */

static short
   isc_96l = 419;
static char
   isc_96 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 0, 10,0, 
	    blr_cstring, 32,0, 
	    blr_cstring, 32,0, 
	    blr_cstring, 32,0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_begin, 
	    blr_for, 
	       blr_rse, 2, 
		  blr_relation, 13, 'R','D','B','$','F','U','N','C','T','I','O','N','S', 0, 
		  blr_relation, 22, 'R','D','B','$','F','U','N','C','T','I','O','N','_','A','R','G','U','M','E','N','T','S', 1, 
		  blr_boolean, 
		     blr_and, 
			blr_eql, 
			   blr_field, 0, 17, 'R','D','B','$','F','U','N','C','T','I','O','N','_','N','A','M','E', 
			   blr_field, 1, 17, 'R','D','B','$','F','U','N','C','T','I','O','N','_','N','A','M','E', 
			blr_eql, 
			   blr_field, 0, 19, 'R','D','B','$','R','E','T','U','R','N','_','A','R','G','U','M','E','N','T', 
			   blr_field, 1, 21, 'R','D','B','$','A','R','G','U','M','E','N','T','_','P','O','S','I','T','I','O','N', 
		  blr_end, 
	       blr_send, 0, 
		  blr_begin, 
		     blr_assignment, 
			blr_field, 1, 17, 'R','D','B','$','F','U','N','C','T','I','O','N','_','N','A','M','E', 
			blr_parameter, 0, 0,0, 
		     blr_assignment, 
			blr_field, 0, 14, 'R','D','B','$','Q','U','E','R','Y','_','N','A','M','E', 
			blr_parameter, 0, 1,0, 
		     blr_assignment, 
			blr_field, 0, 17, 'R','D','B','$','F','U','N','C','T','I','O','N','_','N','A','M','E', 
			blr_parameter, 0, 2,0, 
		     blr_assignment, 
			blr_literal, blr_long, 0, 1,0,0,0,
			blr_parameter, 0, 3,0, 
		     blr_assignment, 
			blr_field, 1, 21, 'R','D','B','$','A','R','G','U','M','E','N','T','_','P','O','S','I','T','I','O','N', 
			blr_parameter, 0, 4,0, 
		     blr_assignment, 
			blr_field, 1, 14, 'R','D','B','$','F','I','E','L','D','_','T','Y','P','E', 
			blr_parameter, 0, 5,0, 
		     blr_assignment, 
			blr_field, 1, 18, 'R','D','B','$','F','I','E','L','D','_','S','U','B','_','T','Y','P','E', 
			blr_parameter, 0, 6,0, 
		     blr_assignment, 
			blr_field, 1, 15, 'R','D','B','$','F','I','E','L','D','_','S','C','A','L','E', 
			blr_parameter, 0, 7,0, 
		     blr_assignment, 
			blr_field, 1, 16, 'R','D','B','$','F','I','E','L','D','_','L','E','N','G','T','H', 
			blr_parameter, 0, 8,0, 
		     blr_assignment, 
			blr_field, 0, 17, 'R','D','B','$','F','U','N','C','T','I','O','N','_','T','Y','P','E', 
			blr_parameter, 0, 9,0, 
		     blr_end, 
	    blr_send, 0, 
	       blr_assignment, 
		  blr_literal, blr_long, 0, 0,0,0,0,
		  blr_parameter, 0, 3,0, 
	    blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_96 */

static short
   isc_108l = 147;
static char
   isc_108 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 0, 5,0, 
	    blr_cstring, 32,0, 
	    blr_cstring, 32,0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_begin, 
	    blr_for, 
	       blr_rse, 1, 
		  blr_relation, 14, 'R','D','B','$','P','R','O','C','E','D','U','R','E','S', 0, 
		  blr_end, 
	       blr_send, 0, 
		  blr_begin, 
		     blr_assignment, 
			blr_field, 0, 14, 'R','D','B','$','O','W','N','E','R','_','N','A','M','E', 
			blr_parameter2, 0, 0,0, 3,0, 
		     blr_assignment, 
			blr_field, 0, 18, 'R','D','B','$','P','R','O','C','E','D','U','R','E','_','N','A','M','E', 
			blr_parameter, 0, 1,0, 
		     blr_assignment, 
			blr_literal, blr_long, 0, 1,0,0,0,
			blr_parameter, 0, 2,0, 
		     blr_assignment, 
			blr_field, 0, 16, 'R','D','B','$','P','R','O','C','E','D','U','R','E','_','I','D', 
			blr_parameter, 0, 4,0, 
		     blr_end, 
	    blr_send, 0, 
	       blr_assignment, 
		  blr_literal, blr_long, 0, 0,0,0,0,
		  blr_parameter, 0, 2,0, 
	    blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_108 */

static short
   isc_115l = 170;
static char
   isc_115 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 0, 6,0, 
	    blr_cstring, 32,0, 
	    blr_cstring, 32,0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_begin, 
	    blr_for, 
	       blr_rse, 1, 
		  blr_relation, 13, 'R','D','B','$','R','E','L','A','T','I','O','N','S', 0, 
		  blr_end, 
	       blr_send, 0, 
		  blr_begin, 
		     blr_assignment, 
			blr_field, 0, 14, 'R','D','B','$','O','W','N','E','R','_','N','A','M','E', 
			blr_parameter2, 0, 0,0, 3,0, 
		     blr_assignment, 
			blr_field, 0, 17, 'R','D','B','$','R','E','L','A','T','I','O','N','_','N','A','M','E', 
			blr_parameter, 0, 1,0, 
		     blr_assignment, 
			blr_literal, blr_long, 0, 1,0,0,0,
			blr_parameter, 0, 2,0, 
		     blr_assignment, 
			blr_field, 0, 16, 'R','D','B','$','D','B','K','E','Y','_','L','E','N','G','T','H', 
			blr_parameter, 0, 4,0, 
		     blr_assignment, 
			blr_field, 0, 15, 'R','D','B','$','R','E','L','A','T','I','O','N','_','I','D', 
			blr_parameter, 0, 5,0, 
		     blr_end, 
	    blr_send, 0, 
	       blr_assignment, 
		  blr_literal, blr_long, 0, 0,0,0,0,
		  blr_parameter, 0, 2,0, 
	    blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_115 */

static short
   isc_123l = 141;
static char
   isc_123 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 0, 4,0, 
	    blr_cstring, 32,0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_begin, 
	    blr_for, 
	       blr_rse, 1, 
		  blr_relation, 13, 'R','D','B','$','R','E','L','A','T','I','O','N','S', 0, 
		  blr_end, 
	       blr_send, 0, 
		  blr_begin, 
		     blr_assignment, 
			blr_field, 0, 17, 'R','D','B','$','R','E','L','A','T','I','O','N','_','N','A','M','E', 
			blr_parameter, 0, 0,0, 
		     blr_assignment, 
			blr_literal, blr_long, 0, 1,0,0,0,
			blr_parameter, 0, 1,0, 
		     blr_assignment, 
			blr_field, 0, 16, 'R','D','B','$','D','B','K','E','Y','_','L','E','N','G','T','H', 
			blr_parameter, 0, 2,0, 
		     blr_assignment, 
			blr_field, 0, 15, 'R','D','B','$','R','E','L','A','T','I','O','N','_','I','D', 
			blr_parameter, 0, 3,0, 
		     blr_end, 
	    blr_send, 0, 
	       blr_assignment, 
		  blr_literal, blr_long, 0, 0,0,0,0,
		  blr_parameter, 0, 1,0, 
	    blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_123 */

static short
   isc_129l = 129;
static char
   isc_129 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 0, 1,0, 
	    blr_short, 0, 
	 blr_begin, 
	    blr_for, 
	       blr_rse, 1, 
		  blr_relation, 13, 'R','D','B','$','R','E','L','A','T','I','O','N','S', 0, 
		  blr_boolean, 
		     blr_and, 
			blr_eql, 
			   blr_field, 0, 17, 'R','D','B','$','R','E','L','A','T','I','O','N','_','N','A','M','E', 
			   blr_literal, blr_text, 14,0, 'R','D','B','$','P','R','O','C','E','D','U','R','E','S',
			blr_eql, 
			   blr_field, 0, 15, 'R','D','B','$','S','Y','S','T','E','M','_','F','L','A','G', 
			   blr_literal, blr_long, 0, 1,0,0,0,
		  blr_end, 
	       blr_send, 0, 
		  blr_begin, 
		     blr_assignment, 
			blr_literal, blr_long, 0, 1,0,0,0,
			blr_parameter, 0, 0,0, 
		     blr_end, 
	    blr_send, 0, 
	       blr_assignment, 
		  blr_literal, blr_long, 0, 0,0,0,0,
		  blr_parameter, 0, 0,0, 
	    blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_129 */

static short
   isc_132l = 120;
static char
   isc_132 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 2,0, 
	    blr_cstring, 32,0, 
	    blr_short, 0, 
	 blr_message, 0, 1,0, 
	    blr_cstring, 32,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 1, 
		     blr_relation, 11, 'R','D','B','$','I','N','D','I','C','E','S', 0, 
		     blr_boolean, 
			blr_eql, 
			   blr_field, 0, 14, 'R','D','B','$','I','N','D','E','X','_','N','A','M','E', 
			   blr_parameter, 0, 0,0, 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_field, 0, 17, 'R','D','B','$','R','E','L','A','T','I','O','N','_','N','A','M','E', 
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 1,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 1,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_132 */

static short
   isc_138l = 153;
static char
   isc_138 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 3,0, 
	    blr_cstring, 32,0, 
	    blr_cstring, 32,0, 
	    blr_short, 0, 
	 blr_message, 0, 1,0, 
	    blr_cstring, 32,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 1, 
		     blr_relation, 18, 'R','D','B','$','V','I','E','W','_','R','E','L','A','T','I','O','N','S', 0, 
		     blr_boolean, 
			blr_eql, 
			   blr_field, 0, 13, 'R','D','B','$','V','I','E','W','_','N','A','M','E', 
			   blr_parameter, 0, 0,0, 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_field, 0, 17, 'R','D','B','$','R','E','L','A','T','I','O','N','_','N','A','M','E', 
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_field, 0, 16, 'R','D','B','$','C','O','N','T','E','X','T','_','N','A','M','E', 
			   blr_parameter, 1, 1,0, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 2,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 2,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_138 */

static short
   isc_145l = 407;
static char
   isc_145 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 8,0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_message, 0, 1,0, 
	    blr_cstring, 32,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 2, 
		     blr_relation, 13, 'R','D','B','$','F','U','N','C','T','I','O','N','S', 0, 
		     blr_relation, 22, 'R','D','B','$','F','U','N','C','T','I','O','N','_','A','R','G','U','M','E','N','T','S', 1, 
		     blr_boolean, 
			blr_and, 
			   blr_eql, 
			      blr_field, 0, 17, 'R','D','B','$','F','U','N','C','T','I','O','N','_','N','A','M','E', 
			      blr_parameter, 0, 0,0, 
			   blr_and, 
			      blr_eql, 
				 blr_field, 0, 17, 'R','D','B','$','F','U','N','C','T','I','O','N','_','N','A','M','E', 
				 blr_field, 1, 17, 'R','D','B','$','F','U','N','C','T','I','O','N','_','N','A','M','E', 
			      blr_neq, 
				 blr_field, 0, 19, 'R','D','B','$','R','E','T','U','R','N','_','A','R','G','U','M','E','N','T', 
				 blr_field, 1, 21, 'R','D','B','$','A','R','G','U','M','E','N','T','_','P','O','S','I','T','I','O','N', 
		     blr_sort, 1, 
			blr_descending, 
			   blr_field, 1, 21, 'R','D','B','$','A','R','G','U','M','E','N','T','_','P','O','S','I','T','I','O','N', 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_field, 1, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','I','D', 
			   blr_parameter2, 1, 2,0, 1,0, 
			blr_assignment, 
			   blr_field, 1, 14, 'R','D','B','$','F','I','E','L','D','_','T','Y','P','E', 
			   blr_parameter, 1, 3,0, 
			blr_assignment, 
			   blr_field, 1, 18, 'R','D','B','$','F','I','E','L','D','_','S','U','B','_','T','Y','P','E', 
			   blr_parameter, 1, 4,0, 
			blr_assignment, 
			   blr_field, 1, 15, 'R','D','B','$','F','I','E','L','D','_','S','C','A','L','E', 
			   blr_parameter, 1, 5,0, 
			blr_assignment, 
			   blr_field, 1, 16, 'R','D','B','$','F','I','E','L','D','_','L','E','N','G','T','H', 
			   blr_parameter, 1, 6,0, 
			blr_assignment, 
			   blr_field, 1, 21, 'R','D','B','$','A','R','G','U','M','E','N','T','_','P','O','S','I','T','I','O','N', 
			   blr_parameter, 1, 7,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 0,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_145 */

static short
   isc_157l = 373;
static char
   isc_157 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 6,0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_message, 0, 1,0, 
	    blr_cstring, 32,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 2, 
		     blr_relation, 13, 'R','D','B','$','F','U','N','C','T','I','O','N','S', 0, 
		     blr_relation, 22, 'R','D','B','$','F','U','N','C','T','I','O','N','_','A','R','G','U','M','E','N','T','S', 1, 
		     blr_boolean, 
			blr_and, 
			   blr_eql, 
			      blr_field, 0, 17, 'R','D','B','$','F','U','N','C','T','I','O','N','_','N','A','M','E', 
			      blr_parameter, 0, 0,0, 
			   blr_and, 
			      blr_eql, 
				 blr_field, 0, 17, 'R','D','B','$','F','U','N','C','T','I','O','N','_','N','A','M','E', 
				 blr_field, 1, 17, 'R','D','B','$','F','U','N','C','T','I','O','N','_','N','A','M','E', 
			      blr_neq, 
				 blr_field, 0, 19, 'R','D','B','$','R','E','T','U','R','N','_','A','R','G','U','M','E','N','T', 
				 blr_field, 1, 21, 'R','D','B','$','A','R','G','U','M','E','N','T','_','P','O','S','I','T','I','O','N', 
		     blr_sort, 1, 
			blr_descending, 
			   blr_field, 1, 21, 'R','D','B','$','A','R','G','U','M','E','N','T','_','P','O','S','I','T','I','O','N', 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_field, 1, 14, 'R','D','B','$','F','I','E','L','D','_','T','Y','P','E', 
			   blr_parameter, 1, 1,0, 
			blr_assignment, 
			   blr_field, 1, 18, 'R','D','B','$','F','I','E','L','D','_','S','U','B','_','T','Y','P','E', 
			   blr_parameter, 1, 2,0, 
			blr_assignment, 
			   blr_field, 1, 15, 'R','D','B','$','F','I','E','L','D','_','S','C','A','L','E', 
			   blr_parameter, 1, 3,0, 
			blr_assignment, 
			   blr_field, 1, 16, 'R','D','B','$','F','I','E','L','D','_','L','E','N','G','T','H', 
			   blr_parameter, 1, 4,0, 
			blr_assignment, 
			   blr_field, 1, 21, 'R','D','B','$','A','R','G','U','M','E','N','T','_','P','O','S','I','T','I','O','N', 
			   blr_parameter, 1, 5,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 0,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_157 */

static short
   isc_167l = 320;
static char
   isc_167 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 12,0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_message, 0, 1,0, 
	    blr_cstring, 32,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 1, 
		     blr_relation, 10, 'R','D','B','$','F','I','E','L','D','S', 0, 
		     blr_boolean, 
			blr_eql, 
			   blr_parameter, 0, 0,0, 
			   blr_field, 0, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_field, 0, 18, 'R','D','B','$','S','E','G','M','E','N','T','_','L','E','N','G','T','H', 
			   blr_parameter, 1, 1,0, 
			blr_assignment, 
			   blr_field, 0, 16, 'R','D','B','$','C','O','L','L','A','T','I','O','N','_','I','D', 
			   blr_parameter2, 1, 3,0, 2,0, 
			blr_assignment, 
			   blr_field, 0, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','I','D', 
			   blr_parameter2, 1, 5,0, 4,0, 
			blr_assignment, 
			   blr_field, 0, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','L','E','N','G','T','H', 
			   blr_parameter2, 1, 7,0, 6,0, 
			blr_assignment, 
			   blr_field, 0, 14, 'R','D','B','$','F','I','E','L','D','_','T','Y','P','E', 
			   blr_parameter, 1, 8,0, 
			blr_assignment, 
			   blr_field, 0, 18, 'R','D','B','$','F','I','E','L','D','_','S','U','B','_','T','Y','P','E', 
			   blr_parameter, 1, 9,0, 
			blr_assignment, 
			   blr_field, 0, 15, 'R','D','B','$','F','I','E','L','D','_','S','C','A','L','E', 
			   blr_parameter, 1, 10,0, 
			blr_assignment, 
			   blr_field, 0, 16, 'R','D','B','$','F','I','E','L','D','_','L','E','N','G','T','H', 
			   blr_parameter, 1, 11,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 0,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_167 */

static short
   isc_183l = 250;
static char
   isc_183 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 4,0, 
	    blr_cstring, 32,0, 
	    blr_cstring, 32,0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_message, 0, 2,0, 
	    blr_cstring, 32,0, 
	    blr_short, 0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 1, 
		     blr_relation, 24, 'R','D','B','$','P','R','O','C','E','D','U','R','E','_','P','A','R','A','M','E','T','E','R','S', 0, 
		     blr_boolean, 
			blr_and, 
			   blr_eql, 
			      blr_field, 0, 18, 'R','D','B','$','P','R','O','C','E','D','U','R','E','_','N','A','M','E', 
			      blr_parameter, 0, 0,0, 
			   blr_eql, 
			      blr_field, 0, 18, 'R','D','B','$','P','A','R','A','M','E','T','E','R','_','T','Y','P','E', 
			      blr_parameter, 0, 1,0, 
		     blr_sort, 1, 
			blr_descending, 
			   blr_field, 0, 20, 'R','D','B','$','P','A','R','A','M','E','T','E','R','_','N','U','M','B','E','R', 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_field, 0, 18, 'R','D','B','$','P','A','R','A','M','E','T','E','R','_','N','A','M','E', 
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_field, 0, 16, 'R','D','B','$','F','I','E','L','D','_','S','O','U','R','C','E', 
			   blr_parameter, 1, 1,0, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 2,0, 
			blr_assignment, 
			   blr_field, 0, 20, 'R','D','B','$','P','A','R','A','M','E','T','E','R','_','N','U','M','B','E','R', 
			   blr_parameter, 1, 3,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 2,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_183 */

static short
   isc_192l = 96;
static char
   isc_192 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 1,0, 
	    blr_short, 0, 
	 blr_message, 0, 1,0, 
	    blr_short, 0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 1, 
		     blr_relation, 14, 'R','D','B','$','P','R','O','C','E','D','U','R','E','S', 0, 
		     blr_boolean, 
			blr_eql, 
			   blr_field, 0, 16, 'R','D','B','$','P','R','O','C','E','D','U','R','E','_','I','D', 
			   blr_parameter, 0, 0,0, 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 0,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 0,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_192 */

static short
   isc_197l = 203;
static char
   isc_197 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 2,0, 
	    blr_cstring, 32,0, 
	    blr_short, 0, 
	 blr_message, 0, 1,0, 
	    blr_cstring, 32,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 2, 
		     blr_relation, 19, 'R','D','B','$','R','E','L','A','T','I','O','N','_','F','I','E','L','D','S', 0, 
		     blr_relation, 10, 'R','D','B','$','F','I','E','L','D','S', 1, 
		     blr_boolean, 
			blr_and, 
			   blr_eql, 
			      blr_field, 0, 16, 'R','D','B','$','F','I','E','L','D','_','S','O','U','R','C','E', 
			      blr_field, 1, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
			   blr_eql, 
			      blr_field, 0, 17, 'R','D','B','$','R','E','L','A','T','I','O','N','_','N','A','M','E', 
			      blr_parameter, 0, 0,0, 
		     blr_sort, 1, 
			blr_ascending, 
			   blr_field, 0, 18, 'R','D','B','$','F','I','E','L','D','_','P','O','S','I','T','I','O','N', 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_field, 0, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 1,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 1,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_197 */

static short
   isc_203l = 413;
static char
   isc_203 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 10,0, 
	    blr_cstring, 32,0, 
	    blr_cstring, 32,0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_message, 0, 2,0, 
	    blr_cstring, 32,0, 
	    blr_cstring, 32,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 2, 
		     blr_relation, 19, 'R','D','B','$','R','E','L','A','T','I','O','N','_','F','I','E','L','D','S', 0, 
		     blr_relation, 10, 'R','D','B','$','F','I','E','L','D','S', 1, 
		     blr_boolean, 
			blr_and, 
			   blr_eql, 
			      blr_field, 0, 16, 'R','D','B','$','F','I','E','L','D','_','S','O','U','R','C','E', 
			      blr_field, 1, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
			   blr_and, 
			      blr_eql, 
				 blr_field, 0, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
				 blr_parameter, 0, 1,0, 
			      blr_eql, 
				 blr_field, 0, 17, 'R','D','B','$','R','E','L','A','T','I','O','N','_','N','A','M','E', 
				 blr_parameter, 0, 0,0, 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_field, 0, 16, 'R','D','B','$','F','I','E','L','D','_','S','O','U','R','C','E', 
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_field, 1, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
			   blr_parameter, 1, 1,0, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 2,0, 
			blr_assignment, 
			   blr_field, 1, 18, 'R','D','B','$','S','E','G','M','E','N','T','_','L','E','N','G','T','H', 
			   blr_parameter, 1, 3,0, 
			blr_assignment, 
			   blr_field, 1, 14, 'R','D','B','$','F','I','E','L','D','_','T','Y','P','E', 
			   blr_parameter, 1, 4,0, 
			blr_assignment, 
			   blr_field, 1, 18, 'R','D','B','$','F','I','E','L','D','_','S','U','B','_','T','Y','P','E', 
			   blr_parameter, 1, 5,0, 
			blr_assignment, 
			   blr_field, 0, 18, 'R','D','B','$','F','I','E','L','D','_','P','O','S','I','T','I','O','N', 
			   blr_parameter, 1, 6,0, 
			blr_assignment, 
			   blr_field, 1, 15, 'R','D','B','$','F','I','E','L','D','_','S','C','A','L','E', 
			   blr_parameter, 1, 7,0, 
			blr_assignment, 
			   blr_field, 1, 16, 'R','D','B','$','F','I','E','L','D','_','L','E','N','G','T','H', 
			   blr_parameter, 1, 8,0, 
			blr_assignment, 
			   blr_field, 0, 12, 'R','D','B','$','F','I','E','L','D','_','I','D', 
			   blr_parameter, 1, 9,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 2,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_203 */

static short
   isc_218l = 565;
static char
   isc_218 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 19,0, 
	    blr_cstring, 32,0, 
	    blr_cstring, 32,0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_message, 0, 2,0, 
	    blr_cstring, 32,0, 
	    blr_cstring, 32,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 2, 
		     blr_relation, 19, 'R','D','B','$','R','E','L','A','T','I','O','N','_','F','I','E','L','D','S', 0, 
		     blr_relation, 10, 'R','D','B','$','F','I','E','L','D','S', 1, 
		     blr_boolean, 
			blr_and, 
			   blr_eql, 
			      blr_field, 0, 16, 'R','D','B','$','F','I','E','L','D','_','S','O','U','R','C','E', 
			      blr_field, 1, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
			   blr_and, 
			      blr_eql, 
				 blr_field, 0, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
				 blr_parameter, 0, 1,0, 
			      blr_eql, 
				 blr_field, 0, 17, 'R','D','B','$','R','E','L','A','T','I','O','N','_','N','A','M','E', 
				 blr_parameter, 0, 0,0, 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_field, 0, 16, 'R','D','B','$','F','I','E','L','D','_','S','O','U','R','C','E', 
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_field, 1, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
			   blr_parameter, 1, 1,0, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 2,0, 
			blr_assignment, 
			   blr_field, 1, 16, 'R','D','B','$','C','O','L','L','A','T','I','O','N','_','I','D', 
			   blr_parameter2, 1, 4,0, 3,0, 
			blr_assignment, 
			   blr_field, 0, 16, 'R','D','B','$','C','O','L','L','A','T','I','O','N','_','I','D', 
			   blr_parameter2, 1, 6,0, 5,0, 
			blr_assignment, 
			   blr_field, 1, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','I','D', 
			   blr_parameter2, 1, 8,0, 7,0, 
			blr_assignment, 
			   blr_field, 1, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','L','E','N','G','T','H', 
			   blr_parameter2, 1, 10,0, 9,0, 
			blr_assignment, 
			   blr_field, 1, 14, 'R','D','B','$','D','I','M','E','N','S','I','O','N','S', 
			   blr_parameter, 1, 11,0, 
			blr_assignment, 
			   blr_field, 1, 18, 'R','D','B','$','S','E','G','M','E','N','T','_','L','E','N','G','T','H', 
			   blr_parameter, 1, 12,0, 
			blr_assignment, 
			   blr_field, 1, 14, 'R','D','B','$','F','I','E','L','D','_','T','Y','P','E', 
			   blr_parameter, 1, 13,0, 
			blr_assignment, 
			   blr_field, 1, 18, 'R','D','B','$','F','I','E','L','D','_','S','U','B','_','T','Y','P','E', 
			   blr_parameter, 1, 14,0, 
			blr_assignment, 
			   blr_field, 0, 18, 'R','D','B','$','F','I','E','L','D','_','P','O','S','I','T','I','O','N', 
			   blr_parameter, 1, 15,0, 
			blr_assignment, 
			   blr_field, 1, 15, 'R','D','B','$','F','I','E','L','D','_','S','C','A','L','E', 
			   blr_parameter, 1, 16,0, 
			blr_assignment, 
			   blr_field, 1, 16, 'R','D','B','$','F','I','E','L','D','_','L','E','N','G','T','H', 
			   blr_parameter, 1, 17,0, 
			blr_assignment, 
			   blr_field, 0, 12, 'R','D','B','$','F','I','E','L','D','_','I','D', 
			   blr_parameter, 1, 18,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 2,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_218 */

static short
   isc_242l = 303;
static char
   isc_242 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 2,0, 
	    blr_cstring, 32,0, 
	    blr_short, 0, 
	 blr_message, 0, 1,0, 
	    blr_cstring, 32,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 3, 
		     blr_relation, 11, 'R','D','B','$','I','N','D','I','C','E','S', 0, 
		     blr_relation, 18, 'R','D','B','$','I','N','D','E','X','_','S','E','G','M','E','N','T','S', 1, 
		     blr_relation, 24, 'R','D','B','$','R','E','L','A','T','I','O','N','_','C','O','N','S','T','R','A','I','N','T','S', 2, 
		     blr_boolean, 
			blr_and, 
			   blr_and, 
			      blr_eql, 
				 blr_field, 1, 14, 'R','D','B','$','I','N','D','E','X','_','N','A','M','E', 
				 blr_field, 0, 14, 'R','D','B','$','I','N','D','E','X','_','N','A','M','E', 
			      blr_eql, 
				 blr_field, 2, 14, 'R','D','B','$','I','N','D','E','X','_','N','A','M','E', 
				 blr_field, 1, 14, 'R','D','B','$','I','N','D','E','X','_','N','A','M','E', 
			   blr_and, 
			      blr_eql, 
				 blr_field, 2, 17, 'R','D','B','$','R','E','L','A','T','I','O','N','_','N','A','M','E', 
				 blr_parameter, 0, 0,0, 
			      blr_eql, 
				 blr_field, 2, 19, 'R','D','B','$','C','O','N','S','T','R','A','I','N','T','_','T','Y','P','E', 
				 blr_literal, blr_text, 11,0, 'P','R','I','M','A','R','Y',32,'K','E','Y',
		     blr_sort, 1, 
			blr_ascending, 
			   blr_field, 1, 18, 'R','D','B','$','F','I','E','L','D','_','P','O','S','I','T','I','O','N', 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_field, 1, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 1,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 1,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_242 */

static short
   isc_248l = 242;
static char
   isc_248 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 5,0, 
	    blr_quad, 0, 
	    blr_quad, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_message, 0, 2,0, 
	    blr_cstring, 32,0, 
	    blr_cstring, 32,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 2, 
		     blr_relation, 19, 'R','D','B','$','R','E','L','A','T','I','O','N','_','F','I','E','L','D','S', 0, 
		     blr_relation, 10, 'R','D','B','$','F','I','E','L','D','S', 1, 
		     blr_boolean, 
			blr_and, 
			   blr_eql, 
			      blr_field, 0, 16, 'R','D','B','$','F','I','E','L','D','_','S','O','U','R','C','E', 
			      blr_field, 1, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
			   blr_and, 
			      blr_eql, 
				 blr_field, 0, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
				 blr_parameter, 0, 1,0, 
			      blr_eql, 
				 blr_field, 0, 17, 'R','D','B','$','R','E','L','A','T','I','O','N','_','N','A','M','E', 
				 blr_parameter, 0, 0,0, 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_field, 1, 17, 'R','D','B','$','D','E','F','A','U','L','T','_','V','A','L','U','E', 
			   blr_parameter2, 1, 0,0, 3,0, 
			blr_assignment, 
			   blr_field, 0, 17, 'R','D','B','$','D','E','F','A','U','L','T','_','V','A','L','U','E', 
			   blr_parameter2, 1, 1,0, 4,0, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 2,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 2,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_248 */

static short
   isc_258l = 122;
static char
   isc_258 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 3,0, 
	    blr_quad, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_message, 0, 1,0, 
	    blr_cstring, 32,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 1, 
		     blr_relation, 10, 'R','D','B','$','F','I','E','L','D','S', 0, 
		     blr_boolean, 
			blr_eql, 
			   blr_field, 0, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
			   blr_parameter, 0, 0,0, 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_field, 0, 17, 'R','D','B','$','D','E','F','A','U','L','T','_','V','A','L','U','E', 
			   blr_parameter2, 1, 0,0, 2,0, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 1,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 1,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_258 */

static short
   isc_265l = 222;
static char
   isc_265 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 6,0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_message, 0, 1,0, 
	    blr_cstring, 32,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 1, 
		     blr_relation, 10, 'R','D','B','$','F','I','E','L','D','S', 0, 
		     blr_boolean, 
			blr_eql, 
			   blr_field, 0, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
			   blr_parameter, 0, 0,0, 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_field, 0, 18, 'R','D','B','$','S','E','G','M','E','N','T','_','L','E','N','G','T','H', 
			   blr_parameter, 1, 1,0, 
			blr_assignment, 
			   blr_field, 0, 14, 'R','D','B','$','F','I','E','L','D','_','T','Y','P','E', 
			   blr_parameter, 1, 2,0, 
			blr_assignment, 
			   blr_field, 0, 18, 'R','D','B','$','F','I','E','L','D','_','S','U','B','_','T','Y','P','E', 
			   blr_parameter, 1, 3,0, 
			blr_assignment, 
			   blr_field, 0, 15, 'R','D','B','$','F','I','E','L','D','_','S','C','A','L','E', 
			   blr_parameter, 1, 4,0, 
			blr_assignment, 
			   blr_field, 0, 16, 'R','D','B','$','F','I','E','L','D','_','L','E','N','G','T','H', 
			   blr_parameter, 1, 5,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 0,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_265 */

static short
   isc_275l = 320;
static char
   isc_275 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 12,0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_message, 0, 1,0, 
	    blr_cstring, 32,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 1, 
		     blr_relation, 10, 'R','D','B','$','F','I','E','L','D','S', 0, 
		     blr_boolean, 
			blr_eql, 
			   blr_field, 0, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
			   blr_parameter, 0, 0,0, 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_field, 0, 16, 'R','D','B','$','C','O','L','L','A','T','I','O','N','_','I','D', 
			   blr_parameter2, 1, 2,0, 1,0, 
			blr_assignment, 
			   blr_field, 0, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','I','D', 
			   blr_parameter2, 1, 4,0, 3,0, 
			blr_assignment, 
			   blr_field, 0, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','L','E','N','G','T','H', 
			   blr_parameter2, 1, 6,0, 5,0, 
			blr_assignment, 
			   blr_field, 0, 18, 'R','D','B','$','S','E','G','M','E','N','T','_','L','E','N','G','T','H', 
			   blr_parameter, 1, 7,0, 
			blr_assignment, 
			   blr_field, 0, 14, 'R','D','B','$','F','I','E','L','D','_','T','Y','P','E', 
			   blr_parameter, 1, 8,0, 
			blr_assignment, 
			   blr_field, 0, 18, 'R','D','B','$','F','I','E','L','D','_','S','U','B','_','T','Y','P','E', 
			   blr_parameter, 1, 9,0, 
			blr_assignment, 
			   blr_field, 0, 15, 'R','D','B','$','F','I','E','L','D','_','S','C','A','L','E', 
			   blr_parameter, 1, 10,0, 
			blr_assignment, 
			   blr_field, 0, 16, 'R','D','B','$','F','I','E','L','D','_','L','E','N','G','T','H', 
			   blr_parameter, 1, 11,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 0,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_275 */


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

#line 51 "gpre_meta.epp"

#ifndef GDS_VAL
#define GDS_VAL(x)		x
#define GDS_REF(x)		&x
#endif

#ifndef NULL_PTR
#define NULL_PTR	0
#endif

extern enum lang_t sw_language;
extern USHORT sw_cstring;
extern DBB isc_databases;

static const UCHAR blr_bpb[] = { isc_bpb_version1,
	isc_bpb_source_type, 1, BLOB_blr,
	isc_bpb_target_type, 1, BLOB_blr
};

#ifdef SCROLLABLE_CURSORS
static SCHAR db_version_info[] = { isc_info_base_level };
#endif

static SLONG array_size(GPRE_FLD);
static void get_array(DBB, const TEXT *, GPRE_FLD);
static int get_intl_char_subtype(SSHORT *, const TEXT *, USHORT, DBB);
static int resolve_charset_and_collation(SSHORT *, const TEXT *, const TEXT *);
static int symbol_length(const TEXT *);
#ifdef NOT_USED_OR_REPLACED
static int upcase(TEXT *, TEXT *);
#endif

/*____________________________________________________________
 *  
 *		Lookup a field by name in a context.
 *		If found, return field block.  If not, return NULL.
 */  

GPRE_FLD MET_context_field( GPRE_CTX context, const char *string)
{
	SYM symbol;
	GPRE_PRC procedure;
	GPRE_FLD field;
	SCHAR name[NAME_SIZE];

	if (context->ctx_relation) {
		return (MET_field(context->ctx_relation, string));
	}

	if (!context->ctx_procedure) {
		return NULL;
	}

	strcpy(name, string);
	procedure = context->ctx_procedure;

/*  At this point the procedure should have been scanned, so all
 *  its fields are in the symbol table.
 */

	field = NULL;
	for (symbol = HSH_lookup(name); symbol; symbol = symbol->sym_homonym) {
		if (symbol->sym_type == SYM_field &&
			(field = (GPRE_FLD) symbol->sym_object) &&
			field->fld_procedure == procedure)
		{
			return field;
		}
	}

	return field;
}


/*____________________________________________________________
 *  
 *		Initialize meta data access to database.  If the
 *		database can't be opened, return FALSE.
 */  

BOOLEAN MET_database(DBB dbb, BOOLEAN print_version)
{
	SCHAR dpb[MAX_PASSWORD_LENGTH + MAX_USER_LENGTH + 5], *d, *p;
	static const UCHAR sql_version_info[] = { isc_info_base_level,
		isc_info_ods_version,
		isc_info_db_sql_dialect,
		isc_info_end
	};
	/* 
	   ** Each info item requested will return 
	   **
	   **     1 byte for the info item tag
	   **     2 bytes for the length of the information that follows
	   **     1 to 4 bytes of integer information
	   **
	   ** isc_info_end will not have a 2-byte length - which gives us
	   ** some padding in the buffer.
	 */
	UCHAR sql_buffer[sizeof(sql_version_info) * (1 + 2 + 4)];
	UCHAR *ptr;


#ifndef REQUESTER
	if (sw_language == lang_internal) {
		JRDMET_init(dbb);
		return TRUE;
	}
#endif

	DB = NULL_HANDLE;

	if (!dbb->dbb_filename) {
		CPR_error("No database specified");
		return FALSE;
	}

/*  generate a dpb for the attach from the specified 
 *  compiletime user name and password 
 */

	d = dpb;
	if (dbb->dbb_c_user || dbb->dbb_c_password) {
		*d++ = isc_dpb_version1;
		if (p = dbb->dbb_c_user) {
			*d++ = isc_dpb_user_name;
			*d++ = strlen(p);
			while (*p)
				*d++ = *p++;
		}
		if (p = dbb->dbb_c_password) {
			*d++ = isc_dpb_password;
			*d++ = strlen(p);
			while (*p)
				*d++ = *p++;
		}
	}

	if (isc_attach_database
		(isc_status, 0, GDS_VAL(dbb->dbb_filename), GDS_REF(DB), d - &dpb[0],
		 dpb)) {
		/* We failed to attach, try in read only mode just in case 
		if (d == dpb)
			*d++ = isc_dpb_version1;
		*d++ = isc_dpb_set_db_readonly;
		*d++ = 1;
		*d++ = TRUE;
		if (isc_attach_database
			(isc_status, 0, GDS_VAL(dbb->dbb_filename), GDS_REF(DB), d - &dpb[0],
		 	dpb)) {*/
				isc_print_status(isc_status);
				return FALSE;
		//}
	}

	dbb->dbb_handle = DB;

	if (sw_version && print_version) {
		printf("    Version(s) for database \"%s\"\n", dbb->dbb_filename);
		isc_version(&DB, NULL, NULL);
	}

	sw_ods_version = 0;
	sw_server_version = 0;
	if (isc_database_info(isc_status, &DB, sizeof(sql_version_info),
		  (char*) sql_version_info, sizeof(sql_buffer), (char*) sql_buffer)) {
		isc_print_status(isc_status);
		return FALSE;
	}

	ptr = sql_buffer;
	while (*ptr != isc_info_end) {
		UCHAR item;
		USHORT length;

		item = (UCHAR) * ptr++;
		length = isc_vax_integer((char*)ptr, sizeof(USHORT));
		ptr += sizeof(USHORT);
		switch (item) {
		case isc_info_base_level:
			sw_server_version = (USHORT) ptr[1];
			break;

		case isc_info_ods_version:
			sw_ods_version = isc_vax_integer((char*)ptr, length);
			break;

		case isc_info_db_sql_dialect:
			compiletime_db_dialect = isc_vax_integer((char*)ptr, length);
			break;
		case isc_info_error:
			/* Error indicates that one of the  option
			   was not understood by thr server. Make sure it was
			   isc_info_db_sql_dialect and assume
			   that the database dialect is SQL_DIALECT_V5
			 */
			if ((sw_server_version != 0) && (sw_ods_version != 0))
				compiletime_db_dialect = SQL_DIALECT_V5;
			else
				printf("Internal error: Unexpected isc_info_value %d\n",
						  item);
			break;
		default:
			printf("Internal error: Unexpected isc_info_value %d\n", item);
			break;
		}
		ptr += length;
	}

	if (!dialect_specified)
		sw_sql_dialect = compiletime_db_dialect;
	if (sw_ods_version < 10) {
		if (sw_sql_dialect != compiletime_db_dialect) {
			char warn_mesg[100];
			sprintf(warn_mesg,
					"Pre 6.0 database. Cannot use dialect %d, Resetting to %d\n",
					sw_sql_dialect, compiletime_db_dialect);
			CPR_warn(warn_mesg);
		}
		sw_sql_dialect = compiletime_db_dialect;
	}

	else if (sw_sql_dialect != compiletime_db_dialect) {
		char warn_mesg[100];
		sprintf(warn_mesg,
				"Client dialect set to %d. Compiletime database dialect is %d\n",
				sw_sql_dialect, compiletime_db_dialect);
		CPR_warn(warn_mesg);
	}

#ifdef DEBUG
	if (sw_version && print_version)
		printf("Gpre Start up values. \
	\n\tServer Ver                   : %d\
        \n\tCompile time db Ver          : %d\
	\n\tCompile time db Sql Dialect  : %d\
	\n\tClient Sql dialect set to    : %d\n\n", sw_server_version, sw_ods_version, compiletime_db_dialect, sw_sql_dialect);
#endif
#ifdef SCROLLABLE_CURSORS

	SCHAR buffer[16];

/*  get the base level of the engine  */

	if (isc_database_info(isc_status,
						   GDS_REF(DB),
						   sizeof(db_version_info),
						   db_version_info, sizeof(buffer), buffer)) {
		isc_print_status(isc_status);
		return FALSE;
	}

/*  this seems like a lot of rigamarole to read one info item, 
 *  but it provides for easy extensibility in case we need 
 *  more info items later 
 */

	SCHAR* data = buffer;
	while ((p = *data++) != isc_info_end) {
		l = isc_vax_integer(data, 2);
		data += 2;

		switch ((int) p) {
			/* This flag indicates the version level of the engine  
			   itself, so we can tell what capabilities the engine 
			   code itself (as opposed to the on-disk structure).
			   Apparently the base level up to now indicated the major 
			   version number, but for 4.1 the base level is being 
			   incremented, so the base level indicates an engine version 
			   as follows:
			   1 == v1.x
			   2 == v2.x
			   3 == v3.x
			   4 == v4.0 only
			   5 == v4.1
			   Note: this info item is so old it apparently uses an 
			   archaic format, not a standard vax integer format.
			 */

		case isc_info_base_level:
			dbb->dbb_base_level = (USHORT) data[1];
			break;

		default:
			;
		}

		data += l;
	}
#endif

	return TRUE;
}


/*____________________________________________________________
 *  
 *		Lookup a domain by name.
 *		Initialize the size of the field.
 */  

USHORT MET_domain_lookup(GPRE_REQ request, GPRE_FLD field, const char *string)
{
   struct {
          short isc_269;	/* isc_utility */
          short isc_270;	/* RDB$SEGMENT_LENGTH */
          short isc_271;	/* RDB$FIELD_TYPE */
          short isc_272;	/* RDB$FIELD_SUB_TYPE */
          short isc_273;	/* RDB$FIELD_SCALE */
          short isc_274;	/* RDB$FIELD_LENGTH */
   } isc_268;
   struct {
          char  isc_267 [32];	/* RDB$FIELD_NAME */
   } isc_266;
   struct {
          short isc_279;	/* isc_utility */
          short isc_280;	/* gds__null_flag */
          short isc_281;	/* RDB$COLLATION_ID */
          short isc_282;	/* gds__null_flag */
          short isc_283;	/* RDB$CHARACTER_SET_ID */
          short isc_284;	/* gds__null_flag */
          short isc_285;	/* RDB$CHARACTER_LENGTH */
          short isc_286;	/* RDB$SEGMENT_LENGTH */
          short isc_287;	/* RDB$FIELD_TYPE */
          short isc_288;	/* RDB$FIELD_SUB_TYPE */
          short isc_289;	/* RDB$FIELD_SCALE */
          short isc_290;	/* RDB$FIELD_LENGTH */
   } isc_278;
   struct {
          char  isc_277 [32];	/* RDB$FIELD_NAME */
   } isc_276;
#line 353 "gpre_meta.epp"
	SYM symbol;
	DBB dbb;
	SCHAR name[NAME_SIZE];
	GPRE_FLD d_field;
	SSHORT found = FALSE;

	strcpy(name, string);

/*  Lookup domain.  If we find it in the hash table, and it is not the
 *  field we a currently looking at, use it.  Else look it up from the
 *  database.
 */

	for (symbol = HSH_lookup(name); symbol; symbol = symbol->sym_homonym)
		if ((symbol->sym_type == SYM_field) &&
			((d_field = (GPRE_FLD) symbol->sym_object) && (d_field != field))) {
			field->fld_length = d_field->fld_length;
			field->fld_scale = d_field->fld_scale;
			field->fld_sub_type = d_field->fld_sub_type;
			field->fld_dtype = d_field->fld_dtype;
			field->fld_ttype = d_field->fld_ttype;
			field->fld_charset_id = d_field->fld_charset_id;
			field->fld_collate_id = d_field->fld_collate_id;
			field->fld_char_length = d_field->fld_char_length;
			return TRUE;
		}

	if (!request)
		return FALSE;

	dbb = request->req_database;
	if (dbb->dbb_flags & DBB_sqlca)
		return FALSE;

	DB = dbb->dbb_handle;
	gds_trans = dbb->dbb_transaction;

	if (!(dbb->dbb_flags & DBB_v3)) {
		/*FOR(REQUEST_HANDLE dbb->dbb_domain_request)
		F IN RDB$FIELDS WITH F.RDB$FIELD_NAME EQ name*/
		{
                if (!dbb->dbb_domain_request)
                   isc_compile_request ((long*) 0L, (isc_db_handle*) &DB, (isc_req_handle*) &dbb->dbb_domain_request, (short) sizeof (isc_275), (char *) isc_275);
		isc_vtov ((char*)name, (char*)isc_276.isc_277, 32);
                isc_start_and_send ((long*) 0L, (isc_req_handle*) &dbb->dbb_domain_request, (isc_req_handle*) &gds_trans, (short) 0, (short) 32, &isc_276, (short) 0);
		while (1)
		   {
                   isc_receive ((long*) 0L, (isc_req_handle*) &dbb->dbb_domain_request, (short) 1, (short) 24, &isc_278, (short) 0);
		   if (!isc_278.isc_279) break; found = TRUE;
#line 393 "gpre_meta.epp"
		field->fld_length = /*F.RDB$FIELD_LENGTH*/
				    isc_278.isc_290;
#line 394 "gpre_meta.epp"
		field->fld_scale = /*F.RDB$FIELD_SCALE*/
				   isc_278.isc_289;
#line 395 "gpre_meta.epp"
		field->fld_sub_type = /*F.RDB$FIELD_SUB_TYPE*/
				      isc_278.isc_288;
#line 396 "gpre_meta.epp"
		field->fld_dtype =
			MET_get_dtype(/*F.RDB$FIELD_TYPE*/
				      isc_278.isc_287, /*F.RDB$FIELD_SUB_TYPE*/
  isc_278.isc_288,
#line 398 "gpre_meta.epp"
						  &field->fld_length);

		switch (field->fld_dtype) {
		case dtype_text:
		case dtype_cstring:
			field->fld_flags |= FLD_text;
			break;

		case dtype_blob:
			field->fld_flags |= FLD_blob;
			field->fld_seg_length = /*F.RDB$SEGMENT_LENGTH*/
						isc_278.isc_286;
#line 409 "gpre_meta.epp"
			break;
		}

		if (field->fld_dtype <= dtype_any_text) {
			if (!/*F.RDB$CHARACTER_LENGTH.NULL*/
			     isc_278.isc_284)
#line 414 "gpre_meta.epp"
				field->fld_char_length = /*F.RDB$CHARACTER_LENGTH*/
							 isc_278.isc_285;
#line 415 "gpre_meta.epp"
			if (!/*F.RDB$CHARACTER_SET_ID.NULL*/
			     isc_278.isc_282)
#line 416 "gpre_meta.epp"
				field->fld_charset_id = /*F.RDB$CHARACTER_SET_ID*/
							isc_278.isc_283;
#line 417 "gpre_meta.epp"
			if (!/*F.RDB$COLLATION_ID.NULL*/
			     isc_278.isc_280)
#line 418 "gpre_meta.epp"
				field->fld_collate_id = /*F.RDB$COLLATION_ID*/
							isc_278.isc_281;
#line 419 "gpre_meta.epp"
		}

		field->fld_ttype =
			INTL_CS_COLL_TO_TTYPE(field->fld_charset_id,
								  field->fld_collate_id);

		/*END_FOR;*/
		   }
		}
#line 426 "gpre_meta.epp"
	}
	else {
		/*FOR(REQUEST_HANDLE dbb->dbb_domain_request)
		F IN RDB$FIELDS WITH F.RDB$FIELD_NAME EQ name*/
		{
                if (!dbb->dbb_domain_request)
                   isc_compile_request ((long*) 0L, (isc_db_handle*) &DB, (isc_req_handle*) &dbb->dbb_domain_request, (short) sizeof (isc_265), (char *) isc_265);
		isc_vtov ((char*)name, (char*)isc_266.isc_267, 32);
                isc_start_and_send ((long*) 0L, (isc_req_handle*) &dbb->dbb_domain_request, (isc_req_handle*) &gds_trans, (short) 0, (short) 32, &isc_266, (short) 0);
		while (1)
		   {
                   isc_receive ((long*) 0L, (isc_req_handle*) &dbb->dbb_domain_request, (short) 1, (short) 12, &isc_268, (short) 0);
		   if (!isc_268.isc_269) break; found = TRUE;
#line 430 "gpre_meta.epp"
		field->fld_length = /*F.RDB$FIELD_LENGTH*/
				    isc_268.isc_274;
#line 431 "gpre_meta.epp"
		field->fld_scale = /*F.RDB$FIELD_SCALE*/
				   isc_268.isc_273;
#line 432 "gpre_meta.epp"
		field->fld_sub_type = /*F.RDB$FIELD_SUB_TYPE*/
				      isc_268.isc_272;
#line 433 "gpre_meta.epp"
		field->fld_dtype =
			MET_get_dtype(/*F.RDB$FIELD_TYPE*/
				      isc_268.isc_271, /*F.RDB$FIELD_SUB_TYPE*/
  isc_268.isc_272,
#line 435 "gpre_meta.epp"
						  &field->fld_length);

		switch (field->fld_dtype) {
		case dtype_text:
		case dtype_cstring:
			field->fld_flags |= FLD_text;
			break;

		case dtype_blob:
			field->fld_flags |= FLD_blob;
			field->fld_seg_length = /*F.RDB$SEGMENT_LENGTH*/
						isc_268.isc_270;
#line 446 "gpre_meta.epp"
			break;
		}

		field->fld_ttype =
			INTL_CS_COLL_TO_TTYPE(field->fld_charset_id,
								  field->fld_collate_id);

		/*END_FOR;*/
		   }
		}
#line 454 "gpre_meta.epp"
	}

	return found;
}


/*____________________________________________________________
 *  
 *		Gets the default value for a domain of an existing table
 */  

BOOLEAN MET_get_domain_default(DBB dbb,
							   const TEXT * domain_name,
							   TEXT * buffer, USHORT buff_length)
{
   struct {
          ISC_QUAD isc_262;	/* RDB$DEFAULT_VALUE */
          short isc_263;	/* isc_utility */
          short isc_264;	/* gds__null_flag */
   } isc_261;
   struct {
          char  isc_260 [32];	/* RDB$FIELD_NAME */
   } isc_259;
#line 469 "gpre_meta.epp"
	isc_handle DB, gds_trans;
	SCHAR name[NAME_SIZE];
	SSHORT length;
	BOOLEAN has_default;
	ISC_STATUS_ARRAY status_vect;
	isc_blob_handle blob_handle = NULL_HANDLE;
	ISC_QUAD *blob_id;
	TEXT *ptr_in_buffer;
	ISC_STATUS stat;


	strcpy(name, domain_name);
	has_default = FALSE;

	if (dbb == NULL)
		return FALSE;

	if ((dbb->dbb_handle == NULL_HANDLE) && !MET_database(dbb, FALSE))
		CPR_exit(FINI_ERROR);

	assert(dbb->dbb_transaction == NULL_HANDLE);
	gds_trans = NULL_HANDLE;

	DB = dbb->dbb_handle;

	/*START_TRANSACTION;*/
	{
	{
	isc_start_transaction ((long*) 0L, (isc_tr_handle*) &gds_trans, (short) 1, &DB, (short) 0, (char*) 0);
	}
	}
#line 495 "gpre_meta.epp"

	/*FOR(REQUEST_HANDLE dbb->dbb_domain_request)
	GPRE_FLD IN RDB$FIELDS WITH
		GPRE_FLD.RDB$FIELD_NAME EQ name*/
	{
        if (!dbb->dbb_domain_request)
           isc_compile_request ((long*) 0L, (isc_db_handle*) &DB, (isc_req_handle*) &dbb->dbb_domain_request, (short) sizeof (isc_258), (char *) isc_258);
	isc_vtov ((char*)name, (char*)isc_259.isc_260, 32);
        isc_start_and_send ((long*) 0L, (isc_req_handle*) &dbb->dbb_domain_request, (isc_req_handle*) &gds_trans, (short) 0, (short) 32, &isc_259, (short) 0);
	while (1)
	   {
           isc_receive ((long*) 0L, (isc_req_handle*) &dbb->dbb_domain_request, (short) 1, (short) 12, &isc_261, (short) 0);
	   if (!isc_261.isc_263) break; if (!/*GPRE_FLD.RDB$DEFAULT_VALUE.NULL*/
      isc_261.isc_264) {
#line 499 "gpre_meta.epp"
		blob_id = &/*GPRE_FLD.RDB$DEFAULT_VALUE*/
			   isc_261.isc_262;
#line 500 "gpre_meta.epp"

		/* open the blob */
		stat = isc_open_blob2(status_vect, &DB, &gds_trans, &blob_handle,
							  blob_id, sizeof(blr_bpb), const_cast<unsigned char*>(blr_bpb));
		if (stat) {
			isc_print_status(status_vect);
			CPR_exit(FINI_ERROR);
		}
		/* fetch segments. Assume buffer is big enough.  */
			ptr_in_buffer = buffer;
		while (TRUE) {
			stat = isc_get_segment(status_vect, &blob_handle, (USHORT*) &length,
								   buff_length, ptr_in_buffer);
			ptr_in_buffer = ptr_in_buffer + length;
			buff_length = buff_length - length;

			if (!stat)
				continue;
			else if (stat == isc_segstr_eof) {
				/* null terminate the buffer */
				*ptr_in_buffer = 0;
				break;
			}
			else
				CPR_exit(FINI_ERROR);
		}
		isc_close_blob(status_vect, &blob_handle);

		/* the default string must be of the form:
		   blr_version4 blr_literal ..... blr_eoc OR
		   blr_version4 blr_null blr_eoc OR
		   blr_version4 blr_user_name  blr_eoc */
		assert(buffer[0] == blr_version4 || buffer[0] == blr_version5);
		assert(buffer[1] == blr_literal ||
			   buffer[1] == blr_null || buffer[1] == blr_user_name);

		has_default = TRUE;
	}
	else {						/* default not found */

		if (sw_sql_dialect > SQL_DIALECT_V5)
			buffer[0] = blr_version5;
		else
			buffer[0] = blr_version4;
		buffer[1] = blr_eoc;
	}
	/*END_FOR*/
	   }
	} /*COMMIT;*/
 {
 isc_commit_transaction ((long*) 0L, (isc_tr_handle*) &gds_trans);
 }
#line 547 "gpre_meta.epp"
	return (has_default);
}


/*____________________________________________________________
 *  
 *		Gets the default value for a column of an existing table.
 *		Will check the default for the column of the table, if that is
 *		not present, will check for the default of the relevant domain
 *  
 *		The default blr is returned in buffer. The blr is of the form
 *		blr_version4 blr_literal ..... blr_eoc
 *  
 *		Reads the system tables RDB$FIELDS and RDB$RELATION_FIELDS.
 */  

BOOLEAN MET_get_column_default(GPRE_REL relation,
							   const TEXT * column_name,
							   TEXT * buffer, USHORT buff_length)
{
   struct {
          ISC_QUAD isc_253;	/* RDB$DEFAULT_VALUE */
          ISC_QUAD isc_254;	/* RDB$DEFAULT_VALUE */
          short isc_255;	/* isc_utility */
          short isc_256;	/* gds__null_flag */
          short isc_257;	/* gds__null_flag */
   } isc_252;
   struct {
          char  isc_250 [32];	/* RDB$RELATION_NAME */
          char  isc_251 [32];	/* RDB$FIELD_NAME */
   } isc_249;
#line 567 "gpre_meta.epp"
	DBB dbb;
	isc_handle DB, gds_trans;
	SCHAR name[NAME_SIZE];
	SSHORT length;
	BOOLEAN has_default;
	ISC_STATUS_ARRAY status_vect;
	isc_blob_handle blob_handle = NULL_HANDLE;
	ISC_QUAD *blob_id;
	TEXT *ptr_in_buffer;
	ISC_STATUS stat;

	strcpy(name, column_name);
	has_default = FALSE;

	if ((dbb = relation->rel_database) == NULL)
		return FALSE;

	if ((dbb->dbb_handle == NULL_HANDLE) && !MET_database(dbb, FALSE))
		CPR_exit(FINI_ERROR);

	assert(dbb->dbb_transaction == NULL_HANDLE);
	gds_trans = NULL_HANDLE;

	DB = dbb->dbb_handle;

	/*START_TRANSACTION;*/
	{
	{
	isc_start_transaction ((long*) 0L, (isc_tr_handle*) &gds_trans, (short) 1, &DB, (short) 0, (char*) 0);
	}
	}
#line 593 "gpre_meta.epp"

	/*FOR(REQUEST_HANDLE dbb->dbb_field_request)
	RFR IN RDB$RELATION_FIELDS CROSS F IN RDB$FIELDS WITH
		RFR.RDB$FIELD_SOURCE EQ F.RDB$FIELD_NAME AND
		RFR.RDB$FIELD_NAME EQ name AND
		RFR.RDB$RELATION_NAME EQ relation->rel_symbol->sym_string*/
	{
        if (!dbb->dbb_field_request)
           isc_compile_request ((long*) 0L, (isc_db_handle*) &DB, (isc_req_handle*) &dbb->dbb_field_request, (short) sizeof (isc_248), (char *) isc_248);
	isc_vtov ((char*)relation->rel_symbol->sym_string, (char*)isc_249.isc_250, 32);
	isc_vtov ((char*)name, (char*)isc_249.isc_251, 32);
        isc_start_and_send ((long*) 0L, (isc_req_handle*) &dbb->dbb_field_request, (isc_req_handle*) &gds_trans, (short) 0, (short) 64, &isc_249, (short) 0);
	while (1)
	   {
           isc_receive ((long*) 0L, (isc_req_handle*) &dbb->dbb_field_request, (short) 1, (short) 22, &isc_252, (short) 0);
	   if (!isc_252.isc_255) break;
#line 599 "gpre_meta.epp"
		if (!/*RFR.RDB$DEFAULT_VALUE.NULL*/
		     isc_252.isc_257) {
#line 600 "gpre_meta.epp"
		blob_id = &/*RFR.RDB$DEFAULT_VALUE*/
			   isc_252.isc_254;
#line 601 "gpre_meta.epp"
		has_default = TRUE;
	}
	else if (!/*F.RDB$DEFAULT_VALUE.NULL*/
		  isc_252.isc_256) {
#line 604 "gpre_meta.epp"
		blob_id = &/*F.RDB$DEFAULT_VALUE*/
			   isc_252.isc_253;
#line 605 "gpre_meta.epp"
		has_default = TRUE;
	}

	if (has_default) {
		/* open the blob */
		stat = isc_open_blob2(status_vect, &DB, &gds_trans,
							  &blob_handle, blob_id, sizeof(blr_bpb),
							  const_cast<unsigned char*>(blr_bpb));
		if (stat) {
			isc_print_status(status_vect);
			CPR_exit(FINI_ERROR);
		}

		/* fetch segments. Assuming here that the buffer is big enough. */
		ptr_in_buffer = buffer;
		while (TRUE) {
			stat = isc_get_segment(status_vect, &blob_handle,
								   (USHORT*)&length, buff_length, ptr_in_buffer);
			ptr_in_buffer = ptr_in_buffer + length;
			buff_length = buff_length - length;
			if (!stat)
				continue;
			else if (stat == isc_segstr_eof) {
				/* null terminate the buffer */
				*ptr_in_buffer = 0;
				break;
			}
			else
				CPR_exit(FINI_ERROR);
		}
		isc_close_blob(status_vect, &blob_handle);

		/* the default string must be of the form:
		   blr_version4 blr_literal ..... blr_eoc OR
		   blr_version4 blr_null blr_eoc OR
		   blr_version4 blr_user_name blr_eoc */

		assert(buffer[0] == blr_version4 || buffer[0] == blr_version5);
		assert(buffer[1] == blr_literal ||
			   buffer[1] == blr_null || buffer[1] == blr_user_name);
	}
	else {
		if (sw_sql_dialect > SQL_DIALECT_V5)
			buffer[0] = blr_version5;
		else
			buffer[0] = blr_version4;
		buffer[1] = blr_eoc;
	}

	/*END_FOR;*/
	   }
	}
#line 655 "gpre_meta.epp"
	/*COMMIT;*/
	{
	isc_commit_transaction ((long*) 0L, (isc_tr_handle*) &gds_trans);
	}
#line 656 "gpre_meta.epp"
	return (has_default);
}


/*____________________________________________________________
 *  
 *		Lookup the fields for the primary key
 *		index on a relation, returning a list
 *		of the fields.
 */  

LLS MET_get_primary_key(DBB dbb, const TEXT * relation_name)
{
   struct {
          char  isc_246 [32];	/* RDB$FIELD_NAME */
          short isc_247;	/* isc_utility */
   } isc_245;
   struct {
          char  isc_244 [32];	/* RDB$RELATION_NAME */
   } isc_243;
#line 669 "gpre_meta.epp"
	LLS fields = NULL, *ptr_fields;
	isc_handle DB, gds_trans;
	SCHAR name[NAME_SIZE];
	STR field_name;
	TEXT *tmp;

	strcpy(name, relation_name);

	if (dbb == NULL)
		return NULL;

	if ((dbb->dbb_handle == NULL_HANDLE) && !MET_database(dbb, FALSE))
		CPR_exit(FINI_ERROR);

	assert(dbb->dbb_transaction == NULL_HANDLE);
	gds_trans = NULL_HANDLE;

	DB = dbb->dbb_handle;

	/*START_TRANSACTION;*/
	{
	{
	isc_start_transaction ((long*) 0L, (isc_tr_handle*) &gds_trans, (short) 1, &DB, (short) 0, (char*) 0);
	}
	}
#line 689 "gpre_meta.epp"

	ptr_fields = &fields;

	/*FOR(REQUEST_HANDLE dbb->dbb_primary_key_request)
		X IN RDB$INDICES CROSS
		Y IN RDB$INDEX_SEGMENTS
		OVER RDB$INDEX_NAME CROSS
		Z IN RDB$RELATION_CONSTRAINTS
		OVER RDB$INDEX_NAME
		WITH Z.RDB$RELATION_NAME EQ name
		AND Z.RDB$CONSTRAINT_TYPE EQ "PRIMARY KEY"
		SORTED BY Y.RDB$FIELD_POSITION*/
	{
        if (!dbb->dbb_primary_key_request)
           isc_compile_request ((long*) 0L, (isc_db_handle*) &DB, (isc_req_handle*) &dbb->dbb_primary_key_request, (short) sizeof (isc_242), (char *) isc_242);
	isc_vtov ((char*)name, (char*)isc_243.isc_244, 32);
        isc_start_and_send ((long*) 0L, (isc_req_handle*) &dbb->dbb_primary_key_request, (isc_req_handle*) &gds_trans, (short) 0, (short) 32, &isc_243, (short) 0);
	while (1)
	   {
           isc_receive ((long*) 0L, (isc_req_handle*) &dbb->dbb_primary_key_request, (short) 1, (short) 34, &isc_245, (short) 0);
	   if (!isc_245.isc_247) break;
#line 701 "gpre_meta.epp"
		field_name = (str*) MAKE_STRING(/*Y.RDB$FIELD_NAME*/
						isc_245.isc_246);
#line 702 "gpre_meta.epp"

	/* Strip off any trailing spaces from field name */
	tmp = (TEXT*) field_name;
	while (*tmp && *tmp != ' ')
		*tmp++;
	*tmp = '\0';

	PUSH((GPRE_NOD) field_name, ptr_fields);
	ptr_fields = &(*ptr_fields)->lls_next;

	/*END_FOR*/
	   }
	} /*COMMIT;*/
 {
 isc_commit_transaction ((long*) 0L, (isc_tr_handle*) &gds_trans);
 }
#line 713 "gpre_meta.epp"

	return fields;
}


/*____________________________________________________________
 *  
 *		Lookup a field by name in a relation.
 *		If found, return field block.  If not, return NULL.
 */  

GPRE_FLD MET_field(GPRE_REL relation, const char *string)
{
   struct {
          char  isc_208 [32];	/* RDB$FIELD_SOURCE */
          char  isc_209 [32];	/* RDB$FIELD_NAME */
          short isc_210;	/* isc_utility */
          short isc_211;	/* RDB$SEGMENT_LENGTH */
          short isc_212;	/* RDB$FIELD_TYPE */
          short isc_213;	/* RDB$FIELD_SUB_TYPE */
          short isc_214;	/* RDB$FIELD_POSITION */
          short isc_215;	/* RDB$FIELD_SCALE */
          short isc_216;	/* RDB$FIELD_LENGTH */
          short isc_217;	/* RDB$FIELD_ID */
   } isc_207;
   struct {
          char  isc_205 [32];	/* RDB$RELATION_NAME */
          char  isc_206 [32];	/* RDB$FIELD_NAME */
   } isc_204;
   struct {
          char  isc_223 [32];	/* RDB$FIELD_SOURCE */
          char  isc_224 [32];	/* RDB$FIELD_NAME */
          short isc_225;	/* isc_utility */
          short isc_226;	/* gds__null_flag */
          short isc_227;	/* RDB$COLLATION_ID */
          short isc_228;	/* gds__null_flag */
          short isc_229;	/* RDB$COLLATION_ID */
          short isc_230;	/* gds__null_flag */
          short isc_231;	/* RDB$CHARACTER_SET_ID */
          short isc_232;	/* gds__null_flag */
          short isc_233;	/* RDB$CHARACTER_LENGTH */
          short isc_234;	/* RDB$DIMENSIONS */
          short isc_235;	/* RDB$SEGMENT_LENGTH */
          short isc_236;	/* RDB$FIELD_TYPE */
          short isc_237;	/* RDB$FIELD_SUB_TYPE */
          short isc_238;	/* RDB$FIELD_POSITION */
          short isc_239;	/* RDB$FIELD_SCALE */
          short isc_240;	/* RDB$FIELD_LENGTH */
          short isc_241;	/* RDB$FIELD_ID */
   } isc_222;
   struct {
          char  isc_220 [32];	/* RDB$RELATION_NAME */
          char  isc_221 [32];	/* RDB$FIELD_NAME */
   } isc_219;
#line 726 "gpre_meta.epp"
	SYM symbol;
	GPRE_FLD field;
	DBB dbb;
	SCHAR name[NAME_SIZE];
	SSHORT length;

	strcpy(name, string);
	length = strlen(name);

/*  Lookup field.  If we find it, nifty.  If not, look it up in the
 *  database.
 */

	for (symbol = HSH_lookup(name); symbol; symbol = symbol->sym_homonym)
		if (symbol->sym_type == SYM_keyword &&
			symbol->sym_keyword == (int) KW_DBKEY) return relation->rel_dbkey;
		else if (symbol->sym_type == SYM_field &&
				 (field = (GPRE_FLD) symbol->sym_object) &&
				 field->fld_relation == relation) return field;

	if (sw_language == lang_internal)
		return NULL;

	dbb = relation->rel_database;

	if (dbb->dbb_flags & DBB_sqlca)
		return NULL;

	DB = dbb->dbb_handle;
	gds_trans = dbb->dbb_transaction;
	field = NULL;

	if (!(dbb->dbb_flags & DBB_v3)) {
		/*FOR(REQUEST_HANDLE dbb->dbb_field_request)
		RFR IN RDB$RELATION_FIELDS CROSS F IN RDB$FIELDS WITH
			RFR.RDB$FIELD_SOURCE EQ F.RDB$FIELD_NAME AND
			RFR.RDB$FIELD_NAME EQ name AND
			RFR.RDB$RELATION_NAME EQ relation->rel_symbol->sym_string*/
		{
                if (!dbb->dbb_field_request)
                   isc_compile_request ((long*) 0L, (isc_db_handle*) &DB, (isc_req_handle*) &dbb->dbb_field_request, (short) sizeof (isc_218), (char *) isc_218);
		isc_vtov ((char*)relation->rel_symbol->sym_string, (char*)isc_219.isc_220, 32);
		isc_vtov ((char*)name, (char*)isc_219.isc_221, 32);
                isc_start_and_send ((long*) 0L, (isc_req_handle*) &dbb->dbb_field_request, (isc_req_handle*) &gds_trans, (short) 0, (short) 64, &isc_219, (short) 0);
		while (1)
		   {
                   isc_receive ((long*) 0L, (isc_req_handle*) &dbb->dbb_field_request, (short) 1, (short) 98, &isc_222, (short) 0);
		   if (!isc_222.isc_225) break;
#line 764 "gpre_meta.epp"
			field = (GPRE_FLD) ALLOC(FLD_LEN);
		field->fld_relation = relation;
		field->fld_next = relation->rel_fields;
		relation->rel_fields = field;
		field->fld_id = /*RFR.RDB$FIELD_ID*/
				isc_222.isc_241;
#line 769 "gpre_meta.epp"
		field->fld_length = /*F.RDB$FIELD_LENGTH*/
				    isc_222.isc_240;
#line 770 "gpre_meta.epp"
		field->fld_scale = /*F.RDB$FIELD_SCALE*/
				   isc_222.isc_239;
#line 771 "gpre_meta.epp"
		field->fld_position = /*RFR.RDB$FIELD_POSITION*/
				      isc_222.isc_238;
#line 772 "gpre_meta.epp"
		field->fld_sub_type = /*F.RDB$FIELD_SUB_TYPE*/
				      isc_222.isc_237;
#line 773 "gpre_meta.epp"
		field->fld_dtype =
			MET_get_dtype(/*F.RDB$FIELD_TYPE*/
				      isc_222.isc_236, /*F.RDB$FIELD_SUB_TYPE*/
  isc_222.isc_237,
#line 775 "gpre_meta.epp"
						  &field->fld_length);

		switch (field->fld_dtype) {
		case dtype_text:
		case dtype_cstring:
			field->fld_flags |= FLD_text;
			break;

		case dtype_blob:
			field->fld_flags |= FLD_blob;
			field->fld_seg_length = /*F.RDB$SEGMENT_LENGTH*/
						isc_222.isc_235;
#line 786 "gpre_meta.epp"
			break;
		}

		if (/*F.RDB$DIMENSIONS*/
		    isc_222.isc_234 && !(dbb->dbb_flags & DBB_no_arrays))
#line 790 "gpre_meta.epp"
			get_array(dbb, /*F.RDB$FIELD_NAME*/
				       isc_222.isc_224, field);
#line 791 "gpre_meta.epp"

		if ((field->fld_dtype <= dtype_any_text)
			|| (field->fld_dtype == dtype_blob)) {
			if (!/*F.RDB$CHARACTER_LENGTH.NULL*/
			     isc_222.isc_232)
#line 795 "gpre_meta.epp"
				field->fld_char_length = /*F.RDB$CHARACTER_LENGTH*/
							 isc_222.isc_233;
#line 796 "gpre_meta.epp"
			if (!/*F.RDB$CHARACTER_SET_ID.NULL*/
			     isc_222.isc_230)
#line 797 "gpre_meta.epp"
				field->fld_charset_id = /*F.RDB$CHARACTER_SET_ID*/
							isc_222.isc_231;
#line 798 "gpre_meta.epp"
			if (!/*RFR.RDB$COLLATION_ID.NULL*/
			     isc_222.isc_228)
#line 799 "gpre_meta.epp"
				field->fld_collate_id = /*RFR.RDB$COLLATION_ID*/
							isc_222.isc_229;
#line 800 "gpre_meta.epp"
			else if (!/*F.RDB$COLLATION_ID.NULL*/
				  isc_222.isc_226)
#line 801 "gpre_meta.epp"
				field->fld_collate_id = /*F.RDB$COLLATION_ID*/
							isc_222.isc_227;
#line 802 "gpre_meta.epp"
		}

		field->fld_ttype =
			INTL_CS_COLL_TO_TTYPE(field->fld_charset_id,
								  field->fld_collate_id);
		field->fld_symbol = symbol =
			MSC_symbol(SYM_field, name, length, (GPRE_CTX) field);
		HSH_insert(symbol);
		field->fld_global = symbol =
			MSC_symbol(SYM_field, /*RFR.RDB$FIELD_SOURCE*/
					      isc_222.isc_223,
#line 812 "gpre_meta.epp"
					   symbol_length(/*RFR.RDB$FIELD_SOURCE*/
							 isc_222.isc_223), (GPRE_CTX) field);
#line 813 "gpre_meta.epp"
		/*END_FOR;*/
		   }
		}
#line 814 "gpre_meta.epp"
	}
	else {
		/*FOR(REQUEST_HANDLE dbb->dbb_field_request)
		RFR IN RDB$RELATION_FIELDS CROSS F IN RDB$FIELDS WITH
			RFR.RDB$FIELD_SOURCE EQ F.RDB$FIELD_NAME AND
			RFR.RDB$FIELD_NAME EQ name AND
			RFR.RDB$RELATION_NAME EQ relation->rel_symbol->sym_string*/
		{
                if (!dbb->dbb_field_request)
                   isc_compile_request ((long*) 0L, (isc_db_handle*) &DB, (isc_req_handle*) &dbb->dbb_field_request, (short) sizeof (isc_203), (char *) isc_203);
		isc_vtov ((char*)relation->rel_symbol->sym_string, (char*)isc_204.isc_205, 32);
		isc_vtov ((char*)name, (char*)isc_204.isc_206, 32);
                isc_start_and_send ((long*) 0L, (isc_req_handle*) &dbb->dbb_field_request, (isc_req_handle*) &gds_trans, (short) 0, (short) 64, &isc_204, (short) 0);
		while (1)
		   {
                   isc_receive ((long*) 0L, (isc_req_handle*) &dbb->dbb_field_request, (short) 1, (short) 80, &isc_207, (short) 0);
		   if (!isc_207.isc_210) break;
#line 821 "gpre_meta.epp"
			field = (GPRE_FLD) ALLOC(FLD_LEN);
		field->fld_relation = relation;
		field->fld_next = relation->rel_fields;
		relation->rel_fields = field;
		field->fld_id = /*RFR.RDB$FIELD_ID*/
				isc_207.isc_217;
#line 826 "gpre_meta.epp"
		field->fld_length = /*F.RDB$FIELD_LENGTH*/
				    isc_207.isc_216;
#line 827 "gpre_meta.epp"
		field->fld_scale = /*F.RDB$FIELD_SCALE*/
				   isc_207.isc_215;
#line 828 "gpre_meta.epp"
		field->fld_position = /*RFR.RDB$FIELD_POSITION*/
				      isc_207.isc_214;
#line 829 "gpre_meta.epp"
		field->fld_sub_type = /*F.RDB$FIELD_SUB_TYPE*/
				      isc_207.isc_213;
#line 830 "gpre_meta.epp"
		field->fld_dtype =
			MET_get_dtype(/*F.RDB$FIELD_TYPE*/
				      isc_207.isc_212, /*F.RDB$FIELD_SUB_TYPE*/
  isc_207.isc_213,
#line 832 "gpre_meta.epp"
						  &field->fld_length);
		switch (field->fld_dtype) {
		case dtype_text:
		case dtype_cstring:
			field->fld_flags |= FLD_text;
			break;

		case dtype_blob:
			field->fld_flags |= FLD_blob;
			field->fld_seg_length = /*F.RDB$SEGMENT_LENGTH*/
						isc_207.isc_211;
#line 842 "gpre_meta.epp"
			break;
		}

		if (!(dbb->dbb_flags & DBB_no_arrays))
			get_array(dbb, /*F.RDB$FIELD_NAME*/
				       isc_207.isc_209, field);
#line 847 "gpre_meta.epp"

		field->fld_ttype =
			INTL_CS_COLL_TO_TTYPE(field->fld_charset_id,
								  field->fld_collate_id);
		field->fld_symbol = symbol =
			MSC_symbol(SYM_field, name, length, (GPRE_CTX) field);
		HSH_insert(symbol);
		field->fld_global = symbol =
			MSC_symbol(SYM_field, /*RFR.RDB$FIELD_SOURCE*/
					      isc_207.isc_208,
#line 856 "gpre_meta.epp"
					   symbol_length(/*RFR.RDB$FIELD_SOURCE*/
							 isc_207.isc_208), (GPRE_CTX) field);
#line 857 "gpre_meta.epp"
		/*END_FOR;*/
		   }
		}
#line 858 "gpre_meta.epp"
	}

	return field;
}


/*____________________________________________________________
 *  
 *     Return a list of the fields in a relation
 */  

GPRE_NOD MET_fields(GPRE_CTX context)
{
   struct {
          char  isc_201 [32];	/* RDB$FIELD_NAME */
          short isc_202;	/* isc_utility */
   } isc_200;
   struct {
          char  isc_199 [32];	/* RDB$RELATION_NAME */
   } isc_198;
#line 871 "gpre_meta.epp"
	DBB dbb;
	GPRE_FLD field;
	LLS stack;
	GPRE_NOD node, field_node;
	REF reference;
	GPRE_PRC procedure;
	GPRE_REL relation;
	TEXT *p;
	int count;

	if (procedure = context->ctx_procedure) {
		node = MAKE_NODE(nod_list, procedure->prc_out_count);
		count = 0;
		for (field = procedure->prc_outputs; field; field = field->fld_next) {
			reference = (REF) ALLOC(REF_LEN);
			reference->ref_field = field;
			reference->ref_context = context;
			field_node = MSC_unary(nod_field, (GPRE_NOD) reference);
			node->nod_arg[field->fld_position] = field_node;
			count++;
		}
		return node;
	}

	relation = context->ctx_relation;
	if (relation->rel_meta) {
		for (count = 0, field = relation->rel_fields; field;
			 field = field->fld_next) count++;
		node = MAKE_NODE(nod_list, count);
		count = 0;
		for (field = relation->rel_fields; field; field = field->fld_next) {
			reference = (REF) ALLOC(REF_LEN);
			reference->ref_field = field;
			reference->ref_context = context;
			field_node = MSC_unary(nod_field, (GPRE_NOD) reference);
			node->nod_arg[field->fld_position] = field_node;
			count++;
		}
		return node;
	}

	if (sw_language == lang_internal)
		return NULL;

	dbb = relation->rel_database;
	DB = dbb->dbb_handle;
	gds_trans = dbb->dbb_transaction;

	count = 0;
	stack = NULL;
	/*FOR(REQUEST_HANDLE dbb->dbb_flds_request)
	RFR IN RDB$RELATION_FIELDS CROSS GPRE_FLD IN RDB$FIELDS WITH
		RFR.RDB$FIELD_SOURCE EQ GPRE_FLD.RDB$FIELD_NAME AND
		RFR.RDB$RELATION_NAME EQ relation->rel_symbol->sym_string
		SORTED BY RFR.RDB$FIELD_POSITION*/
	{
        if (!dbb->dbb_flds_request)
           isc_compile_request ((long*) 0L, (isc_db_handle*) &DB, (isc_req_handle*) &dbb->dbb_flds_request, (short) sizeof (isc_197), (char *) isc_197);
	isc_vtov ((char*)relation->rel_symbol->sym_string, (char*)isc_198.isc_199, 32);
        isc_start_and_send ((long*) 0L, (isc_req_handle*) &dbb->dbb_flds_request, (isc_req_handle*) &gds_trans, (short) 0, (short) 32, &isc_198, (short) 0);
	while (1)
	   {
           isc_receive ((long*) 0L, (isc_req_handle*) &dbb->dbb_flds_request, (short) 1, (short) 34, &isc_200, (short) 0);
	   if (!isc_200.isc_202) break;
#line 926 "gpre_meta.epp"
		for (p = /*RFR.RDB$FIELD_NAME*/
			 isc_200.isc_201; *p && *p != ' '; p++);
#line 927 "gpre_meta.epp"
	*p = 0;
	PUSH((GPRE_NOD) MET_field(relation, /*RFR.RDB$FIELD_NAME*/
					    isc_200.isc_201), &stack);
#line 929 "gpre_meta.epp"
	count++;
	/*END_FOR;*/
	   }
	}
#line 931 "gpre_meta.epp"

	node = MAKE_NODE(nod_list, count);

	while (stack) {
		field = (GPRE_FLD) POP(&stack);
		reference = (REF) ALLOC(REF_LEN);
		reference->ref_field = field;
		reference->ref_context = context;
		field_node = MSC_unary(nod_field, (GPRE_NOD) reference);
		node->nod_arg[--count] = field_node;
	}

	return node;
}


/*____________________________________________________________
 *  
 *		Shutdown all attached databases.
 */  

void MET_fini( DBB end)
{
	DBB dbb;

	for (dbb = isc_databases; dbb && dbb != end; dbb = dbb->dbb_next)
		if (DB = dbb->dbb_handle) {
			if (gds_trans = dbb->dbb_transaction)
				/*COMMIT;*/
				{
				isc_commit_transaction ((long*) 0L, (isc_tr_handle*) &gds_trans);
				}
#line 960 "gpre_meta.epp"
			/*FINISH*/
			{
			if (DB)
			   isc_detach_database ((long*) 0L, &DB);
			};
#line 961 "gpre_meta.epp"
			dbb->dbb_handle = NULL_HANDLE;
			dbb->dbb_transaction = NULL_HANDLE;

			dbb->dbb_field_request = NULL_HANDLE;
			dbb->dbb_flds_request = NULL_HANDLE;
			dbb->dbb_relation_request = NULL_HANDLE;
			dbb->dbb_procedure_request = NULL_HANDLE;
			dbb->dbb_udf_request = NULL_HANDLE;
			dbb->dbb_trigger_request = NULL_HANDLE;
			dbb->dbb_proc_prms_request = NULL_HANDLE;
			dbb->dbb_proc_prm_fld_request = NULL_HANDLE;
			dbb->dbb_index_request = NULL_HANDLE;
			dbb->dbb_type_request = NULL_HANDLE;
			dbb->dbb_array_request = NULL_HANDLE;
			dbb->dbb_dimension_request = NULL_HANDLE;
			dbb->dbb_domain_request = NULL_HANDLE;
			dbb->dbb_generator_request = NULL_HANDLE;
			dbb->dbb_view_request = NULL_HANDLE;
			dbb->dbb_primary_key_request = NULL_HANDLE;
		}
}


/*____________________________________________________________
 *  
 *		Lookup a generator by name.
 *		If found, return string. If not, return NULL.
 */  

const char *MET_generator(const TEXT * string, DBB dbb)
{
	SYM symbol;
	char name[NAME_SIZE];

	strcpy(name, string);

	for (symbol = HSH_lookup(name); symbol; symbol = symbol->sym_homonym)
		if ((symbol->sym_type == SYM_generator) &&
			(dbb == (DBB) (symbol->sym_object))) 
			return symbol->sym_string;

	return NULL;
}


/*____________________________________________________________
 *  
 *		Compute internal datatype and length based on system relation field values.
 */  

USHORT MET_get_dtype(USHORT blr_dtype, USHORT sub_type, USHORT * length)
{
	USHORT l, dtype;

	l = *length;

	switch (blr_dtype) {
	case blr_varying:
	case blr_text:
		dtype = dtype_text;
		if (sw_cstring && !SUBTYPE_ALLOWS_NULLS(sub_type)) {
			++l;
			dtype = dtype_cstring;
		}
		break;

	case blr_cstring:
		dtype = dtype_cstring;
		++l;
		break;

	case blr_short:
		dtype = dtype_short;
		l = sizeof(SSHORT);
		break;

	case blr_long:
		dtype = dtype_long;
		l = sizeof(SLONG);
		break;

	case blr_quad:
		dtype = dtype_quad;
		l = sizeof(GDS__QUAD);
		break;

	case blr_float:
		dtype = dtype_real;
		l = sizeof(float);
		break;

	case blr_double:
		dtype = dtype_double;
		l = sizeof(double);
		break;

	case blr_blob:
		dtype = dtype_blob;
		l = sizeof(GDS__QUAD);
		break;

/** Begin sql date/time/timestamp **/
	case blr_sql_date:
		dtype = dtype_sql_date;
		l = sizeof(ISC_DATE);
		break;

	case blr_sql_time:
		dtype = dtype_sql_time;
		l = sizeof(ISC_TIME);
		break;

	case blr_timestamp:
		dtype = dtype_timestamp;
		l = sizeof(ISC_TIMESTAMP);
		break;
/** Begin sql date/time/timestamp **/

	case blr_int64:
		dtype = dtype_int64;
		l = sizeof(ISC_INT64);
		break;

	default:
		CPR_error("datatype not supported");
	}

	*length = l;

	return dtype;
}


/*____________________________________________________________
 *  
 *		Lookup a procedure (represented by a token) in a database.
 *		Return a procedure block (if name is found) or NULL.
 *  
 *		This function has been cloned into MET_get_udf
 */  

GPRE_PRC MET_get_procedure(DBB dbb, const TEXT * string, const TEXT * owner_name)
{
   struct {
          short isc_171;	/* isc_utility */
          short isc_172;	/* RDB$SEGMENT_LENGTH */
          short isc_173;	/* gds__null_flag */
          short isc_174;	/* RDB$COLLATION_ID */
          short isc_175;	/* gds__null_flag */
          short isc_176;	/* RDB$CHARACTER_SET_ID */
          short isc_177;	/* gds__null_flag */
          short isc_178;	/* RDB$CHARACTER_LENGTH */
          short isc_179;	/* RDB$FIELD_TYPE */
          short isc_180;	/* RDB$FIELD_SUB_TYPE */
          short isc_181;	/* RDB$FIELD_SCALE */
          short isc_182;	/* RDB$FIELD_LENGTH */
   } isc_170;
   struct {
          char  isc_169 [32];	/* RDB$FIELD_SOURCE */
   } isc_168;
   struct {
          char  isc_188 [32];	/* RDB$PARAMETER_NAME */
          char  isc_189 [32];	/* RDB$FIELD_SOURCE */
          short isc_190;	/* isc_utility */
          short isc_191;	/* RDB$PARAMETER_NUMBER */
   } isc_187;
   struct {
          char  isc_185 [32];	/* RDB$PROCEDURE_NAME */
          short isc_186;	/* RDB$PARAMETER_TYPE */
   } isc_184;
   struct {
          short isc_196;	/* isc_utility */
   } isc_195;
   struct {
          short isc_194;	/* RDB$PROCEDURE_ID */
   } isc_193;
#line 1104 "gpre_meta.epp"
	SYM symbol;
	GPRE_FLD *fld_list, field;
	GPRE_PRC procedure;
	USHORT type, count;
	SCHAR name[NAME_SIZE], owner[NAME_SIZE];

	strcpy(name, string);
	strcpy(owner, owner_name);
	procedure = NULL;

	for (symbol = HSH_lookup(name); symbol; symbol = symbol->sym_homonym)
		if (symbol->sym_type == SYM_procedure &&
			(procedure = (GPRE_PRC) symbol->sym_object) &&
			procedure->prc_database == dbb &&
			(!owner[0] ||
			 (procedure->prc_owner
			  && !strcmp(owner, procedure->prc_owner->sym_string)))) break;

	if (!procedure)
		return NULL;

	if (procedure->prc_flags & PRC_scanned)
		return procedure;

	/*FOR(REQUEST_HANDLE dbb->dbb_procedure_request)
	X IN RDB$PROCEDURES WITH X.RDB$PROCEDURE_ID = procedure->prc_id*/
	{
        if (!dbb->dbb_procedure_request)
           isc_compile_request ((long*) 0L, (isc_db_handle*) &DB, (isc_req_handle*) &dbb->dbb_procedure_request, (short) sizeof (isc_192), (char *) isc_192);
	isc_193.isc_194 = procedure->prc_id;
        isc_start_and_send ((long*) 0L, (isc_req_handle*) &dbb->dbb_procedure_request, (isc_req_handle*) &gds_trans, (short) 0, (short) 2, &isc_193, (short) 0);
	while (1)
	   {
           isc_receive ((long*) 0L, (isc_req_handle*) &dbb->dbb_procedure_request, (short) 1, (short) 2, &isc_195, (short) 0);
	   if (!isc_195.isc_196) break;;
#line 1130 "gpre_meta.epp"

	for (type = 0; type < 2; type++) {
		count = 0;
		if (type)
			fld_list = &procedure->prc_outputs;
		else
			fld_list = &procedure->prc_inputs;

		/*FOR(REQUEST_HANDLE dbb->dbb_proc_prms_request)
		Y IN RDB$PROCEDURE_PARAMETERS WITH
			Y.RDB$PROCEDURE_NAME EQ name AND
			Y.RDB$PARAMETER_TYPE EQ type
			SORTED BY DESCENDING Y.RDB$PARAMETER_NUMBER*/
		{
                if (!dbb->dbb_proc_prms_request)
                   isc_compile_request ((long*) 0L, (isc_db_handle*) &DB, (isc_req_handle*) &dbb->dbb_proc_prms_request, (short) sizeof (isc_183), (char *) isc_183);
		isc_vtov ((char*)name, (char*)isc_184.isc_185, 32);
		isc_184.isc_186 = type;
                isc_start_and_send ((long*) 0L, (isc_req_handle*) &dbb->dbb_proc_prms_request, (isc_req_handle*) &gds_trans, (short) 0, (short) 34, &isc_184, (short) 0);
		while (1)
		   {
                   isc_receive ((long*) 0L, (isc_req_handle*) &dbb->dbb_proc_prms_request, (short) 1, (short) 68, &isc_187, (short) 0);
		   if (!isc_187.isc_190) break; count++;
#line 1143 "gpre_meta.epp"

		/*FOR(REQUEST_HANDLE dbb->dbb_proc_prm_fld_request)
		F IN RDB$FIELDS WITH
			Y.RDB$FIELD_SOURCE EQ F.RDB$FIELD_NAME*/
		{
                if (!dbb->dbb_proc_prm_fld_request)
                   isc_compile_request ((long*) 0L, (isc_db_handle*) &DB, (isc_req_handle*) &dbb->dbb_proc_prm_fld_request, (short) sizeof (isc_167), (char *) isc_167);
		isc_vtov ((char*)isc_187.isc_189, (char*)isc_168.isc_169, 32);
                isc_start_and_send ((long*) 0L, (isc_req_handle*) &dbb->dbb_proc_prm_fld_request, (isc_req_handle*) &gds_trans, (short) 0, (short) 32, &isc_168, (short) 0);
		while (1)
		   {
                   isc_receive ((long*) 0L, (isc_req_handle*) &dbb->dbb_proc_prm_fld_request, (short) 1, (short) 24, &isc_170, (short) 0);
		   if (!isc_170.isc_171) break;
#line 1147 "gpre_meta.epp"
			field = (GPRE_FLD) ALLOC(FLD_LEN);
		field->fld_procedure = procedure;
		field->fld_next = *fld_list;
		*fld_list = field;
		field->fld_position = /*Y.RDB$PARAMETER_NUMBER*/
				      isc_187.isc_191;
#line 1152 "gpre_meta.epp"
		field->fld_length = /*F.RDB$FIELD_LENGTH*/
				    isc_170.isc_182;
#line 1153 "gpre_meta.epp"
		field->fld_scale = /*F.RDB$FIELD_SCALE*/
				   isc_170.isc_181;
#line 1154 "gpre_meta.epp"
		field->fld_sub_type = /*F.RDB$FIELD_SUB_TYPE*/
				      isc_170.isc_180;
#line 1155 "gpre_meta.epp"
		field->fld_dtype = MET_get_dtype(/*F.RDB$FIELD_TYPE*/
						 isc_170.isc_179,
#line 1156 "gpre_meta.epp"
										 /*F.RDB$FIELD_SUB_TYPE*/
										 isc_170.isc_180,
#line 1157 "gpre_meta.epp"
										 &field->fld_length);
		switch (field->fld_dtype) {
		case dtype_text:
		case dtype_cstring:
			field->fld_flags |= FLD_text;
			if (!/*F.RDB$CHARACTER_LENGTH.NULL*/
			     isc_170.isc_177)
#line 1163 "gpre_meta.epp"
				field->fld_char_length = /*F.RDB$CHARACTER_LENGTH*/
							 isc_170.isc_178;
#line 1164 "gpre_meta.epp"
			if (!/*F.RDB$CHARACTER_SET_ID.NULL*/
			     isc_170.isc_175)
#line 1165 "gpre_meta.epp"
				field->fld_charset_id = /*F.RDB$CHARACTER_SET_ID*/
							isc_170.isc_176;
#line 1166 "gpre_meta.epp"
			if (!/*F.RDB$COLLATION_ID.NULL*/
			     isc_170.isc_173)
#line 1167 "gpre_meta.epp"
				field->fld_collate_id = /*F.RDB$COLLATION_ID*/
							isc_170.isc_174;
#line 1168 "gpre_meta.epp"
			field->fld_ttype =
				INTL_CS_COLL_TO_TTYPE(field->fld_charset_id,
									  field->fld_collate_id);
			break;

		case dtype_blob:
			field->fld_flags |= FLD_blob;
			field->fld_seg_length = /*F.RDB$SEGMENT_LENGTH*/
						isc_170.isc_172;
#line 1176 "gpre_meta.epp"
			if (!/*F.RDB$CHARACTER_SET_ID.NULL*/
			     isc_170.isc_175)
#line 1177 "gpre_meta.epp"
				field->fld_charset_id = /*F.RDB$CHARACTER_SET_ID*/
							isc_170.isc_176;
#line 1178 "gpre_meta.epp"
			break;
		}
		field->fld_symbol = MSC_symbol(SYM_field,
									   /*Y.RDB$PARAMETER_NAME*/
									   isc_187.isc_188,
#line 1182 "gpre_meta.epp"
									   symbol_length(/*Y.RDB$PARAMETER_NAME*/
											 isc_187.isc_188),
#line 1183 "gpre_meta.epp"
									   (GPRE_CTX) field);
		/* If output parameter, insert in symbol table as a
		   field. */
		if (type)
			HSH_insert(field->fld_symbol);

		/*END_FOR;*/
		   }
		}
#line 1190 "gpre_meta.epp"

		/*END_FOR;*/
		   }
		}
#line 1192 "gpre_meta.epp"

		if (type)
			procedure->prc_out_count = count;
		else
			procedure->prc_in_count = count;
	}

	/*END_FOR;*/
	   }
	}
#line 1200 "gpre_meta.epp"
	procedure->prc_flags |= PRC_scanned;

/*  Generate a dummy relation to point to the procedure to use when procedure
 *  is used as a view.
 */

	return procedure;
}


/*____________________________________________________________
 *  
 *		Lookup a relation (represented by a token) in a database.
 *		Return a relation block (if name is found) or NULL.
 */  

GPRE_REL MET_get_relation(DBB dbb, const TEXT * string, const TEXT * owner_name)
{
	SYM symbol;
	GPRE_REL relation;
	char name[NAME_SIZE], owner[NAME_SIZE];

	strcpy(name, string);
	strcpy(owner, owner_name);

	for (symbol = HSH_lookup(name); symbol; symbol = symbol->sym_homonym)
		if (symbol->sym_type == SYM_relation &&
			(relation = (GPRE_REL) symbol->sym_object) &&
			relation->rel_database == dbb &&
			(!owner[0] ||
			 (relation->rel_owner
			  && !strcmp(owner,
						 relation->rel_owner->sym_string)))) return relation;

	return NULL;
}


/*____________________________________________________________
 *  
 */  

INTLSYM MET_get_text_subtype(SSHORT ttype)
{
	INTLSYM p;

	for (p = text_subtypes; p; p = p->intlsym_next)
		if (p->intlsym_ttype == ttype)
			return p;

	return NULL;
}


/*____________________________________________________________
 *  
 *		Lookup a udf (represented by a token) in a database.
 *		Return a udf block (if name is found) or NULL.
 *  
 *		This function was cloned from MET_get_procedure
 */  

UDF MET_get_udf(DBB dbb, const TEXT * string)
{
   struct {
          short isc_149;	/* isc_utility */
          short isc_150;	/* gds__null_flag */
          short isc_151;	/* RDB$CHARACTER_SET_ID */
          short isc_152;	/* RDB$FIELD_TYPE */
          short isc_153;	/* RDB$FIELD_SUB_TYPE */
          short isc_154;	/* RDB$FIELD_SCALE */
          short isc_155;	/* RDB$FIELD_LENGTH */
          short isc_156;	/* RDB$ARGUMENT_POSITION */
   } isc_148;
   struct {
          char  isc_147 [32];	/* RDB$FUNCTION_NAME */
   } isc_146;
   struct {
          short isc_161;	/* isc_utility */
          short isc_162;	/* RDB$FIELD_TYPE */
          short isc_163;	/* RDB$FIELD_SUB_TYPE */
          short isc_164;	/* RDB$FIELD_SCALE */
          short isc_165;	/* RDB$FIELD_LENGTH */
          short isc_166;	/* RDB$ARGUMENT_POSITION */
   } isc_160;
   struct {
          char  isc_159 [32];	/* RDB$FUNCTION_NAME */
   } isc_158;
#line 1264 "gpre_meta.epp"
	SYM symbol;
	GPRE_FLD field;
	UDF udf;
	SCHAR name[NAME_SIZE];

	strcpy(name, string);
	udf = NULL;
	for (symbol = HSH_lookup(name); symbol; symbol = symbol->sym_homonym)
		if (symbol->sym_type == SYM_udf &&
			(udf = (UDF) symbol->sym_object) && udf->udf_database == dbb)
			break;
	if (!udf)
		return NULL;

	if (udf->udf_flags & UDF_scanned)
		return udf;

	if (dbb->dbb_flags & DBB_v3) {
		/* Version of V4 request without new V4 metadata */
		/*FOR(REQUEST_HANDLE dbb->dbb_udf_request)
		UDF_DEF IN RDB$FUNCTIONS CROSS
			UDF_ARG IN RDB$FUNCTION_ARGUMENTS
			WITH UDF_DEF.RDB$FUNCTION_NAME EQ name AND
			UDF_DEF.RDB$FUNCTION_NAME EQ UDF_ARG.RDB$FUNCTION_NAME AND
			UDF_DEF.RDB$RETURN_ARGUMENT != UDF_ARG.RDB$ARGUMENT_POSITION
			SORTED BY DESCENDING UDF_ARG.RDB$ARGUMENT_POSITION*/
		{
                if (!dbb->dbb_udf_request)
                   isc_compile_request ((long*) 0L, (isc_db_handle*) &DB, (isc_req_handle*) &dbb->dbb_udf_request, (short) sizeof (isc_157), (char *) isc_157);
		isc_vtov ((char*)name, (char*)isc_158.isc_159, 32);
                isc_start_and_send ((long*) 0L, (isc_req_handle*) &dbb->dbb_udf_request, (isc_req_handle*) &gds_trans, (short) 0, (short) 32, &isc_158, (short) 0);
		while (1)
		   {
                   isc_receive ((long*) 0L, (isc_req_handle*) &dbb->dbb_udf_request, (short) 1, (short) 12, &isc_160, (short) 0);
		   if (!isc_160.isc_161) break;;
#line 1290 "gpre_meta.epp"

		field = (GPRE_FLD) ALLOC(FLD_LEN);
		field->fld_next = udf->udf_inputs;
		udf->udf_inputs = field;
		udf->udf_args++;
		field->fld_position = /*UDF_ARG.RDB$ARGUMENT_POSITION*/
				      isc_160.isc_166;
#line 1296 "gpre_meta.epp"
		field->fld_length = /*UDF_ARG.RDB$FIELD_LENGTH*/
				    isc_160.isc_165;
#line 1297 "gpre_meta.epp"
		field->fld_scale = /*UDF_ARG.RDB$FIELD_SCALE*/
				   isc_160.isc_164;
#line 1298 "gpre_meta.epp"
		field->fld_sub_type = /*UDF_ARG.RDB$FIELD_SUB_TYPE*/
				      isc_160.isc_163;
#line 1299 "gpre_meta.epp"
		field->fld_dtype = MET_get_dtype(/*UDF_ARG.RDB$FIELD_TYPE*/
						 isc_160.isc_162,
#line 1300 "gpre_meta.epp"
										 /*UDF_ARG.RDB$FIELD_SUB_TYPE*/
										 isc_160.isc_163,
#line 1301 "gpre_meta.epp"
										 &field->fld_length);
		switch (field->fld_dtype) {
		case dtype_text:
		case dtype_cstring:
			field->fld_flags |= FLD_text;
			break;

		case dtype_blob:
			field->fld_flags |= FLD_blob;
			break;
		}
		/*END_FOR;*/
		   }
		}
#line 1313 "gpre_meta.epp"
	}
	else {
		/* Same request as above, but with V4 metadata also fetched */
		/*FOR(REQUEST_HANDLE dbb->dbb_udf_request)
		UDF_DEF IN RDB$FUNCTIONS CROSS
			UDF_ARG IN RDB$FUNCTION_ARGUMENTS
			WITH UDF_DEF.RDB$FUNCTION_NAME EQ name AND
			UDF_DEF.RDB$FUNCTION_NAME EQ UDF_ARG.RDB$FUNCTION_NAME AND
			UDF_DEF.RDB$RETURN_ARGUMENT != UDF_ARG.RDB$ARGUMENT_POSITION
			SORTED BY DESCENDING UDF_ARG.RDB$ARGUMENT_POSITION*/
		{
                if (!dbb->dbb_udf_request)
                   isc_compile_request ((long*) 0L, (isc_db_handle*) &DB, (isc_req_handle*) &dbb->dbb_udf_request, (short) sizeof (isc_145), (char *) isc_145);
		isc_vtov ((char*)name, (char*)isc_146.isc_147, 32);
                isc_start_and_send ((long*) 0L, (isc_req_handle*) &dbb->dbb_udf_request, (isc_req_handle*) &gds_trans, (short) 0, (short) 32, &isc_146, (short) 0);
		while (1)
		   {
                   isc_receive ((long*) 0L, (isc_req_handle*) &dbb->dbb_udf_request, (short) 1, (short) 16, &isc_148, (short) 0);
		   if (!isc_148.isc_149) break;;
#line 1323 "gpre_meta.epp"

		field = (GPRE_FLD) ALLOC(FLD_LEN);
		field->fld_next = udf->udf_inputs;
		udf->udf_inputs = field;
		udf->udf_args++;
		field->fld_position = /*UDF_ARG.RDB$ARGUMENT_POSITION*/
				      isc_148.isc_156;
#line 1329 "gpre_meta.epp"
		field->fld_length = /*UDF_ARG.RDB$FIELD_LENGTH*/
				    isc_148.isc_155;
#line 1330 "gpre_meta.epp"
		field->fld_scale = /*UDF_ARG.RDB$FIELD_SCALE*/
				   isc_148.isc_154;
#line 1331 "gpre_meta.epp"
		field->fld_sub_type = /*UDF_ARG.RDB$FIELD_SUB_TYPE*/
				      isc_148.isc_153;
#line 1332 "gpre_meta.epp"
		field->fld_dtype = MET_get_dtype(/*UDF_ARG.RDB$FIELD_TYPE*/
						 isc_148.isc_152,
#line 1333 "gpre_meta.epp"
										 /*UDF_ARG.RDB$FIELD_SUB_TYPE*/
										 isc_148.isc_153,
#line 1334 "gpre_meta.epp"
										 &field->fld_length);
		switch (field->fld_dtype) {
		case dtype_text:
		case dtype_cstring:
			field->fld_flags |= FLD_text;
			if (!/*UDF_ARG.RDB$CHARACTER_SET_ID.NULL*/
			     isc_148.isc_150)
#line 1340 "gpre_meta.epp"
				field->fld_charset_id = /*UDF_ARG.RDB$CHARACTER_SET_ID*/
							isc_148.isc_151;
#line 1341 "gpre_meta.epp"
			field->fld_ttype =
				INTL_CS_COLL_TO_TTYPE(field->fld_charset_id,
									  field->fld_collate_id);
			break;

		case dtype_blob:
			field->fld_flags |= FLD_blob;
			if (!/*UDF_ARG.RDB$CHARACTER_SET_ID.NULL*/
			     isc_148.isc_150)
#line 1349 "gpre_meta.epp"
				field->fld_charset_id = /*UDF_ARG.RDB$CHARACTER_SET_ID*/
							isc_148.isc_151;
#line 1350 "gpre_meta.epp"
			break;
		}
		/*END_FOR;*/
		   }
		}
#line 1353 "gpre_meta.epp"
	}

	udf->udf_flags |= UDF_scanned;

	return udf;
}


/*____________________________________________________________
 *  
 *		Return TRUE if the passed view_name represents a 
 *		view with the passed relation as a base table 
 *		(the relation could be an alias).
 */  

GPRE_REL MET_get_view_relation(GPRE_REQ request,
						  const TEXT *view_name,
						  const TEXT *relation_or_alias, USHORT level)
{
   struct {
          char  isc_142 [32];	/* RDB$RELATION_NAME */
          char  isc_143 [32];	/* RDB$CONTEXT_NAME */
          short isc_144;	/* isc_utility */
   } isc_141;
   struct {
          char  isc_140 [32];	/* RDB$VIEW_NAME */
   } isc_139;
#line 1372 "gpre_meta.epp"
	DBB dbb;
	TEXT *p;
	GPRE_REL relation;

	dbb = request->req_database;
	DB = dbb->dbb_handle;
	gds_trans = dbb->dbb_transaction;

	relation = NULL;

	/*FOR(REQUEST_HANDLE dbb->dbb_view_request, LEVEL level)
	X IN RDB$VIEW_RELATIONS WITH
		X.RDB$VIEW_NAME EQ view_name*/
	{
        if (!dbb->dbb_view_request)
           isc_compile_request ((long*) 0L, (isc_db_handle*) &DB, (isc_req_handle*) &dbb->dbb_view_request, (short) sizeof (isc_138), (char *) isc_138);
	isc_vtov ((char*)view_name, (char*)isc_139.isc_140, 32);
        isc_start_and_send ((long*) 0L, (isc_req_handle*) &dbb->dbb_view_request, (isc_req_handle*) &gds_trans, (short) 0, (short) 32, &isc_139, (short) level);
	while (1)
	   {
           isc_receive ((long*) 0L, (isc_req_handle*) &dbb->dbb_view_request, (short) 1, (short) 66, &isc_141, (short) level);
	   if (!isc_141.isc_144) break;
#line 1385 "gpre_meta.epp"
		for (p = /*X.RDB$CONTEXT_NAME*/
			 isc_141.isc_143; *p && *p != ' '; p++);
#line 1386 "gpre_meta.epp"
	*p = 0;

	for (p = /*X.RDB$RELATION_NAME*/
		 isc_141.isc_142; *p && *p != ' '; p++);
#line 1389 "gpre_meta.epp"
	*p = 0;

	if (!strcmp(/*X.RDB$RELATION_NAME*/
		    isc_141.isc_142, relation_or_alias) ||
#line 1392 "gpre_meta.epp"
		!strcmp(/*X.RDB$CONTEXT_NAME*/
			isc_141.isc_143, relation_or_alias))
#line 1393 "gpre_meta.epp"
		return MET_get_relation(dbb, /*X.RDB$RELATION_NAME*/
					     isc_141.isc_142, "");
#line 1394 "gpre_meta.epp"

	if (relation =
		MET_get_view_relation(request, /*X.RDB$RELATION_NAME*/
					       isc_141.isc_142, relation_or_alias,
#line 1397 "gpre_meta.epp"
							  level + 1))
		return relation;

	/*END_FOR;*/
	   }
	}
#line 1401 "gpre_meta.epp"

	return NULL;
}


/*____________________________________________________________
 *  
 *		Lookup an index for a database.
 *		Return an index block (if name is found) or NULL.
 */  

IND MET_index(DBB dbb, const TEXT * string)
{
   struct {
          char  isc_136 [32];	/* RDB$RELATION_NAME */
          short isc_137;	/* isc_utility */
   } isc_135;
   struct {
          char  isc_134 [32];	/* RDB$INDEX_NAME */
   } isc_133;
#line 1414 "gpre_meta.epp"
	SYM symbol;
	IND index;
	SCHAR name[NAME_SIZE];
	SSHORT length;

	strcpy(name, string);
	length = strlen(name);

	for (symbol = HSH_lookup(name); symbol; symbol = symbol->sym_homonym)
		if (symbol->sym_type == SYM_index &&
			(index = (IND) symbol->sym_object) &&
			index->ind_relation->rel_database == dbb)
			return index;

	if (sw_language == lang_internal)
		return NULL;

	if (dbb->dbb_flags & DBB_sqlca)
		return NULL;

	DB = dbb->dbb_handle;
	gds_trans = dbb->dbb_transaction;
	index = NULL;

	/*FOR(REQUEST_HANDLE dbb->dbb_index_request)
	X IN RDB$INDICES WITH X.RDB$INDEX_NAME EQ name*/
	{
        if (!dbb->dbb_index_request)
           isc_compile_request ((long*) 0L, (isc_db_handle*) &DB, (isc_req_handle*) &dbb->dbb_index_request, (short) sizeof (isc_132), (char *) isc_132);
	isc_vtov ((char*)name, (char*)isc_133.isc_134, 32);
        isc_start_and_send ((long*) 0L, (isc_req_handle*) &dbb->dbb_index_request, (isc_req_handle*) &gds_trans, (short) 0, (short) 32, &isc_133, (short) 0);
	while (1)
	   {
           isc_receive ((long*) 0L, (isc_req_handle*) &dbb->dbb_index_request, (short) 1, (short) 34, &isc_135, (short) 0);
	   if (!isc_135.isc_137) break;
#line 1440 "gpre_meta.epp"
		index = (IND) ALLOC(IND_LEN);
	index->ind_symbol = symbol = MSC_symbol(SYM_index, name, length, (GPRE_CTX) index);
	index->ind_relation = MET_get_relation(dbb, /*X.RDB$RELATION_NAME*/
						    isc_135.isc_136, "");
#line 1443 "gpre_meta.epp"
	HSH_insert(symbol);

	/*END_FOR;*/
	   }
	}
#line 1446 "gpre_meta.epp"

	return index;
}


/*____________________________________________________________
 *  
 *		Load all of the relation names
 *       and user defined function names
 *       into the symbol (hash) table.
 */  

void MET_load_hash_table( DBB dbb)
{
   struct {
          char  isc_51 [32];	/* RDB$GENERATOR_NAME */
          short isc_52;	/* isc_utility */
   } isc_50;
   struct {
          char  isc_55 [32];	/* RDB$CHARACTER_SET_NAME */
          short isc_56;	/* isc_utility */
   } isc_54;
   struct {
          char  isc_62 [32];	/* RDB$TYPE_NAME */
          short isc_63;	/* isc_utility */
   } isc_61;
   struct {
          char  isc_59 [32];	/* RDB$CHARACTER_SET_NAME */
          short isc_60;	/* RDB$CHARACTER_SET_ID */
   } isc_58;
   struct {
          char  isc_66 [32];	/* RDB$CHARACTER_SET_NAME */
          short isc_67;	/* isc_utility */
          short isc_68;	/* RDB$CHARACTER_SET_ID */
          short isc_69;	/* gds__null_flag */
          short isc_70;	/* RDB$BYTES_PER_CHARACTER */
          short isc_71;	/* RDB$COLLATION_ID */
          short isc_72;	/* RDB$CHARACTER_SET_ID */
   } isc_65;
   struct {
          char  isc_78 [32];	/* RDB$TYPE_NAME */
          short isc_79;	/* isc_utility */
   } isc_77;
   struct {
          char  isc_75 [32];	/* RDB$COLLATION_NAME */
          short isc_76;	/* RDB$COLLATION_ID */
   } isc_74;
   struct {
          char  isc_82 [32];	/* RDB$COLLATION_NAME */
          short isc_83;	/* isc_utility */
          short isc_84;	/* gds__null_flag */
          short isc_85;	/* RDB$BYTES_PER_CHARACTER */
          short isc_86;	/* RDB$COLLATION_ID */
          short isc_87;	/* RDB$CHARACTER_SET_ID */
   } isc_81;
   struct {
          short isc_93;	/* isc_utility */
          short isc_94;	/* RDB$COLLATION_ID */
          short isc_95;	/* RDB$CHARACTER_SET_ID */
   } isc_92;
   struct {
          char  isc_90 [32];	/* RDB$FUNCTION_NAME */
          short isc_91;	/* RDB$ARGUMENT_POSITION */
   } isc_89;
   struct {
          char  isc_98 [32];	/* RDB$FUNCTION_NAME */
          char  isc_99 [32];	/* RDB$QUERY_NAME */
          char  isc_100 [32];	/* RDB$FUNCTION_NAME */
          short isc_101;	/* isc_utility */
          short isc_102;	/* RDB$ARGUMENT_POSITION */
          short isc_103;	/* RDB$FIELD_TYPE */
          short isc_104;	/* RDB$FIELD_SUB_TYPE */
          short isc_105;	/* RDB$FIELD_SCALE */
          short isc_106;	/* RDB$FIELD_LENGTH */
          short isc_107;	/* RDB$FUNCTION_TYPE */
   } isc_97;
   struct {
          char  isc_110 [32];	/* RDB$OWNER_NAME */
          char  isc_111 [32];	/* RDB$PROCEDURE_NAME */
          short isc_112;	/* isc_utility */
          short isc_113;	/* gds__null_flag */
          short isc_114;	/* RDB$PROCEDURE_ID */
   } isc_109;
   struct {
          char  isc_117 [32];	/* RDB$OWNER_NAME */
          char  isc_118 [32];	/* RDB$RELATION_NAME */
          short isc_119;	/* isc_utility */
          short isc_120;	/* gds__null_flag */
          short isc_121;	/* RDB$DBKEY_LENGTH */
          short isc_122;	/* RDB$RELATION_ID */
   } isc_116;
   struct {
          char  isc_125 [32];	/* RDB$RELATION_NAME */
          short isc_126;	/* isc_utility */
          short isc_127;	/* RDB$DBKEY_LENGTH */
          short isc_128;	/* RDB$RELATION_ID */
   } isc_124;
   struct {
          short isc_131;	/* isc_utility */
   } isc_130;
#line 1460 "gpre_meta.epp"
	GPRE_REL relation;
	GPRE_PRC procedure;
	SYM symbol;
	GPRE_FLD dbkey;
	UDF udf;
	TEXT *p;
	isc_handle handle, handle2;
	USHORT post_v3_flag;
	SLONG length;
	INTLSYM iname;

/*  If this is an internal ISC access method invocation, don't do any of this
 *  stuff
 */

	if (sw_language == lang_internal)
		return;

	if (!dbb->dbb_handle)
		if (!MET_database(dbb, FALSE))
			CPR_exit(FINI_ERROR);

	if (dbb->dbb_transaction)
		/* we must have already loaded this one */
		return;

	gds_trans = NULL_HANDLE;
	DB = dbb->dbb_handle;

	/*START_TRANSACTION;*/
	{
	{
	isc_start_transaction ((long*) 0L, (isc_tr_handle*) &gds_trans, (short) 1, &DB, (short) 0, (char*) 0);
	}
	}
#line 1490 "gpre_meta.epp"

	dbb->dbb_transaction = gds_trans;
	handle = handle2 = NULL_HANDLE;

/*  Determine if the database is V3.  */

	post_v3_flag = FALSE;
	handle = NULL_HANDLE;
	/*FOR(REQUEST_HANDLE handle)
	X IN RDB$RELATIONS WITH
		X.RDB$RELATION_NAME = 'RDB$PROCEDURES' AND
		X.RDB$SYSTEM_FLAG = 1*/
	{
        if (!handle)
           isc_compile_request ((long*) 0L, (isc_db_handle*) &DB, (isc_req_handle*) &handle, (short) sizeof (isc_129), (char *) isc_129);
        isc_start_request ((long*) 0L, (isc_req_handle*) &handle, (isc_tr_handle*) &gds_trans, (short) 0);
	while (1)
	   {
           isc_receive ((long*) 0L, (isc_req_handle*) &handle, (short) 0, (short) 2, &isc_130, (short) 0);
	   if (!isc_130.isc_131) break; post_v3_flag = TRUE;
#line 1502 "gpre_meta.epp"

	/*END_FOR;*/
	   }
	}
#line 1504 "gpre_meta.epp"
	isc_release_request(isc_status, GDS_REF(handle));

	if (!post_v3_flag)
		dbb->dbb_flags |= DBB_v3;

/*  Pick up all relations (necessary to parse parts of the GDML grammar)  */

	if (dbb->dbb_flags & DBB_v3) {
		/*FOR(REQUEST_HANDLE handle)
		X IN RDB$RELATIONS*/
		{
                if (!handle)
                   isc_compile_request ((long*) 0L, (isc_db_handle*) &DB, (isc_req_handle*) &handle, (short) sizeof (isc_123), (char *) isc_123);
                isc_start_request ((long*) 0L, (isc_req_handle*) &handle, (isc_tr_handle*) &gds_trans, (short) 0);
		while (1)
		   {
                   isc_receive ((long*) 0L, (isc_req_handle*) &handle, (short) 0, (short) 38, &isc_124, (short) 0);
		   if (!isc_124.isc_126) break; relation = (GPRE_REL) ALLOC(REL_LEN);
#line 1514 "gpre_meta.epp"
		relation->rel_database = dbb;
		relation->rel_next = dbb->dbb_relations;
		dbb->dbb_relations = relation;
		relation->rel_id = /*X.RDB$RELATION_ID*/
				   isc_124.isc_128;
#line 1518 "gpre_meta.epp"
		relation->rel_symbol = symbol =
			MSC_symbol(SYM_relation, /*X.RDB$RELATION_NAME*/
						 isc_124.isc_125,
#line 1520 "gpre_meta.epp"
					   symbol_length(/*X.RDB$RELATION_NAME*/
							 isc_124.isc_125), (GPRE_CTX) relation);
#line 1521 "gpre_meta.epp"
		HSH_insert(symbol);
		relation->rel_dbkey = dbkey = MET_make_field("rdb$db_key", dtype_text,
													 (/*X.RDB$DBKEY_LENGTH*/
													  isc_124.isc_127) ? /*X.
													 RDB$DBKEY_LENGTH*/
    isc_124.isc_127 : 8,
#line 1525 "gpre_meta.epp"
													 FALSE);
		dbkey->fld_flags |= FLD_dbkey | FLD_text | FLD_charset;
		dbkey->fld_ttype = ttype_binary;

		/*END_FOR;*/
		   }
		}
#line 1530 "gpre_meta.epp"
	}
	else {
		/*FOR(REQUEST_HANDLE handle)
		X IN RDB$RELATIONS*/
		{
                if (!handle)
                   isc_compile_request ((long*) 0L, (isc_db_handle*) &DB, (isc_req_handle*) &handle, (short) sizeof (isc_115), (char *) isc_115);
                isc_start_request ((long*) 0L, (isc_req_handle*) &handle, (isc_tr_handle*) &gds_trans, (short) 0);
		while (1)
		   {
                   isc_receive ((long*) 0L, (isc_req_handle*) &handle, (short) 0, (short) 72, &isc_116, (short) 0);
		   if (!isc_116.isc_119) break; relation = (GPRE_REL) ALLOC(REL_LEN);
#line 1534 "gpre_meta.epp"
		relation->rel_database = dbb;
		relation->rel_next = dbb->dbb_relations;
		dbb->dbb_relations = relation;
		relation->rel_id = /*X.RDB$RELATION_ID*/
				   isc_116.isc_122;
#line 1538 "gpre_meta.epp"
		relation->rel_symbol = symbol =
			MSC_symbol(SYM_relation, /*X.RDB$RELATION_NAME*/
						 isc_116.isc_118,
#line 1540 "gpre_meta.epp"
					   symbol_length(/*X.RDB$RELATION_NAME*/
							 isc_116.isc_118), (GPRE_CTX) relation);
#line 1541 "gpre_meta.epp"
		HSH_insert(symbol);
		relation->rel_dbkey = dbkey = MET_make_field("rdb$db_key", dtype_text,
													 (/*X.RDB$DBKEY_LENGTH*/
													  isc_116.isc_121) ? /*X.
													 RDB$DBKEY_LENGTH*/
    isc_116.isc_121 : 8,
#line 1545 "gpre_meta.epp"
													 FALSE);
		dbkey->fld_flags |= FLD_dbkey | FLD_text | FLD_charset;
		dbkey->fld_ttype = ttype_binary;

		if (!/*X.RDB$OWNER_NAME.NULL*/
		     isc_116.isc_120
#line 1550 "gpre_meta.epp"
			&& (length =
				symbol_length(/*X.RDB$OWNER_NAME*/
					      isc_116.isc_117))) relation->rel_owner =
#line 1552 "gpre_meta.epp"
				MSC_symbol(SYM_username, /*X.RDB$OWNER_NAME*/
							 isc_116.isc_117, length, (GPRE_CTX) NULL_PTR);
#line 1553 "gpre_meta.epp"

		/*END_FOR;*/
		   }
		}
#line 1555 "gpre_meta.epp"
	}

	isc_release_request(isc_status, GDS_REF(handle));

/*  Pick up all procedures (necessary to parse parts of the GDML grammar) */

	/*FOR(REQUEST_HANDLE handle)
	X IN RDB$PROCEDURES*/
	{
        if (!handle)
           isc_compile_request (isc_status, (isc_db_handle*) &DB, (isc_req_handle*) &handle, (short) sizeof (isc_108), (char *) isc_108);
	if (handle)
           isc_start_request (isc_status, (isc_req_handle*) &handle, (isc_tr_handle*) &gds_trans, (short) 0);
	if (!isc_status [1]) {
	while (1)
	   {
           isc_receive (isc_status, (isc_req_handle*) &handle, (short) 0, (short) 70, &isc_109, (short) 0);
	   if (!isc_109.isc_112 || isc_status [1]) break; procedure = (GPRE_PRC) ALLOC(REL_LEN);
#line 1563 "gpre_meta.epp"
	procedure->prc_database = dbb;
	procedure->prc_next = (GPRE_PRC) dbb->dbb_procedures;
	dbb->dbb_procedures = (GPRE_REL) procedure;
	procedure->prc_id = /*X.RDB$PROCEDURE_ID*/
			    isc_109.isc_114;
#line 1567 "gpre_meta.epp"
	procedure->prc_symbol = symbol =
		MSC_symbol(SYM_procedure, /*X.RDB$PROCEDURE_NAME*/
					  isc_109.isc_111,
#line 1569 "gpre_meta.epp"
				   symbol_length(/*X.RDB$PROCEDURE_NAME*/
						 isc_109.isc_111), (GPRE_CTX) procedure);
#line 1570 "gpre_meta.epp"
	HSH_insert(symbol);
	if (!/*X.RDB$OWNER_NAME.NULL*/
	     isc_109.isc_113 && (length = symbol_length(/*X.RDB$OWNER_NAME*/
			    isc_109.isc_110)))
#line 1572 "gpre_meta.epp"
		procedure->prc_owner =
			MSC_symbol(SYM_username, /*X.RDB$OWNER_NAME*/
						 isc_109.isc_110, length, (GPRE_CTX) NULL_PTR);
#line 1574 "gpre_meta.epp"
	/*END_FOR*/
	   }
	   }; /*ON_ERROR*/
 if (isc_status [1])
    {
#line 1575 "gpre_meta.epp"
		/* assume pre V4 database, no procedures */
		/*END_ERROR;*/
		   }
		}
#line 1577 "gpre_meta.epp"

	if (handle)
		isc_release_request(isc_status, GDS_REF(handle));

/*  Pickup any user defined functions.  If the database does not support UDF's,
 *  this may fail
 */

	/*FOR(REQUEST_HANDLE handle)
	FUN IN RDB$FUNCTIONS CROSS ARG IN RDB$FUNCTION_ARGUMENTS WITH
		FUN.RDB$FUNCTION_NAME EQ ARG.RDB$FUNCTION_NAME AND
		FUN.RDB$RETURN_ARGUMENT EQ ARG.RDB$ARGUMENT_POSITION*/
	{
        if (!handle)
           isc_compile_request (isc_status, (isc_db_handle*) &DB, (isc_req_handle*) &handle, (short) sizeof (isc_96), (char *) isc_96);
	if (handle)
           isc_start_request (isc_status, (isc_req_handle*) &handle, (isc_tr_handle*) &gds_trans, (short) 0);
	if (!isc_status [1]) {
	while (1)
	   {
           isc_receive (isc_status, (isc_req_handle*) &handle, (short) 0, (short) 110, &isc_97, (short) 0);
	   if (!isc_97.isc_101 || isc_status [1]) break;
#line 1589 "gpre_meta.epp"
		p = /*FUN.RDB$FUNCTION_NAME*/
		    isc_97.isc_100;
#line 1590 "gpre_meta.epp"
	length = symbol_length(p);
	p[length] = 0;

	udf = (UDF) ALLOC(UDF_LEN + length);
	strcpy(udf->udf_function, p);
	udf->udf_database = dbb;
	udf->udf_type = /*FUN.RDB$FUNCTION_TYPE*/
			isc_97.isc_107;
#line 1597 "gpre_meta.epp"

	if (length = symbol_length(/*FUN.RDB$QUERY_NAME*/
				   isc_97.isc_99)) {
#line 1599 "gpre_meta.epp"
		p = /*FUN.RDB$QUERY_NAME*/
		    isc_97.isc_99;
#line 1600 "gpre_meta.epp"
		p[length] = 0;
	}
	udf->udf_symbol = symbol = MSC_symbol(SYM_udf, p, strlen(p), (GPRE_CTX) udf);
	HSH_insert(symbol);

	udf->udf_length = /*ARG.RDB$FIELD_LENGTH*/
			  isc_97.isc_106;
#line 1606 "gpre_meta.epp"
	udf->udf_scale = /*ARG.RDB$FIELD_SCALE*/
			 isc_97.isc_105;
#line 1607 "gpre_meta.epp"
	udf->udf_sub_type = /*ARG.RDB$FIELD_SUB_TYPE*/
			    isc_97.isc_104;
#line 1608 "gpre_meta.epp"
	udf->udf_dtype =
		MET_get_dtype(/*ARG.RDB$FIELD_TYPE*/
			      isc_97.isc_103, /*ARG.RDB$FIELD_SUB_TYPE*/
  isc_97.isc_104,
#line 1610 "gpre_meta.epp"
					  &udf->udf_length);

	if (post_v3_flag) {
		/*FOR(REQUEST_HANDLE handle2)
		V4ARG IN RDB$FUNCTION_ARGUMENTS
			CROSS CS IN RDB$CHARACTER_SETS
			CROSS COLL IN RDB$COLLATIONS WITH
			V4ARG.RDB$CHARACTER_SET_ID EQ CS.RDB$CHARACTER_SET_ID AND
			COLL.RDB$COLLATION_NAME EQ CS.RDB$DEFAULT_COLLATE_NAME AND
			V4ARG.RDB$CHARACTER_SET_ID NOT MISSING AND
			V4ARG.RDB$FUNCTION_NAME EQ ARG.RDB$FUNCTION_NAME AND
			V4ARG.RDB$ARGUMENT_POSITION EQ ARG.RDB$ARGUMENT_POSITION*/
		{
                if (!handle2)
                   isc_compile_request ((long*) 0L, (isc_db_handle*) &DB, (isc_req_handle*) &handle2, (short) sizeof (isc_88), (char *) isc_88);
		isc_vtov ((char*)isc_97.isc_98, (char*)isc_89.isc_90, 32);
		isc_89.isc_91 = isc_97.isc_102;
                isc_start_and_send ((long*) 0L, (isc_req_handle*) &handle2, (isc_req_handle*) &gds_trans, (short) 0, (short) 34, &isc_89, (short) 0);
		while (1)
		   {
                   isc_receive ((long*) 0L, (isc_req_handle*) &handle2, (short) 1, (short) 6, &isc_92, (short) 0);
		   if (!isc_92.isc_93) break;;
#line 1622 "gpre_meta.epp"

		udf->udf_charset_id = /*V4ARG.RDB$CHARACTER_SET_ID*/
				      isc_92.isc_95;
#line 1624 "gpre_meta.epp"
		udf->udf_ttype =
			INTL_CS_COLL_TO_TTYPE(udf->udf_charset_id, /*COLL.RDB$COLLATION_ID*/
								   isc_92.isc_94);
#line 1626 "gpre_meta.epp"

		/*END_FOR;*/
		   }
		}
#line 1628 "gpre_meta.epp"

	}
	/*END_FOR*/
	   }
	   }; /*ON_ERROR*/
 if (isc_status [1])
    { /*END_ERROR;*/
    }
 }
#line 1631 "gpre_meta.epp"

	isc_release_request(isc_status, GDS_REF(handle));
	if (handle2)
		isc_release_request(isc_status, GDS_REF(handle2));

/*  Pick up all Collation names, might have several collations
 *  for a given character set.
 *  There can also be several alias names for a character set.
 */  

	/*FOR(REQUEST_HANDLE handle)
	CHARSET IN RDB$CHARACTER_SETS CROSS COLL IN RDB$COLLATIONS OVER
		RDB$CHARACTER_SET_ID*/
	{
        if (!handle)
           isc_compile_request (isc_status, (isc_db_handle*) &DB, (isc_req_handle*) &handle, (short) sizeof (isc_80), (char *) isc_80);
	if (handle)
           isc_start_request (isc_status, (isc_req_handle*) &handle, (isc_tr_handle*) &gds_trans, (short) 0);
	if (!isc_status [1]) {
	while (1)
	   {
           isc_receive (isc_status, (isc_req_handle*) &handle, (short) 0, (short) 42, &isc_81, (short) 0);
	   if (!isc_81.isc_83 || isc_status [1]) break;;
#line 1644 "gpre_meta.epp"

	p = /*COLL.RDB$COLLATION_NAME*/
	    isc_81.isc_82;
#line 1646 "gpre_meta.epp"
	length = symbol_length(p);
	p[length] = 0;
	iname = (INTLSYM) ALLOC(INTLSYM_LEN + length);
	strcpy(iname->intlsym_name, p);
	iname->intlsym_database = dbb;
	iname->intlsym_symbol = symbol =
		MSC_symbol(SYM_collate, p, strlen(p), (GPRE_CTX) iname);
	HSH_insert(symbol);
	iname->intlsym_type = INTLSYM_collation;
	iname->intlsym_flags = 0;
	iname->intlsym_charset_id = /*COLL.RDB$CHARACTER_SET_ID*/
				    isc_81.isc_87;
#line 1657 "gpre_meta.epp"
	iname->intlsym_collate_id = /*COLL.RDB$COLLATION_ID*/
				    isc_81.isc_86;
#line 1658 "gpre_meta.epp"
	iname->intlsym_ttype =
		INTL_CS_COLL_TO_TTYPE(iname->intlsym_charset_id,
							  iname->intlsym_collate_id);
	iname->intlsym_bytes_per_char =
		(/*CHARSET.RDB$BYTES_PER_CHARACTER.NULL*/
		 isc_81.isc_84) ? 1 : (/*CHARSET.
													  RDB$BYTES_PER_CHARACTER*/
	 isc_81.isc_85);
#line 1664 "gpre_meta.epp"
	iname->intlsym_next = text_subtypes;
	text_subtypes = iname;

	/*FOR(REQUEST_HANDLE handle2)
	TYPE IN RDB$TYPES
		WITH TYPE.RDB$FIELD_NAME = "RDB$COLLATION_NAME"
		AND TYPE.RDB$TYPE = COLL.RDB$COLLATION_ID
		AND TYPE.RDB$TYPE_NAME != COLL.RDB$COLLATION_NAME*/
	{
        if (!handle2)
           isc_compile_request (isc_status, (isc_db_handle*) &DB, (isc_req_handle*) &handle2, (short) sizeof (isc_73), (char *) isc_73);
	isc_vtov ((char*)isc_81.isc_82, (char*)isc_74.isc_75, 32);
	isc_74.isc_76 = isc_81.isc_86;
	if (handle2)
           isc_start_and_send (isc_status, (isc_req_handle*) &handle2, (isc_req_handle*) &gds_trans, (short) 0, (short) 34, &isc_74, (short) 0);
	if (!isc_status [1]) {
	while (1)
	   {
           isc_receive (isc_status, (isc_req_handle*) &handle2, (short) 1, (short) 34, &isc_77, (short) 0);
	   if (!isc_77.isc_79 || isc_status [1]) break;;
#line 1672 "gpre_meta.epp"

	p = /*TYPE.RDB$TYPE_NAME*/
	    isc_77.isc_78;
#line 1674 "gpre_meta.epp"
	length = symbol_length(p);
	p[length] = 0;
	symbol = MSC_symbol(SYM_collate, p, length, (GPRE_CTX) iname);
	HSH_insert(symbol);
	/*END_FOR*/
	   }
	   }; /*ON_ERROR*/
 if (isc_status [1])
    { /*END_ERROR;*/
    }
 }
#line 1679 "gpre_meta.epp"

	/*END_FOR*/
	   }
	   }; /*ON_ERROR*/
 if (isc_status [1])
    {
#line 1681 "gpre_meta.epp"
		/* assume pre V4 database, no collations */
		/*END_ERROR;*/
		   }
		}
#line 1683 "gpre_meta.epp"

	isc_release_request(isc_status, GDS_REF(handle));
	if (handle2)
		isc_release_request(isc_status, GDS_REF(handle2));

/*  Now pick up all character set names - with the subtype set to
 *  the type of the default collation for the character set.
 */  
	/*FOR(REQUEST_HANDLE handle)
	CHARSET IN RDB$CHARACTER_SETS CROSS COLL IN RDB$COLLATIONS
		WITH CHARSET.RDB$DEFAULT_COLLATE_NAME EQ COLL.RDB$COLLATION_NAME*/
	{
        if (!handle)
           isc_compile_request (isc_status, (isc_db_handle*) &DB, (isc_req_handle*) &handle, (short) sizeof (isc_64), (char *) isc_64);
	if (handle)
           isc_start_request (isc_status, (isc_req_handle*) &handle, (isc_tr_handle*) &gds_trans, (short) 0);
	if (!isc_status [1]) {
	while (1)
	   {
           isc_receive (isc_status, (isc_req_handle*) &handle, (short) 0, (short) 44, &isc_65, (short) 0);
	   if (!isc_65.isc_67 || isc_status [1]) break;;
#line 1694 "gpre_meta.epp"

	p = /*CHARSET.RDB$CHARACTER_SET_NAME*/
	    isc_65.isc_66;
#line 1696 "gpre_meta.epp"
	length = symbol_length(p);
	p[length] = 0;
	iname = (INTLSYM) ALLOC(INTLSYM_LEN + length);
	strcpy(iname->intlsym_name, p);
	iname->intlsym_database = dbb;
	iname->intlsym_symbol = symbol =
		MSC_symbol(SYM_charset, p, strlen(p), (GPRE_CTX) iname);
	HSH_insert(symbol);
	iname->intlsym_type = INTLSYM_collation;
	iname->intlsym_flags = 0;
	iname->intlsym_charset_id = /*COLL.RDB$CHARACTER_SET_ID*/
				    isc_65.isc_72;
#line 1707 "gpre_meta.epp"
	iname->intlsym_collate_id = /*COLL.RDB$COLLATION_ID*/
				    isc_65.isc_71;
#line 1708 "gpre_meta.epp"
	iname->intlsym_ttype =
		INTL_CS_COLL_TO_TTYPE(iname->intlsym_charset_id,
							  iname->intlsym_collate_id);
	iname->intlsym_bytes_per_char =
		(/*CHARSET.RDB$BYTES_PER_CHARACTER.NULL*/
		 isc_65.isc_69) ? 1 : (/*CHARSET.
													  RDB$BYTES_PER_CHARACTER*/
	 isc_65.isc_70);
#line 1714 "gpre_meta.epp"

	/*FOR(REQUEST_HANDLE handle2)
	TYPE IN RDB$TYPES
		WITH TYPE.RDB$FIELD_NAME = "RDB$CHARACTER_SET_NAME"
		AND TYPE.RDB$TYPE = CHARSET.RDB$CHARACTER_SET_ID
		AND TYPE.RDB$TYPE_NAME != CHARSET.RDB$CHARACTER_SET_NAME*/
	{
        if (!handle2)
           isc_compile_request (isc_status, (isc_db_handle*) &DB, (isc_req_handle*) &handle2, (short) sizeof (isc_57), (char *) isc_57);
	isc_vtov ((char*)isc_65.isc_66, (char*)isc_58.isc_59, 32);
	isc_58.isc_60 = isc_65.isc_68;
	if (handle2)
           isc_start_and_send (isc_status, (isc_req_handle*) &handle2, (isc_req_handle*) &gds_trans, (short) 0, (short) 34, &isc_58, (short) 0);
	if (!isc_status [1]) {
	while (1)
	   {
           isc_receive (isc_status, (isc_req_handle*) &handle2, (short) 1, (short) 34, &isc_61, (short) 0);
	   if (!isc_61.isc_63 || isc_status [1]) break;;
#line 1720 "gpre_meta.epp"

	p = /*TYPE.RDB$TYPE_NAME*/
	    isc_61.isc_62;
#line 1722 "gpre_meta.epp"
	length = symbol_length(p);
	p[length] = 0;
	symbol = MSC_symbol(SYM_charset, p, length, (GPRE_CTX) iname);
	HSH_insert(symbol);
	/*END_FOR*/
	   }
	   }; /*ON_ERROR*/
 if (isc_status [1])
    { /*END_ERROR;*/
    }
 }
#line 1727 "gpre_meta.epp"

	/*END_FOR*/
	   }
	   }; /*ON_ERROR*/
 if (isc_status [1])
    {
#line 1729 "gpre_meta.epp"
		/* assume pre V4 database, no character sets */
		/*END_ERROR;*/
		   }
		}
#line 1731 "gpre_meta.epp"

	isc_release_request(isc_status, GDS_REF(handle));
	if (handle2)
		isc_release_request(isc_status, GDS_REF(handle2));

/*  Pick up name of database default character set for SQL  */

	/*FOR(REQUEST_HANDLE handle)
		FIRST 1 DBB IN RDB$DATABASE
		WITH DBB.RDB$CHARACTER_SET_NAME NOT MISSING*/
	{
        if (!handle)
           isc_compile_request (isc_status, (isc_db_handle*) &DB, (isc_req_handle*) &handle, (short) sizeof (isc_53), (char *) isc_53);
	if (handle)
           isc_start_request (isc_status, (isc_req_handle*) &handle, (isc_tr_handle*) &gds_trans, (short) 0);
	if (!isc_status [1]) {
	while (1)
	   {
           isc_receive (isc_status, (isc_req_handle*) &handle, (short) 0, (short) 34, &isc_54, (short) 0);
	   if (!isc_54.isc_56 || isc_status [1]) break;
#line 1741 "gpre_meta.epp"
		p = /*DBB.RDB$CHARACTER_SET_NAME*/
		    isc_54.isc_55;
#line 1742 "gpre_meta.epp"
	length = symbol_length(p);
	p[length] = 0;
	dbb->dbb_def_charset = (TEXT *) ALLOC(length + 1);
	strcpy(dbb->dbb_def_charset, p);
	if (!MSC_find_symbol(HSH_lookup(dbb->dbb_def_charset), SYM_charset))
		CPR_warn("Default character set for database is not known");
	/*END_FOR*/
	   }
	   }; /*ON_ERROR*/
 if (isc_status [1])
    {
#line 1749 "gpre_meta.epp"
		/* Assume V3 Db, no default charset */
		/*END_ERROR;*/
		   }
		}
#line 1751 "gpre_meta.epp"

	isc_release_request(isc_status, GDS_REF(handle));

/*  Pick up all generators for the database  */

	/*FOR(REQUEST_HANDLE handle)
	X IN RDB$GENERATORS*/
	{
        if (!handle)
           isc_compile_request (isc_status, (isc_db_handle*) &DB, (isc_req_handle*) &handle, (short) sizeof (isc_49), (char *) isc_49);
	if (handle)
           isc_start_request (isc_status, (isc_req_handle*) &handle, (isc_tr_handle*) &gds_trans, (short) 0);
	if (!isc_status [1]) {
	while (1)
	   {
           isc_receive (isc_status, (isc_req_handle*) &handle, (short) 0, (short) 34, &isc_50, (short) 0);
	   if (!isc_50.isc_52 || isc_status [1]) break;
#line 1758 "gpre_meta.epp"
		symbol = MSC_symbol(SYM_generator, /*X.RDB$GENERATOR_NAME*/
						   isc_50.isc_51,
#line 1759 "gpre_meta.epp"
							symbol_length(/*X.RDB$GENERATOR_NAME*/
								      isc_50.isc_51), (GPRE_CTX) dbb);
#line 1760 "gpre_meta.epp"
	HSH_insert(symbol);

	/*END_FOR*/
	   }
	   }; /*ON_ERROR*/
 if (isc_status [1])
    { /*END_ERROR;*/
    }
 }
#line 1763 "gpre_meta.epp"

	isc_release_request(isc_status, GDS_REF(handle));

/*  now that we have attached to the database, resolve the character set
 *  request (if any) (and if we can)
 */  

	if (dbb->dbb_c_lc_ctype) {
		if (get_intl_char_subtype
			(&dbb->dbb_char_subtype, dbb->dbb_c_lc_ctype,
			 strlen(dbb->dbb_c_lc_ctype), dbb))
			dbb->dbb_know_subtype = 1;
		else {
			TEXT buffer[200];

			sprintf(buffer, "Cannot recognize character set '%s'",
					dbb->dbb_c_lc_ctype);
			PAR_error(buffer);
		}
		sw_know_interp = dbb->dbb_know_subtype;
		sw_interp = dbb->dbb_char_subtype;
	}
}


/*____________________________________________________________
 *  
 *		Make a field symbol.
 */  

GPRE_FLD MET_make_field(const TEXT * name,
				   SSHORT dtype, SSHORT length, BOOLEAN insert_flag)
{
	GPRE_FLD field;
	SYM symbol;

	field = (GPRE_FLD) ALLOC(FLD_LEN);
	field->fld_length = length;
	field->fld_dtype = dtype;
	field->fld_symbol = symbol =
		MSC_symbol(SYM_field, name, strlen(name), (GPRE_CTX) field);
	if (insert_flag)
		HSH_insert(symbol);

	return field;
}


/*____________________________________________________________
 *  
 *		Make an index symbol.
 */  

IND MET_make_index(const TEXT* name)
{
	IND index;

	index = (IND) ALLOC(IND_LEN);
	index->ind_symbol = MSC_symbol(SYM_index, name, strlen(name), (GPRE_CTX) index);

	return index;
}


/*____________________________________________________________
 *  
 *		Make an relation symbol.
 */  

GPRE_REL MET_make_relation(const TEXT * name)
{
	GPRE_REL relation;

	relation = (GPRE_REL) ALLOC(REL_LEN);
	relation->rel_symbol =
		MSC_symbol(SYM_relation, name, strlen(name), (GPRE_CTX) relation);

	return relation;
}


/*____________________________________________________________
 *  
 *		Lookup a type name for a field.
 */  

BOOLEAN MET_type(GPRE_FLD field, const TEXT* string, SSHORT * ptr)
{
   struct {
          short isc_47;	/* isc_utility */
          short isc_48;	/* RDB$TYPE */
   } isc_46;
   struct {
          char  isc_44 [32];	/* RDB$TYPE_NAME */
          char  isc_45 [32];	/* RDB$FIELD_NAME */
   } isc_43;
#line 1851 "gpre_meta.epp"
	GPRE_REL relation;
	DBB dbb;
	SYM symbol;
	TYP type;
	char buffer[32];			/* BASED ON RDB$TYPES.RDB$TYPE_NAME */
	char *p;

	for (symbol = HSH_lookup(string); symbol; symbol = symbol->sym_homonym)
		if (symbol->sym_type == SYM_type &&
			(type = (TYP) symbol->sym_object) &&
			(!type->typ_field || type->typ_field == field)) {
			*ptr = type->typ_value;
			return TRUE;
		}

	relation = field->fld_relation;
	dbb = relation->rel_database;
	DB = dbb->dbb_handle;
	gds_trans = dbb->dbb_transaction;

/*  Force the name to uppercase, using C locale rules for uppercasing  */
	for (p = buffer; *string && p < &buffer[sizeof(buffer) - 1];
		 p++, string++) *p = UPPER7(*string);
	*p = '\0';

	/*FOR(REQUEST_HANDLE dbb->dbb_type_request)
	X IN RDB$TYPES WITH X.RDB$FIELD_NAME EQ field->fld_global->sym_string AND
		X.RDB$TYPE_NAME EQ buffer*/
	{
        if (!dbb->dbb_type_request)
           isc_compile_request (isc_status, (isc_db_handle*) &DB, (isc_req_handle*) &dbb->dbb_type_request, (short) sizeof (isc_42), (char *) isc_42);
	isc_vtov ((char*)buffer, (char*)isc_43.isc_44, 32);
	isc_vtov ((char*)field->fld_global->sym_string, (char*)isc_43.isc_45, 32);
	if (dbb->dbb_type_request)
           isc_start_and_send (isc_status, (isc_req_handle*) &dbb->dbb_type_request, (isc_req_handle*) &gds_trans, (short) 0, (short) 64, &isc_43, (short) 0);
	if (!isc_status [1]) {
	while (1)
	   {
           isc_receive (isc_status, (isc_req_handle*) &dbb->dbb_type_request, (short) 1, (short) 4, &isc_46, (short) 0);
	   if (!isc_46.isc_47 || isc_status [1]) break; {
#line 1879 "gpre_meta.epp"
		type = (TYP) ALLOC(TYP_LEN);
		type->typ_field = field;
		*ptr = type->typ_value = /*X.RDB$TYPE*/
					 isc_46.isc_48;
#line 1882 "gpre_meta.epp"
		type->typ_symbol = symbol =
			MSC_symbol(SYM_type, string, strlen(string), (GPRE_CTX) type);
		HSH_insert(symbol);
		return TRUE;
	} /*END_FOR*/
	     }
	     }; /*ON_ERROR*/
 if (isc_status [1])
    { /*END_ERROR;*/
    }
 }
#line 1887 "gpre_meta.epp"

	return FALSE;
}


/*____________________________________________________________
 *  
 *		Lookup an index for a database.
 *  
 *  Return: TRUE if the trigger exists
 *		   FALSE otherwise
 */  

BOOLEAN MET_trigger_exists(DBB dbb, const TEXT* trigger_name)
{
   struct {
          short isc_41;	/* isc_utility */
   } isc_40;
   struct {
          char  isc_39 [32];	/* RDB$TRIGGER_NAME */
   } isc_38;
#line 1902 "gpre_meta.epp"
	char name[NAME_SIZE];

	strcpy(name, trigger_name);

	DB = dbb->dbb_handle;
	gds_trans = dbb->dbb_transaction;

	/*FOR(REQUEST_HANDLE dbb->dbb_trigger_request)
	TRIG IN RDB$TRIGGERS WITH TRIG.RDB$TRIGGER_NAME EQ name*/
	{
        if (!dbb->dbb_trigger_request)
           isc_compile_request ((long*) 0L, (isc_db_handle*) &DB, (isc_req_handle*) &dbb->dbb_trigger_request, (short) sizeof (isc_37), (char *) isc_37);
	isc_vtov ((char*)name, (char*)isc_38.isc_39, 32);
        isc_start_and_send ((long*) 0L, (isc_req_handle*) &dbb->dbb_trigger_request, (isc_req_handle*) &gds_trans, (short) 0, (short) 32, &isc_38, (short) 0);
	while (1)
	   {
           isc_receive ((long*) 0L, (isc_req_handle*) &dbb->dbb_trigger_request, (short) 1, (short) 2, &isc_40, (short) 0);
	   if (!isc_40.isc_41) break; return TRUE;
#line 1911 "gpre_meta.epp"

	/*END_FOR;*/
	   }
	}
#line 1913 "gpre_meta.epp"

	return FALSE;
}


/*____________________________________________________________
 *  
 *		Compute and return the size of the array.
 */  

static SLONG array_size( GPRE_FLD field)
{
	ARY array_block;
	DIM dimension;
	SLONG count;

	array_block = field->fld_array_info;
	count = field->fld_array->fld_length;
	for (dimension = array_block->ary_dimension; dimension;
		 dimension = dimension->dim_next) count =
			count * (dimension->dim_upper - dimension->dim_lower + 1);

	return count;
}


/*____________________________________________________________
 *  
 *		See if field is array.
 */  

static void get_array( DBB dbb, const TEXT * field_name, GPRE_FLD field)
{
   struct {
          SLONG isc_27;	/* RDB$UPPER_BOUND */
          SLONG isc_28;	/* RDB$LOWER_BOUND */
          short isc_29;	/* isc_utility */
          short isc_30;	/* RDB$DIMENSION */
   } isc_26;
   struct {
          char  isc_25 [32];	/* RDB$FIELD_NAME */
   } isc_24;
   struct {
          short isc_35;	/* isc_utility */
          short isc_36;	/* RDB$DIMENSIONS */
   } isc_34;
   struct {
          char  isc_33 [32];	/* RDB$FIELD_NAME */
   } isc_32;
#line 1946 "gpre_meta.epp"
	GPRE_FLD sub_field;
	ARY array_block;
	DIM dimension_block, last_dimension_block;

	array_block = NULL;

	/*FOR(REQUEST_HANDLE dbb->dbb_array_request)
	F IN RDB$FIELDS WITH F.RDB$FIELD_NAME EQ field_name*/
	{
        if (!dbb->dbb_array_request)
           isc_compile_request (isc_status, (isc_db_handle*) &DB, (isc_req_handle*) &dbb->dbb_array_request, (short) sizeof (isc_31), (char *) isc_31);
	isc_vtov ((char*)field_name, (char*)isc_32.isc_33, 32);
	if (dbb->dbb_array_request)
           isc_start_and_send (isc_status, (isc_req_handle*) &dbb->dbb_array_request, (isc_req_handle*) &gds_trans, (short) 0, (short) 32, &isc_32, (short) 0);
	if (!isc_status [1]) {
	while (1)
	   {
           isc_receive (isc_status, (isc_req_handle*) &dbb->dbb_array_request, (short) 1, (short) 4, &isc_34, (short) 0);
	   if (!isc_34.isc_35 || isc_status [1]) break; if (/*F.RDB$DIMENSIONS*/
     isc_34.isc_36) {
#line 1954 "gpre_meta.epp"
		sub_field = (GPRE_FLD) ALLOC(FLD_LEN);
		*sub_field = *field;
		field->fld_array = sub_field;
		field->fld_dtype = dtype_blob;
		field->fld_flags |= FLD_blob;
		field->fld_length = 8;
		field->fld_array_info = array_block =
			(ARY) ALLOC(ARY_LEN(/*F.RDB$DIMENSIONS*/
					    isc_34.isc_36));
#line 1962 "gpre_meta.epp"
		array_block->ary_dtype = sub_field->fld_dtype;
	} /*END_FOR*/
	     }
	     }; /*ON_ERROR*/
 if (isc_status [1])
    { {
#line 1964 "gpre_meta.epp"
		dbb->dbb_flags |= DBB_no_arrays;
		return;
	}
	/*END_ERROR;*/
	   }
	}
#line 1968 "gpre_meta.epp"

	if (!array_block)
		return;

	/*FOR(REQUEST_HANDLE dbb->dbb_dimension_request)
	D IN RDB$FIELD_DIMENSIONS WITH D.RDB$FIELD_NAME EQ field_name
		SORTED BY ASCENDING D.RDB$DIMENSION*/
	{
        if (!dbb->dbb_dimension_request)
           isc_compile_request ((long*) 0L, (isc_db_handle*) &DB, (isc_req_handle*) &dbb->dbb_dimension_request, (short) sizeof (isc_23), (char *) isc_23);
	isc_vtov ((char*)field_name, (char*)isc_24.isc_25, 32);
        isc_start_and_send ((long*) 0L, (isc_req_handle*) &dbb->dbb_dimension_request, (isc_req_handle*) &gds_trans, (short) 0, (short) 32, &isc_24, (short) 0);
	while (1)
	   {
           isc_receive ((long*) 0L, (isc_req_handle*) &dbb->dbb_dimension_request, (short) 1, (short) 12, &isc_26, (short) 0);
	   if (!isc_26.isc_29) break;
#line 1975 "gpre_meta.epp"
		array_block->ary_rpt[/*D.RDB$DIMENSION*/
				     isc_26.isc_30].ary_lower = /*D.RDB$LOWER_BOUND*/
	      isc_26.isc_28;
#line 1976 "gpre_meta.epp"
	array_block->ary_rpt[/*D.RDB$DIMENSION*/
			     isc_26.isc_30].ary_upper = /*D.RDB$UPPER_BOUND*/
	      isc_26.isc_27;
#line 1977 "gpre_meta.epp"
	dimension_block = (DIM) ALLOC(DIM_LEN);
	dimension_block->dim_number = /*D.RDB$DIMENSION*/
				      isc_26.isc_30;
#line 1979 "gpre_meta.epp"
	dimension_block->dim_lower = /*D.RDB$LOWER_BOUND*/
				     isc_26.isc_28;
#line 1980 "gpre_meta.epp"
	dimension_block->dim_upper = /*D.RDB$UPPER_BOUND*/
				     isc_26.isc_27;
#line 1981 "gpre_meta.epp"

	if (/*D.RDB$DIMENSION*/
	    isc_26.isc_30 != 0) {
#line 1983 "gpre_meta.epp"
		last_dimension_block->dim_next = dimension_block;
		dimension_block->dim_previous = last_dimension_block;
	}
	else
		array_block->ary_dimension = dimension_block;
	last_dimension_block = dimension_block;
	/*END_FOR;*/
	   }
	}
#line 1990 "gpre_meta.epp"

	array_block->ary_dimension_count = last_dimension_block->dim_number + 1;
	array_block->ary_size = array_size(field);
}


/*____________________________________________________________
 *  
 *		Character types can be specified as either:
 *		   b) A POSIX style locale name "<collation>.<characterset>"
 *		   or
 *		   c) A simple <characterset> name (using default collation)
 *		   d) A simple <collation> name    (use charset for collation)
 *  
 *		Given an ASCII7 string which could be any of the above, try to
 *		resolve the name in the order b, c, d.
 *		b) is only tried iff the name contains a period.
 *		(in which case c) and d) are not tried).
 *  
 *  Return:
 *		1 if no errors (and *id is set).
 *		0 if the name could not be resolved.
 */  

static int get_intl_char_subtype(SSHORT * id,
								 const TEXT* name, USHORT length, DBB dbb)
{
	char buffer[32];			/* BASED ON RDB$COLLATION_NAME */
	char *p;
	char *period = NULL;

	assert(id != NULL);
	assert(name != NULL);
	assert(dbb != NULL);


	DB = dbb->dbb_handle;
	
	if (!DB)
		return (0);
		
	gds_trans = dbb->dbb_transaction;
	const char* end_name = name + length;
	
	/*  Force key to uppercase, following C locale rules for uppercasing 
	 *  At the same time, search for the first period in the string (if any)
	 */
	 
	for (p = buffer; name < end_name && p < (buffer + sizeof(buffer) - 1);
		 p++, name++) 
		 {
		*p = UPPER7(*name);
		if ((*p == '.') && !period)
			period = p;
		};
		
	*p = 0;

/*  Is there a period, separating collation name from character set?  */
	if (period) {
		*period = 0;
		return (resolve_charset_and_collation(id, period + 1, buffer));
	}

	else {
		int res;

		/* Is it a character set name (implying charset default collation sequence) */
		res = resolve_charset_and_collation(id, buffer, NULL);
		if (!res) {
			/* Is it a collation name (implying implementation-default character set) */
			res = resolve_charset_and_collation(id, NULL, buffer);
		}
		return (res);
	}
}


/*____________________________________________________________
 *  
 *		Given ASCII7 name of charset & collation
 *		resolve the specification to a ttype (id) that implements
 *		it.
 *  
 *  Inputs:
 *		(charset) 
 *			ASCII7z name of characterset.
 *			NULL (implying unspecified) means use the character set
 *		        for defined for (collation).
 *  
 *		(collation)
 *			ASCII7z name of collation.
 *			NULL means use the default collation for (charset).
 *  
 *  Outputs:
 *		(*id)	
 *			Set to subtype specified by this name.
 *  
 *  Return:
 *		1 if no errors (and *id is set).
 *		0 if either name not found.
 *		  or if names found, but the collation isn't for the specified
 *		  character set.
 */  

static int resolve_charset_and_collation(SSHORT * id,
										 const TEXT* charset, const TEXT* collation)
{
   struct {
          short isc_5;	/* isc_utility */
          short isc_6;	/* RDB$CHARACTER_SET_ID */
   } isc_4;
   struct {
          char  isc_2 [32];	/* RDB$TYPE_NAME */
          char  isc_3 [32];	/* RDB$COLLATION_NAME */
   } isc_1;
   struct {
          short isc_11;	/* isc_utility */
          short isc_12;	/* RDB$CHARACTER_SET_ID */
   } isc_10;
   struct {
          char  isc_9 [32];	/* RDB$COLLATION_NAME */
   } isc_8;
   struct {
          short isc_17;	/* isc_utility */
          short isc_18;	/* RDB$CHARACTER_SET_ID */
   } isc_16;
   struct {
          char  isc_15 [32];	/* RDB$TYPE_NAME */
   } isc_14;
   struct {
          char  isc_21 [32];	/* RDB$CHARACTER_SET_NAME */
          short isc_22;	/* isc_utility */
   } isc_20;
#line 2098 "gpre_meta.epp"
	int found = 0;
	isc_handle request = NULL_HANDLE;

	assert(id != NULL);

	if (!DB)
		return (0);

	if (collation == NULL) {
		if (charset == NULL) {
			/*FOR(REQUEST_HANDLE request)
				FIRST 1 DBB IN RDB$DATABASE
				WITH DBB.RDB$CHARACTER_SET_NAME NOT MISSING
				AND DBB.RDB$CHARACTER_SET_NAME != " "*/
			{
                        if (!request)
                           isc_compile_request (isc_status, (isc_db_handle*) &DB, (isc_req_handle*) &request, (short) sizeof (isc_19), (char *) isc_19);
			if (request)
                           isc_start_request (isc_status, (isc_req_handle*) &request, (isc_tr_handle*) &gds_trans, (short) 0);
			if (!isc_status [1]) {
			while (1)
			   {
                           isc_receive (isc_status, (isc_req_handle*) &request, (short) 0, (short) 34, &isc_20, (short) 0);
			   if (!isc_20.isc_22 || isc_status [1]) break;;
#line 2112 "gpre_meta.epp"

			charset = /*DBB.RDB$CHARACTER_SET_NAME*/
				  isc_20.isc_21;
#line 2114 "gpre_meta.epp"

			/*END_FOR*/
			   }
			   }; /*ON_ERROR*/
 if (isc_status [1])
    {
#line 2116 "gpre_meta.epp"
				/* Assume V3 DB, without default character set */
				/*END_ERROR;*/
				   }
				}
#line 2118 "gpre_meta.epp"

			isc_release_request(isc_status, GDS_REF(request));

			if (charset == NULL)
				charset = DEFAULT_CHARACTER_SET_NAME;

		}

		/*FOR(REQUEST_HANDLE request)
			FIRST 1 CS IN RDB$CHARACTER_SETS
			CROSS COLL IN RDB$COLLATIONS
			CROSS TYPE IN RDB$TYPES
			WITH TYPE.RDB$TYPE_NAME EQ charset
			AND TYPE.RDB$FIELD_NAME EQ "RDB$CHARACTER_SET_NAME"
			AND TYPE.RDB$TYPE EQ CS.RDB$CHARACTER_SET_ID
			AND CS.RDB$DEFAULT_COLLATE_NAME EQ COLL.RDB$COLLATION_NAME*/
		{
                if (!request)
                   isc_compile_request (isc_status, (isc_db_handle*) &DB, (isc_req_handle*) &request, (short) sizeof (isc_13), (char *) isc_13);
		isc_vtov ((char*)charset, (char*)isc_14.isc_15, 32);
		if (request)
                   isc_start_and_send (isc_status, (isc_req_handle*) &request, (isc_req_handle*) &gds_trans, (short) 0, (short) 32, &isc_14, (short) 0);
		if (!isc_status [1]) {
		while (1)
		   {
                   isc_receive (isc_status, (isc_req_handle*) &request, (short) 1, (short) 4, &isc_16, (short) 0);
		   if (!isc_16.isc_17 || isc_status [1]) break;;
#line 2134 "gpre_meta.epp"

		found++;
		*id = MAP_CHARSET_TO_TTYPE(/*CS.RDB$CHARACTER_SET_ID*/
					   isc_16.isc_18);
#line 2137 "gpre_meta.epp"

		/*END_FOR*/
		   }
		   }; /*ON_ERROR*/
 if (isc_status [1])
    { /*END_ERROR;*/
    }
 }
#line 2139 "gpre_meta.epp"

		isc_release_request(isc_status, GDS_REF(request));

		return (found);
	}
	else if (charset == NULL) {
		/*FOR(REQUEST_HANDLE request)
			FIRST 1 CS IN RDB$CHARACTER_SETS
			CROSS COLL IN RDB$COLLATIONS
			OVER RDB$CHARACTER_SET_ID
			WITH COLL.RDB$COLLATION_NAME EQ collation*/
		{
                if (!request)
                   isc_compile_request (isc_status, (isc_db_handle*) &DB, (isc_req_handle*) &request, (short) sizeof (isc_7), (char *) isc_7);
		isc_vtov ((char*)collation, (char*)isc_8.isc_9, 32);
		if (request)
                   isc_start_and_send (isc_status, (isc_req_handle*) &request, (isc_req_handle*) &gds_trans, (short) 0, (short) 32, &isc_8, (short) 0);
		if (!isc_status [1]) {
		while (1)
		   {
                   isc_receive (isc_status, (isc_req_handle*) &request, (short) 1, (short) 4, &isc_10, (short) 0);
		   if (!isc_10.isc_11 || isc_status [1]) break;;
#line 2150 "gpre_meta.epp"

		found++;
		*id = MAP_CHARSET_TO_TTYPE(/*CS.RDB$CHARACTER_SET_ID*/
					   isc_10.isc_12);
#line 2153 "gpre_meta.epp"
		/*END_FOR*/
		   }
		   }; /*ON_ERROR*/
 if (isc_status [1])
    { /*END_ERROR;*/
    }
 }
#line 2154 "gpre_meta.epp"

		isc_release_request(isc_status, GDS_REF(request));

		return (found);
	}

	/*FOR(REQUEST_HANDLE request)
		FIRST 1 CS IN RDB$CHARACTER_SETS
		CROSS COL IN RDB$COLLATIONS OVER RDB$CHARACTER_SET_ID
		CROSS NAME2 IN RDB$TYPES
		WITH COL.RDB$COLLATION_NAME EQ collation
		AND NAME2.RDB$TYPE_NAME EQ charset
		AND NAME2.RDB$FIELD_NAME EQ "RDB$CHARACTER_SET_NAME"
		AND NAME2.RDB$TYPE EQ CS.RDB$CHARACTER_SET_ID*/
	{
        if (!request)
           isc_compile_request (isc_status, (isc_db_handle*) &DB, (isc_req_handle*) &request, (short) sizeof (isc_0), (char *) isc_0);
	isc_vtov ((char*)charset, (char*)isc_1.isc_2, 32);
	isc_vtov ((char*)collation, (char*)isc_1.isc_3, 32);
	if (request)
           isc_start_and_send (isc_status, (isc_req_handle*) &request, (isc_req_handle*) &gds_trans, (short) 0, (short) 64, &isc_1, (short) 0);
	if (!isc_status [1]) {
	while (1)
	   {
           isc_receive (isc_status, (isc_req_handle*) &request, (short) 1, (short) 4, &isc_4, (short) 0);
	   if (!isc_4.isc_5 || isc_status [1]) break;;
#line 2168 "gpre_meta.epp"

	found++;
	*id = MAP_CHARSET_TO_TTYPE(/*CS.RDB$CHARACTER_SET_ID*/
				   isc_4.isc_6);
#line 2171 "gpre_meta.epp"
	/*END_FOR*/
	   }
	   }; /*ON_ERROR*/
 if (isc_status [1])
    { /*END_ERROR;*/
    }
 }
#line 2172 "gpre_meta.epp"

	isc_release_request(isc_status, GDS_REF(request));

	return (found);
}


/*____________________________________________________________
 *  
 *		Compute significant length of symbol.
 */  

static int symbol_length( const TEXT* string)
{
	int len = (int) strlen(string);
	const TEXT *p = string + (len - 1);

	for (; p >= string && *p == ' '; p--)
		;
		
	if (p < string)
		return 0;
		
	return (int) (p - string) + 1;
}

#ifdef NOT_USED_OR_REPLACED
/*____________________________________________________________
 *  
 *		Upcase a string into another string.  Return
 *		length of string.
 */

static int upcase( const TEXT* from, TEXT * to)
{
	TEXT *p, *end, c;

	p = to;
	end = to + NAME_SIZE;

	while (p < end && (c = *from++))
		*p++ = UPPER(c);

	*p = 0;

	return (int) (p - to);
}
#endif

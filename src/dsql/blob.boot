/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version WI-V2.0.0.4027 Vulcan 1.0 Development **********************/
/*
 *	PROGRAM:	InterBase layered support library
 *	MODULE:		blob.epp
 *	DESCRIPTION:	Dynamic blob support
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
 *
 * 2001.09.10 Claudio Valderrama: get_name() was preventing the API calls
 *   isc_blob_default_desc, isc_blob_lookup_desc & isc_blob_set_desc
 *   from working properly with dialect 3 names. Therefore, incorrect names
 *   could be returned or a lookup for a blob field could fail. In addition,
 *   a possible buffer overrun due to unchecked bounds was closed. The fc
 *   get_name() as been renamed copy_exact_name().
 *
 */

#include "firebird.h"
#include "../jrd/common.h"
#include <stdarg.h>
#include "../jrd/ibase.h"
#include "../jrd/intl.h"
#include "../jrd/constants.h"
#include "../dsql/blob_proto.h"

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
static ISC_STATUS
   isc_status [20],	/* status vector */
   isc_status2 [20];	/* status vector */
static SLONG
   isc_array_length, 	/* array return size */
   SQLCODE;		/* SQL status code */
static isc_req_handle
   isc_0 = 0;		/* request handle */

static const short
   isc_1l = 291;
static const char
   isc_1 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 5,0, 
	    blr_cstring, 32,0, 
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
				 blr_field, 0, 17, 'R','D','B','$','R','E','L','A','T','I','O','N','_','N','A','M','E', 
				 blr_parameter, 0, 1,0, 
			      blr_eql, 
				 blr_field, 0, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
				 blr_parameter, 0, 0,0, 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_field, 1, 14, 'R','D','B','$','F','I','E','L','D','_','N','A','M','E', 
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 1,0, 
			blr_assignment, 
			   blr_field, 1, 18, 'R','D','B','$','S','E','G','M','E','N','T','_','L','E','N','G','T','H', 
			   blr_parameter, 1, 2,0, 
			blr_assignment, 
			   blr_field, 1, 20, 'R','D','B','$','C','H','A','R','A','C','T','E','R','_','S','E','T','_','I','D', 
			   blr_parameter, 1, 3,0, 
			blr_assignment, 
			   blr_field, 1, 18, 'R','D','B','$','F','I','E','L','D','_','S','U','B','_','T','Y','P','E', 
			   blr_parameter, 1, 4,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 1,0, 
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


static void copy_exact_name (const char*, char*, SSHORT);
static ISC_STATUS copy_status(const ISC_STATUS*, ISC_STATUS*);
static ISC_STATUS error(ISC_STATUS*, SSHORT, ...);


void API_ROUTINE isc_blob_default_desc(
									   ISC_BLOB_DESC* desc,
									   const char* relation_name,
									   const char* field_name)
{
/**************************************
 *
 *	i s c _ b l o b _ d e f a u l t _ d e s c
 *
 **************************************
 *
 * Functional description
 *
 *	This function will set the default
 *	values in the blob_descriptor.
 *
 **************************************/

	desc->blob_desc_subtype = BLOB_text;
	desc->blob_desc_charset = CS_dynamic;
	desc->blob_desc_segment_size = 80;

    copy_exact_name (field_name, desc->blob_desc_field_name,
                     sizeof(desc->blob_desc_field_name));
    copy_exact_name (relation_name, desc->blob_desc_relation_name,
                     sizeof(desc->blob_desc_relation_name));
}


ISC_STATUS API_ROUTINE isc_blob_gen_bpb(
									ISC_STATUS* status,
									const ISC_BLOB_DESC* to_desc,
									const ISC_BLOB_DESC* from_desc,
									USHORT bpb_buffer_length,
									UCHAR* bpb_buffer,
									USHORT* bpb_length)
{
/**************************************
 *
 *	i s c _ b l o b _ g e n _ b p b
 *
 **************************************
 *
 * Functional description
 *
 *  	This function will generate a bpb
 *	given a to_desc and a from_desc 
 *	which contain the subtype and
 *	character set information.
 *
 **************************************/
	if (bpb_buffer_length < 17)
		return error(status, 3, (ISC_STATUS) isc_random,
					 isc_arg_string, "BPB buffer too small");

	UCHAR* p = bpb_buffer;
	*p++ = isc_bpb_version1;
	*p++ = isc_bpb_target_type;
	*p++ = 2;
	*p++ = (UCHAR)to_desc->blob_desc_subtype;
	*p++ = (UCHAR)(to_desc->blob_desc_subtype >> 8);
	*p++ = isc_bpb_source_type;
	*p++ = 2;
	*p++ = (UCHAR)from_desc->blob_desc_subtype;
	*p++ = (UCHAR)(from_desc->blob_desc_subtype >> 8);
	*p++ = isc_bpb_target_interp;
	*p++ = 2;
	*p++ = (UCHAR)to_desc->blob_desc_charset;
	*p++ = (UCHAR)(to_desc->blob_desc_charset >> 8);
	*p++ = isc_bpb_source_interp;
	*p++ = 2;
	*p++ = (UCHAR)from_desc->blob_desc_charset;
	*p++ = (UCHAR)(from_desc->blob_desc_charset >> 8);

	*bpb_length = p - bpb_buffer;

	return error(status, 1, FB_SUCCESS);
}


ISC_STATUS API_ROUTINE isc_blob_lookup_desc(ISC_STATUS* status,
										isc_db_handle* db_handle,
										isc_tr_handle* trans_handle,
										const char* relation_name,
										const char* field_name,
										ISC_BLOB_DESC* desc, char* global)
{
   struct {
          char  isc_6 [32];	/* RDB$FIELD_NAME */
          short isc_7;	/* isc_utility */
          short isc_8;	/* RDB$SEGMENT_LENGTH */
          short isc_9;	/* RDB$CHARACTER_SET_ID */
          short isc_10;	/* RDB$FIELD_SUB_TYPE */
   } isc_5;
   struct {
          char  isc_3 [32];	/* RDB$FIELD_NAME */
          char  isc_4 [32];	/* RDB$RELATION_NAME */
   } isc_2;
/***********************************************
 *
 *	i s c _ b l o b _ l o o k u p _ d e s c
 *
 ***********************************************
 *
 * Functional description
 *
 *	This routine will lookup the subtype,
 *	character set and segment size information
 *	from the metadata, given a relation name
 *	and column name.  it will fill in the information
 *	in the BLOB_DESC.	
 *
 ***********************************************/
	if (DB && DB != *db_handle)
		/*RELEASE_REQUESTS;*/
		{
		if (DB && isc_0)
		   isc_release_request (isc_status, &isc_0);
		isc_0 = 0;
		}

	DB = *db_handle;
	gds_trans = *trans_handle;

    copy_exact_name (field_name, desc->blob_desc_field_name, 
                     sizeof(desc->blob_desc_field_name));
    copy_exact_name (relation_name, desc->blob_desc_relation_name, 
                     sizeof(desc->blob_desc_relation_name));

	bool flag = false;

	/*FOR X IN RDB$RELATION_FIELDS CROSS Y IN RDB$FIELDS
		WITH X.RDB$FIELD_SOURCE EQ Y.RDB$FIELD_NAME AND
		X.RDB$RELATION_NAME EQ desc->blob_desc_relation_name AND
		X.RDB$FIELD_NAME EQ desc->blob_desc_field_name*/
	{
        if (!isc_0)
           isc_compile_request2 (isc_status, (isc_db_handle*) &DB, (isc_req_handle*) &isc_0, (short) sizeof (isc_1), (char *) isc_1);
	isc_vtov ((char*)desc->blob_desc_field_name, (char*)isc_2.isc_3, 32);
	isc_vtov ((char*)desc->blob_desc_relation_name, (char*)isc_2.isc_4, 32);
	if (isc_0)
           isc_start_and_send (isc_status, (isc_req_handle*) &isc_0, (isc_req_handle*) &gds_trans, (short) 0, (short) 64, &isc_2, (short) 0);
	if (!isc_status [1]) {
	while (1)
	   {
           isc_receive (isc_status, (isc_req_handle*) &isc_0, (short) 1, (short) 40, &isc_5, (short) 0);
	   if (!isc_5.isc_7 || isc_status [1]) break;
		flag = true;

	    desc->blob_desc_subtype = /*Y.RDB$FIELD_SUB_TYPE*/
				      isc_5.isc_10;
        desc->blob_desc_charset = /*Y.RDB$CHARACTER_SET_ID*/
				  isc_5.isc_9;
        desc->blob_desc_segment_size = /*Y.RDB$SEGMENT_LENGTH*/
				       isc_5.isc_8;

        if (global) {
            copy_exact_name (/*Y.RDB$FIELD_NAME*/
			     isc_5.isc_6, global,
                             sizeof(/*Y.RDB$FIELD_NAME*/
				    isc_5.isc_6));
        }
	/*END_FOR*/
	   }
	   }; 
    /*ON_ERROR*/
    if (isc_status [1])
       { 
      return copy_status(gds_status, status);
	/*END_ERROR;*/
	   }
	}

	if (!flag)
		return error(status, 5, isc_fldnotdef,
					 isc_arg_string, desc->blob_desc_field_name,
					 isc_arg_string, desc->blob_desc_relation_name);

	return error(status, 1, (ISC_STATUS) FB_SUCCESS);
}


ISC_STATUS API_ROUTINE isc_blob_set_desc(ISC_STATUS* status,
									 const char* relation_name,
									 const char* field_name,
									 SSHORT subtype,
									 SSHORT charset,
									 SSHORT segment_size,
									 ISC_BLOB_DESC* desc)
{
/**************************************
 *
 *	i s c _ b l o b _ s e t _ d e s c
 *
 **************************************
 *
 * Functional description
 *
 *	This routine will set the subtype
 *	and character set information in the
 *	BLOB_DESC based on the information
 *	specifically passed in by the user.
 *
 **************************************/

    copy_exact_name (field_name, desc->blob_desc_field_name, 
                     sizeof(desc->blob_desc_field_name));
    copy_exact_name (relation_name, desc->blob_desc_relation_name, 
                     sizeof(desc->blob_desc_relation_name));

	desc->blob_desc_subtype = subtype;
	desc->blob_desc_charset = charset;
	desc->blob_desc_segment_size = segment_size;

	return error(status, 1, FB_SUCCESS);
}




static void copy_exact_name ( 
    const char* from,
    char* to,
    SSHORT bsize)
{
/**************************************
 *
 *  c o p y _ e x a c t _ n a m e
 *
 **************************************
 *
 * Functional description 
 *  Copy null terminated name ot stops at bsize - 1.
 *  CVC: This is just another fc like DYN_terminate.
 *
 **************************************/
	const char* const from_end = from + bsize - 1;
	char *to2 = to - 1;
	
	while (*from && from < from_end) 
		{
		if (*from != ' ') 
			to2 = to;
		*to++ = *from++;
		}
		
	*++to2 = 0;
}


static ISC_STATUS copy_status( const ISC_STATUS* from, ISC_STATUS* to)
{
/**************************************
 *
 *	c o p y _ s t a t u s
 *
 **************************************
 *
 * Functional description
 *	Copy a status vector.
 *
 **************************************/
	const ISC_STATUS status = from[1];

	const ISC_STATUS* const end = from + ISC_STATUS_LENGTH;
	while (from < end)
		*to++ = *from++;

	return status;
}


static ISC_STATUS error( ISC_STATUS* status, SSHORT count, ...)
{
/**************************************
 *
 *	e r r o r	
 *
 **************************************
 *
 * Functional description
 *	Stuff a status vector.
 *
 **************************************/
	ISC_STATUS *stat;
	va_list ptr;

	VA_START(ptr, count);
	stat = status;
	*stat++ = isc_arg_gds;

	for (; count; --count)
		*stat++ = (ISC_STATUS) va_arg(ptr, ISC_STATUS);

	*stat = isc_arg_end;

	return status[1];
}


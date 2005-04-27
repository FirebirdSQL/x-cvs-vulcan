/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version WI-V2.0.0.4027 Vulcan 1.0 Development **********************/
/*
 *	PROGRAM:	Interbase layered support library
 *	MODULE:		array.epp
 *	DESCRIPTION:	Dynamic array support
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
 * 2001.09.18 Claudio Valderrama: get_name() was preventing the API calls
 *   isc_array_lookup_bounds, isc_lookup_desc & isc_array_set_desc
 *   from working properly with dialect 3 names. Therefore, incorrect names
 *   could be returned or a lookup for a blob field could fail. In addition,
 *   a possible buffer overrun due to unchecked bounds was closed. The fc
 *   get_name() as been renamed copy_exact_name().
 * 
 * 2002-02-24 Sean Leyne - Code Cleanup of old Win 3.1 port (WINDOWS_ONLY)
 *
 *
 */

#include "firebird.h"
#include <string.h>
#include "../jrd/common.h"
#include <stdarg.h>
#include "../jrd/ibase.h"
#include "../dsql/array_proto.h"
#include "../jrd/gds_proto.h"

/*DATABASE DB = STATIC FILENAME "yachts.lnk";*/
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
   isc_1l = 304;
static const char
   isc_1 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 6,0, 
	    blr_cstring, 32,0, 
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
			   blr_field, 1, 14, 'R','D','B','$','D','I','M','E','N','S','I','O','N','S', 
			   blr_parameter, 1, 2,0, 
			blr_assignment, 
			   blr_field, 1, 16, 'R','D','B','$','F','I','E','L','D','_','L','E','N','G','T','H', 
			   blr_parameter, 1, 3,0, 
			blr_assignment, 
			   blr_field, 1, 15, 'R','D','B','$','F','I','E','L','D','_','S','C','A','L','E', 
			   blr_parameter, 1, 4,0, 
			blr_assignment, 
			   blr_field, 1, 14, 'R','D','B','$','F','I','E','L','D','_','T','Y','P','E', 
			   blr_parameter, 1, 5,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 1,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_1 */

static isc_req_handle
   isc_12 = 0;		/* request handle */

static const short
   isc_13l = 170;
static const char
   isc_13 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 3,0, 
	    blr_long, 0, 
	    blr_long, 0, 
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
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 2,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_13 */


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


const int array_desc_column_major = 1;	// Set for FORTRAN

typedef struct gen_t {
	UCHAR* gen_sdl;
	UCHAR** gen_sdl_ptr;
	UCHAR* gen_end;
	ISC_STATUS* gen_status;
	SSHORT gen_internal;
} *GEN_T;

static void adjust_length(ISC_ARRAY_DESC*);
static void copy_exact_name (const char*, char*, SSHORT);
static ISC_STATUS copy_status(const ISC_STATUS*, ISC_STATUS*);
static ISC_STATUS error(ISC_STATUS*, SSHORT, ...);
static ISC_STATUS gen_sdl(ISC_STATUS*, const ISC_ARRAY_DESC*, SSHORT*, UCHAR**,
					  SSHORT*, bool);
static ISC_STATUS lookup_desc(ISC_STATUS*, isc_handle*, isc_handle*, const SCHAR*,
						const SCHAR*, ISC_ARRAY_DESC*, SCHAR*);
static ISC_STATUS stuff_args(gen_t*, SSHORT, ...);
static ISC_STATUS stuff_literal(gen_t*, SLONG);
static ISC_STATUS stuff_string(gen_t*, UCHAR, const SCHAR*);

// stuff_sdl used in place of STUFF to avoid confusion with BLR STUFF
// macro defined in dsql.h

static inline ISC_STATUS stuff_sdl(gen_t* gen, int byte)
{
	return stuff_args(gen, 1, byte);
}

static inline ISC_STATUS stuff_sdl_word(gen_t* gen, int word)
{
	return stuff_args(gen, 2, word, word >> 8);
}

static inline ISC_STATUS stuff_sdl_long(gen_t* gen, int word)
{
	return stuff_args(gen, 4, word, word >> 8, word >> 16, word >> 24);
}


ISC_STATUS API_ROUTINE isc_array_gen_sdl(ISC_STATUS* status,
									 const ISC_ARRAY_DESC* desc,
									 SSHORT* sdl_buffer_length,
									 UCHAR* sdl_buffer, SSHORT* sdl_length)
{
/**************************************
  *
  *	i s c _ a r r a y _ g e n _ s d l
  *
  **************************************
  *
  * Functional description
  *
  **************************************/
 
 	return gen_sdl(status, desc, sdl_buffer_length, &sdl_buffer, sdl_length, false);
}
 
 
ISC_STATUS API_ROUTINE isc_array_get_slice(ISC_STATUS* status,
									   isc_handle* db_handle,
									   isc_handle* trans_handle,
									   ISC_QUAD* array_id,
									   const ISC_ARRAY_DESC* desc,
									   void* array, 
									   SLONG* slice_length)
{
/**************************************
 *
 *	i s c _ a r r a y _ g e t _ s l i c e
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	UCHAR sdl_buffer[512];

	SSHORT sdl_length = sizeof(sdl_buffer);
	UCHAR* sdl = sdl_buffer;

	if (gen_sdl(status, desc, &sdl_length, &sdl, &sdl_length, true))
		return status[1];

	isc_get_slice(status, db_handle, trans_handle, array_id,
// SD: I do not complain but in ibase.h sdl is a plain char in functions'
//     declaration while it is UCHAR in the functions' implementations. Damned legacy!
				  sdl_length, reinterpret_cast<const char*>(sdl),
				  0, NULL, *slice_length, array, slice_length);

	if (sdl != sdl_buffer)
		gds__free((SLONG *) sdl);

	return status[1];
}


ISC_STATUS API_ROUTINE isc_array_lookup_bounds(ISC_STATUS* status,
										   isc_handle* db_handle,
										   isc_handle* trans_handle,
										   const SCHAR* relation_name,
										   const SCHAR* field_name,
										   ISC_ARRAY_DESC* desc)
{
   struct {
          SLONG isc_17;	/* RDB$UPPER_BOUND */
          SLONG isc_18;	/* RDB$LOWER_BOUND */
          short isc_19;	/* isc_utility */
   } isc_16;
   struct {
          char  isc_15 [32];	/* RDB$FIELD_NAME */
   } isc_14;
/**************************************
 *
 *	i s c _ a r r a y _ l o o k u p _ b o u n d s
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	SCHAR global[32];

	if (lookup_desc(status, db_handle, trans_handle,
					field_name, relation_name, desc, global))
		  return status[1];

	ISC_ARRAY_BOUND* tail = desc->array_desc_bounds;

	/*FOR X IN RDB$FIELD_DIMENSIONS
	WITH X.RDB$FIELD_NAME EQ global
	SORTED BY X.RDB$DIMENSION*/
	{
        if (!isc_12)
           isc_compile_request2 (isc_status, (isc_db_handle*) &DB, (isc_req_handle*) &isc_12, (short) sizeof (isc_13), (char *) isc_13);
	isc_vtov ((char*)global, (char*)isc_14.isc_15, 32);
	if (isc_12)
           isc_start_and_send (isc_status, (isc_req_handle*) &isc_12, (isc_tr_handle*) &gds_trans, (short) 0, (short) 32, &isc_14, (short) 0);
	if (!isc_status [1]) {
	while (1)
	   {
           isc_receive (isc_status, (isc_req_handle*) &isc_12, (short) 1, (short) 10, &isc_16, (short) 0);
	   if (!isc_16.isc_19 || isc_status [1]) break;
        tail->array_bound_lower = (SSHORT)/*X.RDB$LOWER_BOUND*/
					  isc_16.isc_18;
	    tail->array_bound_upper = (SSHORT)/*X.RDB$UPPER_BOUND*/
					      isc_16.isc_17;
        ++tail;
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

	return status[1];
}


ISC_STATUS API_ROUTINE isc_array_lookup_desc(ISC_STATUS* status,
										 isc_handle* db_handle,
										 isc_handle* trans_handle,
										 const SCHAR* relation_name,
										 const SCHAR* field_name,
										 ISC_ARRAY_DESC* desc)
{
/**************************************
 *
 *	i s c _ a r r a y _ l o o k u p _ d e s c
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	return lookup_desc(status, db_handle, trans_handle,
					   field_name, relation_name, desc, NULL);
}


ISC_STATUS API_ROUTINE isc_array_put_slice(ISC_STATUS* status,
									   isc_handle* db_handle,
									   isc_handle* trans_handle,
									   ISC_QUAD* array_id,
									   const ISC_ARRAY_DESC* desc,
									   void* array, 
									   SLONG* slice_length)
{
/**************************************
 *
 *	i s c _ a r r a y _ p u t _ s l i c e
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	UCHAR sdl_buffer[512];

	SSHORT sdl_length = sizeof(sdl_buffer);
	UCHAR* sdl = sdl_buffer;

	if (gen_sdl(status, desc, &sdl_length, &sdl, &sdl_length, true))
		return status[1];

	isc_put_slice(status, db_handle, trans_handle, array_id,
				  sdl_length, reinterpret_cast<const char*>(sdl),
				  0, NULL, *slice_length, array);

	if (sdl != sdl_buffer)
		gds__free((SLONG *) sdl);

	return status[1];
}


ISC_STATUS API_ROUTINE isc_array_set_desc(ISC_STATUS* status,
									  const SCHAR* relation_name,
									  const SCHAR* field_name,
									  const SSHORT* sql_dtype,
									  const SSHORT* sql_length,
									  const SSHORT* dimensions,
									  ISC_ARRAY_DESC* desc)
{
/**************************************
 *
 *	i s c _ a r r a y _ s e t _ d e s c
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
    copy_exact_name (field_name, desc->array_desc_field_name,
                     sizeof(desc->array_desc_field_name));
    copy_exact_name (relation_name, desc->array_desc_relation_name,
                     sizeof(desc->array_desc_relation_name));
 

	desc->array_desc_flags = 0;
	desc->array_desc_dimensions = *dimensions;
	desc->array_desc_length = *sql_length;
	desc->array_desc_scale = 0;

	const SSHORT dtype = *sql_dtype & ~1;

	switch(dtype) {
	case SQL_VARYING:
		desc->array_desc_dtype = blr_varying;
		break;
	case SQL_TEXT:
		desc->array_desc_dtype = blr_text;
		break;
	case SQL_DOUBLE:
		desc->array_desc_dtype = blr_double;
		break;
	case SQL_FLOAT:
		desc->array_desc_dtype = blr_float;
		break;
	case SQL_D_FLOAT:
		desc->array_desc_dtype = blr_d_float;
		break;
	case SQL_TIMESTAMP:
		desc->array_desc_dtype = blr_timestamp;
		break;
	case SQL_TYPE_DATE:
		desc->array_desc_dtype = blr_sql_date;
		break;
	case SQL_TYPE_TIME:
		desc->array_desc_dtype = blr_sql_time;
		break;
	case SQL_LONG:
		desc->array_desc_dtype = blr_long;
		break;
	case SQL_SHORT:
		desc->array_desc_dtype = blr_short;
		break;
	case SQL_INT64:
		desc->array_desc_dtype = blr_int64;
		break;
	case SQL_QUAD:
		desc->array_desc_dtype = blr_quad;
		break;
	default:
		return error(status, 7, (ISC_STATUS) isc_sqlerr,
					 (ISC_STATUS) isc_arg_number, (ISC_STATUS) - 804,
					 (ISC_STATUS) isc_arg_gds, (ISC_STATUS) isc_random,
					 (ISC_STATUS) isc_arg_string,
					 (ISC_STATUS) "data type not understood");
	}

	return error(status, 1, (ISC_STATUS) FB_SUCCESS);
}


static void adjust_length( ISC_ARRAY_DESC* desc)
{
/**************************************
 *
 *	a d j u s t _ l e n g t h
 *
 **************************************
 *
 * Functional description
 *	Make architectural adjustment to fixed datatypes.
 *
 **************************************/
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
	char* to2 = to - 1;
	while (*from && from < from_end) {
		if (*from != ' ') {
			to2 = to;
		}
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
	ISC_STATUS* stat;
	va_list ptr;

	VA_START(ptr, count);
	stat = status;
	*stat++ = isc_arg_gds;

	for (; count; --count)
		*stat++ = (ISC_STATUS) va_arg(ptr, ISC_STATUS);

	*stat = isc_arg_end;

	return status[1];
}


static ISC_STATUS gen_sdl(ISC_STATUS* status,
					  const ISC_ARRAY_DESC* desc,
					  SSHORT* sdl_buffer_length,
					  UCHAR** sdl_buffer,
                      SSHORT* sdl_length,
					  bool internal_flag)
{
/**************************************
 *
 *	g e n _ s d l
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	const SSHORT dimensions = desc->array_desc_dimensions;

	if (dimensions > 16)
		return error(status, 5, (ISC_STATUS) isc_invalid_dimension,
					 (ISC_STATUS) isc_arg_number, (ISC_STATUS) dimensions,
					 (ISC_STATUS) isc_arg_number, (ISC_STATUS) 16);

	gen_t gen_block;
	gen_t* gen = &gen_block;
	gen->gen_sdl = *sdl_buffer;
	gen->gen_sdl_ptr = sdl_buffer;
	gen->gen_end = *sdl_buffer + *sdl_buffer_length;
	gen->gen_status = status;
	gen->gen_internal = internal_flag ? 0 : -1;

	if (stuff_args(gen, 4, isc_sdl_version1, isc_sdl_struct, 1,
				   desc->array_desc_dtype))
	{
		return status[1];
	}

	switch (desc->array_desc_dtype) {
	case blr_short:
	case blr_long:
	case blr_int64:
	case blr_quad:
		if (stuff_sdl(gen, desc->array_desc_scale))
			return status[1];
		break;

	case blr_text:
	case blr_cstring:
	case blr_varying:
		if (stuff_sdl_word(gen, desc->array_desc_length))
			return status[1];
		break;
	default:
		break;
	}

	if (stuff_string(gen, isc_sdl_relation, desc->array_desc_relation_name))
		return status[1];

	if (stuff_string(gen, isc_sdl_field, desc->array_desc_field_name))
		return status[1];

	SSHORT from, to, increment;
	if (desc->array_desc_flags & array_desc_column_major) {
		from = dimensions - 1;
		to = -1;
		increment = -1;
	}
	else {
		from = 0;
		to = dimensions;
		increment = 1;
	}

    SSHORT n;
	for (n = from; n != to; n += increment) {
		const ISC_ARRAY_BOUND* tail = desc->array_desc_bounds + n;
		if (tail->array_bound_lower == 1) {
			if (stuff_args(gen, 2, isc_sdl_do1, n))
				return status[1];
		}
		else {
			if (stuff_args(gen, 2, isc_sdl_do2, n))
				return status[1];
			if (stuff_literal(gen, (SLONG) tail->array_bound_lower))
				return status[1];
		}
		if (stuff_literal(gen, (SLONG) tail->array_bound_upper))
			return status[1];
	}

	if (stuff_args(gen, 5, isc_sdl_element, 1, isc_sdl_scalar, 0, dimensions))
		return status[1];

	for (n = 0; n < dimensions; n++)
		if (stuff_args(gen, 2, isc_sdl_variable, n))
			return status[1];

	if (stuff_sdl(gen, isc_sdl_eoc))
		return status[1];
	*sdl_length = gen->gen_sdl - *gen->gen_sdl_ptr;

	return error(status, 1, (ISC_STATUS) FB_SUCCESS);
}



static ISC_STATUS lookup_desc(ISC_STATUS* status,
						  isc_handle* db_handle,
						  isc_handle* trans_handle,
						  const SCHAR* field_name,
						  const SCHAR* relation_name,
						  ISC_ARRAY_DESC* desc,
						  SCHAR* global)
{
   struct {
          char  isc_6 [32];	/* RDB$FIELD_NAME */
          short isc_7;	/* isc_utility */
          short isc_8;	/* RDB$DIMENSIONS */
          short isc_9;	/* RDB$FIELD_LENGTH */
          short isc_10;	/* RDB$FIELD_SCALE */
          short isc_11;	/* RDB$FIELD_TYPE */
   } isc_5;
   struct {
          char  isc_3 [32];	/* RDB$FIELD_NAME */
          char  isc_4 [32];	/* RDB$RELATION_NAME */
   } isc_2;
/**************************************
 *
 *	l o o k u p _ d e s c
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	if (DB && DB != *db_handle)
		/*RELEASE_REQUESTS;*/
		{
		if (DB && isc_0)
		   isc_release_request (isc_status, &isc_0);
		isc_0 = 0;
		if (DB && isc_12)
		   isc_release_request (isc_status, &isc_12);
		isc_12 = 0;
		}

	DB = *db_handle;
	gds_trans = *trans_handle;

    copy_exact_name (field_name, desc->array_desc_field_name,
                     sizeof(desc->array_desc_field_name));
    copy_exact_name (relation_name, desc->array_desc_relation_name,
                     sizeof(desc->array_desc_relation_name));
 

	desc->array_desc_flags = 0;

	bool flag = false;

	/*FOR X IN RDB$RELATION_FIELDS CROSS Y IN RDB$FIELDS
		WITH X.RDB$FIELD_SOURCE EQ Y.RDB$FIELD_NAME AND
		X.RDB$RELATION_NAME EQ desc->array_desc_relation_name AND
		X.RDB$FIELD_NAME EQ desc->array_desc_field_name*/
	{
        if (!isc_0)
           isc_compile_request2 (isc_status, (isc_db_handle*) &DB, (isc_req_handle*) &isc_0, (short) sizeof (isc_1), (char *) isc_1);
	isc_vtov ((char*)desc->array_desc_field_name, (char*)isc_2.isc_3, 32);
	isc_vtov ((char*)desc->array_desc_relation_name, (char*)isc_2.isc_4, 32);
	if (isc_0)
           isc_start_and_send (isc_status, (isc_req_handle*) &isc_0, (isc_tr_handle*) &gds_trans, (short) 0, (short) 64, &isc_2, (short) 0);
	if (!isc_status [1]) {
	while (1)
	   {
           isc_receive (isc_status, (isc_req_handle*) &isc_0, (short) 1, (short) 42, &isc_5, (short) 0);
	   if (!isc_5.isc_7 || isc_status [1]) break;
		flag = true;
	    desc->array_desc_dtype = (UCHAR)/*Y.RDB$FIELD_TYPE*/
					    isc_5.isc_11;
        desc->array_desc_scale = (SCHAR)/*Y.RDB$FIELD_SCALE*/
					isc_5.isc_10;
        desc->array_desc_length = /*Y.RDB$FIELD_LENGTH*/
				  isc_5.isc_9;
        adjust_length(desc);
        desc->array_desc_dimensions = /*Y.RDB$DIMENSIONS*/
				      isc_5.isc_8;
        if (global) {
            copy_exact_name (/*Y.RDB$FIELD_NAME*/
			     isc_5.isc_6, global, sizeof(/*Y.RDB$FIELD_NAME*/
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
		return error(status, 5, (ISC_STATUS) isc_fldnotdef,
					 (ISC_STATUS) isc_arg_string,
					 (ISC_STATUS) desc->array_desc_field_name,
					 (ISC_STATUS) isc_arg_string,
					 (ISC_STATUS) desc->array_desc_relation_name);

	return error(status, 1, (ISC_STATUS) FB_SUCCESS);
}


static ISC_STATUS stuff_args(gen_t* gen, SSHORT count, ...)
{
/**************************************
 *
 *	s t u f f
 *
 **************************************
 *
 * Functional description
 *	Stuff a SDL byte.
 *
 **************************************/
	UCHAR c;
	va_list ptr;

	if (gen->gen_sdl + count >= gen->gen_end) {
		if (gen->gen_internal < 0) {
			return error(gen->gen_status, 3, (ISC_STATUS) isc_misc_interpreted,
						 (ISC_STATUS) isc_arg_string,
						 (ISC_STATUS) "SDL buffer overflow");
		}

		// The sdl buffer is too small.  Allocate a larger one. 

		const SSHORT new_len = gen->gen_end - *gen->gen_sdl_ptr + 512 + count;
		UCHAR* const new_sdl = (UCHAR*) gds__alloc(new_len);
		if (!new_sdl) {
			return error(gen->gen_status, 5, (ISC_STATUS) isc_misc_interpreted,
						 (ISC_STATUS) isc_arg_string,
						 (ISC_STATUS) "SDL buffer overflow", (ISC_STATUS) isc_arg_gds,
						 (ISC_STATUS) isc_virmemexh);
		}

		const SSHORT current_len = gen->gen_sdl - *gen->gen_sdl_ptr;
		memcpy(new_sdl, *gen->gen_sdl_ptr, current_len);
		if (gen->gen_internal++)
			gds__free(*gen->gen_sdl_ptr);
		gen->gen_sdl = new_sdl + current_len;
		*gen->gen_sdl_ptr = new_sdl;
		gen->gen_end = new_sdl + new_len;
	}

	VA_START(ptr, count);

	for (; count; --count) {
		c = va_arg(ptr, int);
		*(gen->gen_sdl)++ = c;
	}

	return 0;
}


static ISC_STATUS stuff_literal(gen_t* gen, SLONG literal)
{
/**************************************
 *
 *	s t u f f _ l i t e r a l
 *
 **************************************
 *
 * Functional description
 *	Stuff an SDL literal.
 *
 **************************************/
	ISC_STATUS*	status = gen->gen_status;

	if (literal >= -128 && literal <= 127)
		return stuff_args(gen, 2, isc_sdl_tiny_integer, literal);

	if (literal >= -32768 && literal <= 32767)
		return stuff_args(gen, 3, isc_sdl_short_integer, literal, literal >> 8);

	if (stuff_sdl(gen, isc_sdl_long_integer))
		return status[1];
	if (stuff_sdl_long(gen, literal))
		return status[1];

	return FB_SUCCESS;
}


static ISC_STATUS stuff_string(gen_t* gen, UCHAR sdl, const SCHAR* string)
{
/**************************************
 *
 *	s t u f f _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *	Stuff a "thing" then a counted string.
 *
 **************************************/
	ISC_STATUS* status = gen->gen_status;

	if (stuff_sdl(gen, sdl))
		return status[1];
	if (stuff_sdl(gen, strlen(string)))
		return status[1];

	while (*string) {
		if (stuff_sdl(gen, *string++))
			return status[1];
	}

	return FB_SUCCESS;
}


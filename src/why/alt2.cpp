/*
 *      PROGRAM:        Firebird Y-valve
 *      MODULE:         alt2.c
 *      DESCRIPTION:    Alternative entrypoints
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
 * 21-December-2003	James A. Starkey, cloned from jrd/alt.c
 *
 */

#include <string.h>
#include <stdarg.h>
#include "firebird.h"
#include "common.h"
#include "ibase.h"
#include "../jrd/event.h"
#include "../jrd/gds_proto.h"

extern "C" {

SLONG ISC_EXPORT isc_sqlcode(const ISC_STATUS * status_vector)
{
	return gds__sqlcode(status_vector);
}

SLONG API_ROUTINE_VARARG isc_event_block(SCHAR ** event_buffer,
										 SCHAR ** result_buffer,
										 USHORT count, ...)
{
/**************************************
 *
 *      i s c _ e v e n t _ b l o c k
 *
 **************************************
 *
 * Functional description
 *      Create an initialized event parameter block from a
 *      variable number of input arguments.
 *      Return the size of the block.
 *
 *	Return 0 if any error occurs.
 *
 **************************************/
	SCHAR *p, *q;
	SCHAR *end;
	SLONG length;
	USHORT i;
	va_list ptr;

	VA_START(ptr, count);

/* calculate length of event parameter block, 
   setting initial length to include version
   and counts for each argument */

	length = 1;
	i = count;
	while (i--) {
		q = va_arg(ptr, SCHAR *);
		length += strlen(q) + 5;
	}

	p = *event_buffer = new SCHAR [length]; //(SCHAR *) gds__alloc((SLONG) length);
/* FREE: apparently never freed */
	if (!*event_buffer)			/* NOMEM: */
		return 0;
	//if ((*result_buffer = (SCHAR *) gds__alloc((SLONG) length)) == NULL) {	/* NOMEM: */
	if (!(*result_buffer = new SCHAR [length]))
		{
		/* FREE: apparently never freed */
		delete [] event_buffer;	//gds__free(*event_buffer);
		*event_buffer = NULL;
		return 0;
		}

#ifdef DEBUG_GDS_ALLOC
/* I can find no place where these are freed */
/* 1994-October-25 David Schnepper  */
	//gds_alloc_flag_unfreed((void *) *event_buffer);
	//gds_alloc_flag_unfreed((void *) *result_buffer);
#endif /* DEBUG_GDS_ALLOC */

/* initialize the block with event names and counts */

	*p++ = EPB_version1;

	VA_START(ptr, count);

	i = count;
	while (i--) {
		q = va_arg(ptr, SCHAR *);

		/* Strip the blanks from the ends */

		for (end = q + strlen(q); --end >= q && *end == ' ';);
		*p++ = end - q + 1;
		while (q <= end)
			*p++ = *q++;
		*p++ = 0;
		*p++ = 0;
		*p++ = 0;
		*p++ = 0;
	}

	return (int) (p - *event_buffer);
}

/*** handled in utl.cpp
void API_ROUTINE isc_event_counts(ULONG * result_vector,
								  SSHORT length,
								  SCHAR * before, const SCHAR * after)
{
	gds__event_counts(result_vector, length, before, after);
}
***/

void API_ROUTINE gds__event_counts(
								  ULONG* result_vector,
								  SSHORT length,
								  SCHAR* before, const SCHAR* after)
{
	isc_event_counts(result_vector, length, before, after);
}

ISC_STATUS API_ROUTINE isc_print_blr(const SCHAR * blr,
          FPTR_PRINT_CALLBACK callback,
		  //void (*callback) (), 
		  void *callback_argument, SSHORT language)
{
        return gds__print_blr((UCHAR *) blr, callback,
                                                  (SCHAR *) callback_argument, language);
}

void API_ROUTINE gds__decode_date(GDS_QUAD * date, void *time_structure)
{
	isc_decode_date(date, time_structure);
}

void API_ROUTINE gds__encode_date(void *time_structure, GDS_QUAD * date)
{
	isc_encode_date(time_structure, date);
}

ISC_STATUS API_ROUTINE isc_interprete(char * buffer, ISC_STATUS ** status_vector_p)
{
	return gds__interprete(buffer, status_vector_p);
}


SLONG API_ROUTINE isc_vax_integer(const SCHAR * input, SSHORT length)
{
	return gds__vax_integer((UCHAR *) input, length);
}

int API_ROUTINE gds__version(
							isc_db_handle* db_handle,
							FPTR_VERSION_CALLBACK callback,
							void* callback_argument)
{
	return isc_version((isc_handle*) db_handle, callback, callback_argument);
}

SSHORT API_ROUTINE isc_msg_lookup(void* handle,
								   USHORT facility,
								   USHORT number,
								   USHORT length,
								   TEXT* buffer, USHORT* flags)
	{
	return gds__msg_lookup (handle, facility, number, length, buffer, flags);
	}


}		// extern "C"

SLONG API_ROUTINE isc_ftof(const char* string1,
						   USHORT length1, SCHAR * string2, USHORT length2)
{
	return gds__ftof(string1, length1, string2, length2);
}

void API_ROUTINE isc_vtof(const char * string1, char * string2, USHORT length)
{
	gds__vtof(string1, string2, length);
}

void API_ROUTINE isc_vtov(const char * string1, char * string2, SSHORT length)
{
	gds__vtov(string1, string2, length);
}

/***
int API_ROUTINE isc_version(
							FRBRD **db_handle,
							void (*callback) (), void *callback_argument)
{
	return gds__version(db_handle, callback, callback_argument);
}
***/



void API_ROUTINE isc_qtoq(GDS_QUAD * quad1, GDS_QUAD * quad2)
{
	gds__qtoq(quad1, quad2);
}

SLONG API_ROUTINE isc_free(SCHAR * blk)
{
	return gds__free((SLONG *) blk);
}

/***
void API_ROUTINE isc_set_debug(int flag)
{
}
***/

void API_ROUTINE gds__set_debug(int flag)
{
}

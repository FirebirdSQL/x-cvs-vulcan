/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		err.cpp
 *	DESCRIPTION:	Error handlers
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
 * 27 Nov 2001  Ann W. Harrison - preserve string arguments in
 *              ERRD_post_warning
 *
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "MPEXL" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */

#include "fbdev.h"
#include "../jrd/ib_stdio.h"
#include <string.h>
#include "../jrd/common.h"
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#include "../dsql/dsql.h"
#include "../dsql/sqlda.h"
#include "gen/iberror.h"
#include "../jrd/iberr.h"
#include "../dsql/errd_proto.h"
#include "../dsql/utld_proto.h"
#include "OSRIException.h"

// This is the only one place in dsql code, where we need both
// dsql.h and err_proto.h. 
// To avoid warnings, undefine some macro's here
//#undef BUGCHECK
//#undef IBERROR

#include "../jrd/err_proto.h"

// To ensure that suspicious macro's not used in this file,
// undefine them once more.
//#undef BUGCHECK
//#undef IBERROR

#include "../jrd/gds_proto.h"
#include "../jrd/thd_proto.h"


/**
  
 	ERRD_assert_msg
  
    @brief	Generate an assertion failure with a message
 

    @param msg
    @param file
    @param lineno

 **/
void ERRD_assert_msg(const char* msg, const char* file, ULONG lineno)
{

	char buffer[256];

	sprintf(buffer, "Assertion failure: %s File: %s Line: %ld\n",	// NTX: dev build 
			(msg ? msg : ""), (file ? file : ""), lineno);
	ERRD_bugcheck(buffer);
}


/**
  
 	ERRD_bugcheck
  
    @brief	Somebody has screwed up.  Bugcheck.
 

    @param text

 **/
void ERRD_bugcheck(const char* text)
{
	TEXT s[128];

	sprintf(s, "INTERNAL: %s", text);	// TXNN 
	ERRD_error(-1, s);
}


/**
  
 	ERRD_error
  
    @brief	This routine should only be used by fatal
 	error messages, those that cannot use the
 	normal error routines because something
 	is very badly wrong.  ERRD_post() should
  	be used by most error messages, especially
 	so that strings will be handled.
 

    @param code
    @param text

 **/
void ERRD_error( int code, const char* text)
{
	TEXT s[256];
	//TSQL tdsql = GET_THREAD_DATA;

	sprintf(s, "** DSQL error: %s **\n", text);
	TRACE(s);
	//ERRD_post (isc_arg_gds, isc_random, isc_arg_string, s, isc_arg_end);	
	ERRD_post (isc_random, isc_arg_string, s, isc_arg_end);	

	/***
	ISC_STATUS* status_vector = tdsql->tsql_status;
	
    if (status_vector) 
		{
        *status_vector++ = isc_arg_gds;
        *status_vector++ = isc_random;
        *status_vector++ = isc_arg_string;
        *status_vector++ = reinterpret_cast<long>(s);
        *status_vector++ = isc_arg_end;
		}

    ERRD_punt();
    ***/
}


/**
  
 ERRD_post_warning
  
    @brief      Post a warning to the current status vector.
 

    @param status
    @param 

 **/
bool ERRD_post_warning(ISC_STATUS status, ...)
{
	va_list args;

//#pragma FB_COMPILER_MESSAGE("Warning, using STATUS array to hold pointers to STATUSes!")
// meaning; if sizeof(long) != sizeof(void*), this code WILL crash something.


	VA_START(args, status);

	ISC_STATUS* status_vector = ((TSQL) GET_THREAD_DATA)->tsql_status;
	int indx = 0;

	if (status_vector[0] != isc_arg_gds ||
		(status_vector[0] == isc_arg_gds && status_vector[1] == 0 &&
		 status_vector[2] != isc_arg_warning))
		{
		// this is a blank status vector 
		status_vector[0] = isc_arg_gds;
		status_vector[1] = 0;
		status_vector[2] = isc_arg_end;
		indx = 2;
		}
	else
		{
		// find end of a status vector 
		int warning_indx = 0;
		PARSE_STATUS(status_vector, indx, warning_indx);
		if (indx) 
			--indx;
		}

	// stuff the warning 
	
	if (indx + 3 >= ISC_STATUS_LENGTH)
		return false;

	status_vector[indx++] = isc_arg_warning;
	status_vector[indx++] = status;
	int type, len;
	
	while ((type = va_arg(args, int)) && (indx + 3 < ISC_STATUS_LENGTH))
		{
        char* pszTmp = NULL;
		switch (status_vector[indx++] = type)
			{
			case isc_arg_warning:
				status_vector[indx++] = (ISC_STATUS) va_arg(args, ISC_STATUS);
				break;

			case isc_arg_string: 
				pszTmp = va_arg(args, char*);
				if (strlen(pszTmp) >= MAX_ERRSTR_LEN) {
					status_vector[(indx - 1)] = isc_arg_cstring;
					status_vector[indx++] = MAX_ERRSTR_LEN;
				}
				status_vector[indx++] = reinterpret_cast<long>(ERR_cstring(pszTmp));
				break;

			case isc_arg_interpreted: 
				pszTmp = va_arg(args, char*);
				status_vector[indx++] = reinterpret_cast<long>(ERR_cstring(pszTmp));
				break;

			case isc_arg_cstring:
				len = va_arg(args, int);
				status_vector[indx++] =
					(ISC_STATUS) (len >= MAX_ERRSTR_LEN) ? MAX_ERRSTR_LEN : len;
				pszTmp = va_arg(args, char*);
				status_vector[indx++] = reinterpret_cast<long>(ERR_cstring(pszTmp));
				break;

			case isc_arg_number:
				status_vector[indx++] = (ISC_STATUS) va_arg(args, SLONG);
				break;

			case isc_arg_vms:
			case isc_arg_unix:
			case isc_arg_win32:
			default:
				status_vector[indx++] = (ISC_STATUS) va_arg(args, int);
				break;
			}
		}
		
	status_vector[indx] = isc_arg_end;
	return true;
	}


/**
  
 	ERRD_post
  
    @brief	Post an error, copying any potentially
 	transient data before we punt.
 

    @param status
    @param 

 **/
void ERRD_post(ISC_STATUS status, ...)
{
	va_list		args;
	va_start	(args, status);
	throw OSRIException (args, status);
//#pragma FB_COMPILER_MESSAGE("No va_end here, maybe create the exception, va_end and throw it?")
}


/**
  
 	ERRD_punt
  
    @brief	Error stuff has been copied to
 	status vector.  Now punt.
 


 **/

#ifdef OBSOLETE
void ERRD_punt(void)
{
	TSQL tdsql = GET_THREAD_DATA;

	// Save any strings in a permanent location 

	UTLD_save_status_strings(tdsql->tsql_status);

	// Give up whatever we were doing and return to the user. 

	firebird::status_exception::raise(tdsql->tsql_status[1]);
}
#endif

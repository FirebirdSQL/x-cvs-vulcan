/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		isc_exception.cpp
 *	DESCRIPTION:	General purpose but non-user routines.
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
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "XENIX" port
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "DELTA" port
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "IMP" port
 *
 * 2002-02-23 Sean Leyne - Code Cleanup, removed old M88K and NCR3000 port
 *
 * 2002.10.27 Sean Leyne - Completed removal of obsolete "DG_X86" port
 * 2002.10.27 Sean Leyne - Completed removal of obsolete "M88K" port
 *
 * 2002.10.28 Sean Leyne - Completed removal of obsolete "DGUX" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "DecOSF" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "SGI" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 * 2004.11.11 Jim Starkey -- split out of isc_sync.cpp
 *
 */

#include "firebird.h"
#include "../jrd/ib_stdio.h"
#include <stdlib.h>
#include <string.h>

#include "../jrd/jrd_time.h"
#include "../jrd/common.h"
#include "gen/iberror.h"
#include "../jrd/thd_proto.h"
#include "../jrd/isc.h"
#include "../jrd/gds_proto.h"
#include "../jrd/isc_proto.h"
#include "../jrd/os/isc_i_proto.h"
#include "../jrd/isc_s_proto.h"
#include "../jrd/file_params.h"
#include "../jrd/gdsassert.h"
#include "../jrd/jrd.h"
#include "../jrd/sch_proto.h"
#include "../jrd/err_proto.h"

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef SUPERSERVER
#ifdef UNIX
void ISC_exception_post(ULONG sig_num, const char* err_msg)
{
/**************************************
 *
 *	I S C _ e x c e p t i o n _ p o s t ( U N I X )
 *
 **************************************
 *
 * Functional description
 *     When we got a sync exception, fomulate the error code
 *     write it to the log file, and abort.
 *
 **************************************/
	TEXT *log_msg;

	if (!SCH_thread_enter_check())
		THREAD_ENTER;

	if (err_msg)
		log_msg = (TEXT *) gds__alloc(strlen(err_msg) + 256);

	switch (sig_num) {
	case SIGSEGV:
		sprintf(log_msg, "%s Segmentation Fault.\n"
				"\t\tThe code attempted to access memory\n"
				"\t\twithout privilege to do so.\n"
				"\tThis exception will cause the Firebird server\n"
				"\tto terminate abnormally.", err_msg);
		break;
	case SIGBUS:
		sprintf(log_msg, "%s Bus Error.\n"
				"\t\tThe code caused a system bus error.\n"
				"\tThis exception will cause the Firebird server\n"
				"\tto terminate abnormally.", err_msg);
		break;
	case SIGILL:

		sprintf(log_msg, "%s Illegal Instruction.\n"
				"\t\tThe code attempted to perfrom an\n"
				"\t\tillegal operation."
				"\tThis exception will cause the Firebird server\n"
				"\tto terminate abnormally.", err_msg);
		break;

	case SIGFPE:
		sprintf(log_msg, "%s Floating Point Error.\n"
				"\t\tThe code caused an arithmetic exception\n"
				"\t\tor floating point exception."
				"\tThis exception will cause the Firebird server\n"
				"\tto terminate abnormally.", err_msg);
		break;
	default:
		sprintf(log_msg, "%s Unknown Exception.\n"
				"\t\tException number %ld."
				"\tThis exception will cause the Firebird server\n"
				"\tto terminate abnormally.", err_msg, sig_num);
		break;
	}

	if (err_msg) {
		gds__log(log_msg);
		gds__free(log_msg);
	}
	abort();
}
#endif /* UNIX */


#ifdef WIN_NT
ULONG ISC_exception_post(ULONG except_code, const char* err_msg)
{
/**************************************
 *
 *	I S C _ e x c e p t i o n _ p o s t ( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *     When we got a sync exception, fomulate the error code
 *     write it to the log file, and abort. Note: We can not
 *     actually call "abort" since in windows this will cause
 *     a dialog to appear stating the obvious!  Since on NT we
 *     would not get a core file, there is actually no difference
 *     between abort() and exit(3).
 *
 **************************************/
	ULONG result;
	bool is_critical = true;
	thread_db* tdbb = GET_THREAD_DATA;

	if (!err_msg)
		err_msg = "";

	TEXT* log_msg = (TEXT*) gds__alloc(strlen(err_msg) + 256);
	log_msg[0] = '\0';

	switch (except_code) 
		{
		case EXCEPTION_ACCESS_VIOLATION:
			sprintf(log_msg, "%s Access violation.\n"
					"\t\tThe code attempted to access a virtual\n"
					"\t\taddress without privilege to do so.\n"
					"\tThis exception will cause the Firebird server\n"
					"\tto terminate abnormally.", err_msg);
			break;
			
		case EXCEPTION_DATATYPE_MISALIGNMENT:
			sprintf(log_msg, "%s Datatype misalignment.\n"
					"\t\tThe attempted to read or write a value\n"
					"\t\tthat was not stored on a memory boundary.\n"
					"\tThis exception will cause the Firebird server\n"
					"\tto terminate abnormally.", err_msg);
			break;
			
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
			sprintf(log_msg, "%s Array bounds exceeded.\n"
					"\t\tThe code attempted to access an array\n"
					"\t\telement that is out of bounds.\n"
					"\tThis exception will cause the Firebird server\n"
					"\tto terminate abnormally.", err_msg);
			break;
			
		case EXCEPTION_FLT_DENORMAL_OPERAND:
			sprintf(log_msg, "%s Float denormal operand.\n"
					"\t\tOne of the floating-point operands is too\n"
					"\t\tsmall to represent as a standard floating-point\n"
					"\t\tvalue.\n"
					"\tThis exception will cause the Firebird server\n"
					"\tto terminate abnormally.", err_msg);
			break;
			
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
			sprintf(log_msg, "%s Floating-point divide by zero.\n"
					"\t\tThe code attempted to divide a floating-point\n"
					"\t\tvalue by a floating-point divisor of zero.\n"
					"\tThis exception will cause the Firebird server\n"
					"\tto terminate abnormally.", err_msg);
			break;
			
		case EXCEPTION_FLT_INEXACT_RESULT:
			sprintf(log_msg, "%s Floating-point inexact result.\n"
					"\t\tThe result of a floating-point operation cannot\n"
					"\t\tbe represented exactly as a decimal fraction.\n"
					"\tThis exception will cause the Firebird server\n"
					"\tto terminate abnormally.", err_msg);
			break;
			
		case EXCEPTION_FLT_INVALID_OPERATION:
			sprintf(log_msg, "%s Floating-point invalid operand.\n"
					"\t\tAn indeterminant error occurred during a\n"
					"\t\tfloating-point operation.\n"
					"\tThis exception will cause the Firebird server\n"
					"\tto terminate abnormally.", err_msg);
			break;
			
		case EXCEPTION_FLT_OVERFLOW:
			sprintf(log_msg, "%s Floating-point overflow.\n"
					"\t\tThe exponent of a floating-point operation\n"
					"\t\tis greater than the magnitude allowed.\n"
					"\tThis exception will cause the Firebird server\n"
					"\tto terminate abnormally.", err_msg);
			break;
			
		case EXCEPTION_FLT_STACK_CHECK:
			sprintf(log_msg, "%s Floating-point stack check.\n"
					"\t\tThe stack overflowed or underflowed as the\n"
					"result of a floating-point operation.\n"
					"\tThis exception will cause the Firebird server\n"
					"\tto terminate abnormally.", err_msg);
			break;
			
		case EXCEPTION_FLT_UNDERFLOW:
			sprintf(log_msg, "%s Floating-point underflow.\n"
					"\t\tThe exponent of a floating-point operation\n"
					"\t\tis less than the magnitude allowed.\n"
					"\tThis exception will cause the Firebird server\n"
					"\tto terminate abnormally.", err_msg);
			break;
			
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
			sprintf(log_msg, "%s Integer divide by zero.\n"
					"\t\tThe code attempted to divide an integer value\n"
					"\t\tby an integer divisor of zero.\n"
					"\tThis exception will cause the Firebird server\n"
					"\tto terminate abnormally.", err_msg);
			break;
			
		case EXCEPTION_INT_OVERFLOW:
			sprintf(log_msg, "%s Interger overflow.\n"
					"\t\tThe result of an integer operation caused the\n"
					"\t\tmost significant bit of the result to carry.\n"
					"\tThis exception will cause the Firebird server\n"
					"\tto terminate abnormally.", err_msg);
			break;
			
		case EXCEPTION_STACK_OVERFLOW:
			ERR_post(isc_exception_stack_overflow, 0);
			/* This will never be called, but to be safe it's here */
			result = (ULONG) EXCEPTION_CONTINUE_EXECUTION;
			is_critical = false;
			break;

		case EXCEPTION_BREAKPOINT:
		case EXCEPTION_SINGLE_STEP:
		case EXCEPTION_NONCONTINUABLE_EXCEPTION:
		case EXCEPTION_INVALID_DISPOSITION:
		case EXCEPTION_PRIV_INSTRUCTION:
		case EXCEPTION_IN_PAGE_ERROR:
		case EXCEPTION_ILLEGAL_INSTRUCTION:
		case EXCEPTION_GUARD_PAGE:
			/* Pass these exception on to someone else,
			probably the OS or the debugger, since there
			isn't a dam thing we can do with them */
			result = EXCEPTION_CONTINUE_SEARCH;
			is_critical = false;
			break;
			
		default:
			/* If we've catched our own software exception,
			continue rewinding the stack to properly handle it
			and deliver an error information to the client side */
			
			if (tdbb->tdbb_status_vector[0] == 1 && tdbb->tdbb_status_vector[1] > 0)
				{
				result = EXCEPTION_CONTINUE_SEARCH;
				is_critical = false;
				}
			else
				sprintf (log_msg, "%s An exception occurred that does\n"
						"\t\tnot have a description.  Exception number %"XLONGFORMAT".\n"
						"\tThis exception will cause the Firebird server\n"
						"\tto terminate abnormally.", err_msg, except_code);
			break; 
		}

	if (is_critical)
		gds__log(log_msg);

	gds__free(log_msg);

	if (is_critical)
		exit(3);
	else
		return result;
}

#endif /* WIN_NT */
#endif /* SUPERSERVER */


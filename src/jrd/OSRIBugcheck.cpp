/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
 *
 *     Software distributed under the License is distributed on 
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *     express or implied.  See the License for the specific 
 *     language governing rights and limitations under the License.
 *
 *     The contents of this file or any work derived from this file
 *     may not be distributed under any other license whatsoever 
 *     without the express prior written permission of the original 
 *     author.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 2004 James A. Starkey
 *  All Rights Reserved.
 */

#include <stdarg.h>
#include "fbdev.h"
#include "OSRIBugcheck.h"
#include "gen/iberror.h"

OSRIBugcheck::OSRIBugcheck(ISC_STATUS code, ...) : OSRIException (osriBugcheck)
{
	va_list		args;
	va_start	(args, code);
	post (NULL, code, args);
	va_end(args);
}


OSRIBugcheck::OSRIBugcheck(const ISC_STATUS* vector) : OSRIException (vector)
{
	setBugcheck();
}

OSRIBugcheck::OSRIBugcheck(const char* text): OSRIException (osriBugcheck)
{
	bugcheck (isc_bug_check, isc_arg_string, text, isc_arg_end);
}

OSRIBugcheck::~OSRIBugcheck(void)
{
}

void OSRIBugcheck::bugcheck(ISC_STATUS code, ...)
{
	va_list		args;
	va_start	(args, code);
	post (NULL, code, args);
	va_end(args);
}

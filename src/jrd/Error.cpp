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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "fbdev.h"
#include "common.h"
#include "Error.h"
#include "OSRIBugcheck.h"
#include "gds_proto.h"

#ifdef _WIN32
#define vsnprintf	_vsnprintf
#endif

#define FB_GDS_ASSERT_FAILURE_STRING	"GDS Assertion failure: %s %"LINEFORMAT"\n"

void Error::assertionFailure(void)
{
#ifdef SUPERSERVER
	throw OSRIBugcheck("assertion failed");
#else
	abort();
#endif
}

void Error::assertionFailure(const char* fileName, int lineNumber)
{
	log(FB_GDS_ASSERT_FAILURE_STRING, fileName, lineNumber);
	char buffer [256];
	snprintf(buffer, sizeof(buffer), FB_GDS_ASSERT_FAILURE_STRING, fileName, lineNumber);
	
#ifdef SUPERSERVER
	throw OSRIBugcheck(buffer);
#else
	abort();
#endif
}

void Error::assertContinue(const char* fileName, int lineNumber)
{
	log(FB_GDS_ASSERT_FAILURE_STRING, fileName, lineNumber);
}

void Error::log(const char* text, ...)
{
	va_list		args;
	va_start	(args, text);
	char		buffer [1024];

	if (vsnprintf (buffer, sizeof (buffer) - 1, text, args) < 0)
		buffer [sizeof (buffer) - 1] = 0;

	va_end(args);
#ifdef SUPERSERVER
	gds__log (buffer);

#else	// !SUPERSERVER

	fprintf (stderr, buffer);

#endif	// SUPERSERVER
}


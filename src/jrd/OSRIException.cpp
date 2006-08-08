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

#include <string.h>
#include <memory.h>
#include "fbdev.h"
#include "common.h"
#include "../include/gen/iberror.h"
#include "OSRIException.h"
#include "Mutex.h"
#include "Sync.h"
#include "../IscDbc/SQLException.h"
#include "../config/AdminException.h"

#ifdef FIREBIRD_ENGINE
#define EXCEPTION_TRACE
#endif

#ifdef EXCEPTION_TRACE
#include "StatusPrint.h"
static int trace;
#endif

#ifndef va_copy
#define va_copy(to,from) to = from
#endif

static char stringSpace [STRING_SPACE];
static char *nextString;
static Mutex mutex;

OSRIException::OSRIException(ISC_STATUS code, ...)
{
	error();
	type = osriException;
	va_list		args;
	va_start	(args, code);
	post (NULL, code, args);
	va_end(args);
	printStatus();
}


OSRIException::OSRIException(OSRIException* exception, ISC_STATUS code, ...)
{
	error();
	type = osriException;
	va_list		args;
	va_start	(args, code);

	post (exception, code, args);
	va_end(args);
	printStatus();
}

OSRIException::OSRIException(va_list args, ISC_STATUS code)
{
	error();
	type = osriException;
	post (NULL, code, args);
	printStatus();
	va_end (args);
}


OSRIException::OSRIException(const OSRIException& source)
{
	error();
	memcpy (statusVector, source.statusVector, sizeof (statusVector));
	type = source.type;
	
	if (strings = source.strings)
        {
		++strings [0];	
        stringsLength = source.stringsLength;
        }
        
	printStatus();
}

OSRIException::OSRIException(ExceptionType exceptionType)
{
	error();
	type = exceptionType;
	strings = NULL;
}

OSRIException::OSRIException(const ISC_STATUS *vector)
{
	error();
	type = osriException;
	strings = NULL;
	stringsLength = getStringsLength (vector);
	
	if (stringsLength)
		{
		strings = new char [stringsLength + 1];
		strings [0] = 1;
		}
	
	char *p = strings + 1;
	ISC_STATUS *status = statusVector;
	
	for (const ISC_STATUS *input = vector, *end = input + 20; input < end;)
		{
		ISC_STATUS argType = *input++;
		*status++ = argType;
		if (argType == isc_arg_end)
			break;
		switch (argType)
			{
			case isc_arg_cstring:
				{
				int l = (int) *input++;
				const char *q = (const char*) *input++;
				*status++ = l;
				*status++ = (ISC_STATUS) p;
				while (l--)
					*p++ = *q++;
				*p++ = 0;
				}
				break;
			
			case isc_arg_string:
				{
				const char *q = (const char*) *input++;
				*status++ = (ISC_STATUS) p;
				while (*p++ = *q++)
					;
				}
				break;
			
			case isc_arg_warning:
			case isc_arg_vms:
			case isc_arg_unix:
			case isc_arg_win32:
			case isc_arg_gds:
			default:
				*status++ = *input++;
			}
		}

	printStatus();
}


OSRIException::OSRIException(void)
{
	//error();
	stringsLength = 0;
	strings = NULL;
}


OSRIException::OSRIException(SQLException* exception)
{
	postException(isc_misc_interpreted, isc_arg_string, exception->getText(), isc_arg_end);
	printStatus();
}


OSRIException::OSRIException(AdminException* exception)
{
	postException(isc_misc_interpreted, isc_arg_string, exception->getText(), isc_arg_end);
	printStatus();
}

OSRIException::~OSRIException(void)
{
	release();
	
}

void OSRIException::post(OSRIException *exception, int code, va_list stuff)
{
	va_list args;
	va_copy (args, stuff);
	strings = NULL;
	stringsLength = (exception) ? getStringsLength (exception->statusVector) : 0;
	
	// Make a first pass counting up string lengths 

	for (;;)
		{
		ISC_STATUS argType = va_arg (args, int);
		if (argType == isc_arg_end)
			break;
		switch (argType)
			{
			case isc_arg_cstring:
				{
				stringsLength += va_arg (args, int) + 1;
				const char *p = va_arg (args, const char*);
				break;
				}
			
			case isc_arg_string:
				stringsLength += (int) strlen (va_arg (args, const char*)) + 1;
				break;
			
			case isc_arg_warning:
			case isc_arg_vms:
			case isc_arg_unix:
			case isc_arg_win32:
			default:
				va_arg (args, ISC_STATUS);
			}
		}
	
	va_end (args);

	// Allocate string space if needed
	
	if (stringsLength)
		{
		strings = new char [stringsLength + 1];
		strings [0] = 1;
		}
	
	char *p = strings + 1;
	ISC_STATUS *status = statusVector;
	ISC_STATUS *statusEnd = statusVector + 19;
	va_copy (args, stuff);
	*status++ = isc_arg_gds;
	*status++ = code;
	
	// Copy primary arguments
	
	for (;;)
		{
		ISC_STATUS argType = va_arg (args, int);
		if (argType == isc_arg_end)
			break;
		*status++ = argType;
		switch (argType)
			{
			case isc_arg_cstring:
				{
				int l = va_arg (args, int);
				const char *q = va_arg (args, const char*);
				*status++ = l;
				*status++ = (ISC_STATUS) p;
				while (l--)
					*p++ = *q++;
				*p++ = 0;
				}
				break;
			
			case isc_arg_string:
				{
				const char *q = va_arg (args, const char*);
				*status++ = (ISC_STATUS) p;
				while (*p++ = *q++)
					;
				}
				break;
			
			case isc_arg_warning:
			case isc_arg_vms:
			case isc_arg_unix:
			case isc_arg_win32:
			case isc_arg_gds:
			default:
				*status++ = va_arg (args, ISC_STATUS);
			}
		}
	
	va_end (args);

	// If necessary, copy secondary argument
	
	if (exception)
		{
		ISC_STATUS *lastStatus = NULL;
		for (const ISC_STATUS *input = exception->statusVector, *end = input + 20; input < end;)
			{
			ISC_STATUS argType = *input++;
			if (argType == isc_arg_end)
				{
				lastStatus = NULL;
				break;
				}
			if (status + 1 >= statusEnd)
				break;
			*status++ = argType;
			switch (argType)
				{
				case isc_arg_cstring:
					{
					if (status + 2 >= statusEnd)
						break;
					int l = (int) *input++;
					const char *q = (const char*) *input++;
					*status++ = l;
					*status++ = (ISC_STATUS) p;
					while (l--)
						*p++ = *q++;
					*p++ = 0;
					}
					break;
				
				case isc_arg_string:
					{
					if (status + 1 >= statusEnd)
						break;
					const char *q = (const char*) *input++;
					*status++ = (ISC_STATUS) p;
					while (*p++ = *q++)
						;
					}
					break;
				
				case isc_arg_warning:
				case isc_arg_vms:
				case isc_arg_unix:
				case isc_arg_win32:
				case isc_arg_gds:
					lastStatus = status;
				default:
					*status++ = *input++;
				}
			}
		if (lastStatus)
			status = lastStatus;
		}

	*status = isc_arg_end;
}

void OSRIException::appendException(ISC_STATUS code, ...)
{
	va_list		args;
	va_start	(args, code);

	int oldStringsLength = stringsLength;

	// Make a first pass counting up string lengths 
	for (;;)
	{
		ISC_STATUS argType = va_arg(args, ISC_STATUS);
		if (argType == isc_arg_end)
			break;
		switch (argType)
		{
			case isc_arg_cstring:
			{
				stringsLength += va_arg(args, int) + 1;
				const char *p = va_arg(args, const char*);
				break;
			}
			
			case isc_arg_string:
				stringsLength += (int) strlen (va_arg (args, const char*)) + 1;
				break;
			
			case isc_arg_warning:
			case isc_arg_vms:
			case isc_arg_unix:
			case isc_arg_win32:
			default:
				va_arg (args, ISC_STATUS);
		}
	}
	va_end (args);

	// Allocate string space if needed
	char* oldStrings = strings;
	if (oldStringsLength != stringsLength)
	{
		strings = new char [stringsLength + 1];
		strings[0] = 1;

		if (oldStrings)
		{
			strncpy(strings + 1, oldStrings + 1, oldStringsLength);
			delete[] oldStrings;
		}
	}
	
	char *p = strings + oldStringsLength + 1;
	ISC_STATUS *status = statusVector;
	ISC_STATUS *statusEnd = statusVector + ISC_STATUS_LENGTH;

	// search end of current status-vector and relocate strings
	while(status < statusEnd)
	{
		if (*status == isc_arg_end)
			break;

		switch (*status)
		{
			case isc_arg_cstring:
				status += 2;
				*status = (ISC_STATUS) (strings + ((char*)(*status) - oldStrings));
			break;
			
			case isc_arg_string:
				status++;
				*status = (ISC_STATUS) (strings + ((char*)(*status) - oldStrings));
			break;

			case isc_arg_warning:
			case isc_arg_vms:
			case isc_arg_unix:
			case isc_arg_win32:
			default:
				status++;
		}
	}

	if (status >= statusEnd - 3)
		return;

	// copy stack arguments
	va_start	(args, code);
	*status++ = isc_arg_gds;
	*status++ = code;

	while (status < statusEnd - 3)
	{
		ISC_STATUS argType = va_arg (args, ISC_STATUS);
		if (argType == isc_arg_end)
			break;

		*status++ = argType;
		switch (argType)
		{
			case isc_arg_cstring:
			{
				int l = va_arg (args, int);
				const char *q = va_arg (args, const char*);
				*status++ = l;
				*status++ = (ISC_STATUS) p;
				while (l--)
					*p++ = *q++;
				*p++ = 0;
			}
			break;
			
			case isc_arg_string:
			{
				const char *q = va_arg (args, const char*);
				*status++ = (ISC_STATUS) p;
				while (*p++ = *q++)
					;
			}
			break;
			
			case isc_arg_warning:
			case isc_arg_vms:
			case isc_arg_unix:
			case isc_arg_win32:
			case isc_arg_gds:
			default:
				*status++ = va_arg (args, ISC_STATUS);
		}
	}
	
	va_end (args);
	*status = isc_arg_end;
}

ISC_STATUS OSRIException::copy(ISC_STATUS* vector, char* stringSpace)
{
	if (type == osriAlreadyStuffed)
		return vector [1];

	ISC_STATUS *out = vector;
	const ISC_STATUS *in = statusVector;
	char *p = stringSpace;
	
	for (const ISC_STATUS *end = in + 20; in < end;)
		{
		ISC_STATUS argType = *out++ = *in++;
		
		if (argType == isc_arg_end)
			break;
		
		switch (argType)
			{
			case isc_arg_cstring:
				{
				int length = *in++;
				const char *q = (const char*) *in++;
				*out++ = length;
				if (p)
					{
					*out++ = (ISC_STATUS) p;
					while (length--)
						*p++ = *q++;
					*p++ = 0;
					}
				else
					*out++ = (ISC_STATUS) q;
				}
				break;
			
			case isc_arg_string:
				{
				const char *q = (const char*) *in++;
				if (p)
					{
					*out++ = (ISC_STATUS) p;
					while (*p++ = *q++)
						;
					*p++ = 0;
					}
				else
					*out++ = (ISC_STATUS) q;
				}
				break;
			
			case isc_arg_warning:
			case isc_arg_vms:
			case isc_arg_unix:
			case isc_arg_win32:
			case isc_arg_gds:
			default:
				*out++ = *in++;
			}
		}
	
	return statusVector [1];
}

ISC_STATUS OSRIException::copy(ISC_STATUS* vector)
{
	if (type == osriAlreadyStuffed)
		return vector [1];

	return copy (vector, getStringSpace());
}

char* OSRIException::getStringSpace(void)
{
	if (stringsLength == 0)
		return NULL;

	Sync sync (&mutex, "OSRIException::getStringSpace");
		
	if (!nextString || nextString + stringsLength >= stringSpace + sizeof (stringSpace))
		nextString = stringSpace;
	
	char *ptr = nextString;
	nextString += stringsLength;
	
	return ptr;	
}

void OSRIException::setType(ExceptionType exceptionType)
{
	type = exceptionType;
}

void OSRIException::error(void)
{
}

int OSRIException::getStringsLength(const ISC_STATUS* vector)
{
	const ISC_STATUS *input;
	const ISC_STATUS *end = vector + 20;
	int stringsLength = 0;
	
	for (input = vector; input < end;)
		{
		ISC_STATUS argType = *input++;
		if (argType == isc_arg_end)
			break;
		switch (argType)
			{
			case isc_arg_cstring:
				stringsLength += *input++ + 1;
				++input;
				break;
			
			case isc_arg_string:
				stringsLength += (int) strlen ((const char*) *input++) + 1;
				break;
			
			case isc_arg_warning:
			case isc_arg_vms:
			case isc_arg_unix:
			case isc_arg_win32:
			default:
				++input;
			}
		}
	
	return stringsLength;
}

OSRIException& OSRIException::operator =(const OSRIException& source)
{
	release();
	memcpy (statusVector, source.statusVector, sizeof (statusVector));
	type = source.type;
	
	if (strings = source.strings)
        {
		++strings [0];	
        stringsLength = source.stringsLength;
        }

	return *this;
}

void OSRIException::release(void)
{
	if (strings)
		if (--strings [0] == 0)
			{
			delete [] strings;
			strings = NULL;
			}
        stringsLength = 0;
}

void OSRIException::postException(ISC_STATUS code, ...)
{
	error();
	type = osriException;
	va_list		args;
	va_start	(args, code);
	post (NULL, code, args);
	va_end(args);
}

void OSRIException::setBugcheck(void)
{
	type = osriBugcheck;
}

bool OSRIException::isBugcheck(void)
{
	return type == osriBugcheck;
}

void OSRIException::printStatus(void)
{
#ifdef EXCEPTION_TRACE
	if (trace)
		{
		StatusPrint statusPrint;
		statusPrint.printStatus(statusVector);
		}
#endif
}

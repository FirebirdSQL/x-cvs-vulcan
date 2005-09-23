/*
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
 *  Copyright (c) 2005 James A. Starkey
 *  All Rights Reserved.
 */

//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "firebird.h"
#include "common.h"
#include "MsgFormat.h"
#include "gds_proto.h"

#ifndef va_copy
#define va_copy(to,from) to = from
#endif

MsgFormat::MsgFormat(void)
{
}

MsgFormat::~MsgFormat(void)
{
}

int MsgFormat::format(int facility, int number, int bufferLength, TEXT* buffer, ...)
{
	va_list		args;
	va_start	(args, buffer);
	int ret = format(facility, number, args, bufferLength, buffer);
	va_end(args);
	
	return ret;
}

int MsgFormat::format(const TEXT* message, int bufferLength, TEXT* buffer, ...)
{
	va_list		args;
	va_start	(args, buffer);
	int ret = format(message, args, bufferLength, buffer);
	va_end(args);
	
	return ret;
}

int MsgFormat::format(const TEXT* message, va_list stuff, int bufferLength, TEXT* buffer)
{
	va_list args;
	va_copy (args, stuff);
	int ret = vsnprintf(buffer, bufferLength, message, args);
	va_end(args);
	
	return ret;
}

int MsgFormat::format(int facility, int number, va_list stuff, int bufferLength, TEXT* buffer)
{
	va_list args;
	va_copy (args, stuff);
	TEXT message[1024];
	
	int len = gds__msg_lookup(NULL, facility, number, sizeof(message), message, NULL);

	if (len < 0)
		switch (len)
			{
			case -1:
				len = snprintf(buffer, bufferLength, "can't format message %d:%d -- message text not found", 
								facility, number);
				return len;
			
			case -2:
				len = snprintf(buffer, bufferLength, "can't format message %d:%d -- message file not found", 
								facility, number);
				return len;
			
			default:
				len = snprintf(buffer, bufferLength, "can't format message %d:%d -- unknown error %d", 
								facility, number, len);
				return len;
			}
			
			
	if (len < sizeof(message))
		return format(message, args, bufferLength, buffer);
	
	TEXT *msg = new TEXT [len + 1];
	len = gds__msg_lookup(NULL, facility, number, len, msg, NULL);
	len = format(msg, args, bufferLength, buffer);
	delete [] msg;
	va_end(args);

	return len;
}

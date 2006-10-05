/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		ExecutionPathInfo.cpp
 *	DESCRIPTION:	Execution path information
 *
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Arno Brinkman
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2005 Arno Brinkman <firebird@abvisie.nl>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#include <memory.h>
#include <string.h>
#include "fbdev.h"
#include "ExecutionPathInfoGen.h"
#include "ibase.h"
#include "jrd.h"
#include "Relation.h"
#include "Request.h"
#include "RecordSource.h"

ExecutionPathInfoGen::ExecutionPathInfoGen(thread_db* tdbb, UCHAR* outputBuffer, int bufferLength)
{
	threadData = tdbb;
	buffer = outputBuffer;
	ptr = buffer;
	bufferEnd = buffer + bufferLength;
}

ExecutionPathInfoGen::~ExecutionPathInfoGen()
{
	//
}

bool ExecutionPathInfoGen::put(const UCHAR* value, int length)
{
	if (!putByte((UCHAR) length))
		return false;

	if (ptr + length > bufferEnd)
		{
		return false;
		}

	memcpy (ptr, value, length);
	ptr += length;

	return true;
}

bool ExecutionPathInfoGen::putBegin()
{
	return putByte(isc_info_rsb_begin);
}

bool ExecutionPathInfoGen::putByte(UCHAR value)
{
	if (ptr + 1 > bufferEnd)
		{
		return false;
		}

	*ptr++ = value;

	return true;
}

bool ExecutionPathInfoGen::putDouble(double value)
{
	return putInt64( *((SINT64*) &value));
}

bool ExecutionPathInfoGen::putEnd()
{
	return putByte(isc_info_rsb_end);
}

bool ExecutionPathInfoGen::putInt(int value)
{
	if (!putShort (value))
		return false;

	return putShort (value >> 16);
}

bool ExecutionPathInfoGen::putInt64(SINT64 value)
{
	if (ptr + 8 > bufferEnd)
		{
		return false;
		}

    *ptr++ = (value);
    *ptr++ = (value >> 8);
    *ptr++ = (value >> 16);
    *ptr++ = (value >> 24);
    *ptr++ = (value >> 32);
    *ptr++ = (value >> 40);
    *ptr++ = (value >> 48);
    *ptr++ = (value >> 56);

	return true;
}

bool ExecutionPathInfoGen::putItemValueDouble(UCHAR item, double value)
{
	if (!putByte(item))
		return false;

	if (!putShort(8))
		return false;

	return putDouble(value);
}

bool ExecutionPathInfoGen::putItemValueInt(UCHAR item, int value)
{
	if (!putByte(item))
		return false;

	if (!putShort(4))
		return false;

	return putInt(value);
}

bool ExecutionPathInfoGen::putRelation(Relation* relation, const str* alias)
{
	if (relation)
		{
		int length = 0;
		const char* name = NULL;
		if (alias)
			{
			length = alias->str_length;
			name = (char*) alias->str_data;
			}
		else
			{
			length = strlen(relation->rel_name);
			name = relation->rel_name;
			}

		if (!putByte(isc_info_rsb_relation))
			return false;

		if (!putString(name, length))
			return false;
		}

	return true;
}

bool ExecutionPathInfoGen::putRequest(Request* request)
{
	if (!request)
		return false;

	for (int i = 0; i < request->req_fors.getCount(); i++) 
		{
		RecordSource* rsb = request->req_fors[i];
		if (rsb && !rsb->getExecutionPathInfo(request, this))
			return false;
		}
	return true;
}

bool ExecutionPathInfoGen::putShort(int value)
{
	if (ptr + 2 > bufferEnd)
		{
		return false;
		}

	*ptr++ = value;
	*ptr++ = value >> 8;

	return true;
}

bool ExecutionPathInfoGen::putString(const char* value, int length)
{
	return put((const UCHAR*) value, length);
}

bool ExecutionPathInfoGen::putType(UCHAR item)
{
	if (!putByte(isc_info_rsb_type))
		return false;

	return putByte(item);
}


int ExecutionPathInfoGen::length()
{
	return ptr - buffer;
}

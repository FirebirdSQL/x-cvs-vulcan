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
 *  Copyright (c) 2004 Arno Brinkman <firebird@abvisie.nl>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#include <memory.h>
#include <string.h>
#include "ExecutionPathInfoGen.h"
#include "firebird.h"
#include "ibase.h"

ExecutionPathInfoGen::ExecutionPathInfoGen(UCHAR* outputBuffer, int bufferLength)
{
	buffer = outputBuffer;
	ptr = buffer;
	bufferEnd = buffer + bufferLength;
}

ExecutionPathInfoGen::~ExecutionPathInfoGen()
{
	//
}

bool ExecutionPathInfoGen::put(UCHAR item)
{
	if (ptr + 1 > bufferEnd)
		{
		return false;
		}

	*ptr++ = item;

	return true;
}

bool ExecutionPathInfoGen::put(const UCHAR* value, int length)
{
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
	return put(isc_info_rsb_begin);
}

bool ExecutionPathInfoGen::putEnd()
{
	return put(isc_info_rsb_end);
}

bool ExecutionPathInfoGen::putType(UCHAR item)
{
	if (!put(isc_info_rsb_type))
		return false;

	return put(item);
}


int ExecutionPathInfoGen::length()
{
	return ptr - buffer;
}

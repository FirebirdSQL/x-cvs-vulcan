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
 *  Copyright (c) 1997 - 2000, 2001, 2003 James A. Starkey
 *  All Rights Reserved.
 */

// Generate.cpp: implementation of the Generate class.
//
//////////////////////////////////////////////////////////////////////

#include <memory.h>
#include <string.h>
#include "fbdev.h"
#include "Generate.h"

#define ALLOCATION_DELTA	1024


Generate::Generate(void)
{
	bufferSize = ALLOCATION_DELTA;
	ptr = buffer = new UCHAR [ALLOCATION_DELTA];
	bufferYellow = buffer + bufferSize - 1;
}

Generate::~Generate(void)
{
	delete [] buffer;
}

void Generate::appendUCHAR(UCHAR value)
{
	while (ptr >= bufferYellow)
		expandBuffer();

	*ptr++ = value;
}

void Generate::appendShort(int value)
{
	appendUCHAR (value);
	appendUCHAR (value >> 8);
}

void Generate::appendInt(int value)
{
	appendShort (value);
	appendShort (value >> 16);
}

void Generate::expandBuffer()
{
	int offset = ptr - buffer;
	UCHAR *oldBuffer = buffer;
	int newSize = bufferSize + ALLOCATION_DELTA;
	buffer = new UCHAR [newSize];
	memcpy (buffer, oldBuffer, offset);
	ptr = buffer + offset;
	bufferSize = newSize;
	bufferYellow = buffer + bufferSize;
	delete [] oldBuffer;
}

int Generate::getLength()
{
	return ptr - buffer;
}

void Generate::appendCharacters(int length, const TEXT* string)
{
	while (ptr + length >= bufferYellow)
		expandBuffer();

	memcpy (ptr, string, length);
	ptr += length;
}

void Generate::appendData(int length, const UCHAR *data)
{
	while (ptr + length >= bufferYellow)
		expandBuffer();

	memcpy (ptr, data, length);
	ptr += length;
}

void Generate::reset(void)
{
	ptr = buffer;
}

void Generate::excise(UCHAR* loc, int length)
{
	int len = ptr - (loc + length);
	
	if (len > 0)
		memmove(loc, loc + length, len);
		
	ptr -= length;
}

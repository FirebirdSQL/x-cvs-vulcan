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
 *  Copyright (c) 1997 - 2000, 2001, 2003 James A. Starkey
 *  Copyright (c) 1997 - 2000, 2001, 2003 Netfrastructure, Inc.
 *  All Rights Reserved.
 */

// BlrGen.cpp: implementation of the BlrGen class.
//
//////////////////////////////////////////////////////////////////////

#include <memory.h>
#include <string.h>
#include "firebird.h"
#include "common.h"
#include "MemMgr.h"
#include "BlrGen.h"
#include "BlrPrint.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

BlrGen::BlrGen()
{
}

BlrGen::~BlrGen()
{
}

void BlrGen::appendDynString(UCHAR verb, const TEXT *string)
{
	if (string)
		appendDynString (verb, strlen (string), string);
	else
		appendDynString (verb, 0, "");
}

void BlrGen::appendDynString(UCHAR verb, int length, const TEXT *string)
{
	if (verb)
		{
		appendUCHAR (verb);
		appendShort (length);
		}
	else
		appendUCHAR (length);

	appendCharacters (length, string);
}

void BlrGen::appendBlrString(int length, const TEXT* string)
{
	appendUCHAR (length);
	appendCharacters (length, string);
}

void BlrGen::appendBlrString(const TEXT* string)
{
	appendBlrString (strlen(string), string);
}

void BlrGen::appendNumber(int value)
{
	if (value >= 0 && value < 128)
		{
		appendShort (1);
		appendUCHAR (value);
		}
	else if (value >= -32768 && value < 32767)
		{
		appendShort (2);
		appendShort (value);
		}
	else 
		{
		appendShort (4);
		appendInt (value);
		}

}

void BlrGen::appendNumber(UCHAR verb, int number)
{
	if (verb)
		appendUCHAR (verb);

	appendNumber (number);
}

void BlrGen::appendUCHARs(UCHAR value, int count)
{
	if (ptr + count >= bufferYellow)
		expandBuffer();

	memset (ptr, value, count);
	ptr += count;
}

void BlrGen::blrBeginSubstring(UCHAR verb)
{
	if (verb)
		appendUCHAR (verb);

	appendShort (0);
	substringOffset = ptr - buffer;
}

void BlrGen::blrEndSubstring()
{
	int length = ptr - buffer - substringOffset;
	UCHAR *p = buffer + substringOffset - 2;
	*p++ = (UCHAR) length;
	*p++ = (UCHAR) (length >> 8);
	
}

void BlrGen::print(void)
{
	BlrPrint blrPrint (buffer, NULL, NULL, NULL);
	blrPrint.print();
}

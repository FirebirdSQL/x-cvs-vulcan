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

#include <memory.h>
#include "fbdev.h"
#include "common.h"
#include "TempSpace.h"

#define INCREMENT		512

TempSpace::TempSpace(void)
{
	size = 0;
	length = 0;
	space = NULL;
	increment = INCREMENT;
	initialSpace = NULL;
}

TempSpace::TempSpace(int newLength)
{
	size = 0;
	length = 0;
	space = NULL;
	increment = INCREMENT;
	resize(newLength);
	initialSpace = NULL;
}


TempSpace::TempSpace(int initialSize, void* initial)
{
	size = initialSize;
	length = 0;
	increment = INCREMENT;
	space = initialSpace = (UCHAR*) initial;
}

TempSpace::~TempSpace(void)
{
	if (space && space != initialSpace)
		delete [] space;
}

UCHAR* TempSpace::resize(int newLength, bool copy)
{
	if (newLength <= size)
		return space;
	
	UCHAR *old = space;
	space = new UCHAR [newLength];
	
	if (copy)
		memcpy (space, old, size);

	size = newLength;
	
	if (old != initialSpace)
		delete [] old;
	
	return space;
}

void TempSpace::addByte(UCHAR data)
{
	if (length + 1 >= size)
		resize (MAX(size+increment, length + 1));

	space[length++] = data;
}

void TempSpace::addBytes(int numberBytes, const UCHAR* data)
{
	if (length + numberBytes >= size)
		resize(MAX(size+increment, length + numberBytes));
	
	memcpy(space + length, data, numberBytes);
	length += numberBytes;
}

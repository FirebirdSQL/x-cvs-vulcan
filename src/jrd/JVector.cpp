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

// JVector.cpp: implementation of the JVector class.
//
//////////////////////////////////////////////////////////////////////

#include <memory.h>
#include "firebird.h"
#include "common.h"
#include "JVector.h"
#include "AdminException.h"

static const int INITIAL_SIZE	= 10;
static const int DELTA			= 10;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

JVector::JVector()
{
	init (INITIAL_SIZE);
}

JVector::~JVector()
{
	if (vector)
		delete [] vector;
}

JVector::JVector(int initialSize)
{
	init (initialSize);
}

void JVector::init(int initialSize)
{
	count = 0;
	allocated = initialSize;
	vector = new void* [allocated];
	memset (vector, 0, allocated * sizeof (void*));
}

void JVector::extend(int newSize)
{
	if (newSize <= count)
		throw AdminException ("can't shrink vector below active elements");
		
	void **oldVector = vector;
	vector = new void* [newSize];
	int cnt = MIN (newSize, allocated);
	
	if (cnt)
		memcpy (vector, oldVector, cnt * sizeof (void*));
	
	if (newSize > allocated)
	memset (vector + cnt, 0, (newSize - allocated) * sizeof (void*));
	allocated = newSize;
	delete [] oldVector;
}

void* JVector::findElement(int index)
{
	if (index < 0 || index >= count)
		return NULL;
		
	return vector [index];
}

void* JVector::operator [](int index)
{
	if (index < 0 || index >= count)
		throw AdminException ("vector reference out of range");
		
	return vector [index];
}

void JVector::append(void* object)
{
	if (count >= allocated)
		extend (count + DELTA);
	
	vector [count++] = object;
}

void JVector::setElement(int index, void* object)
{
	if (index >= allocated)
		extend (index + DELTA);
	
	vector [index] = object;
}

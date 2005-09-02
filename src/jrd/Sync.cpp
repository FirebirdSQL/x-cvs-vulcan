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
// Sync.cpp: implementation of the Sync class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "firebird.h"
#include "../jrd/common.h"
#include "Sync.h"
#include "SynchronizationObject.h"
//#include "Synchronize.h"
//#include "Log.h"
#include "LinkedList.h"

#ifndef ASSERT
#define ASSERT(bool)
#endif

#ifndef LOG_DEBUG
#define LOG_DEBUG	printf
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Sync::Sync(SynchronizationObject *obj, const char *fromWhere)
{
	ASSERT (obj);
	state = None;
	syncObject = obj;
	where = fromWhere;
	prior = NULL;
}

Sync::~Sync()
{
	ASSERT (state != Invalid);

	if (syncObject && state != None)
		{
		syncObject->unlock(this, state);
		state = Invalid;
		}
}

void Sync::lock(LockType type)
{
	request = type;
	syncObject->lock (this, type);
	state = type;
}

void Sync::lock(LockType type, const char *fromWhere)
{
	where = fromWhere;
	lock (type);
}

void Sync::unlock()
{
	ASSERT (state != None);
	syncObject->unlock (this, state);
	state = None;
}

void Sync::setObject(SynchronizationObject * obj)
{
	if (syncObject && state != None)
		syncObject->unlock(this, state);

	state = None;
	syncObject = obj;
}

/***
void Sync::print(int level)
{
	LOG_DEBUG ("%*s%s (%x) state %d (%d)\n", level * 2, "", where, this, state, request);

	if (syncObject)
		syncObject->print(level + 1);

}
***/

void Sync::findLocks(LinkedList &threads, LinkedList &syncObjects)
{
	if (syncObject)
		syncObject->findLocks (threads, syncObjects);
}

void Sync::print(const char *label)
{
	LOG_DEBUG ("%s %s state %d (%d) syncObject %lx\n", 
			   label, where, state, request, syncObject);
}

void Sync::downGrade(LockType type)
{
	ASSERT (state == Exclusive);
	syncObject->downGrade(type);
	state = Shared;
}

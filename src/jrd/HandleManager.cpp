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

#include <memory.h>
#include "firebird.h"
#include "common.h"
#include "HandleManager.h"
#include "Sync.h"
#include "gdsassert.h"

static const int INITIAL_ALLOCATION = 100;

HandleManager::HandleManager(void)
{
	handlesAllocated = INITIAL_ALLOCATION;
	nextAvailable = 1;
	objects = new void* [handlesAllocated];
	
	for (int n = 1; n < handlesAllocated; ++n)
		objects [n] = (void*) (long) (n + 1);

}

HandleManager::~HandleManager(void)
{
	delete [] objects;
}

int HandleManager::allocateHandle(void* object)
{
	Sync sync (&syncObject, "HandleManager::allocateHandle");
	sync.lock (Exclusive);
	
	if (nextAvailable >= handlesAllocated)
		extend (handlesAllocated + INITIAL_ALLOCATION);
		
	int handle = nextAvailable;
	nextAvailable = (int)(long) objects [handle];
	objects [handle] = object;
	
	return handle;
}


void* HandleManager::getObject(void* handle)
{
	return getObject ((int)(long) handle);
}

void* HandleManager::getObject(int handle)
{
	Sync sync (&syncObject, "HandleManager::allocateHandle");
	sync.lock (Shared);
	
	if (handle <= 0 || handle >= handlesAllocated)
		return NULL;
	
	void *obj = objects [handle];
	
	if ((long) obj > 0 && (long) obj < handlesAllocated)
		return NULL;
		
	return obj;
}

void HandleManager::releaseHandle(int handle)
{
	Sync sync (&syncObject, "HandleManager::allocateHandle");
	sync.lock (Exclusive);

	fb_assert(handle != 0);  // make sure we don't try to release a nulled handle

	objects [handle] = (void*) (long) nextAvailable;
	nextAvailable = handle;
}
void HandleManager::setObject(void* object, int handle)
{
	Sync sync (&syncObject, "HandleManager::setObject");
	sync.lock (Exclusive);
	
	if (handle >= handlesAllocated)
		extend (handle + INITIAL_ALLOCATION);
	
	objects [handle] = object;
}

void HandleManager::extend(int number)
{
	void **oldObjects = objects;
	objects = new void* [number];
	memcpy (objects, oldObjects, handlesAllocated * sizeof (void*));
	delete [] oldObjects;
	
	for (int n = handlesAllocated; n < number; ++n)
		objects [n] = (void*) (long) (n + 1);
		
	handlesAllocated = number;
}

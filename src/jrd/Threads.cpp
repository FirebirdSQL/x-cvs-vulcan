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

// Threads.cpp: implementation of the Threads class.
//
//////////////////////////////////////////////////////////////////////

#include "firebird.h"
#include "../jrd/common.h"
#include "Threads.h"
#include "Thread.h"
#include "Sync.h"
#include "Interlock.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Threads::Threads()
{
	threads = NULL;
	useCount = 1;
}

Threads::~Threads()
{
	for (Thread *thread; thread = threads;)
		thread->clearPool();
}

Thread* Threads::start(const char *desc, void (fn)(void*), void * arg)
{
	Sync sync (&syncObject, "Threads::start");
	sync.lock (Exclusive);
	Thread *thread;

	for (thread = threads; thread; thread = thread->next)
		if (!thread->active)
			{
			thread->start (desc, fn, arg);
			return thread;
			}

	addRef();
	thread = new Thread (desc, this, fn, arg);
	thread->next = threads;
	threads = thread;
	
	return thread;		
}

void Threads::exitting(Thread * thread)
{
	Sync sync (&syncObject, "Threads::exiting");
	sync.lock (Exclusive);

	for (Thread **ptr = &threads; *ptr; ptr = &(*ptr)->next)
		if (*ptr == thread)
			{
			*ptr = thread->next;
			sync.unlock();
			release();
			break;
			}

	wake();
}

void Threads::shutdownAll()
{
	Thread *thisThread = Thread::getThread("Threads::shutdownAll");
	Sync sync (&syncObject, "Threads::shutdownAll");
	sync.lock (Exclusive);

	for (Thread *thread = threads; thread; thread = thread->next)
		thread->shutdown();

}

void Threads::clear()
{
	for (Thread *thread = threads; thread; thread = thread->next)
		thread->marked = false;
}

void Threads::waitForAll()
{
	Thread *thisThread = Thread::getThread("Threads::waitForAll");
	Sync sync (&syncObject, "Threads::waitForAll");

	for (;;)
		{
		sync.lock (Exclusive);
		bool done = true;
		for (Thread *thread = threads; thread; thread = thread->next)
			if (thread->threadId != thisThread->threadId)
				done = false;
		if (done)
			break;
		sync.unlock();
		sleep();
		}
}

void Threads::addRef()
{
	INTERLOCKED_INCREMENT (useCount);
}

void Threads::release()
{
	if (INTERLOCKED_DECREMENT (useCount) == 0)
		delete this;
}

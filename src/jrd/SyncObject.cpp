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

// SyncObject.cpp: implementation of the SyncObject class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <memory.h>

#ifdef _WIN32
#include <windows.h>
#undef ERROR
#undef ASSERT
#undef TRACE
#endif

#include "firebird.h"
#include "../jrd/common.h"
#include "../jrd/err_proto.h"

#include "SyncObject.h"
#include "Thread.h"
#include "Threads.h"
#include "Sync.h"
#include "Interlock.h"
#include "LinkedList.h"
#include "OSRIBugcheck.h"
#include "gen/iberror.h"


#ifndef ASSERT
#define ASSERT(assert)	if (!(assert)) assertionFailed();
#endif

/***
#ifdef _DEBUG
static char THIS_FILE[]=__FILE__;
#endif
***/

static int cas_emulation (volatile int *state, int compare, int exchange);

#ifdef EMULATE
#undef COMPARE_EXCHANGE
#define COMPARE_EXCHANGE(target,compare,exchange) \
	(cas_emulation(target,compare,exchange) == compare)
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

int cas_emulation (volatile int *state, int compare, int exchange)
{
	int result = *state;

	if (result == compare)
		*state = exchange;
	
	return result;
}

SyncObject::SyncObject()
{
	waiters = 0;
	lockState = 0;
	que = NULL;
	monitorCount = 0;
	exclusiveThread = NULL;
}

SyncObject::~SyncObject()
{
}

void SyncObject::lock(Sync *sync, LockType type)
{
	Thread *thread;
	
	if (type == Shared)
		{
		while (true)
			{
			int oldState = lockState;
			if (oldState < 0)
				break;
			int newState = oldState + 1;
			if (COMPARE_EXCHANGE(&lockState,oldState,newState))
				return;
			}
			
		mutex.lock();
		bumpWaiters(1);
		
		while (true)
			{
			int oldState = lockState;
			if (oldState < 0)
				break;
			int newState = oldState + 1;
			if (COMPARE_EXCHANGE(&lockState,oldState,newState))
				{
				bumpWaiters(-1);
				mutex.release();
				return;
				}
			}
			
		thread = Thread::getThread("SyncObject::lock");
		
		if (thread == exclusiveThread)
			{
			++monitorCount;
			bumpWaiters(-1);
			return;
			}
		}
	else
		{
		thread = Thread::getThread("SyncObject::lock");
		ASSERT(thread);
		
		if (thread == exclusiveThread)
			{
			++monitorCount;
			return;
			}
			
		while (waiters == 0)
			{
			int oldState = lockState;
			if (oldState != 0)
				break;
			if (COMPARE_EXCHANGE(&lockState,oldState,-1))
				{
				exclusiveThread = thread;
				return; 
				}
			}
			
		mutex.lock();
		bumpWaiters(1);
		
		while (que == NULL)
			{
			int oldState = lockState;
			if (oldState != 0)
				break;
			if (COMPARE_EXCHANGE(&lockState,oldState,-1))
				{
				exclusiveThread = thread;
				bumpWaiters(-1);
				mutex.release();
				return;
				}
			}
		}

	wait (type, thread, sync);
}

bool SyncObject::lockConditional(LockType type)
{
	if (type == Shared)
		{
		while (true)
			{
			int oldState = lockState;
			if (oldState < 0)
				break;
			int newState = oldState + 1;
			if (COMPARE_EXCHANGE(&lockState,oldState,newState))
				return true;
			}
		
		return false;	
		}
	else
		{
		Thread *thread = Thread::getThread("SyncObject::lock");
		ASSERT(thread);
		
		if (thread == exclusiveThread)
			{
			++monitorCount;
			return true;
			}
			
		while (waiters == 0)
			{
			int oldState = lockState;
			if (oldState != 0)
				break;
			if (COMPARE_EXCHANGE(&lockState,oldState,-1))
				{
				exclusiveThread = thread;
				return true; 
				}
			}
		
		return false;
		}
}

void SyncObject::unlock(Sync *sync, LockType type)
{
	if (monitorCount)
		{
		ASSERT (monitorCount > 0);
		--monitorCount;
		return;
		}
		
	ASSERT ((type == Shared && lockState > 0) || (type == Exclusive && lockState == -1));
	
	for (;;)
		{
		int oldState = lockState;
		int newState = (type == Shared) ? oldState - 1 : 0;
		exclusiveThread = NULL;
		
		if (COMPARE_EXCHANGE(&lockState, oldState, newState))
			{
			if (waiters)
				grantLocks();
			return;
			}
		}
}

void SyncObject::downGrade(LockType type)
{
	ASSERT (monitorCount == 0);
	ASSERT (type == Shared);
	ASSERT (lockState == -1);
	
	for (;;)
		if (COMPARE_EXCHANGE(&lockState, -1, 1))
			{
			if (waiters)
				grantLocks();
			return;
			}
}

void SyncObject::wait(LockType type, Thread *thread, Sync *sync)
{
	Thread *volatile *ptr;
	
	for (ptr = &que; *ptr; ptr = &(*ptr)->que)
		if (*ptr == thread)
			{
			LOG_DEBUG ("Apparent single thread deadlock for thread %d (%x)\n", thread->threadId, thread);
			for (Thread *thread = que; thread; thread = thread->que)
				thread->print();
			mutex.release();
			throw OSRIBugcheck ("single thread deadlock");
			//throw SQLEXCEPTION (BUG_CHECK, "Single thread deadlock");
			}

	thread->que = NULL;
	thread->lockType = type;
	*ptr = thread;
	thread->lockGranted = false;
	thread->lockPending = sync;
	mutex.release();

	while (!thread->lockGranted)
		{
		bool wakeup = thread->sleep (10000);
		if (thread->lockGranted)
			break;
		if (!wakeup)
			{
			stalled (thread);
			break;
			}
		}

	while (!thread->lockGranted)
		thread->sleep();
}

bool SyncObject::isLocked()
{
	return lockState != 0;
}


void SyncObject::stalled(Thread *thread)
{
#ifdef HOMEBREW_SYNC
	mutex.lock();
	LinkedList threads;
	LinkedList syncObjects;
	thread->findLocks (threads, syncObjects);

	LOG_DEBUG ("Stalled threads\n");

	FOR_OBJECTS (Thread*, thrd, &threads)
		thrd->print();
	END_FOR;

	LOG_DEBUG ("Stalled synchronization objects:\n");

	FOR_OBJECTS (SyncObject*, syncObject, &syncObjects)
		syncObject->print();
	END_FOR;

	/***
	if (thread->pool)
		thread->pool->clear();
	else
		LOG_DEBUG ("SyncObject::wait: no thread pool\n");

	if (exclusiveThread)
		exclusiveThread->marked = false;

	LOG_DEBUG ("Thread stalled\n");
	print (1);
	thread->print (1);
	***/
	LOG_DEBUG ("------------------------------------\n");
	mutex.release();
#endif
}

void SyncObject::findLocks(LinkedList &threads, LinkedList &syncObjects)
{
#ifdef HOMEBREW_SYNC
	if (syncObjects.appendUnique (this))
		{
		for (Thread *thread = que; thread; thread = thread->que)
			thread->findLocks (threads, syncObjects);
		if (exclusiveThread)
			exclusiveThread->findLocks (threads, syncObjects);
		}
#endif
}

void SyncObject::print()
{
#ifdef HOMEBREW_SYNC
	LOG_DEBUG ("  SyncObject %x: r %d (%d), w %d (%d), m %d, wt %d\n", 
				this, 
				readers, readsGranted,
				writers, writesGranted,
				monitorCount, waiters);

	if (exclusiveThread)
		exclusiveThread->print ("    Exclusive thread");

	for (Thread *volatile thread = que; thread; thread = thread->que)
		thread->print ("    Waiting thread");
#endif
}


void SyncObject::sysServiceFailed(const char* service, int code)
{
	throw OSRIException (isc_sys_request, 
						 isc_arg_string, service, 
						 SYS_ARG, code, 
						 isc_arg_end);
}

void SyncObject::bumpWaiters(int delta)
{
	for (;;)
		{
		int oldValue = waiters;
		int newValue = waiters + delta;
		if (COMPARE_EXCHANGE(&waiters, oldValue, newValue))
			return;
		}
}

void SyncObject::grantLocks(void)
{
	mutex.lock();
	ASSERT ((waiters && que) || (!waiters && !que));
	
	for (Thread *thread, *volatile*ptr = &que; thread = *ptr;)
		{
		bool granted = false;
				
		if (thread->lockType == Shared)
			for (int oldState; (oldState = lockState) >= 0;)
				{
				int newState = oldState + 1;
				if (COMPARE_EXCHANGE(&lockState, oldState, newState))
					{
					//que = thread->que;
					*ptr = thread->que;
					granted = true;
					bumpWaiters(-1);
					thread->grantLock (this);
					break;
					}
				}
		else
			while (lockState == 0)
				if (COMPARE_EXCHANGE(&lockState, 0, -1))
					{
					//que = thread->que;
					*ptr = thread->que;
					granted = true;
					exclusiveThread = thread;
					bumpWaiters(-1);
					thread->grantLock (this);
					break;
					}
		
		if (!granted)
			ptr = &thread->que;
		}
			
	mutex.release();
}

void SyncObject::assertionFailed(void)
{
	throw OSRIException (isc_sys_request, 
						 isc_arg_string, "SyncObject assertion failed", 
						 SYS_ARG, lockState, 
						 isc_arg_end);
}

LockType SyncObject::getState(void)
{
	if (lockState == 0)
		return None;
	
	if (lockState < 0)
		return Exclusive;
	
	return Shared;
}

void SyncObject::validate(LockType lockType)
{
	switch (lockType)
		{
		case None:
			ASSERT (lockState == 0);
			break;
		
		case Shared:
			ASSERT (lockState > 0);
			break;
		
		case Exclusive:
			ASSERT (lockState == -1);
			break;
		}
}

void SyncObject::unlock(void)
{
	if (lockState > 0)
		unlock (NULL, Shared);
	else if (lockState == -1)
		unlock (NULL, Exclusive);
	else
		ASSERT(false);
}

bool SyncObject::ourExclusiveLock(void)
{
	if (lockState != -1)
		return false;
	
	return exclusiveThread == Thread::getThread("SyncObject::ourExclusiveLock");
}

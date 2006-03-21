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

// Thread.cpp: implementation of the Thread class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>

#ifdef _WIN32
//#include <afx.h>
#include <windows.h>
#undef TRACE
#undef ERROR
#undef ASSERT
#endif

#include "fbdev.h"
#include "../jrd/common.h"
#include "Thread.h"
#include "Threads.h"
#include "SyncObject.h"
#include "Sync.h"
//#include "Log.h"
//#include "SyncWait.h"
#include "LinkedList.h"

#ifdef _WIN32
static int threadIndex = TlsAlloc();
#endif

#ifdef _PTHREADS
static pthread_key_t	threadIndex;
static int initThreads();
static int foo = initThreads();
#endif

#ifdef SOLARIS_MT
static thread_key_t	threadIndex;
static int initThreads();
static int foo = initThreads();
#endif

#ifndef ASSERT
#define ASSERT(bool)
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#ifdef _PTHREADS
static int initThreads()
{
	int keyCreateRet = pthread_key_create (&threadIndex, NULL);
	return 1;
}
#endif

#ifdef SOLARIS_MT
static int initThreads()
{
	int keyCreateRet = thr_keycreate (&threadIndex, NULL);
	return 1;
}
#endif


Thread::Thread(const char *desc)
{
	init(desc);
	pool = NULL;
}


Thread::Thread(const char *desc, Threads *threads, void (*fn)(void*), void *arg)
{
	init(desc);
	pool = threads;
	createThread (fn, arg);
}

void Thread::init(const char *desc)
{
	//printf ("Thread::init %s %x\n", desc, this);
	description = desc;
	useCount = 1;
	//active = false;
	activeLocks = 0;
	locks = NULL;
	lockPending = NULL;
	syncWait = NULL;
	lockType = None;
	//defaultTimeZone = NULL;
	//javaThread = NULL;
	pool = NULL;
}

Thread::~Thread()
{
	//printf ("Thread::~Thread %s %x\n", description, this);
	clearPool();
	setThread (NULL);
}

THREAD_RET Thread::thread(void * parameter)
{
	Thread *thread = (Thread*) parameter;
	thread->thread();
	thread->release();

	return 0;
}

#ifdef MVS
extern "C"
{
   typedef void *(*start_routine)(void*);
}
#endif

void Thread::thread()
{
	setThread (this);

	try
		{
		while (!shutdownInProgress)
			{
			locks = NULL;
			lockPending = NULL;
			if (function)
				{
				//active = true;
				(*function)(argument);
				if (!shutdownInProgress)
					{
					ASSERT (locks == NULL);
					ASSERT (javaThread == NULL);
					}
				if (activeLocks)
					activeLocks = 0;
				function = NULL;
				//active = false;
				}
			if (shutdownInProgress)
				break;
			if (!function)
				sleep();
			}
		}
	catch (...)
		{
		release();
		throw;
		}

	release();
}

void Thread::start(const char *desc, void (*fn)(void*), void * arg)
{
	description = desc;
	function = fn;
	argument = arg;
	wake();
}

Thread* Thread::getThread(const char *desc)
{
	Thread *thread = findThread();

	if (!thread)
		{
		thread = new Thread (desc);
		thread->threadId = getCurrentThreadId();
		setThread (thread);
		}

	return thread;
}


void Thread::deleteThreadObject()
{
	Thread *thread = findThread();

	if (thread)
		{
		thread->clearPool();
		thread->release();
		}
}

void Thread::setThread(Thread * thread)
{
#ifdef _WIN32
	TlsSetValue (threadIndex, thread);
#endif

#ifdef _PTHREADS
	int ret = pthread_setspecific (threadIndex, thread);
#endif

#ifdef SOLARIS_THREADS
	int ret = thr_setspecific (threadIndex, thread);
#endif
}

void Thread::grantLock(SyncObject *lock)
{
	ASSERT (!lockGranted);
	ASSERT (!lockPending || lockPending->syncObject == lock);
	lockGranted = true;
	lockPending = NULL;

	wake();
}

void Thread::clearPool()
{
	if (pool)
		pool->exitting (this);

	pool = NULL;
}

void Thread::validateLocks()
{
	Thread *thread = getThread("Thread::validateLocks");

	if (thread->locks)
		{
		LOG_DEBUG ("thread %d has active locks:\n", thread->threadId);
		for (Sync *sync = thread->locks; sync; sync = sync->prior)
			LOG_DEBUG ("   %s\n", sync->where);
		}

}

void Thread::createThread(void (*fn)(void *), void *arg)
{
	function = fn;
	argument = arg;
	//active = true;
	addRef();

#ifdef _WIN32
	threadHandle = CreateThread (NULL, 0, thread, this, 0, &threadId);
#endif

#ifdef _PTHREADS
#ifdef MVS
	int ret = pthread_create (&threadId, NULL, (start_routine)(void* (*)(void*))thread, this);
#else
	int ret = pthread_create (&threadId, NULL, thread, this);
#endif
#endif

#ifdef SOLARIS_MT
	/***
	int thr_create(void *stack_base, size_t stack_size, 
				   void *(*start_func) (void*), void *arg, 
				   long flags, thread_t *new_thread_ID); 
	***/

	int ret = thr_create (NULL, NULL, thread, this, 0, &threadId);
#endif
}

void Thread::addRef()
{
	++useCount;
}

void Thread::release()
{
	if (--useCount == 0)
		delete this;
}

/***
void Thread::setLock(Sync *sync)
{
	ASSERT (sync != locks);
	ASSERT (!locks || (locks->state == Shared || locks->state == Exclusive));
	ASSERT (sync->request == Shared || sync->request == Exclusive);
	sync->prior = locks;
	locks = sync;
}

void Thread::clearLock(Sync *sync)
{
	ASSERT (locks == sync);
	ASSERT (sync->state == Shared || sync->state == Exclusive);

	if (locks = sync->prior)
		{
		ASSERT (locks->state == Shared || locks->state == Exclusive);
		}
}
***/

void Thread::findLocks(LinkedList &threads, LinkedList &syncObjects)
{
	if (threads.appendUnique (this))
		{
		for (Sync *sync = locks; sync; sync = sync->prior)
			sync->findLocks (threads, syncObjects);
		if (lockPending)
			lockPending->findLocks (threads, syncObjects);
		}
}

void Thread::print()
{
	/***
	LOG_DEBUG ("  Thread %x (%d) sleeping=%d, granted=%d, locks=%d, who %d\n",
				this, threadId, sleeping, lockGranted, activeLocks, lockGranted);

	for (Sync *sync = locks; sync; sync = sync->prior)
		sync->print ("    Holding");

	if (lockPending)
		lockPending->print ("    Pending");

	if (syncWait)
		syncWait->print ("    Waiting");
	***/
}

void Thread::print(const char *label)
{
	LOG_DEBUG ("%s %x (%d), type %d; %s\n", label, this, threadId, lockType, getWhere()); 
}

const char* Thread::getWhere()
{
	if (lockPending && lockPending->where)
		return lockPending->where;

	return "";
}

/***
void Thread::setTimeZone(const TimeZone *timeZone)
{
	defaultTimeZone = timeZone;
}
***/

Thread* Thread::findThread()
{
#ifdef _WIN32
	return (Thread*) TlsGetValue (threadIndex);
#endif

#ifdef _PTHREADS
#ifdef MVS
	Thread *thread = NULL;
	int ret = pthread_getspecific (threadIndex, (void**) &thread);
	return thread;
#else
	return (Thread*) pthread_getspecific (threadIndex);
#endif
#endif

#ifdef SOLARIS_MT
	Thread *thread = NULL;
	int ret = thr_getspecific (threadIndex, (void**) &thread);
	
	return thread;
#endif
}

THREAD_ID Thread::getCurrentThreadId(void)
{
#ifdef _WIN32
	return GetCurrentThreadId();
#endif

#ifdef _PTHREADS
	return pthread_self();
#endif
}

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

// Thread.h: interface for the Thread class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_THREAD_H__84FD1988_A97F_11D2_AB5C_0000C01D2301__INCLUDED_)
#define AFX_THREAD_H__84FD1988_A97F_11D2_AB5C_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifdef _WIN32
#define THREAD_ID		unsigned long
#define THREAD_RET		unsigned long
#else
#define _stdcall
#endif

#ifdef _PTHREADS
#include <pthread.h>
#define THREAD_ID		pthread_t
#define THREAD_RET		void*
#endif

#ifdef SOLARIS_MT
#include <thread.h>
#include <signal.h>
#define THREAD_ID		thread_t
#define THREAD_RET		void*
#endif

#include "Synchronize.h"

CLASS(Sync);
CLASS(SyncObject);

class Threads;
class SyncWait;
class LinkedList;

struct TimeZone;

class Thread : public Synchronize
{
public:
	static Thread* findThread();
	const char* getWhere();
	void print (const char *label);
	void print();
	void findLocks (LinkedList &threads, LinkedList& syncObjects);
	void release();
	void addRef();
	void createThread (void (*fn)(void*), void *arg);
	static void validateLocks();
	void clearPool();
	void grantLock(SyncObject *lock);
	static void deleteThreadObject();
	void init(const char *description);
	 Thread(const char *desc);
	static Thread* getThread(const char *desc);
	void start (const char *desc, void (*fn)(void*), void*arg);
	void thread();
	static THREAD_RET _stdcall thread (void* parameter);
	Thread(const char *desc, Threads *threads, void (*fn)(void*), void *arg);

	void			*argument;
	void			(*function)(void*);
	Threads			*pool;
	void*			threadHandle;

	THREAD_ID		threadId;
	Thread			*next;				// next thread in pool
	Thread			*que;				// next thread in wait que (see SyncObject)
	LockType		lockType;			// requested lock type (see SyncObject)
	bool			active;
	volatile bool	lockGranted;
	volatile long	activeLocks;
	Sync			*locks;
	Sync			*lockPending;
	SyncWait		*syncWait;
	bool			marked;
	int				useCount;
	const char		*description;

protected:
	static void setThread (Thread *thread);
	virtual ~Thread();
public:
	static THREAD_ID getCurrentThreadId(void);
};

#endif // !defined(AFX_THREAD_H__84FD1988_A97F_11D2_AB5C_0000C01D2301__INCLUDED_)

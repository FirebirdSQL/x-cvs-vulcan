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

// SyncObject.h: interface for the SyncObject class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SYNCOBJECT_H__59333A53_BC53_11D2_AB5E_0000C01D2301__INCLUDED_)
#define AFX_SYNCOBJECT_H__59333A53_BC53_11D2_AB5E_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "SynchronizationObject.h"

#ifdef _WIN32
#define HOMEBREW_SYNC2
#endif

#ifdef POSIX_THREADS
#define HOMEBREW_SYNC2
#include <pthread.h>
#endif

#ifdef SOLARIS_MT
#include <sys/mutex.h>
#include <thread.h>
#endif

#ifdef HOMEBREW_SYNC2
#include "Mutex.h"
#endif

CLASS(Sync);

class Thread;

START_NAMESPACE

class LinkedList;

class SyncObject : public SynchronizationObject
{
public:
	void print();
	virtual void findLocks (LinkedList &threads, LinkedList& syncObjects);
	void stalled(Thread *thread);
	void printEvents(int level);
	void postEvent (Thread *thread, const char *what, Thread *granting);
	//void print(int level);
	void downGrade (LockType type);
	bool isLocked();
	virtual void unlock (Sync *sync, LockType type);
	virtual void lock (Sync *sync, LockType type);
	SyncObject();
	virtual ~SyncObject();

protected:
	void wait(LockType type, Thread *thread, Sync *sync);

	INTERLOCK_TYPE		lockState;
	volatile INTERLOCK_TYPE	waiters;
	long				monitorCount;
	Thread				*volatile exclusiveThread;
	Mutex				mutex;
	Thread				*volatile que;

public:
	void sysServiceFailed(const char* server, int code);
	void bumpWaiters(int delta);
	void grantLocks(void);
	void assertionFailed(void);
	LockType getState(void);
	void validate(LockType lockType);
	void unlock(void);
	bool ourExclusiveLock(void);
	bool lockConditional(LockType type);
};

END_NAMESPACE

#endif // !defined(AFX_SYNCOBJECT_H__59333A53_BC53_11D2_AB5E_0000C01D2301__INCLUDED_)

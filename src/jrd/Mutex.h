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
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 */

// Mutex.h: interface for the Mutex class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MUTEX_H__F3F1D3A7_4083_11D4_98E8_0000C01D2301__INCLUDED_)
#define AFX_MUTEX_H__F3F1D3A7_4083_11D4_98E8_0000C01D2301__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SynchronizationObject.h"

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _PTHREADS
#include <pthread.h>
#endif

#ifdef SOLARIS_MT
#include <sys/mutex.h>
#include <thread.h>
#endif

START_NAMESPACE

class Mutex : public SynchronizationObject
{
public:
	void release();
	void lock();
	Mutex();
	~Mutex();
	Sync	*holder;
	
#ifdef _WIN32
	//void*	mutex;
	CRITICAL_SECTION criticalSection;
#endif

#ifdef _PTHREADS
	pthread_mutex_t	mutex;
#endif

#ifdef SOLARIS_MT
	cond_t			condition;
	mutex_t			mutex;
#endif

	virtual void unlock(Sync* sync, LockType type);
	virtual void lock(Sync* sync, LockType type);
	virtual void findLocks(LinkedList& threads, LinkedList& syncObjects);
	void downGrade(LockType type);
};

END_NAMESPACE

#endif // !defined(AFX_MUTEX_H__F3F1D3A7_4083_11D4_98E8_0000C01D2301__INCLUDED_)

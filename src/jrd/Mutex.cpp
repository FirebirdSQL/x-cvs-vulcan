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

// Mutex.cpp: implementation of the Mutex class.
//
//////////////////////////////////////////////////////////////////////

#include "firebird.h"
#include "common.h"
#include "Mutex.h"

#ifdef _WIN32
#include <windows.h>
#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Mutex::Mutex()
{
#ifdef _WIN32
	//mutex = CreateMutex (NULL, false, NULL);
	InitializeCriticalSection (&criticalSection);
#endif

#ifdef _PTHREADS
	int ret = pthread_mutex_init (&mutex, NULL);
#endif

#ifdef SOLARIS_MT
	int ret = mutex_init (&mutex, USYNC_THREAD, NULL);
#endif

	holder = NULL;
}

Mutex::~Mutex()
{
#ifdef _WIN32
	//CloseHandle (mutex);
	DeleteCriticalSection (&criticalSection);
#endif

#ifdef _PTHREADS
	int ret = pthread_mutex_destroy (&mutex);
#endif

#ifdef SOLARIS_MT
	int ret = mutex_destroy (&mutex);
#endif
}

void Mutex::lock()
{
#ifdef _WIN32
	//int result = WaitForSingleObject (mutex, INFINITE);
	EnterCriticalSection (&criticalSection);
#endif

#ifdef _PTHREADS
	int ret = pthread_mutex_lock (&mutex);
#endif

#ifdef SOLARIS_MT
	int ret = mutex_lock (&mutex);
#endif
}

void Mutex::release()
{
#ifdef _WIN32
	//ReleaseMutex (mutex);
	LeaveCriticalSection (&criticalSection);
#endif

#ifdef _PTHREADS
	int ret = pthread_mutex_unlock (&mutex);
#endif

#ifdef SOLARIS_MT
	int ret = mutex_unlock (&mutex);
#endif
}

void Mutex::unlock(Sync* sync, LockType type)
{
	holder = NULL;
	release();
}

void Mutex::lock(Sync* sync, LockType type)
{
	lock();
	holder = sync;
}

void Mutex::findLocks(LinkedList& threads, LinkedList& syncObjects)
{
}

void Mutex::downGrade(LockType type)
{
}

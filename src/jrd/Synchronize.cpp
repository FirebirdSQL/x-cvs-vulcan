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

// Synchronize.cpp: implementation of the Synchronize class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>

#ifdef _WIN32
//#include <AFX.h>
//#include <process.h>
//#include <signal.h>
#include <windows.h>
#undef TRACE
#else
#include <sys/time.h>
#endif

#ifdef _PTHREADS
#include <pthread.h>
#include <errno.h>
#endif

#ifdef SOLARIS_MT
#include <thread.h>
#include <synch.h>
#include <errno.h>
#endif

#include "firebird.h"
#include "../jrd/common.h"
#include "Synchronize.h"
//#include "Log.h"

#ifdef ENGINE
#define CHECK_RET(text,code)	if (ret) Error::error (text,code)
#else
#define CHECK_RET(text,code)
#endif

#define NANO		1000000000
#define MICRO		1000000

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Synchronize::Synchronize()
{
	shutdownInProgress = false;
	sleeping = false;
	wakeup = false;

#ifdef _WIN32
	event = CreateEvent (NULL, false, false, NULL);
#endif

#ifdef _PTHREADS
	int ret = pthread_mutex_init (&mutex, NULL);
	pthread_cond_init (&condition, NULL);
#endif

#ifdef SOLARIS_MT
	int ret = mutex_init (&mutex, USYNC_THREAD, NULL);
	cond_init (&condition, USYNC_THREAD, NULL);
#endif
}

Synchronize::~Synchronize()
{
#ifdef _WIN32
	CloseHandle (event);
#endif

#ifdef _PTHREADS
	int ret = pthread_mutex_destroy (&mutex);
	ret = pthread_cond_destroy (&condition);
#endif

#ifdef SOLARIS_MT
	int ret = mutex_destroy (&mutex);
#endif
}


void Synchronize::sleep()
{
	sleeping = true;
#ifdef _WIN32
#ifdef _DEBUG
	for (;;)
		{
		int n = WaitForSingleObject (event, 10000);
		sleeping = false;
		if (n != WAIT_TIMEOUT)
			return;
		}
#else
	sleep (INFINITE);
#endif
#endif

#ifdef _PTHREADS
	int ret = pthread_mutex_lock (&mutex);
	CHECK_RET ("pthread_mutex_lock failed, errno %d", errno);

	while (!wakeup)
		pthread_cond_wait (&condition, &mutex);

	wakeup = false;
	ret = pthread_mutex_unlock (&mutex);
	CHECK_RET ("pthread_mutex_unlock failed, errno %d", errno);
#endif

#ifdef SOLARIS_MT
	int ret = mutex_lock (&mutex);
	CHECK_RET ("mutex_lock failed, errno %d", errno);

	while (!wakeup)
		cond_wait (&condition, &mutex);

	wakeup = false;
	ret = mutex_unlock (&mutex);
	CHECK_RET ("mutex_unlock failed, errno %d", errno);
#endif

	sleeping = false;
}

bool Synchronize::sleep(int milliseconds)
{
	sleeping = true;

#ifdef _WIN32
	int n = WaitForSingleObject (event, milliseconds);
	sleeping = false;

	return n != WAIT_TIMEOUT;
#endif

#ifdef _PTHREADS
	struct timeval microTime;
	int ret = gettimeofday (&microTime, NULL);
	QUAD nanos = (QUAD) microTime.tv_sec * NANO + microTime.tv_usec * 1000 +
				 (QUAD) milliseconds * 1000000;
	struct timespec nanoTime;
	nanoTime.tv_sec = nanos / NANO;
	nanoTime.tv_nsec = nanos % NANO;
	ret = pthread_mutex_lock (&mutex);
	CHECK_RET ("pthread_mutex_lock failed, errno %d", errno);
	int seconds = nanoTime.tv_sec - microTime.tv_sec;

	while (!wakeup)
		{
#ifdef MVS
		ret = pthread_cond_timedwait (&condition, &mutex, &nanoTime);
		if (ret == -1 && errno == EAGAIN)
		   ret = ETIMEDOUT;
			break;
#else
		ret = pthread_cond_timedwait (&condition, &mutex, &nanoTime);
		if (ret == ETIMEDOUT)
			break;
#endif
		/***
		if (!wakeup)
			Log::debug ("Synchronize::sleep(milliseconds): unexpected wakeup, ret %d\n", ret);
		***/
		}

	sleeping = false;
	wakeup = false;
	pthread_mutex_unlock (&mutex);
	return ret != ETIMEDOUT;
#endif

#ifdef SOLARIS_MT
	struct timeval microTime;
	int ret = gettimeofday (&microTime, NULL);
	QUAD nanos = (QUAD) microTime.tv_sec * NANO + microTime.tv_usec * 1000 +
				 (QUAD) milliseconds * 1000000;
	struct timespec nanoTime;
	nanoTime.tv_sec = nanos / NANO;
	nanoTime.tv_nsec = nanos % NANO;
	ret = mutex_lock (&mutex);
	CHECK_RET ("mutex_lock failed, errno %d", errno);
	int seconds = nanoTime.tv_sec - microTime.tv_sec;

	while (!wakeup)
		{
		ret = cond_timedwait (&condition, &mutex, &nanoTime);
		if (ret == ETIMEDOUT)
			break;
		/***
		if (!wakeup)
			Log::debug ("Synchronize::sleep(milliseconds): unexpected wakeup, ret %d\n", ret);
		***/
		}

	sleeping = false;
	wakeup = false;
	mutex_unlock (&mutex);

	return ret != ETIMEDOUT;
#endif
}

void Synchronize::wake()
{
#ifdef _WIN32
	SetEvent (event);
#endif

#ifdef _PTHREADS
	int ret = pthread_mutex_lock (&mutex);
	CHECK_RET ("pthread_mutex_lock failed, errno %d", errno);
	wakeup = true;
	pthread_cond_broadcast (&condition);
	ret = pthread_mutex_unlock (&mutex);
	CHECK_RET ("pthread_mutex_unlock failed, errno %d", errno);
#endif

#ifdef SOLARIS_MT
	int ret = mutex_lock (&mutex);
	CHECK_RET ("mutex_lock failed, errno %d", errno);
	wakeup = true;
	cond_broadcast (&condition);
	ret = mutex_unlock (&mutex);
	CHECK_RET ("mutex_unlock failed, errno %d", errno);
#endif
}

void Synchronize::shutdown()
{
	shutdownInProgress = true;
	wake();
}

/*
 *	PROGRAM:		Client/Server Common Code
 *	MODULE:			rwlock.h
 *	DESCRIPTION:	Read/write multi-state locks
 *
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Nickolay Samofatov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Nickolay Samofatov <nickolay@broadviewsoftware.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 * 
 *
 */

#ifndef RWLOCK_H
#define RWLOCK_H

#ifdef WIN_NT

#include <windows.h>
#include <limits.h>

#define LOCK_WRITER_OFFSET 50000

namespace firebird {

// This class works on Windows 98/NT4 or later. Win95 is not supported
// Should work pretty fast.
class RWLock {
private:
	volatile LONG lock; // This is the actual lock
	           // -50000 - writer is active
			   // 0 - noone owns the lock
			   // positive value - number of concurrent readers
	volatile LONG blockedReaders;
	volatile LONG blockedWriters;
	HANDLE writers_event, readers_semaphore;

	//
	// Those inlines are needed due to the different argument taken by msvc6 and msvc7
	// funcions
	//
#if (defined(_MSC_VER) && (_MSC_VER <= 1200)) || defined(MINGW)
	inline LONG InterlockedIncrement_uni(volatile LONG* lock_p){
		return InterlockedIncrement(const_cast<LONG*>(lock_p));
	}
	inline LONG InterlockedDecrement_uni(volatile LONG* lock_p){
		return InterlockedDecrement(const_cast<LONG*>(lock_p));
	}
	inline LONG InterlockedExchangeAdd_uni(volatile LONG* lock_p, LONG value){
		return InterlockedExchangeAdd(const_cast<LONG*>(lock_p), value);
	}
#else
	inline LONG InterlockedIncrement_uni(volatile long* lock_p){
		return InterlockedIncrement(lock_p);
	}
	inline LONG InterlockedDecrement_uni(volatile long* lock_p){
		return InterlockedDecrement(lock_p);
	}
	inline LONG InterlockedExchangeAdd_uni(volatile LONG* lock_p, LONG value){
		return InterlockedExchangeAdd(lock_p, value);
	}
#endif

public:
	RWLock() : lock(0), blockedReaders(0), blockedWriters(0) { 
		readers_semaphore = CreateSemaphore(NULL, 0 /*initial count*/, 
			INT_MAX, NULL); 
		writers_event = CreateEvent(NULL, FALSE/*auto-reset*/, FALSE, NULL);
	}
	~RWLock() { CloseHandle(readers_semaphore); CloseHandle(writers_event); }
	// Returns negative value if writer is active.
	// Otherwise returns a number of readers
	LONG getState() {
		return lock;
	}
	void unblockWaiting() {
		if (blockedWriters) 
			SetEvent(writers_event);
		else
			if (blockedReaders)
				ReleaseSemaphore(readers_semaphore, blockedReaders, NULL);
	}
	bool tryBeginRead() {
		if (lock < 0) return false;
		if (InterlockedIncrement_uni(&lock) > 0) return true;
		// We stepped on writer's toes. Fix our mistake 
		if (InterlockedDecrement_uni(&lock) == 0)
			unblockWaiting();
		return false;
	}
	bool tryBeginWrite() {
		if (lock) return false;
		if (InterlockedExchangeAdd_uni(&lock, -LOCK_WRITER_OFFSET) == 0) return true;
		// We stepped on somebody's toes. Fix our mistake
		if (InterlockedExchangeAdd_uni(&lock, LOCK_WRITER_OFFSET) == -LOCK_WRITER_OFFSET)
			unblockWaiting();
		return false;
	}
	void beginRead() {
		if (!tryBeginRead()) {
			InterlockedIncrement_uni(&blockedReaders);
			while (!tryBeginRead())
				WaitForSingleObject(readers_semaphore, INFINITE);
			InterlockedDecrement_uni(&blockedReaders); 
		}
	}
	void beginWrite() {
		if (!tryBeginWrite()) {
			InterlockedIncrement_uni(&blockedWriters);
			while (!tryBeginWrite())
				WaitForSingleObject(writers_event, INFINITE);
			InterlockedDecrement_uni(&blockedWriters);
		}
	}
	void endRead() {
		if (InterlockedDecrement_uni(&lock) == 0)
			unblockWaiting();
	}
	void endWrite() {
		if (InterlockedExchangeAdd_uni(&lock, LOCK_WRITER_OFFSET) == -LOCK_WRITER_OFFSET)
			unblockWaiting();
	}
};


}

#else

#ifdef MULTI_THREAD

#ifdef SOLARIS_MT

#include <thread.h>
#include <synch.h>
#include <errno.h>

namespace firebird {

class RWLock {
private:
	rwlock_t lock;
public:
	RWLock() {		
		if (rwlock_init(&lock, USYNC_PROCESS, NULL))
		{
			system_call_failed::raise();
		}
	}
	~RWLock() {
		if (rwlock_destroy(&lock))
			system_call_failed::raise();
	}
	void beginRead() {
		if (rw_rdlock(&lock))	
			system_call_failed::raise();
	}
	bool tryBeginRead() {
		int code = rw_tryrdlock(&lock);
		if (code == EBUSY) return false;
		if (code) system_call_failed::raise();
		return true;
	}
	void endRead() {
		if (rw_unlock(&lock))	
			system_call_failed::raise();
	}
	bool tryBeginWrite() {
		int code = rw_trywrlock(&lock);
		if (code == EBUSY) return false;
		if (code) system_call_failed::raise();
		return true;
	}
	void beginWrite() {
		if (rw_wrlock(&lock))	
			system_call_failed::raise();
	}
	void endWrite() {
		if (rw_unlock(&lock))	
			system_call_failed::raise();
	}
};

}




#else

#include <pthread.h>
#include <errno.h>

namespace firebird {

/* Since this is only used in NBAK, and NBAK is not functional in Vulcan yet */
/* I'm going to remove these locks for the private cache mode, because they */
/* cause failures on MVS when one thread initialized a lock, and another thread */
/* uses it. SEK */

class RWLock {
private:
	pthread_rwlock_t lock;
public:
	RWLock() {		
#ifdef SHARED_CACHE
#ifdef LINUX
		pthread_rwlockattr_t attr;
		if (pthread_rwlockattr_init(&attr) ||
			pthread_rwlockattr_setkind_np(&attr, 
				PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP) ||
			pthread_rwlock_init(&lock, NULL) ||
			pthread_rwlockattr_destroy(&attr) )
		{
			system_call_failed::raise();
		}
#else
		if (pthread_rwlock_init(&lock, NULL))
		{
			system_call_failed::raise();
		}
#endif
#endif
	}
	~RWLock() {
#ifdef SHARED_CACHE
		if (pthread_rwlock_destroy(&lock))
			system_call_failed::raise();
#endif
	}
	void beginRead() {
#ifdef SHARED_CACHE
		if (pthread_rwlock_rdlock(&lock))	
			system_call_failed::raise();
#endif
	}
	bool tryBeginRead() {
#ifdef SHARED_CACHE
		int code = pthread_rwlock_tryrdlock(&lock);
		if (code == EBUSY) return false;
		if (code) system_call_failed::raise();
#endif
		return true;
	}
	void endRead() {
#ifdef SHARED_CACHE
		if (pthread_rwlock_unlock(&lock))	
			system_call_failed::raise();
#endif
	}
	bool tryBeginWrite() {
#ifdef SHARED_CACHE
		int code = pthread_rwlock_trywrlock(&lock);
		if (code == EBUSY) return false;
		if (code) system_call_failed::raise();
#endif
		return true;
	}
	void beginWrite() {
#ifdef SHARED_CACHE
		if (pthread_rwlock_wrlock(&lock))	
			system_call_failed::raise();
#endif
	}
	void endWrite() {
#ifdef SHARED_CACHE
		if (pthread_rwlock_unlock(&lock))	
			system_call_failed::raise();
#endif
	}
};

}
#endif /*solaris*/
#endif /*MULTI_THREAD*/

#endif /*!WIN_NT*/

#endif

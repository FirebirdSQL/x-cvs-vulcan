// SyncObject.cpp: implementation of the SyncObject class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "SyncObject.h"
#include "Thread.h"
#include "ScriptError.h"

#ifdef _WIN32
#define ENTER_CRITICAL_SECTION	EnterCriticalSection (&criticalSection)
#define LEAVE_CRITICAL_SECTION	LeaveCriticalSection (&criticalSection)
static CRITICAL_SECTION criticalSection;
#endif

#ifdef _PTHREADS
#define ENTER_CRITICAL_SECTION	pthread_mutex_lock (&mutex)
#define LEAVE_CRITICAL_SECTION	pthread_mutex_unlock (&mutex)
	pthread_mutex_t	mutex;
#endif

static bool junk = SyncObject::initialize ();

/***
#ifdef _DEBUG
static char THIS_FILE[]=__FILE__;
#endif
***/

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SyncObject::SyncObject()
{
	readers = 0;
	writers = 0;
	que = NULL;
	exclusiveThread = NULL;
}

SyncObject::~SyncObject()
{

}

void SyncObject::lock(LockType type)
{
	ASSERT (type != None);
	Thread *thread = Thread::getThread();
	//++thread->activeLocks;

	if (type == Shared && !writers)
		{
		++readers;
		return;
		}

	if (type == Exclusive)
		{
		ENTER_CRITICAL_SECTION;
		++writers;
		if (readers || writers != 1)
			{
			wait (type);
			exclusiveThread = thread;
			return;
			}
		exclusiveThread = thread;
		LEAVE_CRITICAL_SECTION;
		return;
		}

	ENTER_CRITICAL_SECTION;
	++readers;

	if (writers)
		{
		wait (type);
		return;
		}

	LEAVE_CRITICAL_SECTION;
}

void SyncObject::unlock(LockType type)
{
	Thread *thread = Thread::getThread();
	--thread->activeLocks;
    exclusiveThread = NULL;

	// If the lock is shared, and there are no writers waiting, there
	// is nothing to be done (the "shares" aren't waiting).

	if (type == Shared && !writers)
		{
		--readers;
		return;
		}

	// Downgrading a write lock is more serious -- other guys may be waiting.

	ENTER_CRITICAL_SECTION;
		if (type == Exclusive)
			--writers;
		else
			--readers;
		if (que)
			if (que->lockType == Shared)
				while (que && que->lockType == Shared)
					{
					que->grantLock();
					que = que->que;
					}
			else
				{
				exclusiveThread = que;
				que->grantLock();
				que = que->que;
				}
	LEAVE_CRITICAL_SECTION;
}

bool SyncObject::initialize()
{
#ifdef _WIN32
	InitializeCriticalSection (&criticalSection);
#endif

#ifdef _PTHREADS
	//pthread_mutexaddr_t addr = PTHREAD_MUTEX_FAST_NP;
	int ret = pthread_mutex_init (&mutex, NULL);
#endif

	return true;
}

void SyncObject::wait(LockType type)
{
	Thread **ptr, *thread = Thread::getThread();
	
	for (ptr = &que; *ptr; ptr = &(*ptr)->que)
		if (*ptr == thread)
			{
			LEAVE_CRITICAL_SECTION;
			throw ScriptError ("Single thread deadlock");
			}

	thread->que = NULL;
	thread->lockType = type;
	*ptr = thread;
	thread->lockGranted = false;
	LEAVE_CRITICAL_SECTION;

	while (!thread->lockGranted)
		thread->sleep();
}

bool SyncObject::isLocked()
{
	return readers > 0 || writers > 0;
}

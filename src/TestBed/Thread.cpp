// Thread.cpp: implementation of the Thread class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TestBed.h"
#include "Thread.h"
#include "Sync.h"
#include "SyncObject.h"
#include "Action.h"
#include "Action.h"
#include "SQLError.h"

#ifdef _WIN32
static int threadIndex = TlsAlloc();
#endif

#ifdef _PTHREADS
static pthread_key_t	threadIndex;
static int initThreads();
int foo = initThreads();
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Thread::Thread()
{
	/***
	threadHandle = NULL;
	firstAction = lastAction = NULL;
	running = false;
	stalled = false;
	***/
}

Thread::~Thread()
{

}

void Thread::start()
{
	//threadHandle = CreateThread (NULL, 0, thread, this, 0, &threadId);
	start (thread, this);
}


void Thread::start(ULONG (_stdcall *fn)(void*), void * arg)
{
	threadHandle = CreateThread (NULL, 0, fn, arg, 0, &threadId);
}

ULONG Thread::thread(void * parameter)
{
	return ((Thread*) parameter)->thread();
}

ULONG Thread::thread()
{
	/***
	running = true;
	error = false;
	stallSync.wake();
	Sync sync (&syncObject);

	try
		{
		while (!shutdownInProgress)
			{
			if (firstAction)
				{
				sync.lock (Exclusive);
				Action *action = firstAction;
				if (action)
					{
					if (!(firstAction = action->que))
						lastAction = NULL;
					sync.unlock();
					action->eval (stat);
					}
				else
					sync.unlock();
				AfxCheckMemory();
				}
			else
				{
				AfxCheckMemory();
				stalled = true;
				stallSync.wake();
				AfxCheckMemory();
				sleep();
				AfxCheckMemory();
				stalled = false;
				AfxCheckMemory();
				}
			AfxCheckMemory();
			}
		}
	catch (SQLException& exception)
		{
		error = true;
		errorText = exception.getText();
		sqlcode = exception.getSqlcode();
		}
	catch (...)
		{
		running = false;
		error = true;
		errorText = "unknown exception";
		//throw;
		}

	running = false;
	AfxCheckMemory();
	***/

	return 0;
}

Thread* Thread::getThread()
{
#ifdef _WIN32
	Thread *thread = (Thread*) TlsGetValue (threadIndex);
#endif

#ifdef _PTHREADS
	Thread *thread = (Thread*) pthread_getspecific (threadIndex);
#endif

	if (!thread)
		{
		thread = new Thread;
		setThread (thread);
		}

	return thread;
}

void Thread::setThread(Thread * thread)
{
#ifdef _WIN32
	TlsSetValue (threadIndex, thread);
#endif

#ifdef _PTHREADS
	int ret = pthread_setspecific (threadIndex, thread);
#endif
}

void Thread::grantLock()
{
	lockGranted = true;
	wake();
}

/***
void Thread::queAction(Action * action, Stat *statistic)
{
	action->que = NULL;
	stat = statistic;
	Sync sync (&syncObject);
	sync.lock (Exclusive);

	if (lastAction)
		{
		lastAction->next = action;
		lastAction = action;
		}
	else
		firstAction = lastAction = action;

	wake();
}
***/
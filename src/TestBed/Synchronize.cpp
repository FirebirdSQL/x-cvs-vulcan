// Synchronize.cpp: implementation of the Synchronize class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#include <AFX.h>
#else
#include <sys/time.h>
#endif

#ifdef _PTHREADS
#include <pthread.h>
#include <errno.h>
#endif

#include "Engine.h"
#include "Synchronize.h"

#define NANO		1000000000

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Synchronize::Synchronize()
{
	shutdownInProgress = false;

#ifdef _WIN32
	event = CreateEvent (NULL, false, false, NULL);
#endif

#ifdef _PTHREADS
	//pthread_mutexattr_t attr = PTHREAD_MUTEX_FAST_NP;
	int ret = pthread_mutex_init (&mutex, NULL);
	pthread_cond_init (&condition, NULL);
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
}


bool Synchronize::sleep()
{
#ifdef _WIN32
	return sleep (INFINITE);
#endif

#ifdef _PTHREADS
	int ret = pthread_mutex_lock (&mutex);
	ret = pthread_cond_wait (&condition, &mutex);
	ret = pthread_mutex_unlock (&mutex);
#endif
}

bool Synchronize::sleep(int milliseconds)
{
	//printf ("sleeping for %d milliseconds\n", milliseconds);

#ifdef _WIN32
	int n = WaitForSingleObject (event, milliseconds);

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

	for (;;)
		{
		ret = pthread_cond_timedwait (&condition, &mutex, &nanoTime);
		if (ret != EINTR)
			break;
		}

	ret = pthread_mutex_unlock (&mutex);

	if (ret == ETIMEDOUT)
		return false;

	return true;		
#endif
}

void Synchronize::wake()
{
#ifdef _WIN32
	SetEvent (event);
#endif

#ifdef _PTHREADS
	int ret = pthread_mutex_lock (&mutex);
	ret = pthread_cond_signal (&condition);
	ret = pthread_mutex_unlock (&mutex);
#endif
}

void Synchronize::shutdown()
{
	shutdownInProgress = true;
	wake();
}

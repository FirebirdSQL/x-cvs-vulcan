// Mutex.cpp: implementation of the Mutex class.
//
//////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#include <windows.h>
#endif

#include "Mutex.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Mutex::Mutex()
{
#ifdef _WIN32
	mutex = CreateMutex (NULL, false, NULL);
#endif

#ifdef _PTHREADS
	int ret = pthread_mutex_init (&mutex, NULL);
#endif

}

Mutex::~Mutex()
{
#ifdef _WIN32
	CloseHandle (mutex);
#endif

#ifdef _PTHREADS
	int ret = pthread_mutex_destroy (&mutex);
#endif
}

void Mutex::lock()
{
#ifdef _WIN32
	int result = WaitForSingleObject (mutex, INFINITE);
#endif

#ifdef _PTHREADS
	int ret = pthread_mutex_lock (&mutex);
#endif
}

void Mutex::release()
{
#ifdef _WIN32
	ReleaseMutex (mutex);
#endif

#ifdef _PTHREADS
	int ret = pthread_mutex_unlock (&mutex);
#endif
}

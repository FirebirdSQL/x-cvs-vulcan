// Mutex.h: interface for the Mutex class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MUTEX_H__F3F1D3A7_4083_11D4_98E8_0000C01D2301__INCLUDED_)
#define AFX_MUTEX_H__F3F1D3A7_4083_11D4_98E8_0000C01D2301__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef _PTHREADS
#include <pthread.h>
#endif

class Mutex  
{
public:
	void release();
	void lock();
	Mutex();
	virtual ~Mutex();

#ifdef _WIN32
	void*	mutex;
#endif

#ifdef _PTHREADS
	pthread_mutex_t	mutex;
#endif

};

#endif // !defined(AFX_MUTEX_H__F3F1D3A7_4083_11D4_98E8_0000C01D2301__INCLUDED_)

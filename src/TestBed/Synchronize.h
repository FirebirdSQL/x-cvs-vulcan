// Synchronize.h: interface for the Synchronize class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SYNCHRONIZE_H__9E13C6D8_1F3E_11D3_AB74_0000C01D2301__INCLUDED_)
#define AFX_SYNCHRONIZE_H__9E13C6D8_1F3E_11D3_AB74_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifdef _PTHREADS
#include <pthread.h>
#endif

class Synchronize  
{
public:
	virtual void shutdown();
	void wake();
	bool sleep (int milliseconds);
	bool sleep();
	Synchronize();
	virtual ~Synchronize();

	bool	shutdownInProgress;

#ifdef _WIN32
	void	*event;
#endif

#ifdef _PTHREADS
	pthread_cond_t	condition;
	pthread_mutex_t	mutex;
#endif
};

#endif // !defined(AFX_SYNCHRONIZE_H__9E13C6D8_1F3E_11D3_AB74_0000C01D2301__INCLUDED_)

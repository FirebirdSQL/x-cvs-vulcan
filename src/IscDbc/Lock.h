// Lock.h: interface for the Lock class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LOCK_H__F3F1D3A8_4083_11D4_98E8_0000C01D2301__INCLUDED_)
#define AFX_LOCK_H__F3F1D3A8_4083_11D4_98E8_0000C01D2301__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class Mutex;

class Lock  
{
public:
	void release();
	void lock();
	Lock(Mutex *mutex);
	virtual ~Lock();

	Mutex	*mutex;
	bool	locked;
};

#endif // !defined(AFX_LOCK_H__F3F1D3A8_4083_11D4_98E8_0000C01D2301__INCLUDED_)

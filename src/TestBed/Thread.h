// Thread.h: interface for the Thread class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_THREAD_H__E11366AC_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
#define AFX_THREAD_H__E11366AC_0C91_11D4_98DD_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Synchronize.h"
#include "SyncObject.h"
#include "Locktype.h"

class Action;
class Stat;


class Thread : public Synchronize
{
public:
	void start (ULONG (_stdcall *fn)(void*), void*arg);
	//void queAction (Action *action, Stat *stat);
	void grantLock();
	static Thread* getThread();
	virtual ULONG thread();
	static ULONG _stdcall thread (void *parameter);
	void start();
	Thread();
	virtual ~Thread();

	Thread			*que;				// next thread in wait que (see SyncObject)
	//SyncObject		syncObject;
	void			*threadHandle;
	unsigned long	threadId;
	LockType		lockType;
	volatile bool	lockGranted;
	int				activeLocks;
	/***
	Action			*firstAction;
	Action			*lastAction;
	volatile bool	running;
	volatile bool	stalled;
	Synchronize		stallSync;
	Stat			*stat;
	CString			errorText;
	int				sqlcode;
	bool			error;
	***/

protected:
	static void setThread (Thread *thread);
};

#endif // !defined(AFX_THREAD_H__E11366AC_0C91_11D4_98DD_0000C01D2301__INCLUDED_)

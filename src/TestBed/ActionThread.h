// ActionThread.h: interface for the ActionThread class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTIONTHREAD_H__E11366B6_0C91_11D4_98DD_0000C01D2301__INCLUDED_)
#define AFX_ACTIONTHREAD_H__E11366B6_0C91_11D4_98DD_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Action.h"
#include "Thread.h"
#include "SyncObject.h"

class ActionThread : public Action  
{
public:
	ULONG runThread();
	static ULONG _stdcall run (void *arg);
	virtual void queAction (Action *action, Stat *stat);
	virtual void wait();
	virtual void fini();
	virtual Action* eval(Stat *stat);
	virtual Action* compile (Context *context);
	ActionThread();
	virtual ~ActionThread();

	Thread		thread;
	SyncObject	syncObject;
	Action			*firstAction;
	Action			*lastAction;
	volatile bool	running;
	volatile bool	stalled;
	Synchronize		stallSync;
	Stat			*stat;
	CString			errorText;
	int				sqlcode;
	bool			error;
};

#endif // !defined(AFX_ACTIONTHREAD_H__E11366B6_0C91_11D4_98DD_0000C01D2301__INCLUDED_)

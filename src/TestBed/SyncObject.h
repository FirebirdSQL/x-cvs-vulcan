// SyncObject.h: interface for the SyncObject class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SYNCOBJECT_H__59333A53_BC53_11D2_AB5E_0000C01D2301__INCLUDED_)
#define AFX_SYNCOBJECT_H__59333A53_BC53_11D2_AB5E_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "LockType.h"

class Thread;


class SyncObject  
{
public:
	bool isLocked();
	void wait(LockType type);
	void unlock (LockType type);
	void lock (LockType type);
	SyncObject();
	virtual ~SyncObject();
	static bool initialize ();

	volatile int	readers;
	volatile int	writers;
	Thread			*que;
	Thread			*exclusiveThread;
};

#endif // !defined(AFX_SYNCOBJECT_H__59333A53_BC53_11D2_AB5E_0000C01D2301__INCLUDED_)

// Sync.h: interface for the Sync class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SYNC_H__59333A55_BC53_11D2_AB5E_0000C01D2301__INCLUDED_)
#define AFX_SYNC_H__59333A55_BC53_11D2_AB5E_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "LockType.h"

class SyncObject;


class Sync  
{
public:
	void setObject (SyncObject *obj);
	void unlock();
	void lock (LockType type);
	Sync(SyncObject *obj);
	virtual ~Sync();

	SyncObject	*syncObject;
	LockType	state;
};

#endif // !defined(AFX_SYNC_H__59333A55_BC53_11D2_AB5E_0000C01D2301__INCLUDED_)

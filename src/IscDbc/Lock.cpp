// Lock.cpp: implementation of the Lock class.
//
//////////////////////////////////////////////////////////////////////

#include "Lock.h"
#include "Mutex.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Lock::Lock(Mutex *mtx)
{
	locked = false;
	mutex = mtx;
}

Lock::~Lock()
{
	if (locked)
		mutex->release();
}

void Lock::lock()
{
	if (!locked)
		{
		mutex->lock();
		locked = true;
		}
}

void Lock::release()
{
	if (locked)
		{
		mutex->release();
		locked = false;
		}
}

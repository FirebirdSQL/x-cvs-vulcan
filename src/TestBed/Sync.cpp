// Sync.cpp: implementation of the Sync class.
//
//////////////////////////////////////////////////////////////////////

#include "Sync.h"
#include "SyncObject.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Sync::Sync(SyncObject *obj)
{
	state = None;
	syncObject = obj;
}

Sync::~Sync()
{
	if (syncObject && state != None)
		syncObject->unlock(state);
}

void Sync::lock(LockType type)
{
	syncObject->lock (type);
	state = type;
}

void Sync::unlock()
{
	syncObject->unlock (state);
	state = None;
}

void Sync::setObject(SyncObject * obj)
{
	if (syncObject && state != None)
		syncObject->unlock(state);

	state = None;
	syncObject = obj;
}

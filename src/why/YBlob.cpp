#include "firebird.h"
#include "YBlob.h"
#include "SubsysHandle.h"
#include "StatusVector.h"

YBlob::YBlob(SubsysHandle *subsys, BlbHandle orgHandle)
{
	subsystem = subsys;
	handle = orgHandle;
	userPtr = NULL;
	subsystem->addBlob (this);
}

YBlob::~YBlob(void)
{
}

ISC_STATUS YBlob::cancelBlob(StatusVector& statusVector)
{
	if (userPtr)
		{
		*userPtr = NULL;
		userPtr = NULL;
		}

	if (!subsystem)
		return 0;

	ISC_STATUS ret = 0;

	if (handle)	
		ret = subsystem->subsystem->cancelBlob (statusVector, &handle);

	subsystem->removeBlob (this);
	subsystem = NULL;

	return ret;
}

ISC_STATUS YBlob::closeBlob(StatusVector& statusVector)
{
	if (userPtr)
		{
		*userPtr = NULL;
		userPtr = NULL;
		}

	if (!subsystem)
		return 0;

	ISC_STATUS ret = 0;

	if (handle)	
		ret = subsystem->subsystem->closeBlob (statusVector, &handle);

	subsystem->removeBlob (this);
	subsystem = NULL;

	return ret;
}

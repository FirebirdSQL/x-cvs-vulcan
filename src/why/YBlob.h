#ifndef _YBLOB_H_
#define _YBLOB_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Subsystem.h"

class SubsysHandle;
class StatusVector;

class YBlob
{
public:
	YBlob(SubsysHandle *subsys, BlbHandle orgHandle);
	~YBlob(void);

	SubsysHandle	*subsystem;
	BlbHandle		handle;
	BlbHandle		*userPtr;
	YBlob			*next;
	YBlob			*prior;
	ISC_STATUS cancelBlob(StatusVector& statusVector);
	ISC_STATUS closeBlob(StatusVector& statusVector);
};

#endif

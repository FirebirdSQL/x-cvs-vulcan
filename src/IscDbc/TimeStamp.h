// Timestamp.h: interface for the Timestamp class.
//
//////////////////////////////////////////////////////////////////////

// copyright (c) 1999 - 2000 by James A. Starkey


#if !defined(AFX_TIMESTAMP_H__35227BA2_2C14_11D4_98E0_0000C01D2301__INCLUDED_)
#define AFX_TIMESTAMP_H__35227BA2_2C14_11D4_98E0_0000C01D2301__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "DateTime.h"

class TimeStamp : public DateTime
{
public:
	int getString(int length, char * buffer);
	//TimeStamp& operator = (long value);
	//TimeStamp& operator = (DateTime value);

	long	nanos;					// nano seconds
};

#endif // !defined(AFX_TIMESTAMP_H__35227BA2_2C14_11D4_98E0_0000C01D2301__INCLUDED_)

/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
 *
 *     Software distributed under the License is distributed on 
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *     express or implied.  See the License for the specific 
 *     language governing rights and limitations under the License.
 *
 *
 *  The Original Code was created by James A. Starkey
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 */

// Timestamp.h: interface for the Timestamp class.
//
//////////////////////////////////////////////////////////////////////


#ifndef _TIMESTAMP_H_
#define _TIMESTAMP_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "SqlTime.h"
#include "DateTime.h"

class TimeStamp : public DateTime
{
public:
	int getString(int length, char * buffer);
	//TimeStamp& operator = (long value);
	//TimeStamp& operator = (DateTime value);

	long	nanos;					// nano seconds
	void setValue(DateTime value);
};

#endif

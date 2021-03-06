// SqlTime.h: interface for the SqlTime class.
//
//////////////////////////////////////////////////////////////////////

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
 *  The Original Code was created by Ann W. Harrison for IBPhoenix.
 *
 *  Copyright (c) Ann W. Harrison
 *  All Rights Reserved.
 */
 
#ifndef _SQLTIME_H_
#define _SQLTIME_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "DateTime.h"

class SqlTime : public DateTime  
{
public:
	static SqlTime convert (const char *string, int length);
	int getString (int length, char *buffer);
	int getString (const char * format, int length, char *buffer);
	//SqlTime& operator = (int value);

	int	timeValue;

	void setValue(int value);
};


#endif

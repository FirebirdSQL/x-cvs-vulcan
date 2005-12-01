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
 *     The contents of this file or any work derived from this file
 *     may not be distributed under any other license whatsoever 
 *     without the express prior written permission of the original 
 *     author.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 2004 James A. Starkey
 *  All Rights Reserved.
 */

#ifndef _OSRIEXCEPTION_H_
#define _OSRIEXCEPTION_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <stdarg.h>

enum ExceptionType
	{
	osriException,
	osriBugcheck,
	osriMemExhausted,
	osriAlreadyStuffed
	};

class SQLException;
CLASS (AdminException);
	
static const int STRING_SPACE = 4096;

//START_NAMESPACE

class OSRIException
{
public:
	OSRIException(ISC_STATUS code, ...);
	OSRIException(OSRIException* exception, ISC_STATUS code, ...);
	OSRIException(ExceptionType exceptionType);
	OSRIException(const ISC_STATUS *input);
	OSRIException(const OSRIException& source);
	OSRIException(va_list  args, ISC_STATUS code);
	virtual ~OSRIException(void);
	virtual void post(OSRIException *exception, int code, va_list stuff);
	ISC_STATUS copy(ISC_STATUS* vector, char* stringSpace);
	ISC_STATUS copy(ISC_STATUS* vector);
	char* getStringSpace(void);
	
	ISC_STATUS	statusVector [20];
	char		*strings;
	int			stringsLength;
	ExceptionType	type;
	void setType(ExceptionType exceptionType);
	void error(void);
	int getStringsLength(const ISC_STATUS* vector);
	OSRIException(void);
	OSRIException& operator =(const OSRIException& source);
	void release(void);
	OSRIException(SQLException* exception);
	void postException(ISC_STATUS code, ...);
	OSRIException(AdminException* exception);
	void setBugcheck(void);
	bool isBugcheck(void);
	void printStatus(void);
};

//END_NAMESPACE

#endif


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
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 *
 *	Changes
 *	
 *	2002-05-20	TimeStamp.cpp
 *				Contributed by Bernhard Schulte
 *				o Bring operator() up-to-date with other timestamp changes.
 *				o ditto decodeTime().
 *
 *
 */

// Timestamp.cpp: implementation of the Timestamp class.
//
//////////////////////////////////////////////////////////////////////

#ifdef __BORLANDC__
#include <iostream.h>
#else
#include <stdio.h>
#endif

#include <time.h>
#include <string.h>
#include "firebird.h"
#include "ibase.h"
#include "Connection.h"
#include "TimeStamp.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

/***
TimeStamp& TimeStamp::operator =(DateTime value)
{
	date = value.date;
	nanos = 0;

	return *this;
}

TimeStamp& TimeStamp::operator =(long value)
{
	date = value;
	nanos = 0;

	return *this;
}
***/

int TimeStamp::getString(int length, char * buffer)
{
	return DateTime::getString ("%Y-%m-%d %H:%M:%S", length, buffer);
}

void TimeStamp::setValue(DateTime value)
{
	date = value.date;
	nanos = 0;
}

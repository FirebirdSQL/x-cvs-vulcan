// Timestamp.cpp: implementation of the Timestamp class.
//
//////////////////////////////////////////////////////////////////////

// copyright (c) 1999 - 2000 by James A. Starkey


#include "Engine.h"
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

// InfoGen.cpp: implementation of the InfoGen class.
//
//////////////////////////////////////////////////////////////////////

#include <memory.h>
#include <string.h>
#include "fbdev.h"
#include "InfoGen.h"
#include "ibase.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

InfoGen::InfoGen(UCHAR *buff, int bufferLength)
{
	buffer = buff;
	ptr = buffer;
	yellow = buffer + bufferLength - 2;	// room for end or trunc
	full = false;
}

InfoGen::~InfoGen()
{

}

bool InfoGen::put(UCHAR item, int length)
{
	if (full)
		return false;

	if (ptr + 4 + length >= yellow)
		{
		full = true;
		*ptr++ = isc_info_truncated;
		return false;
		}

	*ptr++ = item;
	putShort (length);

	return true;
}

bool InfoGen::putShort(int value)
{
	*ptr++ = value;
	*ptr++ = value >> 8;

	return true;
}

bool InfoGen::putShort(UCHAR item, int value)
{
	if (!put (item, 2))
		return false;

	return putShort (value);
}

bool InfoGen::putInt(UCHAR item, int value)
{
	if (!put (item, 4))
		return false;

	return putInt(value);
}

int InfoGen::size()
{
	return ptr - buffer;
}

int InfoGen::fini()
{
	if (!full)
		*ptr++ = isc_info_end;
	
	return ptr - buffer;
}

bool InfoGen::put(UCHAR item, int length, const UCHAR* stuff)
{
	if (!put (item, length))
		return false;
	
	memcpy (ptr, stuff, length);
	ptr += length;
	
	return true;
}

bool InfoGen::putString(UCHAR item, const char* value)
{
	return put(item, (int) strlen(value), (const UCHAR*) value);
}

bool InfoGen::putUnknown(UCHAR item)
{
	if (!put(isc_info_error, 4 + 1))
		return false;
	
	*ptr++ = item;
	
	return putInt(isc_infunk);
}

bool InfoGen::putInt(int value)
{
	if (!putShort (value))
		return false;

	return putShort (value >> 16);
}

bool InfoGen::putByte(UCHAR item, UCHAR stuff)
{
	return put(item, 1, &stuff);
}

int InfoGen::maxRemaining(void)
{
	int n = yellow - ptr - 3;
	
	return (n >= 0) ? n : 0;
}

void InfoGen::forceTruncation(void)
{
	full = true;
	*ptr++ = isc_info_truncated;
}

bool InfoGen::putItem(UCHAR item)
{
	if (full)
		return false;

	if (ptr + 5 >= yellow)
		{
		full = true;
		*ptr++ = isc_info_truncated;
		return false;
		}

	*ptr++ = item;
	return true;
}

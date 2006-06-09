/*
 *	PROGRAM:	implementation for engine info item classes.
 *	MODULE:		EngineInfo.h
 *	DESCRIPTION:	EngineInfo
 *
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Arno Brinkman
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2006 Arno Brinkman <firebird@abvisie.nl>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#include <memory.h>
#include <string.h>
#include "fbdev.h"
#include "EngineInfoGen.h"
#include "ibase.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

EngineInfoGen::EngineInfoGen(UCHAR* buff, int bufferLength) : InfoGen(buff, bufferLength)
{
	//
}

EngineInfoGen::~EngineInfoGen()
{
	//
}

bool EngineInfoGen::putListBegin()
{
	return putItem(isc_info_engine_list_begin);
}

bool EngineInfoGen::putListEnd()
{
	return putItem(isc_info_engine_list_end);
}

bool EngineInfoGen::putItemValueInt(UCHAR item, int value)
{
	if (!putItem(item))
		return false;

	return putInt(isc_info_engine_value, value);
}

bool EngineInfoGen::putItemValueString(UCHAR item, const char* value)
{
	if (!putItem(item))
		return false;

	return putString(isc_info_engine_value, value);
}

EngineInfoReader::EngineInfoReader(UCHAR* buff, int bufferLength)
{
	buffer = buff;
	ptr = buffer;
	end = buffer + bufferLength;
}

EngineInfoReader::~EngineInfoReader()
{
	//
}

int EngineInfoReader::currentPosition()
{
	return (ptr - buffer);
}

bool EngineInfoReader::eof()
{
	return (ptr >= end);
}

UCHAR* EngineInfoReader::getBuffer(int bufferLength)
{
	UCHAR* retPtr = ptr;
	ptr += bufferLength;
	return retPtr;
}

UCHAR EngineInfoReader::getItem()
{
	return *ptr++;
}

JString EngineInfoReader::getValueString()
{
	if (*ptr == isc_info_engine_value)
	{
		ptr++;
		int length = getShort();
		JString value;
		value.setString((char*) getBuffer(length), length);
		return value;
	}
	else
		return "";
}

int EngineInfoReader::getValueInt()
{
	if (*ptr == isc_info_engine_value)
	{
		ptr++;
		int length = getShort();
		int value = getShort();
		value += (getShort() << 16);
		return value;
	}
	else
		return 0;
}

int EngineInfoReader::getShort()
{
	int length = (int)*ptr++;
	length += ((int)*ptr++ << 8);
	return length;
}

bool EngineInfoReader::nextItem()
{
	ptr++;
	return !eof();
}

void EngineInfoReader::skipItem()
{
	UCHAR item = *ptr;
	switch (item)
	{
		case isc_info_engine_value:
		{
			ptr++;
			int length = getShort();
			ptr += length;
			break;
		}

		case isc_info_engine_list_begin:
		{
			ptr++;
			item = getItem();
			while (!eof() && (item != isc_info_engine_list_end))
			{
				skipItem();
				item = getItem();
			}
			break;
		}

		default:
			break;
	}
}

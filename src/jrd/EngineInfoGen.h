/*
 *	PROGRAM:	interfaces for engine info item classes.
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

#ifndef ENGINEINFOGEN_H
#define ENGINEINFOGEN_H

#include "InfoGen.h"
#include "JString.h"

class EngineInfoGen : public InfoGen 
{
public:
	EngineInfoGen(UCHAR* buff, int bufferLength);
	~EngineInfoGen();

	bool putListBegin();
	bool putListEnd();
	bool putItemValueInt(UCHAR item, int value);
	bool putItemValueString(UCHAR item, const char* value);
};

class EngineInfoReader
{
private:
	UCHAR* buffer;
	UCHAR* ptr;
	UCHAR* end;
public:
	EngineInfoReader(UCHAR* buff, int bufferLength);
	~EngineInfoReader();

	inline int currentPosition();
	inline bool eof();
	UCHAR* getBuffer(int bufferLength);
	inline UCHAR getItem();
	JString getValueString();
	int getValueInt();
	inline int getShort();
	inline bool nextItem();
	void skipItem();
};

#endif // ENGINEINFOGEN_H

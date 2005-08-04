/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		ExecutionPathInfo.h
 *	DESCRIPTION:	Execution path information
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
 *  Copyright (c) 2004 Arno Brinkman <firebird@abvisie.nl>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#ifndef EXECUTION_PATH_INFO_GEN_H
#define EXECUTION_PATH_INFO_GEN_H

#include "../common/classes/array.h"

class ExecutionPathInfoGen
{
private:
	UCHAR* buffer;
	UCHAR* ptr;
	UCHAR* bufferEnd;
public:
	ExecutionPathInfoGen(UCHAR* outputBuffer, int bufferLength);
	~ExecutionPathInfoGen();

	bool put(UCHAR item);
	bool put(const UCHAR* value, int length);
	bool putType(UCHAR item);
	bool putBegin();
	bool putEnd();

	int length();
};

#endif // EXECUTION_PATH_INFO_GEN_H


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
 *  Copyright (c) 2005 Arno Brinkman <firebird@abvisie.nl>
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

class Relation;
class Request;
class str;

struct thread_db;

class ExecutionPathInfoGen
{
private:
	UCHAR* buffer;
	UCHAR* ptr;
	UCHAR* bufferEnd;
public:
	ExecutionPathInfoGen(thread_db* tdbb, UCHAR* outputBuffer, int bufferLength);
	~ExecutionPathInfoGen();

	bool put(const UCHAR* value, int length);
	bool putBegin();
	bool putByte(UCHAR value);
	bool putEnd();
	bool putRelation(Relation* relation, const str* alias);
	bool putRequest(Request* request);
	bool putString(const char* value, int length);
	bool putType(UCHAR item);

	int length();
	thread_db* threadData;
};

#endif // EXECUTION_PATH_INFO_GEN_H


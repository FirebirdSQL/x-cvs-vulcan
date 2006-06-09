/*
 *	PROGRAM:	Engine information interface
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

#ifndef ENGINEINFO_H
#define ENGINEINFO_H

class DatabaseManager;

class EngineInfoRequest
{
public:
	EngineInfoRequest();

	void init();

	bool databases;
	bool databasePaths;
	bool databaseAttachments;
	bool engineVersion;

	bool databaseList();
};

class EngineInfo
{
private:
	DatabaseManager* databaseManager;
public:
	EngineInfo(DatabaseManager* dbManager);

	ISC_STATUS getInfo(ISC_STATUS* userStatus, int itemsLength, const UCHAR *items, 
						int bufferLength, UCHAR *buffer);
};



#endif // ENGINEINFO

/*
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
 *  The Original Code was created by [Initial Developer's Name]
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c)  2004 James A. Starkey
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): James A. Starkey
 */

#ifndef _XDUMP_H_
#define _XDUMP_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "JString.h"

class Connection;
CLASS(Element);
CLASS(Stream);
class ResultSet;
class DatabaseMetaData;

class XDump
{
public:
	XDump(Connection *connect);
	virtual ~XDump(void);
	
	Connection	*connection;
	Element		*tree;
	Element		*metaData;
	Element		*data;
	char		*temp;
	int			tempLength;
	DatabaseMetaData	*databaseMetaData;
	
	void genXML(Stream *stream);
	void dumpTable(ResultSet* resultSet, bool saveData);
	const char* trim(const char* string);
	void dumpTable(const char* tableName, bool saveData);
	JString dumpPrimaryKey(Element *table);
	void dumpIndexes(Element* table, const char *primaryKey);
	void dumpAllTables(bool saveData);
	JString encode64(int length, void *gook, int count);
};
#endif

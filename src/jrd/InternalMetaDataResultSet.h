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
 *  Copyright (c) 1997 - 2000, 2001, 2003 James A. Starkey
 *  Copyright (c) 1997 - 2000, 2001, 2003 Netfrastructure, Inc.
 *  All Rights Reserved.
 */

#ifndef _INTERNALMETADATARESULTSET_H_
#define _INTERNALMETADATARESULTSET_H_


#include "InternalResultSet.h"
#include "InternalResultSet.h"
#include "JString.h"	// Added by ClassView

class InternalDatabaseMetaData;
class InternalResultSet;

class InternalMetaDataResultSet : public InternalResultSet
{
public:
	virtual bool next();
	JString expandPattern (const char *string, const char *pattern);
	bool isWildcarded (const char *pattern);
	void trimBlanks (int id);
	virtual void prepareStatement (const char *sql);
	virtual Value* getValue (int index);
	virtual int findColumn (const char *columnName);
	InternalMetaDataResultSet(InternalDatabaseMetaData *meta);
	virtual ~InternalMetaDataResultSet();

	virtual bool		isNullable (int index);
	virtual int			getScale (int index);
	virtual int			getPrecision (int index);
	virtual const char* getTableName (int index);
	virtual const char* getColumnName (int index);
	virtual int			getColumnDisplaySize (int index);
	virtual int			getColumnType (int index);


	InternalDatabaseMetaData	*metaData;
	InternalResultSet		*resultSet;
	PreparedStatement	*statement;
};

#endif

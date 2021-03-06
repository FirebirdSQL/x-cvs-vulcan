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

#ifndef _INTERNALRESULTSETMETADATA_H_
#define _INTERNALRESULTSETMETADATA_H_

#include "Connection.h"

class InternalResultSet;
class Value;

class InternalResultSetMetaData : public ResultSetMetaData  
{
public:
	virtual bool isDefinitelyWritable (int index);
	virtual bool isReadOnly (int index);
	virtual bool isWritable (int index);
	virtual bool isSigned (int index);
	virtual const char* getColumnLabel (int index);
	virtual int objectVersion();
	virtual bool isNullable (int index);
	virtual int getScale (int index);
	virtual int getPrecision (int index);
	InternalResultSetMetaData(InternalResultSet *results, int numberColumns);
	virtual ~InternalResultSetMetaData();
	virtual const char* getTableName (int index);
	virtual const char* getColumnName (int index);
	virtual int getColumnDisplaySize (int index);
	virtual int getColumnType (int index);
	virtual int getColumnCount();

	InternalResultSet	*resultSet;
	int				numberColumns;
	char			*query;
};

#endif

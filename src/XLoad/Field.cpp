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
 *  The contents of this file or any work derived from this file
 *  may not be distributed under any other license whatsoever 
 *  without the express prior written permission of the original 
 *  author.
 *
 *  The Original Code was created by James A. Starkey
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c)  2004 James A. Starkey
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): James A. Starkey
 */

#include "firebird.h"
#include "Field.h"
#include "Connection.h"

Field::Field(Table *tbl, ResultSet *resultSet)
{
	table = tbl;
	next = NULL;
	name = resultSet->getString(4);
	nullable = resultSet->getInt(11) != 0;
	type = resultSet->getInt(5);
}

Field::~Field(void)
{
}

JString Field::getTypeString(int type)
{
	switch (type)
		{
		case JDBC_CHAR:
			return "char";
			
		case JDBC_VARCHAR:
			return "varchar";
			
		case JDBC_SMALLINT:
			return "smallint";
			
		case JDBC_INTEGER:
			return "int";
			
		case JDBC_BIGINT:
			return "bigint";
			
		case JDBC_FLOAT:
			return "float";
			
		case JDBC_DOUBLE:
			return "double";
			
		case JDBC_DECIMAL:
			return "decimal";
			
		case JDBC_NUMERIC:
			return "numeric";
			
		case JDBC_DATE:
			return "date";
			
		case JDBC_TIME:
			return "time";
			
		case JDBC_TIMESTAMP:
			return "timestamp";
			
		case JDBC_BLOB:
		case JDBC_LONGVARBINARY:
			return "blob";
			
		case JDBC_CLOB:
		case JDBC_LONGVARCHAR:
			return "clob";
		
		case JDBC_ARRAY:
			return "array";
			
		default:
			return "unknown";
		}
}

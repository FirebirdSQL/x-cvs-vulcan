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

#ifndef _FIELD_H_
#define _FIELD_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "JString.h"
#include "JavaType.h"

/***
static const int JDBC_ARRAY			= -8;
static const int JDBC_BIT			= -7;
static const int JDBC_TINYINT		= -6;
static const int JDBC_SMALLINT		= 5;
static const int JDBC_INTEGER		= 4;
static const int JDBC_BIGINT		= -5;

static const int JDBC_FLOAT			= 6;
static const int JDBC_REAL			= 7;
static const int JDBC_DOUBLE		= 8;

static const int JDBC_NUMERIC		= 2;
static const int JDBC_DECIMAL		= 3;

static const int JDBC_CHAR			= 1;
static const int JDBC_VARCHAR		= 12;
static const int JDBC_LONGVARCHAR	= -1;

static const int JDBC_SQL_DATE		= 9;
static const int JDBC_SQL_TIME		= 10;
static const int JDBC_SQL_TIMESTAMP	= 11;

static const int JDBC_DATE			= 91;
static const int JDBC_TIME			= 92;
static const int JDBC_TIMESTAMP		= 93;

static const int JDBC_BINARY		= -2;
static const int JDBC_VARBINARY		= -3;
static const int JDBC_LONGVARBINARY	= -4;
static const int JDBC_BLOB			= 2004;
static const int JDBC_CLOB			= 2005;
***/

class ResultSet;
class Table;

class Field
{
public:
	Field(Table *tbl, ResultSet *resultSet);
	virtual ~Field(void);
	
	Table	*table;
	JString	name;
	Field	*next;
	bool	nullable;
	int		type;
	
	static JString getTypeString(int type);
};

#endif


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
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1999, 2000, 2001 James A. Starkey
 *  All Rights Reserved.
 *
 */

#ifndef _INTERNALSTATEMENT_H_
#define _INTERNALSTATEMENT_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Connection.h"
#include "LinkedList.h"
#include "JString.h"

class Value;
class InternalResultSet;
class InternalConnection;
class CStatement;
class Transaction;
class dsql_msg;

class InternalStatement : public Statement  
{
public:
	void getUpdateCounts();
	virtual int objectVersion();
	void clearResults();
	virtual bool execute();
	void prepareStatement (const char *sqlString);
	void deleteResultSet (InternalResultSet *resultSet);
	InternalStatement(InternalConnection *connect);
	virtual int getUpdateCount();
	virtual bool getMoreResults();
	virtual void setCursorName (const char *name);
	virtual ResultSet* executeQuery (const char *sqlString);
	virtual ResultSet* getResultSet();
	virtual ResultList* search (const char *searchString);
	virtual int executeUpdate (const char *sqlString);
	virtual bool execute (const char *sqlString);
	virtual void close();
	virtual int release();
	virtual void addRef();
	virtual ~InternalStatement();
	void reset(void);

	InternalResultSet*	createResultSet();
	LinkedList		resultSets;
	InternalConnection	*connection;
	int				useCount;
	int				numberColumns;
	int				numberParameters;
	int				resultsCount;
	int				resultsSequence;
	CStatement		*statement;
	int				updateCount;
	int				updateDelta;
	int				deleteCount;
	int				deleteDelta;
	int				insertCount;
	int				insertDelta;
	int				summaryUpdateCount;
	UCHAR			*sendBuffer;
	int				requestInstantiation;
	virtual void mapParameters(dsql_msg* message);
	int findColumn(const char* columnName);
};

#endif


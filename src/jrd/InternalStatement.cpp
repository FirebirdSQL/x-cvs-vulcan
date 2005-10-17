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
 *
 *	Changes
 *
 *	2002-05-21	BinaryBlob.cpp
 *				Change release() to test useCount <=0
 *	
 *	2002-05-20	BinaryBlob.cpp
 *				Contributed by Robert Milharcic
 *				o Start with useCount of 0
 *
 */

#include <stdlib.h>
#include <time.h>
#include "fbdev.h"
#include "common.h"
//#include "ibase.h"
#include "InternalStatement.h"
#include "InternalResultSet.h"
#include "InternalConnection.h"
#include "InternalBlob.h"
#include "Attachment.h"
#include "SQLError.h"
#include "Value.h"
#include "TimeStamp.h"
#include "ibsetjmp.h"
#include "ThreadData.h"
#include "jrd_proto.h"
#include "Request.h"
#include "../dsql/CStatement.h"
#include "../dsql/dsql.h"

#define SQLEXCEPTION		SQLError
#define NOT_YET_IMPLEMENTED	throw SQLError (FEATURE_NOT_YET_IMPLEMENTED, "not yet implemented")

/***
static char requestInfo [] = { isc_info_sql_records,
							   isc_info_end };
***/

static int init();
static struct tm baseTm = { 0, 0, 0, 1, 0, 70 };
static long baseDate = init();

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

int init()
{
	ISC_QUAD baseDate;
	isc_encode_date (&baseTm, &baseDate);

	return baseDate.gds_quad_high;
}

InternalStatement::InternalStatement(InternalConnection *connect)
{
	CStatement::staticInitialization();
	connection = connect;
	useCount = 1;
	numberColumns = 0;
	statement = NULL;
	updateCount = insertCount = deleteCount = 0;
	sendBuffer = NULL;
}

InternalStatement::~InternalStatement()
{
	reset();
		
	if (connection)
		{
		connection->deleteStatement (this);
		connection = NULL;
		}
}

InternalResultSet* InternalStatement::createResultSet()
{
	InternalResultSet *resultSet = new InternalResultSet (this);
	resultSets.append (resultSet);

	return resultSet;
}

void InternalStatement::close()
{
	FOR_OBJECTS (InternalResultSet*, resultSet, &resultSets)
		resultSet->close();
	END_FOR;
	
	delete statement;
}

bool InternalStatement::execute(const char * sqlString)
{
	NOT_YET_IMPLEMENTED;

	return false;
}

int InternalStatement::executeUpdate(const char * sqlString)
{
	NOT_YET_IMPLEMENTED;

	return 0;
}

ResultList* InternalStatement::search(const char * searchString)
{
	NOT_YET_IMPLEMENTED;

	return NULL;
}

ResultSet* InternalStatement::getResultSet()
{
	if (!statement)
		throw SQLEXCEPTION (RUNTIME_ERROR, "no active statement");

	/***
	if (outputSqlda.sqlda->sqld < 1)
		throw SQLEXCEPTION (RUNTIME_ERROR, "current statement doesn't return results");
	***/
	
	return createResultSet();
}

ResultSet* InternalStatement::executeQuery(const char * sqlString)
{
	NOT_YET_IMPLEMENTED;

	return NULL;
}

void InternalStatement::setCursorName(const char * name)
{
	/***
	ISC_STATUS statusVector [20];
	isc_dsql_set_cursor_name (statusVector, &statementHandle, (char*) name, 0);

	if (statusVector [1])
		THROW_ISC_EXCEPTION (statusVector);
	***/
}

void InternalStatement::addRef()
{
	++useCount;
}

int InternalStatement::release()
{
	if (--useCount == 0)
		{
		delete this;
		return 0;
		}

	return useCount;
}

bool InternalStatement::getMoreResults()
{
	if (resultsSequence >= resultsCount)
		return false;

	++resultsSequence;

	/***
	if (outputSqlda.sqlda->sqld > 0)
		return true;
	***/
	
	return false;
}

int InternalStatement::getUpdateCount()
{
	/***
	if (outputSqlda.sqlda->sqld > 0)
		return -1;
	***/
	
	return summaryUpdateCount;
}

void InternalStatement::deleteResultSet(InternalResultSet * resultSet)
{
	resultSets.deleteItem (resultSet);
}

void InternalStatement::prepareStatement(const char * sqlString)
{
	reset();
	ISC_STATUS statusVector [20];
	ThreadData thread (statusVector, connection->attachment);
	clearResults();
	statement = new CStatement (connection->attachment);
	thread.setDsqlPool (statement->pool);
	statement->prepare (thread, strlen (sqlString), sqlString, SQL_DIALECT_V6);
	requestInstantiation = statement->getInstantiation();
	
	insertCount = deleteCount = updateCount = 0;
	
	if (statement->sendMessage)
		{
		numberParameters = statement->sendMessage->msg_index;
		sendBuffer = new UCHAR [statement->sendMessage->msg_length];
		}
	else
		numberParameters = 0;
	
	if (statement->receiveMessage)
		numberColumns = statement->receiveMessage->msg_index;
}

bool InternalStatement::execute()
{
	ISC_STATUS statusVector [20];
	
	if (statement->req_type == REQ_DDL)
		{
		if (jrd8_ddl (statusVector, &connection->attachment, &connection->transaction, statement->blrGen->getLength(), statement->blrGen->buffer))
			throw OSRIException(statusVector);
		
		return false;
		}
		
	dsql_msg *message = statement->sendMessage;
	
	if (message)
		{
		mapParameters (message);
		if (jrd8_start_and_send (statusVector, &statement->request, &connection->transaction,
								 message->msg_number, message->msg_length, sendBuffer, requestInstantiation))
			throw OSRIException(statusVector);
		}
	else
		{
		if (jrd8_start_request (statusVector, &statement->request, &connection->transaction, requestInstantiation))
			throw OSRIException(statusVector);
		}
	
	resultsCount = 1;
	resultsSequence = 0;
	getUpdateCounts();

	if (connection->autoCommit && summaryUpdateCount > 0)
		connection->commitAuto();

	//return outputSqlda.sqlda->sqld > 0;
	return true;
}

void InternalStatement::clearResults()
{

}

int InternalStatement::objectVersion()
{
	return STATEMENT_VERSION;
}

void InternalStatement::getUpdateCounts()
{
	Request *request = statement->request->getInstantiatedRequest (requestInstantiation);

	int n = request->req_records_deleted;
	deleteDelta = n - deleteCount;
	deleteCount = n;
	
	n = request->req_records_updated;
	updateDelta = n - updateCount;
	updateCount = n;
	
	n = request->req_records_inserted;
	insertDelta = n - insertCount;
	insertCount = n;
	
	summaryUpdateCount = MAX (insertDelta, deleteDelta);
	summaryUpdateCount = MAX (summaryUpdateCount, updateDelta);
}

/***
void InternalStatement::setValue(Value *value, XSQLVAR *var)
{
	if ((var->sqltype & 1) && *var->sqlind == -1)
		value->setNull();
	else
		switch (var->sqltype & ~1)
			{
			case SQL_TEXT:
				{
				char *data = (char*) var->sqldata;
				//printf ("%d '%s'\n", n, data);
				data [var->sqllen - 1] = 0;
				value->setString (data, false);
				}
				break;

			case SQL_VARYING:
				{
				int length = *((short*) var->sqldata);
				char *data = var->sqldata + 2;
				if (length < var->sqllen)
					{
					data [length] = 0;
					value->setString (data, false);
					}
				else
					value->setString (length, data, false);
				//printf ("%d '%*s'\n", n, length, data);
				}
				break;

			case SQL_SHORT:
				value->setValue (*(short*) var->sqldata);
				break;

			case SQL_LONG:
				value->setValue (*(long*) var->sqldata);
				break;

			case SQL_FLOAT:
				value->setValue (*(float*) var->sqldata);
				break;

			case SQL_DOUBLE:
				value->setValue (*(double*) var->sqldata);
				break;

			case SQL_QUAD:
			case SQL_INT64:
				value->setValue (*(QUAD*) var->sqldata);
				break;

			case SQL_BLOB:
				value->setValue (new InternalBlob (this, (ISC_QUAD*) var->sqldata));
				break;

			case SQL_TIMESTAMP:
				{
				ISC_TIMESTAMP *date = (ISC_TIMESTAMP*) var->sqldata;
				long days = date->timestamp_date - baseDate;
				TimeStamp timestamp;
				timestamp = (long) (days * 24 * 60 * 60 + date->timestamp_time / 10000);
				value->setValue (timestamp);
				}
				break;

			case SQL_TYPE_DATE:
				{
				ISC_DATE date = *(ISC_DATE*) var->sqldata;
				long days = date - baseDate;
				DateTime dateTime;
				dateTime = (long) (days * 24 * 60 * 60);
				value->setValue (dateTime);
				}
				break;

			case SQL_D_FLOAT:
			case SQL_ARRAY:
			case SQL_TYPE_TIME:
				NOT_YET_IMPLEMENTED;
				break;
			}
}
***/

void InternalStatement::reset(void)
{
	if (statement)
		{
		statement->release();
		statement = NULL;
		}
	
	if (sendBuffer)
		{
		delete [] sendBuffer;
		sendBuffer = NULL;
		}

}

void InternalStatement::mapParameters(dsql_msg* message)
{
}

int InternalStatement::findColumn(const char* columnName)
{
	int index = statement->findColumn(columnName);
	
	if (index <= 0)
		return -1;
	
	return index;
}

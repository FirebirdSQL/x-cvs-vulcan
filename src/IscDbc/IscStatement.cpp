// IscStatement.cpp: implementation of the IscStatement class.
//
//////////////////////////////////////////////////////////////////////

#include <malloc.h>
#include <time.h>
#include "IscDbc.h"
#include "IscStatement.h"
#include "IscResultSet.h"
#include "IscConnection.h"
#include "IscBlob.h"
#include "SQLError.h"
#include "Value.h"

static char requestInfo [] = { isc_info_sql_records,
							   isc_info_end };

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

IscStatement::IscStatement(IscConnection *connect)
{
	connection = connect;
	useCount = 1;
	numberColumns = 0;
	statementHandle = NULL;
	updateCount = insertCount = deleteCount = 0;
}

IscStatement::~IscStatement()
{
	if (connection)
		{
		connection->deleteStatement (this);
		connection = NULL;
		}
}

IscResultSet* IscStatement::createResultSet()
{
	IscResultSet *resultSet = new IscResultSet (this);
	resultSets.append (resultSet);

	return resultSet;
}

void IscStatement::close()
{
	reset();
	release();
}

bool IscStatement::execute(const char * sqlString)
{
	prepareStatement (sqlString);
	
	return execute();
}

int IscStatement::executeUpdate(const char * sqlString)
{
	NOT_YET_IMPLEMENTED;

	return 0;
}

ResultList* IscStatement::search(const char * searchString)
{
	NOT_YET_IMPLEMENTED;

	return NULL;
}

ResultSet* IscStatement::getResultSet()
{
	if (!statementHandle)
		throw SQLEXCEPTION (RUNTIME_ERROR, "no active statement");

	if (outputSqlda.sqlda->sqld < 1)
		throw SQLEXCEPTION (RUNTIME_ERROR, "current statement doesn't return results");
	
	return createResultSet();
}

ResultSet* IscStatement::executeQuery(const char * sqlString)
{
	NOT_YET_IMPLEMENTED;

	return NULL;
}

void IscStatement::setCursorName(const char * name)
{
	ISC_STATUS statusVector [20];
	isc_dsql_set_cursor_name (statusVector, &statementHandle, (char*) name, 0);

	if (statusVector [1])
		THROW_ISC_EXCEPTION (statusVector);
}

void IscStatement::addRef()
{
	++useCount;
}

int IscStatement::release()
{
	if (--useCount == 0)
		{
		delete this;
		return 0;
		}

	return useCount;
}

bool IscStatement::getMoreResults()
{
	if (resultsSequence >= resultsCount)
		return false;

	++resultsSequence;

	if (outputSqlda.sqlda->sqld > 0)
		return true;

	return false;
}

int IscStatement::getUpdateCount()
{
	if (outputSqlda.sqlda->sqld > 0)
		return -1;

	int n = summaryUpdateCount;
	summaryUpdateCount = -1;
	
	return n;
}

void IscStatement::deleteResultSet(IscResultSet * resultSet)
{
	resultSets.deleteItem (resultSet);
}

void IscStatement::prepareStatement(const char * sqlString)
{
	clearResults();

	// Make sure we have a transaction started.  Allocate a statement.

	connection->startTransaction();
	ISC_STATUS statusVector [20];
	isc_dsql_allocate_statement (statusVector, &connection->databaseHandle, &statementHandle);

	if (statusVector [1])
		THROW_ISC_EXCEPTION (statusVector);

	// Prepare dynamic SQL statement.  Make first attempt to get parameters

	isc_dsql_prepare (statusVector, &connection->transactionHandle, &statementHandle,
					  0, (char*) sqlString, SQL_DIALECT_V6, outputSqlda);

	if (statusVector [1])
		THROW_ISC_EXCEPTION (statusVector);

	// If we didn't allocate a large enough SQLDA, try again.

	if (outputSqlda.checkOverflow())
		{
		isc_dsql_describe (statusVector, &statementHandle, 1, outputSqlda);
		if (statusVector [1])
			THROW_ISC_EXCEPTION (statusVector);
		}

	numberColumns = outputSqlda.getColumnCount();
	XSQLVAR *var = outputSqlda.sqlda->sqlvar;
	insertCount = deleteCount = updateCount = 0;
}

bool IscStatement::execute()
{
	// Make sure there is a transaction

	connection->startTransaction();
	ISC_STATUS statusVector [20];

	if (isc_dsql_execute (statusVector, &connection->transactionHandle, &statementHandle,
						  3, inputSqlda))
		{
		if (connection->autoCommit)
			connection->rollbackAuto();
		THROW_ISC_EXCEPTION (statusVector);
		}

	resultsCount = 1;
	resultsSequence = 0;
	getUpdateCounts();

	if (connection->autoCommit && summaryUpdateCount > 0)
		connection->commitAuto();

	return outputSqlda.sqlda->sqld > 0;
}

void IscStatement::clearResults()
{
	FOR_OBJECTS (IscResultSet*, resultSet, &resultSets)
		resultSet->close();
	END_FOR;
}

int IscStatement::objectVersion()
{
	return STATEMENT_VERSION;
}

void IscStatement::getUpdateCounts()
{
	char buffer [128];
	ISC_STATUS	statusVector [20];
	isc_dsql_sql_info (statusVector, &statementHandle, 
					   sizeof (requestInfo), requestInfo,
					   sizeof (buffer), buffer);

	int n;

	for (char *p = buffer; *p != isc_info_end;)
		{
		char item = *p++;
		int length = isc_vax_integer (p, 2);
		p += 2;
		if (item == isc_info_sql_records)
			for (char *q = p; *q != isc_info_end;)
				{
				char item = *q++;
				int l = isc_vax_integer (q, 2);
				q += 2;
				switch (item)
					{
					case isc_info_req_insert_count:
						n = isc_vax_integer (q, l);
						insertDelta = n - insertCount;
						insertCount = n;
						break;

					case isc_info_req_delete_count:
						n = isc_vax_integer (q, l);
						deleteDelta = n - deleteCount;
						deleteCount = n;
						break;

					case isc_info_req_update_count:
						n = isc_vax_integer (q, l);
						updateDelta = n - updateCount;
						updateCount = n;
						break;
					}
				q += l;
				}
		p += length;
		}

	summaryUpdateCount = MAX (insertDelta, deleteDelta);
	summaryUpdateCount = MAX (summaryUpdateCount, updateDelta);
}

void IscStatement::setValue(Value *value, XSQLVAR *var)
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
				if (length < var->sqllen - 2)
					{
					data [length] = 0;
					value->setString (length, data, false);
					}
				else
					value->setString (length, data, true);
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

			case SQL_ARRAY:
			case SQL_BLOB:
				value->setValue (new IscBlob (this, (ISC_QUAD*) var->sqldata));
				break;

			case SQL_TIMESTAMP:
				{
				ISC_TIMESTAMP *date = (ISC_TIMESTAMP*) var->sqldata;
				long days = date->timestamp_date - baseDate;
				TimeStamp timestamp;
				timestamp.setDate(days * 24 * 60 * 60 + date->timestamp_time / 10000);
				value->setValue (timestamp);
				}
				break;

			case SQL_TYPE_DATE:
				{
				ISC_DATE date = *(ISC_DATE*) var->sqldata;
				long days = date - baseDate;
				DateTime dateTime;
				dateTime.setDate(days * 24 * 60 * 60);
				value->setValue (dateTime);
				}
				break;

			case SQL_D_FLOAT:
			case SQL_TYPE_TIME:
			default:
				NOT_YET_IMPLEMENTED;
				break;
			}
}

void IscStatement::reset(void)
{
	ISC_STATUS statusVector [20];
	clearResults();
	
	if (statementHandle)
		isc_dsql_free_statement (statusVector, &statementHandle, DSQL_drop);
	
}

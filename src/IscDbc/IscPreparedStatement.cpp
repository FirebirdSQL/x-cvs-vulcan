// IscPreparedStatement.cpp: implementation of the IscPreparedStatement class.
//
//////////////////////////////////////////////////////////////////////

#include <malloc.h>
#include "IscDbc.h"
#include "IscPreparedStatement.h"
#include "SQLError.h"
#include "IscResultSet.h"
#include "IscConnection.h"
#include "BinaryBlob.h"
#include "AsciiBlob.h"
#include "Value.h"
#include "IscStatementMetaData.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscPreparedStatement::IscPreparedStatement(IscConnection *connection) : IscStatement (connection)
{
	statementMetaData = NULL;
}

IscPreparedStatement::~IscPreparedStatement()
{
	if (statementMetaData)
		delete statementMetaData;
}

ResultSet* IscPreparedStatement::executeQuery()
{
	if (outputSqlda.sqlda->sqld < 1)
		throw SQLEXCEPTION (RUNTIME_ERROR, "statement is not a Select");

	execute();
	getMoreResults();

	return getResultSet();
}

Value* IscPreparedStatement::getParameter(int index)
{
	if (index < 0 || index >= parameters.count)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid parameter index %d", index);

	return parameters.values + index;
}

void IscPreparedStatement::setInt(int index, long value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setNull(int index, int type)
{
	getParameter (index - 1)->clear();
}

void IscPreparedStatement::setDate(int index, DateTime value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setDouble(int index, double value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setString(int index, const char * string)
{
	getParameter (index - 1)->setString (string, true);
}

bool IscPreparedStatement::execute()
{
	int numberParameters = inputSqlda.getColumnCount();

	for (int n = 0; n < numberParameters; ++n)
		inputSqlda.setValue (n, parameters.values + n, connection);

	return IscStatement::execute();
}

int IscPreparedStatement::executeUpdate()
{
	connection->startTransaction();
	execute();

	return updateCount;
}

void IscPreparedStatement::setBytes(int index, int length, const void* bytes)
{
	BinaryBlob *blob = new BinaryBlob();
	getParameter (index - 1)->setValue (blob);
	blob->putSegment (length, (char*) bytes, true);
	blob->release();
}


bool IscPreparedStatement::execute (const char *sqlString)
{
	return IscStatement::execute (sqlString);
}

ResultSet*	 IscPreparedStatement::executeQuery (const char *sqlString)
{
	return IscStatement::executeQuery (sqlString);
}

int	IscPreparedStatement::getUpdateCount()
{
	return IscStatement::getUpdateCount ();
}

bool IscPreparedStatement::getMoreResults()
{
	return IscStatement::getMoreResults();
}

void IscPreparedStatement::setCursorName (const char *name)
{
	IscStatement::setCursorName (name);
}

ResultSet* IscPreparedStatement::getResultSet()
{
	return IscStatement::getResultSet ();
}

ResultList* IscPreparedStatement::search (const char *searchString)
{
	return IscStatement::search (searchString);
}

int	IscPreparedStatement::executeUpdate (const char *sqlString)
{
	return IscStatement::executeUpdate (sqlString);
}

void IscPreparedStatement::close()
{
	IscStatement::close ();
}

int IscPreparedStatement::release()
{
	return IscStatement::release ();
}

void IscPreparedStatement::addRef()
{
	IscStatement::addRef ();
}


void IscPreparedStatement::prepare(const char * sqlString)
{
	prepareStatement (sqlString);
	getInputParameters();
}

void IscPreparedStatement::getInputParameters()
{
	ISC_STATUS statusVector [20];

	isc_dsql_describe_bind (statusVector, &statementHandle, 1, inputSqlda);

	if (statusVector [1])
		THROW_ISC_EXCEPTION (statusVector);

	if (inputSqlda.checkOverflow())
		{
		isc_dsql_describe_bind (statusVector, &statementHandle, 1, inputSqlda);
		if (statusVector [1])
			THROW_ISC_EXCEPTION (statusVector);
		}

	parameters.alloc (inputSqlda.getColumnCount());
	inputSqlda.allocBuffer();
}

StatementMetaData* IscPreparedStatement::getStatementMetaData()
{
	if (statementMetaData)
		return statementMetaData;

	statementMetaData = new IscStatementMetaData (this);

	return statementMetaData;
}

void IscPreparedStatement::setByte(int index, char value)
{
	getParameter (index - 1)->setValue ((short) value);
}

void IscPreparedStatement::setLong(int index, QUAD value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setFloat(int index, float value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setTime(int index, Time value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setTimestamp(int index, TimeStamp value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setShort(int index, short value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setBlob(int index, Blob * value)
{
	getParameter (index - 1)->setValue (value);
}

void IscPreparedStatement::setClob(int index, Clob * value)
{
	getParameter (index - 1)->setValue (value);
}

int IscPreparedStatement::objectVersion()
{
	return PREPAREDSTATEMENT_VERSION;
}

// IscResultSet.cpp: implementation of the IscResultSet class.
//
//////////////////////////////////////////////////////////////////////

#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "IscDbc.h"
#include "IscResultSet.h"
#include "IscStatement.h"
#include "IscResultSetMetaData.h"
#include "IscBlob.h"
#include "IscConnection.h"
#include "SQLError.h"
#include "Value.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


IscResultSet::IscResultSet(IscStatement *iscStatement)
{
	useCount = 1;
	statement = iscStatement;
	metaData = NULL;
	conversions = NULL;
	sqlda = NULL;

	if (iscStatement)
		numberColumns = statement->numberColumns;

	if (statement)
		{
		statement->addRef();
		sqlda = &statement->outputSqlda;
		numberColumns = sqlda->getColumnCount();
		sqlda->allocBuffer();
		values.alloc (numberColumns);
		allocConversions();
		}
}

IscResultSet::~IscResultSet()
{
	close();

	if (metaData)
		delete metaData;
}

ResultSetMetaData* IscResultSet::getMetaData()
{
	if (metaData)
		return (ResultSetMetaData*) metaData;

	metaData = new IscResultSetMetaData (this, numberColumns);

	return (ResultSetMetaData*) metaData;
}

bool IscResultSet::next()
{
	deleteBlobs();
	reset();
	ISC_STATUS statusVector [20];

	int ret = isc_dsql_fetch (statusVector, &statement->statementHandle, 1, *sqlda);

	if (ret)
		{
		if (ret == 100)
			return false;
		THROW_ISC_EXCEPTION (statusVector);
		}

	//sqlda->print();
	XSQLVAR *var = sqlda->sqlda->sqlvar;
    Value *value = values.values;

	for (int n = 0; n < numberColumns; ++n, ++var, ++value)
		statement->setValue (value, var);

	return true;
}

const char* IscResultSet::getString(int id)
{
	if (id < 1 || id > numberColumns)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column index for result set");

	if (conversions [id - 1])
		return conversions [id - 1];

	return getValue (id)->getString(conversions + id - 1);
}


const char* IscResultSet::getString(const char * columnName)
{
	return getString (findColumn (columnName));
}

long IscResultSet::getInt(int id)
{
	return getValue (id)->getLong();
}

long IscResultSet::getInt(const char * columnName)
{
	return getValue (columnName)->getLong();
}

Value* IscResultSet::getValue(int index)
{
	if (index < 1 || index > numberColumns)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column index for result set");

	Value *value = values.values + index - 1;
	valueWasNull = value->type == Null;

	return value;
}

Value* IscResultSet::getValue(const char * columnName)
{
	return getValue (findColumn (columnName));
}

void IscResultSet::close()
{
	if (statement)
		{
		if (statement->connection->autoCommit)
			statement->connection->commitAuto();
		statement->deleteResultSet (this);
		statement->release();
		statement = NULL;
		}
}

Blob* IscResultSet::getBlob(int index)
{
	Blob *blob = getValue (index)->getBlob();
	blob->addRef();
	blobs.append (blob);

	return blob;
}

Blob* IscResultSet::getBlob(const char * columnName)
{
	Blob *blob = getValue (columnName)->getBlob();
	blob->addRef();
	blobs.append (blob);

	return blob;
}


Clob* IscResultSet::getClob(int index)
{
	Clob *blob = getValue (index)->getClob();
	blob->addRef();
	clobs.append (blob);

	return blob;
}

Clob* IscResultSet::getClob(const char * columnName)
{
	Clob *blob = getValue (columnName)->getClob();
	blob->addRef();
	clobs.append (blob);

	return blob;
}

void IscResultSet::deleteBlobs()
{
	FOR_OBJECTS (Blob*, blob, &blobs)
		blob->release();
	END_FOR;

	blobs.clear();

	FOR_OBJECTS (Clob*, blob, &clobs)
		blob->release();
	END_FOR;

	clobs.clear();
}

const char* IscResultSet::genHTML(const char *series, const char *type, Properties *context)
{
	throw SQLEXCEPTION (FEATURE_NOT_YET_IMPLEMENTED, "function is not implemented");

	return NULL;
}

void IscResultSet::freeHTML(const char *html)
{
	delete [] (char*) html;
}

void IscResultSet::addRef()
{
	++useCount;
}

int IscResultSet::release()
{
	if (--useCount == 0)
		{
		delete this;
		return 0;
		}

	return useCount;
}

void IscResultSet::reset()
{
	if (conversions)
		for (int n = 0; n < numberColumns; ++n)
			if (conversions [n])
				{
				delete [] conversions [n];
				conversions [n] = NULL;
				}
}

int IscResultSet::findColumn(const char * columnName)
{
	int n = sqlda->findColumn (columnName);

	if (n >= 0)
		return n + 1;

	/***
	for (int n = 0; n < numberColumns; ++n)
		if (!strcasecmp (columnNames [n], columnName))
			return n + 1;
	***/

	throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column name %s for result set",
							columnName);

	return -1;
}



bool IscResultSet::wasNull()
{
	return valueWasNull;
}

void IscResultSet::allocConversions()
{
	conversions = new char* [numberColumns];
	memset (conversions, 0, sizeof (char*) * numberColumns);
}

void IscResultSet::setValue(int index, const char * value)
{
	values.values [index - 1].setString (value, true);
}

void IscResultSet::setValue(int index, long value)
{
	values.values [index - 1].setValue (value, true);
}

short IscResultSet::getShort(int id)
{
	return getValue (id)->getShort();
}

short IscResultSet::getShort(const char * columnName)
{
	return getValue (columnName)->getShort();
}

QUAD IscResultSet::getLong(int id)
{
	return getValue (id)->getQuad();
}

QUAD IscResultSet::getLong(const char * columnName)
{
	return getValue (columnName)->getQuad();
}

double IscResultSet::getDouble(int id)
{
	return getValue (id)->getDouble();
}

double IscResultSet::getDouble(const char * columnName)
{
	return getValue (columnName)->getDouble();
}

char IscResultSet::getByte(int id)
{
	return getValue (id)->getByte();
}

char IscResultSet::getByte(const char * columnName)
{
	return getValue (columnName)->getByte();
}

float IscResultSet::getFloat(int id)
{
	return (float) getValue (id)->getDouble();
}

float IscResultSet::getFloat(const char * columnName)
{
	return (float) getValue (columnName)->getDouble();
}

int IscResultSet::getColumnType(int index)
{
	return sqlda->getColumnType (index);
}

int IscResultSet::getColumnDisplaySize(int index)
{
	return sqlda->getDisplaySize (index);
}

const char* IscResultSet::getColumnName(int index)
{
	return sqlda->getColumnName (index);
}

const char* IscResultSet::getTableName(int index)
{
	return sqlda->getTableName (index);
}

int IscResultSet::getPrecision(int index)
{
	return sqlda->getPrecision (index);
}

int IscResultSet::getScale(int index)
{
	return sqlda->getScale (index);
}

bool IscResultSet::isNullable(int index)
{
	return sqlda->isNullable (index);
}

DateTime IscResultSet::getDate(int id)
{
	return getValue (id)->getDate();
}

DateTime IscResultSet::getDate(const char * columnName)
{
	return getValue (columnName)->getDate();
}

Time IscResultSet::getTime(int id)
{
	return getValue (id)->getTime();
}

Time IscResultSet::getTime(const char * columnName)
{
	return getValue (columnName)->getTime();
}

TimeStamp IscResultSet::getTimestamp(int id)
{
	return getValue (id)->getTimestamp();
}

TimeStamp IscResultSet::getTimestamp(const char * columnName)
{
	return getValue (columnName)->getTimestamp();
}

int IscResultSet::objectVersion()
{
	return RESULTSET_VERSION;
}

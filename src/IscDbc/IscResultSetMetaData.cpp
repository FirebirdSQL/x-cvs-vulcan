// IscResultSetMetaData.cpp: implementation of the IscResultSetMetaData class.
//
//////////////////////////////////////////////////////////////////////

#include "IscDbc.h"
#include "IscResultSetMetaData.h"
#include "IscResultSet.h"
#include "SQLError.h"
#include "Sqlda.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscResultSetMetaData::IscResultSetMetaData(IscResultSet *results, int columns)
{
	resultSet = results;
	numberColumns = columns;
}

IscResultSetMetaData::~IscResultSetMetaData()
{

}

int IscResultSetMetaData::getColumnCount()
{
	return numberColumns;
}

int IscResultSetMetaData::getColumnType(int index)
{
	return resultSet->getColumnType (index);
}

int IscResultSetMetaData::getColumnDisplaySize(int index)
{
	return resultSet->getColumnDisplaySize (index);
}

const char* IscResultSetMetaData::getColumnName(int index)
{
	return resultSet->getColumnName (index);
}

const char* IscResultSetMetaData::getTableName(int index)
{
	return resultSet->getTableName (index);
}

int IscResultSetMetaData::getPrecision(int index)
{
	return resultSet->getPrecision (index);
}

int IscResultSetMetaData::getScale(int index)
{
	return resultSet->getScale (index);
}

bool IscResultSetMetaData::isNullable(int index)
{
	return resultSet->isNullable (index);
}

int IscResultSetMetaData::objectVersion()
{
	return RESULTSETMETADATA_VERSION;
}

const char* IscResultSetMetaData::getColumnLabel(int index)
{
	return getColumnName (index);
}

bool IscResultSetMetaData::isSigned(int index)
{
	return true;
}

bool IscResultSetMetaData::isWritable(int index)
{
	return true;
}

bool IscResultSetMetaData::isReadOnly(int index)
{
	return false;
}

bool IscResultSetMetaData::isDefinitelyWritable(int index)
{
	return false;
}

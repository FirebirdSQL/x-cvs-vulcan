#include "firebird.h"
#include "InternalResultSetMetaData.h"
#include "InternalResultSet.h"
#include "SQLError.h"
//#include "Sqlda.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

InternalResultSetMetaData::InternalResultSetMetaData(InternalResultSet *results, int columns)
{
	resultSet = results;
	numberColumns = columns;
}

InternalResultSetMetaData::~InternalResultSetMetaData()
{

}

int InternalResultSetMetaData::getColumnCount()
{
	return numberColumns;
}

int InternalResultSetMetaData::getColumnType(int index)
{
	return resultSet->getColumnType (index);
}

int InternalResultSetMetaData::getColumnDisplaySize(int index)
{
	return resultSet->getColumnDisplaySize (index);
}

const char* InternalResultSetMetaData::getColumnName(int index)
{
	return resultSet->getColumnName (index);
}

const char* InternalResultSetMetaData::getTableName(int index)
{
	return resultSet->getTableName (index);
}

int InternalResultSetMetaData::getPrecision(int index)
{
	return resultSet->getPrecision (index);
}

int InternalResultSetMetaData::getScale(int index)
{
	return resultSet->getScale (index);
}

bool InternalResultSetMetaData::isNullable(int index)
{
	return resultSet->isNullable (index);
}

int InternalResultSetMetaData::objectVersion()
{
	return RESULTSETMETADATA_VERSION;
}

const char* InternalResultSetMetaData::getColumnLabel(int index)
{
	return getColumnName (index);
}

bool InternalResultSetMetaData::isSigned(int index)
{
	return true;
}

bool InternalResultSetMetaData::isWritable(int index)
{
	return true;
}

bool InternalResultSetMetaData::isReadOnly(int index)
{
	return false;
}

bool InternalResultSetMetaData::isDefinitelyWritable(int index)
{
	return false;
}

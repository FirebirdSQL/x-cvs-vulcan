#include <stdio.h>
#include "firebird.h"
#include "InternalMetaDataResultSet.h"
#include "InternalDatabaseMetaData.h"
#include "InternalResultSet.h"
#include "SQLError.h"
#include "InternalConnection.h"
#include "Value.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

InternalMetaDataResultSet::InternalMetaDataResultSet(InternalDatabaseMetaData *meta) : InternalResultSet (NULL)
{
	metaData = meta;
	resultSet = NULL;
	statement = NULL;
}

InternalMetaDataResultSet::~InternalMetaDataResultSet()
{

}


int InternalMetaDataResultSet::findColumn(const char * columnName)
{
	return resultSet->findColumn (columnName);
}

Value* InternalMetaDataResultSet::getValue(int index)
{
	return resultSet->getValue (index);
}

void InternalMetaDataResultSet::prepareStatement(const char * sql)
{
	statement = metaData->connection->prepareStatement (sql);
	resultSet = (InternalResultSet*) statement->executeQuery();
	numberColumns = resultSet->numberColumns;
	allocConversions();
}

void InternalMetaDataResultSet::trimBlanks(int id)
{
	Value *value = getValue (id);

	if (value->type == String)
		{
		char *data = value->data.string.string;
		int l = value->data.string.length;
		while (l && data [l - 1] == ' ')
			data [--l] = 0;
		value->data.string.length = l;
		}
}

bool InternalMetaDataResultSet::isWildcarded(const char * pattern)
{
	for (const char *p = pattern; *p; ++p)
		if (*p == '%' || *p == '*')
			return true;

	return false;
}

JString InternalMetaDataResultSet::expandPattern(const char * string, const char * pattern)
{
	char temp [128];

	if (isWildcarded (pattern))
		sprintf (temp, string, "like", pattern);
	else
		sprintf (temp, string, "=", pattern);

	return temp;
}

int InternalMetaDataResultSet::getColumnType(int index)
{
	return resultSet->getColumnType (index);
}

int InternalMetaDataResultSet::getColumnDisplaySize(int index)
{
	return resultSet->getColumnDisplaySize (index);
}

const char* InternalMetaDataResultSet::getColumnName(int index)
{
	return resultSet->getColumnName (index);
}

const char* InternalMetaDataResultSet::getTableName(int index)
{
	return resultSet->getTableName (index);
}

int InternalMetaDataResultSet::getPrecision(int index)
{
	return resultSet->getPrecision (index);
}

int InternalMetaDataResultSet::getScale(int index)
{
	return resultSet->getScale (index);
}

bool InternalMetaDataResultSet::isNullable(int index)
{
	return resultSet->isNullable (index);
}

bool InternalMetaDataResultSet::next()
{
	return resultSet->next();
}


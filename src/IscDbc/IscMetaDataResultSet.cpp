// IscMetaDataResultSet.cpp: implementation of the IscMetaDataResultSet class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "IscDbc.h"
#include "IscMetaDataResultSet.h"
#include "IscDatabaseMetaData.h"
#include "IscResultSet.h"
#include "SQLError.h"
#include "IscConnection.h"
#include "Value.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscMetaDataResultSet::IscMetaDataResultSet(IscDatabaseMetaData *meta) : IscResultSet (NULL)
{
	metaData = meta;
	resultSet = NULL;
	statement = NULL;
}

IscMetaDataResultSet::~IscMetaDataResultSet()
{

}


int IscMetaDataResultSet::findColumn(const char * columnName)
{
	return resultSet->findColumn (columnName);
}

Value* IscMetaDataResultSet::getValue(int index)
{
	return resultSet->getValue (index);
}

void IscMetaDataResultSet::prepareStatement(const char * sql)
{
	statement = metaData->connection->prepareStatement (sql);
	resultSet = (IscResultSet*) statement->executeQuery();
	numberColumns = resultSet->numberColumns;
	allocConversions();
}

void IscMetaDataResultSet::trimBlanks(int id)
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

bool IscMetaDataResultSet::isWildcarded(const char * pattern)
{
	for (const char *p = pattern; *p; ++p)
		if (*p == '%' || *p == '*')
			return true;

	return false;
}

JString IscMetaDataResultSet::expandPattern(const char * string, const char * pattern)
{
	char temp [128];

	if (isWildcarded (pattern))
		sprintf (temp, string, "like", pattern);
	else
		sprintf (temp, string, "=", pattern);

	return temp;
}

int IscMetaDataResultSet::getColumnType(int index)
{
	return resultSet->getColumnType (index);
}

int IscMetaDataResultSet::getColumnDisplaySize(int index)
{
	return resultSet->getColumnDisplaySize (index);
}

const char* IscMetaDataResultSet::getColumnName(int index)
{
	return resultSet->getColumnName (index);
}

const char* IscMetaDataResultSet::getTableName(int index)
{
	return resultSet->getTableName (index);
}

int IscMetaDataResultSet::getPrecision(int index)
{
	return resultSet->getPrecision (index);
}

int IscMetaDataResultSet::getScale(int index)
{
	return resultSet->getScale (index);
}

bool IscMetaDataResultSet::isNullable(int index)
{
	return resultSet->isNullable (index);
}

bool IscMetaDataResultSet::next()
{
	return resultSet->next();
}


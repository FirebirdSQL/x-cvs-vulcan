// IscTablesResultSet.cpp: implementation of the IscTablesResultSet class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include "IscDbc.h"
#include "IscTablesResultSet.h"
#include "IscConnection.h"
#include "IscDatabaseMetaData.h"
#include "IscResultSet.h"
#include "IscPreparedStatement.h"
#include "IscBlob.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscTablesResultSet::IscTablesResultSet(IscDatabaseMetaData *metaData)
		: IscMetaDataResultSet(metaData)
{
	resultSet = NULL;
}

IscTablesResultSet::~IscTablesResultSet()
{
	if (resultSet)
		resultSet->release();
}

void IscTablesResultSet::getTables(const char * catalog, const char * schemaPattern, const char * tableNamePattern, int typeCount, const char * * types)
{
	JString sql = "select NULL as table_cat,"
				          "NULL as table_schem,"
						  "rdb$relation_name as table_name,"
						  "rdb$view_blr as table_type,"
						  "rdb$description as remarks,"
						  "rdb$system_flag "
						  "from rdb$relations\n";

	if (tableNamePattern)
		sql += expandPattern (" where rdb$relation_name %s '%s'\n", tableNamePattern);

	const char *sep = " and (";
	JString adjunct;
		
	for (int n = 0; n < typeCount; ++n)
		if (!strcmp (types [0], "TABLE"))
			{
			adjunct += sep;
			adjunct += "(rdb$view_blr is null and rdb$system_flag = 0)";
			sep = " or ";
			}
		else if (!strcmp (types [0], "VIEW"))
			{
			adjunct += sep;
			adjunct += "rdb$view_blr is not null";
			sep = " or ";
			}
		else if (!strcmp (types [0], "SYSTEM TABLE"))
			{
			adjunct += sep;
			adjunct += "(rdb$view_blr is null and rdb$system_flag = 1)";
			sep = " or ";
			}

	if (!adjunct.IsEmpty())
		{
		sql += adjunct;
		sql += ")\n";
		}

	sql += " order by rdb$owner_name, rdb$relation_name";
	prepareStatement (sql);
	numberColumns = 5;
}

bool IscTablesResultSet::next()
{
	if (!resultSet->next())
		return false;

	const char *type = "TABLE";

	if (resultSet->getInt (6))
		type = "SYSTEM TABLE";
	else
		{
		Blob *blob = resultSet->getBlob (5);
		if (!resultSet->wasNull())
			type = "VIEW";
		//blob->release();
		}

	resultSet->setValue (4, type);
	trimBlanks (3);

	return true;
}

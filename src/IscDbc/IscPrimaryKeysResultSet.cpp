// IscPrimaryKeysResultSet.cpp: implementation of the IscPrimaryKeysResultSet class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "IscDbc.h"
#include "IscPrimaryKeysResultSet.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscPrimaryKeysResultSet::IscPrimaryKeysResultSet(IscDatabaseMetaData *metaData)
		: IscMetaDataResultSet(metaData)
{

}

IscPrimaryKeysResultSet::~IscPrimaryKeysResultSet()
{

}

void IscPrimaryKeysResultSet::getPrimaryKeys(const char * catalog, const char * schemaPattern, const char * tableNamePattern)
{
	JString sql = 
		"select NULL as table_cat,\n"						// 1
				"\tNULL as table_schem,\n"					// 2
				"\trel.rdb$relation_name as table_name,\n"		// 3
				"\trdb$field_name as column_name,\n"		// 4
				"\trdb$field_position as key_seq,\n"		// 5
				"\tidx.rdb$index_name as pk_name\n"				// 6
		"from rdb$relation_constraints rel, rdb$indices idx, rdb$index_segments seg\n"
		" where rel.rdb$constraint_type = 'PRIMARY KEY'\n"
		" and rel.rdb$index_name = idx.rdb$index_name\n"
		" and idx.rdb$index_name = seg.rdb$index_name\n";

	if (tableNamePattern)
		sql += expandPattern (" and rel.rdb$relation_name %s '%s'", tableNamePattern);

	sql += " order by rel.rdb$relation_name, idx.rdb$index_name, rdb$field_position";
	prepareStatement (sql);
	numberColumns = 6;
}

bool IscPrimaryKeysResultSet::next()
{
	if (!resultSet->next())
		return false;

	trimBlanks (3);			// table name
	trimBlanks (4);			// field name
	trimBlanks (6);			// index name

	return true;
}

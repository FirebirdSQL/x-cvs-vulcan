// InternalPrimaryKeysResultSet.cpp: implementation of the InternalPrimaryKeysResultSet class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "firebird.h"
#include "InternalPrimaryKeysResultSet.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

InternalPrimaryKeysResultSet::InternalPrimaryKeysResultSet(InternalDatabaseMetaData *metaData)
		: InternalMetaDataResultSet(metaData)
{

}

InternalPrimaryKeysResultSet::~InternalPrimaryKeysResultSet()
{

}

void InternalPrimaryKeysResultSet::getPrimaryKeys(const char * catalog, const char * schemaPattern, const char * tableNamePattern)
{
	JString sql = 
		"select NULL as table_cat,\n"						// 1
				"\tNULL as table_schem,\n"					// 2
				"\trdb$relation_name as table_name,\n"		// 3
				"\trdb$field_name as column_name,\n"		// 4
				"\trdb$field_position as key_seq,\n"		// 5
				"\trdb$index_name as pk_name\n"				// 6
		"from rdb$relation_constraints rel, rdb$indices idx, rdb$index_segments seg\n"
		" where rel.rdb$constraint_type = 'PRIMARY KEY'\n"
		" and rel.rdb$index_name = idx.rdb$index_name\n"
		" and idx.rdb$index_name = seg.rdb$index_name\n";

	if (tableNamePattern)
		sql += expandPattern (" and rel.rdb$relation_name %s '%s'", tableNamePattern);

	sql += " order by rel.rdb$relation_name, rdb$index_name, rdb$field_position";
	prepareStatement (sql);
	numberColumns = 6;
}

bool InternalPrimaryKeysResultSet::next()
{
	if (!resultSet->next())
		return false;

	trimBlanks (3);			// table name
	trimBlanks (4);			// field name
	trimBlanks (6);			// index name

	return true;
}

// InternalIndexInfoResultSet.cpp: implementation of the InternalIndexInfoResultSet class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "fbdev.h"
#include "InternalIndexInfoResultSet.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

InternalIndexInfoResultSet::InternalIndexInfoResultSet(InternalDatabaseMetaData *metaData)
		: InternalMetaDataResultSet(metaData)
{

}

InternalIndexInfoResultSet::~InternalIndexInfoResultSet()
{

}

void InternalIndexInfoResultSet::getIndexInfo(const char * catalog, 
										 const char * schemaPattern, 
										 const char * tableNamePattern, 
										 bool unique, bool approximate)
{
	JString sql = 
		"select NULL as table_cat,\n"						// 1
				"\tNULL as table_schem,\n"					// 2
				"\trdb$relation_name as table_name,\n"		// 3
				"\t(1 - rdb$unique_flag) as non_unique,\n"	// 4
				"\tNULL as index_qualifier,\n"				// 5
				//"\trdb$index_name as index_name,\n"			// 6
				"\trdb$index_name,\n"			// 6
				"\t(3) as \"type\",\n"							// 7 (SQL_INDEX_OTHER)
				"\trdb$field_position as ordinal_position,\n" // 8
				"\trdb$field_name as column_name,\n"		// 9
				"\trdb$index_type as asc_or_desc,\n"		// 10
				"\trdb$statistics as cardinality,\n"		// 11
				"\tNULL as \"pages\",\n"					// 12
				"\tNULL as filter_condition\n"				// 13
		"from rdb$indices idx, rdb$index_segments seg\n"
		" where idx.rdb$index_name = seg.rdb$index_name\n";

	if (tableNamePattern)
		sql += expandPattern (" and rdb$relation_name %s '%s'\n", tableNamePattern);

	if (unique)
		sql += " and rdb$unique_flag == 1";

	sql += " order by rdb$relation_name, rdb$unique_flag, rdb$index_name, rdb$field_position";
	//printf ("%s\n", sql);
	prepareStatement (sql);
	numberColumns = 13;
}

bool InternalIndexInfoResultSet::next()
{
	if (!resultSet->next())
		return false;

	int type = resultSet->getInt (10);
	resultSet->setValue (10, (type) ? "D" : "A");
	trimBlanks (3);				// table name
	trimBlanks (6);				// index name
	trimBlanks (9);				// field name

	return true;
}

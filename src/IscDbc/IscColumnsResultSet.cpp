// IscColumnsResultSet.cpp: implementation of the IscColumnsResultSet class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "IscDbc.h"
#include "IscColumnsResultSet.h"
#include "IscConnection.h"
#include "IscDatabaseMetaData.h"
#include "IscResultSet.h"
#include "IscPreparedStatement.h"
#include "IscBlob.h"
#include "IscSqlType.h"

#define TYPE_NAME	6

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscColumnsResultSet::IscColumnsResultSet(IscDatabaseMetaData *metaData)
		: IscMetaDataResultSet(metaData)
{

}

IscColumnsResultSet::~IscColumnsResultSet()
{

}

void IscColumnsResultSet::getColumns(const char * catalog, const char * schemaPattern, const char * tableNamePattern, const char * fieldNamePattern)
{
	JString sql = 
		"select NULL as table_cat,\n"						// 1
				"\tNULL as table_schem,\n"				// 2
				"\trdb$relation_name as table_name,"	// 3
				"\trfr.rdb$field_name as column_name,\n"// 4
				"\trdb$field_type as data_type,\n"		// 5
				"\trdb$field_sub_type as type_name,\n"	// 6
				"\trdb$field_length as column_size,\n"	// 7
				"\tnull as buffer_length,\n"			// 8
				"\trdb$field_scale as decimal_digits,\n"// 9
				"\t10 as num_prec_radix,\n"				// 10
				"\trfr.rdb$null_flag as nullable,\n"		// 11
				"\trfr.rdb$description as remarks,\n"	// 12
				"\trfr.rdb$default_value as column_def,\n"	// 13
				"\tnull as SQL_DATA_TYPE,\n"			// 14
				"\tnull as SQL_DATETIME_SUB,\n"			// 15
				"\trdb$field_length as CHAR_OCTET_LENGTH,\n"// 16
				"\trdb$field_Id as ordinal_position,\n"	// 17
				"\t'YES' as IS_NULLABLE\n"				// 18
		"from rdb$relation_fields rfr, rdb$fields fld\n"
		" where rfr.rdb$field_source = fld.rdb$field_name\n";

	if (tableNamePattern)
		sql += expandPattern (" and rdb$relation_name %s '%s'", tableNamePattern);

	if (fieldNamePattern)
		sql += expandPattern (" and rfr.rdb$field_name %s '%s'", fieldNamePattern);

	sql += " order by rdb$relation_name, rfr.rdb$field_name";
	prepareStatement (sql);
	numberColumns = 18;
}

bool IscColumnsResultSet::next()
{
	if (!resultSet->next())
		return false;

	trimBlanks (3);							// table name
	trimBlanks (4);							// field name
	int blrType = resultSet->getInt (5);	// field type
	int subType = resultSet->getInt (6);
	int length = resultSet->getInt (7);
	IscSqlType sqlType (blrType, subType, length);

	resultSet->setValue (5, sqlType.type);
	resultSet->setValue (6, sqlType.typeName);

	return true;
}

int IscColumnsResultSet::getColumnType(int index)
{
	switch (index)
		{
		case TYPE_NAME:					//	TYPE_NAME
			return JDBC_VARCHAR;
		}

	return Parent::getColumnType (index);
}

int IscColumnsResultSet::getColumnDisplaySize(int index)
{
	switch (index)
		{
		case TYPE_NAME:					//	TYPE_NAME
			return 128;
		}

	return Parent::getColumnDisplaySize (index);
}

int IscColumnsResultSet::getPrecision(int index)
{
	switch (index)
		{
		case TYPE_NAME:					//	TYPE_NAME
			return 128;
		}

	return Parent::getPrecision (index);
}

// InternalProcedureColumnsResultSet.cpp: implementation of the InternalProcedureColumnsResultSet class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "firebird.h"
#include "InternalProcedureColumnsResultSet.h"
#include "InternalSqlType.h"

#ifndef SQL_PARAM_INPUT
#define SQL_PARAM_TYPE_UNKNOWN           0
#define SQL_PARAM_INPUT                  1
#define SQL_PARAM_INPUT_OUTPUT           2
#define SQL_RESULT_COL                   3
#define SQL_PARAM_OUTPUT                 4
#define SQL_RETURN_VALUE                 5
#endif

#define TYPE_NAME	7

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

InternalProcedureColumnsResultSet::InternalProcedureColumnsResultSet(InternalDatabaseMetaData *metaData)
		: InternalMetaDataResultSet(metaData)
{

}

InternalProcedureColumnsResultSet::~InternalProcedureColumnsResultSet()
{

}

void InternalProcedureColumnsResultSet::getProcedureColumns(const char * catalog, 
													   const char * schemaPattern, 
													   const char * procedureNamePattern, 
													   const char * columnNamePattern)
{
	JString sql = 
		"select NULL as table_cat,\n"								// 1
				"\tNULL as table_schem,\n"							// 2
				"\trdb$procedure_name as procedure_name,\n"			// 3
				"\trdb$parameter_name as column_name,\n"			// 4
				"\trdb$parameter_type as column_type,\n"			// 5
				"\trdb$field_type as data_type,\n"					// 5 + 1
				"\trdb$field_sub_type as type_name,\n"				// 6 + 1
				"\trdb$field_length as column_size,\n"				// 7 + 1
				"\tnull as buffer_length,\n"						// 8 + 1
				"\trdb$field_scale as decimal_digits,\n"			// 9 + 1
				"\t10 as num_prec_radix,\n"							// 10 + 1
				"\trdb$null_flag as nullable,\n"					// 11 + 1
				"\trdb$description as remarks,\n"					// 12 + 1
				"\trdb$default_value as column_def,\n"				// 13 + 1
				"\tnull as SQL_DATA_TYPE,\n"						// 14 + 1
				"\tnull as SQL_DATETIME_SUB,\n"						// 15 + 1
				"\trdb$field_length as CHAR_OCTET_LENGTH,\n"		// 16 + 1
				"\trdb$parameter_number as ordinal_position,\n"		// 17 + 1
				"\t'YES' as IS_NULLABLE\n"							// 18 + 1
		"from rdb$procedure_parameters, rdb$fields\n"
		"where rdb$field_source = rdb$field_name";

	if (procedureNamePattern)
		sql += expandPattern (" and rdb$procedure_name %s '%s'", procedureNamePattern);

	if (columnNamePattern)
		sql += expandPattern (" and rdb$parameter_name %s '%s'", columnNamePattern);

	sql += " order by rdb$procedure_name, rdb$parameter_number";
	prepareStatement (sql);
	numberColumns = 19;
}

bool InternalProcedureColumnsResultSet::next()
{
	if (!resultSet->next())
		return false;

	trimBlanks (3);							// procedure name
	trimBlanks (4);							// parameter name

	int parameterType = resultSet->getInt (5);
	int type = (parameterType) ? SQL_PARAM_INPUT : SQL_PARAM_OUTPUT;
	resultSet->setValue (5, type);

	int blrType = resultSet->getInt (6);	// field type
	int subType = resultSet->getInt (7);
	int length = resultSet->getInt (8);
	InternalSqlType sqlType (blrType, subType, length);

	resultSet->setValue (6, sqlType.type);
	resultSet->setValue (7, sqlType.typeName);

	return true;
}

int InternalProcedureColumnsResultSet::getColumnDisplaySize(int index)
{
	switch (index)
		{
		case TYPE_NAME:					//	TYPE_NAME
			return 128;
		}

	return Parent::getColumnDisplaySize (index);
}

int InternalProcedureColumnsResultSet::getColumnType(int index)
{
	switch (index)
		{
		case TYPE_NAME:					//	TYPE_NAME
			return JDBC_VARCHAR;
		}

	return Parent::getColumnType (index);
}

int InternalProcedureColumnsResultSet::getPrecision(int index)
{
	switch (index)
		{
		case TYPE_NAME:					//	TYPE_NAME
			return 128;
		}

	return Parent::getPrecision (index);
}

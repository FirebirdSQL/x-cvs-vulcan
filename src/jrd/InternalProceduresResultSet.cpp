// InternalProceduresResultSet.cpp: implementation of the InternalProceduresResultSet class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "firebird.h"
#include "InternalProceduresResultSet.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

InternalProceduresResultSet::InternalProceduresResultSet(InternalDatabaseMetaData *metaData)
		: InternalMetaDataResultSet(metaData)
{

}

InternalProceduresResultSet::~InternalProceduresResultSet()
{

}

void InternalProceduresResultSet::getProcedures(const char * catalog, const char * schemaPattern, const char * procedureNamePattern)
{
	JString sql = 
		"select NULL as table_cat,\n"								// 1
				"\tNULL as table_schem,\n"							// 2
				"\trdb$procedure_name as procedure_name,\n"			// 3
				"\trdb$procedure_inputs as num_input_params,\n"		// 4
				"\trdb$procedure_outputs as num_output_params,\n"	// 5
				"\t0 as num_result_sets,\n"							// 6
				"\trdb$description as remarks,\n"					// 7
				"\t0 as procedure_type\n"							// 8 SQL_PT_UNKNOWN
		"from rdb$procedures\n";

	if (procedureNamePattern)
		sql += expandPattern (" where rdb$procedure_name %s '%s'", procedureNamePattern);

	sql += " order by rdb$procedure_name";
	prepareStatement (sql);
	numberColumns = 8;
}

bool InternalProceduresResultSet::next()
{
	if (!resultSet->next())
		return false;

	trimBlanks (3);							// table name

	return true;
}

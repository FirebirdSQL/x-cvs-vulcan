// IscProceduresResultSet.cpp: implementation of the IscProceduresResultSet class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "IscDbc.h"
#include "IscProceduresResultSet.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscProceduresResultSet::IscProceduresResultSet(IscDatabaseMetaData *metaData)
		: IscMetaDataResultSet(metaData)
{

}

IscProceduresResultSet::~IscProceduresResultSet()
{

}

void IscProceduresResultSet::getProcedures(const char * catalog, const char * schemaPattern, const char * procedureNamePattern)
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

bool IscProceduresResultSet::next()
{
	if (!resultSet->next())
		return false;

	trimBlanks (3);							// table name

	return true;
}

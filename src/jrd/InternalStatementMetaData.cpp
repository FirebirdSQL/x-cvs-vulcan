#include "firebird.h"
#include "InternalStatementMetaData.h"
#include "InternalPreparedStatement.h"
//#include "Sqlda.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

InternalStatementMetaData::InternalStatementMetaData(InternalPreparedStatement *preparedStatement)
{
	statement = preparedStatement;
}

InternalStatementMetaData::~InternalStatementMetaData()
{

}

int InternalStatementMetaData::getParameterCount()
{
	//return statement->inputSqlda.getColumnCount();
	return 0;
}

int InternalStatementMetaData::getParameterType(int index)
{
	//return statement->inputSqlda.getColumnType (index);
	return 0;
}

int InternalStatementMetaData::getPrecision(int index)
{
	//return statement->inputSqlda.getPrecision (index);
	return 0;
}

int InternalStatementMetaData::getScale(int index)
{
	//return statement->inputSqlda.getScale (index);
	return 0;
}

bool InternalStatementMetaData::isNullable(int index)
{
	//return statement->inputSqlda.isNullable (index);
	return false;
}

int InternalStatementMetaData::objectVersion()
{
	return STATEMENTMETADATA_VERSION;
}

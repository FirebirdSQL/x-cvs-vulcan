// IscStatementMetaData.cpp: implementation of the IscStatementMetaData class.
//
//////////////////////////////////////////////////////////////////////

#include "IscDbc.h"
#include "IscStatementMetaData.h"
#include "IscPreparedStatement.h"
#include "Sqlda.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscStatementMetaData::IscStatementMetaData(IscPreparedStatement *preparedStatement)
{
	statement = preparedStatement;
}

IscStatementMetaData::~IscStatementMetaData()
{

}

int IscStatementMetaData::getParameterCount()
{
	return statement->inputSqlda.getColumnCount();
}

int IscStatementMetaData::getParameterType(int index)
{
	return statement->inputSqlda.getColumnType (index);
}

int IscStatementMetaData::getPrecision(int index)
{
	return statement->inputSqlda.getPrecision (index);
}

int IscStatementMetaData::getScale(int index)
{
	return statement->inputSqlda.getScale (index);
}

bool IscStatementMetaData::isNullable(int index)
{
	return statement->inputSqlda.isNullable (index);
}

int IscStatementMetaData::objectVersion()
{
	return STATEMENTMETADATA_VERSION;
}

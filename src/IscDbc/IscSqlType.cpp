// IscSqlType.cpp: implementation of the IscSqlType class.
//
//////////////////////////////////////////////////////////////////////

#include "IscDbc.h"
#include "IscSqlType.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IscSqlType::IscSqlType(int blrType, int subType, int length)
{
	getType (blrType, subType, length);
}

IscSqlType::~IscSqlType()
{

}

void IscSqlType::getType(int blrType, int subType, int len)
{
	length = len;

	switch (subType)
		{
		case 1:
			switch (blrType)
				{
				case blr_short:
				case blr_long:
				case blr_quad:
				case blr_int64:
				case blr_double:
				case blr_d_float:
					type = JDBC_NUMERIC;
					typeName = "NUMERIC";
					return;
					
				case blr_blob:
					type = JDBC_LONGVARCHAR;
					typeName = "LONG VARCHAR";
					return;
				}
			break;
	
		case 2:
			switch (blrType)
				{
				case blr_short:
				case blr_long:
				case blr_quad:
				case blr_int64:
				case blr_double:
				case blr_d_float:
					type = JDBC_DECIMAL;
					typeName = "DECIMAL";
					return;
					
				case blr_blob:
					type = JDBC_LONGVARBINARY;
					typeName = "LONG VARBINARY";
					return;
				}
			break;
		}
	
	switch (blrType)
		{
		case blr_text:
		case blr_text2:
			type = JDBC_CHAR;
			typeName = "CHAR";
			break;

		case blr_short:
			type = JDBC_SMALLINT;
			typeName = "SMALLINT";
			length = 6;
			break;

		case blr_long:
			type = JDBC_INTEGER;
			typeName = "INTEGER";
			length = 10;
			break;

		case blr_quad:
		case blr_int64:
			type = JDBC_BIGINT;
			typeName = "BIGINT";
			length = 19;
			break;

		case blr_float:
			type = JDBC_REAL;
			typeName = "REAL";
			break;

		case blr_double:
		case blr_d_float:
			type = JDBC_DOUBLE;
			typeName = "DOUBLE PRECISION";
			break;

		case blr_timestamp:
			type = JDBC_TIMESTAMP;
			typeName = "TIMESTAMP";
			break;

		case blr_varying:
		case blr_varying2:
			type = JDBC_VARCHAR;
			typeName = "VARCHAR";
			break;

		case blr_blob:
			if (subType == 1)
				{
				type = JDBC_LONGVARCHAR;
				typeName = "LONG VARCHAR";
				}
			else
				{
				type = JDBC_LONGVARBINARY;
				typeName = "LONG VARBINARY";
				}
			break;

		case blr_sql_date:
			type = JDBC_DATE;
			typeName = "DATE";
			break;

		case blr_sql_time:
			type = JDBC_TIME;
			typeName = "TIME";
			break;

		default:
			typeName = "UNKNOWN";
			type = 0;
		}
}

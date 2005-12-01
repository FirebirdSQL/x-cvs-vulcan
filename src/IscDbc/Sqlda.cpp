// Sqlda.cpp: implementation of the Sqlda class.
//
//////////////////////////////////////////////////////////////////////

#ifdef DARWIN
#included <stdlib>
#else
#include <malloc.h>
#endif
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include "IscDbc.h"
#include "Sqlda.h"
#include "SQLError.h"
#include "Value.h"
#include "IscConnection.h"

static short sqlNull = -1;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Sqlda::Sqlda()
{
	sqlda = (XSQLDA*) tempSqlda;
	sqlda->version = SQLDA_VERSION1;
	sqlda->sqln = DEFAULT_SQLDA_COUNT;
	sqlda->sqld = 0;
	buffer = NULL;
}

Sqlda::~Sqlda()
{
	deleteSqlda();

	if (buffer)
		delete buffer;
}

Sqlda::operator XSQLDA* ()
{
	return sqlda;
}

void Sqlda::deleteSqlda()
{
	if (sqlda != (XSQLDA*) tempSqlda)
		free (sqlda);
}

bool Sqlda::checkOverflow()
{
	if (sqlda->sqld <= sqlda->sqln)
		return false;

	int count = sqlda->sqln;
	deleteSqlda();
	sqlda = (XSQLDA*) malloc (XSQLDA_LENGTH (count));
	sqlda->version = SQLDA_VERSION1;
	sqlda->sqln = count;

	return true;
}

void Sqlda::allocBuffer()
{
	if (buffer)
		{
		delete [] buffer;
		buffer = NULL;
		}

	int offset = 0;
	int n = 0;
	int numberColumns = sqlda->sqld;
	XSQLVAR *var = sqlda->sqlvar;

	for (n = 0; n < numberColumns; ++n, ++var)
		{
		int length = var->sqllen;
		int boundary = length;
		switch (var->sqltype & ~1)
			{
			case SQL_TEXT:
				boundary = 1;
				++length;
				break;

			case SQL_VARYING:
				boundary = 2;
				length += 2;
				break;

			case SQL_SHORT:
				length = sizeof (short);
				break;

			case SQL_LONG:
				length = sizeof (long);
				break;

			case SQL_FLOAT:
				length = sizeof (float);
				break;

			case SQL_DOUBLE:
				length = sizeof (double);
				break;

			case SQL_QUAD:
			case SQL_INT64:
			case SQL_TIMESTAMP:
			case SQL_TYPE_TIME:
			case SQL_TYPE_DATE:
				length = sizeof (QUAD);
				break;

			case SQL_BLOB:
			case SQL_ARRAY:
				length = sizeof (ISC_QUAD);
				boundary = 4;
				break;

			default:
				NOT_YET_IMPLEMENTED;
				break;
			}
		if (length == 0)
			throw SQLEXCEPTION (COMPILE_ERROR, "Sqlda variable has zero length");
		offset = ROUNDUP (offset, boundary);
		var->sqldata = (char*) offset;
		var->sqllen = length;
		offset += length;
		}

	offset = ROUNDUP (offset, sizeof (short));
	int indicatorsOffset = offset;
	offset += sizeof (short) * numberColumns;
	buffer = new char [offset];
	short *indicators = (short*) (buffer + indicatorsOffset);
	var = sqlda->sqlvar;

	for (n = 0; n < numberColumns; ++n, ++var)
		{
		var->sqldata = buffer + (long) var->sqldata;
		var->sqlind = indicators + n;
		}
}

int Sqlda::getColumnCount()
{
	return sqlda->sqld;
}

void Sqlda::print()
{
	XSQLVAR *var = sqlda->sqlvar;

	for (int n = 0; n < sqlda->sqld; ++n, ++var)
		{
		char *p = var->sqldata;
		printf ("%d. type %d, len %d, addr %x (%x) ",
				n, var->sqltype, var->sqllen, p, var->sqlind);
		if ((var->sqltype & 1) && *var->sqlind)
			printf ("<null>");
		else
			switch (var->sqltype & ~1)
				{
				case SQL_TEXT:
					printf ("'%.*s'", var->sqllen, p);
					break;

				case SQL_VARYING:
					printf ("'%.*s'", *(short*) p, p + 2);
					break;

				case SQL_SHORT:
					printf ("%d", *(short*) p);
					break;

				case SQL_LONG:
					printf ("%d", *(long*) p);
					break;

				case SQL_FLOAT:
					printf ("%g", *(float*) p);
					break;

				case SQL_DOUBLE:
					printf ("%g", *(double*) p);
					break;

				case SQL_QUAD:
				case SQL_INT64:
					printf ("big");
					break;

				case SQL_BLOB:
					printf ("blob");
					break;

				case SQL_TIMESTAMP:
				case SQL_ARRAY:
				case SQL_TYPE_TIME:
				case SQL_TYPE_DATE:
					NOT_YET_IMPLEMENTED;
					break;
				}
		printf ("\n");
		}
}

int Sqlda::getColumnType(int index)
{
	XSQLVAR *var = sqlda->sqlvar + index - 1;

	return getSqlType (var->sqltype, var->sqlsubtype);
}

int Sqlda::getDisplaySize(int index)
{
	XSQLVAR *var = sqlda->sqlvar + index - 1;

	switch (var->sqltype & ~1)
		{
		case SQL_SHORT:
			return 6;

		case SQL_LONG:
			return 11;

		case SQL_FLOAT:
			return 15;

		case SQL_DOUBLE:
			return 22;

		case SQL_QUAD:
		case SQL_INT64:
			return 20;

		case SQL_BLOB:
		case SQL_ARRAY:
			return 100000000;

		case SQL_TYPE_TIME:
			return 8;

		case SQL_TYPE_DATE:
			return 10;

		case SQL_TIMESTAMP:
			return 23;
		}

	return var->sqllen;
}

const char* Sqlda::getColumnName(int index)
{
	XSQLVAR *var = sqlda->sqlvar + index - 1;

	if (var->aliasname [0])
		return var->aliasname;

	return var->sqlname;
}

int Sqlda::getPrecision(int index)
{
	XSQLVAR *var = sqlda->sqlvar + index - 1;

	switch (var->sqltype & ~1)
		{
		case SQL_SHORT:
			return 5;

		case SQL_LONG:
			return 10;

		case SQL_FLOAT:
			return 15;

		case SQL_DOUBLE:
			return 15;

		case SQL_QUAD:
		case SQL_INT64:
			return 19;

		case SQL_BLOB:
		case SQL_ARRAY:
			return 100000000;

		case SQL_TYPE_TIME:
			return 8;

		case SQL_TYPE_DATE:
			return 10;

		case SQL_TIMESTAMP:
			return 23;
		
		case SQL_VARYING:	
			return var->sqllen - 2;
		}

	return var->sqllen;
}

int Sqlda::getScale(int index)
{
	XSQLVAR *var = sqlda->sqlvar + index - 1;

	return var->sqlscale;
}

bool Sqlda::isNullable(int index)
{
	XSQLVAR *var = sqlda->sqlvar + index - 1;

	return (var->sqltype & 1) ? true : false;
}


int Sqlda::getSqlType(int iscType, int subType)
{
	switch (subType)
		{
		case 1:
			switch (iscType & ~1)
				{
				case SQL_SHORT:
				case SQL_LONG:
				case SQL_FLOAT:
				case SQL_DOUBLE:
				case SQL_QUAD:
				case SQL_INT64:
					return JDBC_NUMERIC;
				}
			break;
		
		case 2:
			switch (iscType & ~1)
				{
				case SQL_SHORT:
				case SQL_LONG:
				case SQL_FLOAT:
				case SQL_DOUBLE:
				case SQL_QUAD:
				case SQL_INT64:
					return JDBC_DECIMAL;
				}
			break;
		}
		
	switch (iscType & ~1)
		{
		case SQL_TEXT:
			return JDBC_CHAR;

		case SQL_VARYING:
			return JDBC_VARCHAR;

		case SQL_SHORT:
			return JDBC_SMALLINT;

		case SQL_LONG:
			return JDBC_INTEGER;

		case SQL_FLOAT:
			return JDBC_REAL;

		case SQL_DOUBLE:
			return JDBC_DOUBLE;

		case SQL_QUAD:
		case SQL_INT64:
			return JDBC_BIGINT;

		case SQL_BLOB:
			if (subType == 1)
				return JDBC_LONGVARCHAR;
			return JDBC_LONGVARBINARY;

		case SQL_TIMESTAMP:
			return JDBC_TIMESTAMP;

		case SQL_TYPE_TIME:
			return JDBC_TIME;
			
		case SQL_TYPE_DATE:
			return JDBC_DATE;
		
		case SQL_ARRAY:
			return JDBC_ARRAY;
			
		default:
			NOT_YET_IMPLEMENTED;
		}

	return 0;
}

const char* Sqlda::getSqlTypeName(int iscType, int subType)
{
	switch (iscType & ~1)
		{
		case SQL_TEXT:
			return "CHAR";

		case SQL_VARYING:
			return "VARCHAR";

		case SQL_SHORT:
			return "SMALLINT";

		case SQL_LONG:
			return "INTEGER";

		case SQL_FLOAT:
			return "REAL";

		case SQL_DOUBLE:
			return "DOUBLE PRECISION";

		case SQL_QUAD:
		case SQL_INT64:
			return "BIGINT";

		case SQL_BLOB:
			if (subType == 1)
				return "LONG VARCHAR";
			return "LONG VARBINARY";

		case SQL_TIMESTAMP:
			return "TIMESTAMP";

		case SQL_ARRAY:
		case SQL_TYPE_TIME:
		case SQL_TYPE_DATE:
			NOT_YET_IMPLEMENTED;
		}

	return "*unknown type*";
}

const char* Sqlda::getTableName(int index)
{
	XSQLVAR *var = sqlda->sqlvar + index - 1;

	return var->relname;
}

void Sqlda::setValue(int slot, Value * value, IscConnection *connection)
{
	XSQLVAR *var = sqlda->sqlvar + slot;

	// Check to see if we need to do the conversion.  Otherwise, the
	// InterBase do it.

	switch (var->sqltype & ~1)
		{
		case SQL_BLOB:	
			setBlob (var, value, connection);
			return;
		}

	var->sqlscale = 0;
	var->sqldata = (char*) &value->data;
	var->sqlind = NULL;

	switch (value->type)
		{
		case Null:
			var->sqltype = SQL_LONG;
			var->sqlind = &sqlNull;
			break;

		case String:
			var->sqltype = SQL_TEXT;
			var->sqldata = value->data.string.string;
			var->sqllen = value->data.string.length;
			break;

		case Short:
			var->sqltype = SQL_SHORT;
			var->sqllen = sizeof (short);
			break;

		case Long:
			var->sqltype = SQL_LONG;
			var->sqllen = sizeof (long);
			break;

		case Quad:
			var->sqltype = SQL_INT64;
			var->sqllen = sizeof (QUAD);
			break;

		case Float:
			var->sqltype = SQL_FLOAT;
			var->sqllen = sizeof (float);
			break;

		case Double:
			var->sqltype = SQL_DOUBLE;
			var->sqllen = sizeof (double);
			break;

		case BlobPtr:
		case ClobPtr:
		case Date:
									
		default:
			NOT_YET_IMPLEMENTED;
		}			

}


void Sqlda::setBlob(XSQLVAR * var, Value * value, IscConnection *connection)
{
	if (value->type == Null)
		{
		var->sqltype |= 1;
		*var->sqlind = -1;
		memset (var->sqldata, 0, var->sqllen);
		return;
		}

	var->sqltype &= ~1;
	ISC_STATUS statusVector [20];
	isc_blob_handle blobHandle = 0;
	isc_tr_handle transactionHandle = connection->startTransaction();
	isc_create_blob2 (statusVector, 
					  &connection->databaseHandle,
					  &transactionHandle,
					  &blobHandle,
					  (ISC_QUAD*) var->sqldata,
					  0, NULL);

	if (statusVector [1])
		THROW_ISC_EXCEPTION (statusVector);

	char *address = (char*) &value->data;
	int length = 0;

	switch (value->type)
		{
		case String:
			address = value->data.string.string;
			length = value->data.string.length;
			break;

		case Short:
			length = sizeof (short);
			break;

		case Long:
			length = sizeof (long);
			break;

		case Quad:
			length = sizeof (QUAD);
			break;

		case Float:
			length = sizeof (float);
			break;

		case Double:
			length = sizeof (double);
			break;

		case BlobPtr:
			{
			length = 0;
			Blob *blob = value->data.blob;
			for (int len, offset = 0; len = blob->getSegmentLength (offset); offset += len)
				{
				isc_put_segment (statusVector, &blobHandle, len, (char*) blob->getSegment (offset));
				if (statusVector [1])
					THROW_ISC_EXCEPTION (statusVector);
				}
			}
			break;

		case ClobPtr:
			{
			length = 0;
			Clob *blob = value->data.clob;
			for (int len, offset = 0; len = blob->getSegmentLength (offset); offset += len)
				{
				isc_put_segment (statusVector, &blobHandle, len, (char*) blob->getSegment (offset));
				if (statusVector [1])
					THROW_ISC_EXCEPTION (statusVector);
				}
			}
			break;

		case Date:
									
		default:
			NOT_YET_IMPLEMENTED;
		}			

	if (length)
		{
		isc_put_segment (statusVector, &blobHandle, length, address);
		if (statusVector [1])
			THROW_ISC_EXCEPTION (statusVector);
		}

	isc_close_blob (statusVector, &blobHandle);
	if (statusVector [1])
		THROW_ISC_EXCEPTION (statusVector);
}

int Sqlda::findColumn(const char * columnName)
{
	for (int n = 0; n < sqlda->sqld; ++n)
		if (strcasecmp (sqlda->sqlvar [n].sqlname, columnName) == 0)
			return n;

	return -1;
}

// InternalCallableStatement.cpp: implementation of the InternalCallableStatement class.
//
//////////////////////////////////////////////////////////////////////

#include <string.h>
#ifdef MVS
#include <strings.h> // for strcasecmp
#endif
#include "fbdev.h"
#include "common.h"
#include "InternalCallableStatement.h"
#include "InternalConnection.h"
#include "DateTime.h"
#include "TimeStamp.h"
#include "Value.h"
#include "SQLError.h"

#define SKIP_WHITE(p)	while (charTable [*p] == WHITE) ++p

#define PUNCT			1
#define WHITE			2
#define DIGIT			4
#define LETTER			8
#define QUOTE			16
#define IDENT			32

#ifdef _WIN32
#define strcasecmp		stricmp
#define strncasecmp		strnicmp
#endif

static char charTable [256];
static int init();
static int foo = init();

int init ()
{
	int n;
	const char *p;

	for (p = " \t\n"; *p; ++p)
		charTable [*p] = WHITE;

	for (p = "?=(),{}"; *p; ++p)
		charTable [*p] = PUNCT;

	for (n = 'a'; n <= 'z'; ++n)
		charTable [n] = LETTER | IDENT;

	for (n = 'A'; n <= 'A'; ++n)
		charTable [n] = LETTER | IDENT;

	for (n = '0'; n <= '9'; ++n)
		charTable [n] = DIGIT | IDENT;

	charTable ['\''] = QUOTE;
	charTable ['"'] = QUOTE;
	charTable ['_'] = IDENT;

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

InternalCallableStatement::InternalCallableStatement(InternalConnection *connection)
		: InternalPreparedStatement (connection)
{
	minOutputVariable = 0;
}

InternalCallableStatement::~InternalCallableStatement()
{

}

ResultSet* InternalCallableStatement::executeQuery()
{
	return Parent::executeQuery();
}

void InternalCallableStatement::setInt(int index, int value)
{
	Parent::setInt(index, value);
}

void InternalCallableStatement::setNull(int index, int type)
{
	Parent::setNull(index, type);
}

void InternalCallableStatement::setDate(int index, DateTime value)
{
	Parent::setDate(index, value);
}

void InternalCallableStatement::setDouble(int index, double value)
{
	Parent::setDouble(index, value);
}

void InternalCallableStatement::setString(int index, const char * string)
{
	Parent::setString(index, string);
}

bool InternalCallableStatement::execute()
{
	/***
	connection->startTransaction();
	ISC_STATUS statusVector [20];
	outputSqlda.allocBuffer();
	values.alloc (numberColumns);
	int numberParameters = inputSqlda.getColumnCount();
	int n;

	for (n = 0; n < numberParameters; ++n)
		inputSqlda.setValue (n, parameters.values + n, connection);

	if (isc_dsql_execute2 (statusVector, &connection->transactionHandle, &statementHandle,
						  3, inputSqlda, outputSqlda))
		THROW_ISC_EXCEPTION (statusVector);

	resultsCount = 1;
	resultsSequence = 0;
	getUpdateCounts();

	XSQLVAR *var = outputSqlda.sqlda->sqlvar;
    Value *value = values.values;

	for (n = 0; n < numberColumns; ++n, ++var, ++value)
		setValue (value, var);

	return outputSqlda.sqlda->sqld > 0;
	***/
	NOT_YET_IMPLEMENTED;
}

int InternalCallableStatement::executeUpdate()
{
	return Parent::executeUpdate();
}

void InternalCallableStatement::setBytes(int index, int length, const void* bytes)
{
	Parent::setBytes(index, length, bytes);
}


bool InternalCallableStatement::execute (const char *sqlString)
{
	return Parent::execute(sqlString);
}

ResultSet*	 InternalCallableStatement::executeQuery (const char *sqlString)
{
	return Parent::executeQuery(sqlString);
}

int	InternalCallableStatement::getUpdateCount()
{
	return Parent::getUpdateCount();
}

bool InternalCallableStatement::getMoreResults()
{
	return Parent::getMoreResults();
}

void InternalCallableStatement::setCursorName (const char *name)
{
	Parent::setCursorName(name);
}

ResultSet* InternalCallableStatement::getResultSet()
{
	return Parent::getResultSet();
}

ResultList* InternalCallableStatement::search (const char *searchString)
{
	return Parent::search(searchString);
}

int	InternalCallableStatement::executeUpdate (const char *sqlString)
{
	return Parent::executeUpdate(sqlString);
}

void InternalCallableStatement::close()
{
	Parent::close();
}

int InternalCallableStatement::release()
{
	return Parent::release();
}

void InternalCallableStatement::addRef()
{
	Parent::addRef();
}

StatementMetaData* InternalCallableStatement::getStatementMetaData()
{
	return Parent::getStatementMetaData();
}

void InternalCallableStatement::setByte(int index, char value)
{
	Parent::setByte(index, value);
}

void InternalCallableStatement::setLong(int index, INT64 value)
{
	Parent::setLong(index, value);
}

void InternalCallableStatement::setFloat(int index, float value)
{
	Parent::setFloat(index, value);
}

void InternalCallableStatement::setTime(int index, SqlTime value)
{
	Parent::setTime(index, value);
}

void InternalCallableStatement::setTimestamp(int index, TimeStamp value)
{
	Parent::setTimestamp(index, value);
}

void InternalCallableStatement::setShort(int index, short value)
{
	Parent::setShort(index, value);
}

void InternalCallableStatement::setBlob(int index, Blob * value)
{
	Parent::setBlob(index, value);
}

void InternalCallableStatement::setClob(int index, Clob * value)
{
	Parent::setClob(index, value);
}

int InternalCallableStatement::objectVersion()
{
	return CALLABLESTATEMENT_VERSION;
}

short InternalCallableStatement::getShort(int id)
{
	return getValue (id)->getShort();
}

char InternalCallableStatement::getByte(int id)
{
	return getValue (id)->getByte();
}

int InternalCallableStatement::getInt(int id)
{
	return getValue (id)->getLong();
}

INT64 InternalCallableStatement::getLong(int id)
{
	return getValue (id)->getQuad();
}

float InternalCallableStatement::getFloat(int id)
{
	return (float) getValue (id)->getDouble();
}

double InternalCallableStatement::getDouble(int id)
{
	return getValue (id)->getDouble();
}

Clob* InternalCallableStatement::getClob(int id)
{
	return getValue (id)->getClob();
}

Blob* InternalCallableStatement::getBlob(int id)
{
	return getValue (id)->getBlob();
}

SqlTime InternalCallableStatement::getTime(int id)
{
	return getValue (id)->getTime();
}

DateTime InternalCallableStatement::getDate(int id)
{
	return getValue (id)->getDate();
}

TimeStamp InternalCallableStatement::getTimestamp(int id)
{
	return getValue (id)->getTimestamp();
}

bool InternalCallableStatement::wasNull()
{
	return valueWasNull;
}

const char* InternalCallableStatement::getString(int id)
{
	return getValue (id)->getString();
}

Value* InternalCallableStatement::getValue(int index)
{
	if (index < minOutputVariable || index >= minOutputVariable + numberColumns)
		throw SQLEXCEPTION (RUNTIME_ERROR, "invalid column index for procedure call");

	Value *value = values.values + index - minOutputVariable;
	valueWasNull = value->type == Null;

	return value;
}

void InternalCallableStatement::registerOutParameter(int parameterIndex, int sqlType)
{
	minOutputVariable = (minOutputVariable == 0) ? parameterIndex : MIN (minOutputVariable, parameterIndex);
}

void InternalCallableStatement::registerOutParameter(int parameterIndex, int sqlType, int scale)
{
	minOutputVariable = (minOutputVariable == 0) ? parameterIndex : MIN (minOutputVariable, parameterIndex);
}

void InternalCallableStatement::prepare(const char * originalSql)
{
	char	buffer [1024];
	const char *sql = rewriteSql (originalSql, buffer, sizeof (buffer));

	Parent::prepare (sql);
}

const char* InternalCallableStatement::rewriteSql(const char *originalSql, char *buffer, int length)
{
	const char *p = originalSql;
	char token [256];
	getToken (&p, token);

	if (token [0] != '{')
		return originalSql;

	getToken (&p, token);

	if (strcasecmp (token, "call") != 0)
		throw SQLEXCEPTION (SYNTAX_ERROR, "unsupported form of procedure call");

	char *q = buffer;
	strcpy (q, "execute procedure ");
	while (*q) ++q;

	while (*p)
		{
		getToken (&p, q);
		if (*q == '}')
			break;
		while (*q) ++q;
		}

	*q = 0;

	return buffer;
}

void InternalCallableStatement::getToken(const char **ptr, char *token)
{
	const char *p = *ptr;
	SKIP_WHITE (p);
	char *q = token;

	if (*p)
		{
		char c = charTable [*p];
		*q++ = *p++;
		if (c & IDENT)
			while (charTable [*p] & IDENT)
				*q++ = *p++;
		else if (c & QUOTE)
			{
			char quote = p [-1];
			while (*p && (*p != quote || q [-1] == '\\'))
				*q++ = *p++;
			if (*p)
				*q++ = *p++;
			}
		}

	*q = 0;
	*ptr = p;
}

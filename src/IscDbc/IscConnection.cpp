// IscConnection.cpp: implementation of the IscConnection class.
//
//////////////////////////////////////////////////////////////////////

#include <string.h>
#include "IscDbc.h"
#include "IscConnection.h"
#include "SQLError.h"
#include "IscCallableStatement.h"
#include "IscDatabaseMetaData.h"
#include "Parameters.h"
#include "Attachment.h"

static char databaseInfoItems [] = { 
	isc_info_db_SQL_dialect,
	isc_info_version, 
	isc_info_end 
	};


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern "C"
{
Connection* createConnection()
{
	return new IscConnection;
}
}

IscConnection::IscConnection()
{
	init();
}


IscConnection::IscConnection(IscConnection * source)
{
	init();
	attachment = source->attachment;
	++attachment;
}

void IscConnection::init()
{
	useCount = 1;
	metaData = NULL;
	transactionHandle = 0;
	transactionIsolation = 0;
	autoCommit = true;
	attachment = NULL;
}

IscConnection::~IscConnection()
{
	if (metaData)
		delete metaData;

	if (attachment)
		attachment->release();
}

bool IscConnection::isConnected()
{
	return attachment != NULL;
}

void IscConnection::close()
{
	FOR_OBJECTS (IscStatement*, statement, &statements)
		statement->close();
	END_FOR;

	delete this;
}

PreparedStatement* IscConnection::prepareStatement(const char * sqlString)
{
	IscPreparedStatement *statement = NULL;

	try
		{
		statement = new IscPreparedStatement (this);
		statement->prepare (sqlString);
		}
	catch (...)
		{
		if (statement)
			delete statement;
		throw;
		}

	return statement;
}

void IscConnection::commit()
{
	if (transactionHandle)
		{
		ISC_STATUS statusVector [20];
		isc_commit_transaction (statusVector, &transactionHandle);

		if (statusVector [1])
			throw SQLEXCEPTION (statusVector [1], getIscStatusText (statusVector));
		}
}

void IscConnection::rollback()
{
	if (transactionHandle)
		{
		ISC_STATUS statusVector [20];
		isc_rollback_transaction (statusVector, &transactionHandle);

		if (statusVector [1])
			throw SQLEXCEPTION (statusVector [1], getIscStatusText (statusVector));
		}
}

void IscConnection::prepareTransaction()
{
}

isc_tr_handle IscConnection::startTransaction()
{
	if (transactionHandle)
		return transactionHandle;

	ISC_STATUS statusVector [20];
	isc_start_transaction (statusVector, &transactionHandle, 1, &attachment->databaseHandle, 0, NULL);

	if (statusVector [1])
		throw SQLEXCEPTION (statusVector [1], getIscStatusText (statusVector));

	return transactionHandle;
}

Statement* IscConnection::createStatement()
{
	IscStatement *statement = new IscStatement (this);
	statements.append (statement);

	return statement;
}


Clob* IscConnection::genHTML(Properties * parameters, long genHeaders)
{
	NOT_YET_IMPLEMENTED;

	return NULL;
}

/***
void IscConnection::freeHTML(const char * html)
{
	delete [] (char*) html;
}
***/

DatabaseMetaData* IscConnection::getMetaData()
{
	if (metaData)
		return metaData;

	metaData = new IscDatabaseMetaData (this);

	return metaData;
}

int IscConnection::hasRole(const char * schemaName, const char * roleName)
{
	NOT_YET_IMPLEMENTED;

	return false;
}

void IscConnection::ping()
{
}

void IscConnection::createDatabase(const char * host, const char * dbName, Properties * properties)
{
}

void IscConnection::openDatabase(const char * dbName, Properties * properties)
{
	try
		{
		attachment = new Attachment;
		attachment->openDatabase (dbName, properties);
		databaseHandle = attachment->databaseHandle;
		}
	catch (...)
		{
		delete attachment;
		attachment = NULL;
		throw;
		}

	/***
	databaseName = dbName;
	char dpb [256], *p = dpb;
	*p++ = isc_dpb_version1;

	const char *user = properties->findValue ("user", NULL);

	if (user)
		{
		userName = user;
		*p++ = isc_dpb_user_name,
		*p++ = strlen (user);
		for (const char *q = user; *q;)
			*p++ = *q++;
		}

	const char *password = properties->findValue ("password", NULL);

	if (password)
		{
		*p++ = isc_dpb_password,
		*p++ = strlen (password);
		for (const char *q = password; *q;)
			*p++ = *q++;
		}

	int dpbLength = p - dpb;
	ISC_STATUS statusVector [20];

	if (isc_attach_database (statusVector, strlen (dbName), (char*) dbName, &databaseHandle, 
							 dpbLength, dpb))
		{
		JString text = getIscStatusText (statusVector);
		throw SQLEXCEPTION (statusVector [1], text);
		}

	char result [100];
	dialect = 0;

	if (!isc_database_info (statusVector, &databaseHandle, sizeof (databaseInfoItems), databaseInfoItems, sizeof (result), result))
		{
		for (char *p = result; p < result + sizeof (result) && *p != isc_info_end;)
			{
			char item = *p++;
			int length = isc_vax_integer (p, 2);
			p += 2;
			switch (item)
				{
				case isc_info_db_SQL_dialect:
					dialect = isc_vax_integer (p, length);
					break;

				case isc_info_version:
					databaseVersion = JString (p + 2, p [1]);
					break;
				}
			p += length;
			}
		}

	switch (dialect)
		{
		case 0:
			quotedIdentifiers = false;
			break;

		case SQL_DIALECT_V5:
		case SQL_DIALECT_V6:
		default:
			quotedIdentifiers = true;
		}
	***/
}


void IscConnection::deleteStatement(IscStatement * statement)
{
	statements.deleteItem (statement);
}


JString IscConnection::getIscStatusText(ISC_STATUS * statusVector)
{
	char text [4096], *p = text;
	ISC_STATUS *status = statusVector;
	bool first = true;

	while (isc_interprete (p, &status))
		{
		while (*p)
			++p;
		*p++ = '\n';
		}

	if (p > text)
		--p;

	*p = 0;

	return text;
}


int IscConnection::getInfoItem(char * buffer, int infoItem, int defaultValue)
{
	for (char *p = buffer; *p != isc_info_end;)
		{
		char item = *p++;
		int length = isc_vax_integer (p, 2);
		p += 2;
		if (item == infoItem)
			return isc_vax_integer (p, length);
		p += length;
		}

	return defaultValue;			
}

JString IscConnection::getInfoString(char * buffer, int infoItem, const char * defaultString)
{
	for (char *p = buffer; *p != isc_info_end;)
		{
		char item = *p++;
		int length = isc_vax_integer (p, 2);
		p += 2;
		if (item == infoItem)
			return JString (p, length);
		p += length;
		}

	return defaultString;			
}

Properties* IscConnection::allocProperties()
{
	return new Parameters;
}

int IscConnection::objectVersion()
{
	return CONNECTION_VERSION;
}

Connection* IscConnection::clone()
{
	return new IscConnection (this);
}

void IscConnection::setAutoCommit(bool setting)
{
	autoCommit = setting;
}

bool IscConnection::getAutoCommit()
{
	return autoCommit;
}

void IscConnection::setTransactionIsolation(int level)
{
	transactionIsolation = level;
}

int IscConnection::getTransactionIsolation()
{
	return transactionIsolation;
}

void IscConnection::addRef()
{
	++useCount;
}

int IscConnection::release()
{
	if (--useCount == 0)
		{
		close();
		return 0;
		}

	return useCount;
}

CallableStatement* IscConnection::prepareCall(const char * sqlString)
{
	IscCallableStatement *statement = NULL;

	try
		{
		statement = new IscCallableStatement (this);
		statement->prepare (sqlString);
		}
	catch (...)
		{
		if (statement)
			delete statement;
		throw;
		}

	return statement;
}

void IscConnection::commitAuto()
{
	commit();
}

void IscConnection::rollbackAuto()
{
	rollback();
}

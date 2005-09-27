/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
 *
 *     Software distributed under the License is distributed on 
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *     express or implied.  See the License for the specific 
 *     language governing rights and limitations under the License.
 *
 *     The contents of this file or any work derived from this file
 *     may not be distributed under any other license whatsoever 
 *     without the express prior written permission of the original 
 *     author.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1997 - 2000, 2001, 2003 James A. Starkey
 *  Copyright (c) 1997 - 2000, 2001, 2003 Netfrastructure, Inc.
 *  All Rights Reserved.
 */

#include <time.h>
#include <string.h>
#include "fbdev.h"
#include "common.h"
#include "InternalConnection.h"
#include "SQLError.h"
#include "InternalCallableStatement.h"
#include "InternalDatabaseMetaData.h"
#include "InternalPreparedStatement.h"
#include "Parameters.h"
#include "Attachment.h"

InternalConnection::InternalConnection(Attachment *attach, Transaction *transact)
{
	init(attach, transact);
}


InternalConnection::InternalConnection(InternalConnection * source)
{
	init(source->attachment, source->transaction);
}

void InternalConnection::init(Attachment *attach, Transaction *transact)
{
	useCount = 1;
	metaData = NULL;
	transactionIsolation = 0;
	autoCommit = true;
	attachment = attach;
	transaction = transact;
}

InternalConnection::~InternalConnection()
{
	if (attachment)
		attachment->closeConnection(this);
		
	if (metaData)
		delete metaData;

}

bool InternalConnection::isConnected()
{
	return attachment != NULL;
}

void InternalConnection::close()
{
	FOR_OBJECTS (InternalStatement*, statement, &statements)
		statement->close();
	END_FOR;

	delete this;
}

PreparedStatement* InternalConnection::prepareStatement(const char * sqlString)
{
	InternalPreparedStatement *statement = NULL;

	try
		{
		statement = new InternalPreparedStatement (this);
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

void InternalConnection::commit()
{
	/***
	if (transactionHandle)
		{
		ISC_STATUS statusVector [20];
		isc_commit_transaction (statusVector, &transactionHandle);

		if (statusVector [1])
			throw SQLEXCEPTION (statusVector [1], getInternalStatusText (statusVector));
		}
	***/
}

void InternalConnection::rollback()
{
	/***
	if (transactionHandle)
		{
		ISC_STATUS statusVector [20];
		isc_rollback_transaction (statusVector, &transactionHandle);

		if (statusVector [1])
			throw SQLEXCEPTION (statusVector [1], getInternalStatusText (statusVector));
		}
	***/
}

void InternalConnection::prepareTransaction()
{
}

void* InternalConnection::startTransaction()
{
	/***
	if (transactionHandle)
		return transactionHandle;

	ISC_STATUS statusVector [20];
	isc_start_transaction (statusVector, &transactionHandle, 1, &attachment->databaseHandle, 0, NULL);

	if (statusVector [1])
		throw SQLEXCEPTION (statusVector [1], getInternalStatusText (statusVector));

	return transactionHandle;
	***/
	return NULL;
}

Statement* InternalConnection::createStatement()
{
	InternalStatement *statement = new InternalStatement (this);
	statements.append (statement);

	return statement;
}


/***
void InternalConnection::freeHTML(const char * html)
{
	delete [] (char*) html;
}
***/

DatabaseMetaData* InternalConnection::getMetaData()
{
	if (metaData)
		return metaData;

	metaData = new InternalDatabaseMetaData (this);

	return metaData;
}

int InternalConnection::hasRole(const char * schemaName, const char * roleName)
{
	//NOT_YET_IMPLEMENTED;

	return false;
}

void InternalConnection::ping()
{
}

void InternalConnection::createDatabase(const char * host, const char * dbName, Properties * properties)
{
}

void InternalConnection::openDatabase(const char * dbName, Properties * properties)
{

}


void InternalConnection::deleteStatement(InternalStatement * statement)
{

}


Properties* InternalConnection::allocProperties()
{
	//return new Parameters;
	return NULL;
}

int InternalConnection::objectVersion()
{
	return CONNECTION_VERSION;
}

Connection* InternalConnection::clone()
{
	//return new InternalConnection (this);
	return NULL;
}

void InternalConnection::setAutoCommit(bool setting)
{
	autoCommit = setting;
}

bool InternalConnection::getAutoCommit()
{
	return autoCommit;
}

void InternalConnection::setTransactionIsolation(int level)
{
	transactionIsolation = level;
}

int InternalConnection::getTransactionIsolation()
{
	return transactionIsolation;
}

void InternalConnection::addRef()
{
	++useCount;
}

int InternalConnection::release()
{
	if (--useCount == 0)
		{
		close();
		return 0;
		}

	return useCount;
}

CallableStatement* InternalConnection::prepareCall(const char * sqlString)
{
	InternalCallableStatement *statement = NULL;

	try
		{
		statement = new InternalCallableStatement (this);
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

void InternalConnection::commitAuto()
{
	commit();
}

void InternalConnection::rollbackAuto()
{
	rollback();
}

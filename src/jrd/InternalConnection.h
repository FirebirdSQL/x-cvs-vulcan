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

#ifndef _INTERNALCONNECTION_H_
#define _INTERNALCONNECTION_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Connection.h"
#include "LinkedList.h"
#include "JString.h"	// Added by ClassView

#define SQLEXCEPTION		SQLError
#define NOT_YET_IMPLEMENTED	throw SQLError (FEATURE_NOT_YET_IMPLEMENTED, "not yet implemented")

class InternalStatement;
class InternalDatabaseMetaData;
class Attachment;
class Transaction;


class InternalConnection : public Connection
{
public:
	InternalConnection(Attachment *attach, Transaction *transact);
	InternalConnection (InternalConnection *source);

	void rollbackAuto();
	void commitAuto();
	virtual CallableStatement* prepareCall (const char *sql);
	virtual int release();
	virtual void addRef();
	virtual int getTransactionIsolation();
	virtual void setTransactionIsolation (int level);
	virtual bool getAutoCommit();
	virtual void setAutoCommit (bool setting);
	void init(Attachment *attach, Transaction *transact);
	virtual Connection* clone();
	virtual int objectVersion();
	virtual Properties* allocProperties();
	JString getInfoString (char *buffer, int item, const char *defaultString);
	int getInfoItem (char *buffer, int item, int defaultValue);
	static JString getInternalStatusText (ISC_STATUS *statusVector);
	void* startTransaction();
	void deleteStatement (InternalStatement *statement);
	virtual ~InternalConnection();
	virtual void openDatabase (const char *dbName, Properties *properties);
	virtual void createDatabase (const char *host, const char * dbName, Properties *properties);
	virtual void ping();
	virtual int hasRole (const char *schemaName, const char *roleName);
	virtual bool isConnected();
	virtual Statement* createStatement();
	virtual void prepareTransaction();
	virtual void rollback();
	virtual void commit();
	virtual PreparedStatement* prepareStatement (const char *sqlString);
	virtual void close();
	virtual DatabaseMetaData* getMetaData();

	Attachment	*attachment;
	Transaction	*transaction;
	void		*databaseHandle;
	LinkedList	statements;
	InternalDatabaseMetaData	*metaData;
	int			transactionIsolation;
	bool		autoCommit;
	int			useCount;
	InternalConnection	*prior;
	InternalConnection	*next;
	InternalConnection	*nextInTransaction;
	void transactionCompleted(Transaction* transaction);
};

#endif

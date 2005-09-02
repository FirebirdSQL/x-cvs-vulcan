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

// YTransaction.cpp: implementation of the YTransaction class.
//
//////////////////////////////////////////////////////////////////////

#include <string.h>
#include "firebird.h"
#include "common.h"
#include "iberror.h"
#include "YTransaction.h"
#include "SubsysHandle.h"
#include "StatusVector.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

YTransaction::YTransaction(int dbCount)
{
	numberActive = 0;
	numberDatabases = dbCount;
	inLimbo = false;
	
	if (numberDatabases > LOCAL_VECTOR_SIZE)
		databases = new TranDb [numberDatabases];
	else
		databases = localDatabases;

	memset(databases, 0, dbCount * sizeof(*databases));

	/***
	for (int n = 0; n < numberDatabases; ++n)
		{
		TranDb *db = databases + n;
		if (!(db->subsystem = (SubsysHandle*) tebs->dbHandle))
			statusVector.postCode (isc_bad_db_handle);
		db->element = tebs [n];
		if (db->element.tpbLength && !db->element.tpb)
			statusVector.postCode (isc_bad_tpb_form);
		db->subsystem = (SubsysHandle*) *db->element.dbHandle;
		db->element.dbHandle = &db->subsystem->handle;
		db->handle = NULL;
		}
	***/
}

YTransaction::~YTransaction()
{
	if (databases != localDatabases)
		delete [] databases;
}

ISC_STATUS YTransaction::start(StatusVector& statusVector)
{
	for (int n = 0; n < numberDatabases; ++n)
		{
		TranDb *database = databases + n;
		if (database->subsystem->subsystem->startMultiple (statusVector, &database->handle, 1, &database->element))
			{
			StatusVector dummy (NULL);
			rollback (dummy);
			break;
			}
		else
			database->subsystem->transactionStarted (this);
		}

	return statusVector.getCode();		
}

ISC_STATUS YTransaction::rollback(StatusVector &statusVector)
{
	for (int n = 0; n < numberDatabases; ++n)
		{
		TranDb *database = databases + n;
		if (database->handle &&
			!database->subsystem->subsystem->rollbackTransaction (statusVector, &database->handle))
			database->subsystem->transactionEnded (this);
		}

	return statusVector.getCode();		
}

DbHandle YTransaction::getDbHandle(SubsysHandle *subsystem)
{
	for (int n = 0; n < numberDatabases; ++n)
		if (databases [n].subsystem == subsystem)
			return databases [n].handle;

	return NULL;
}

ISC_STATUS YTransaction::commit(StatusVector &statusVector)
{
	if (numberDatabases > 1 && !inLimbo)
		if (prepare(statusVector, 0, NULL))
			return statusVector.getCode();

	for (int n = 0; n < numberDatabases; ++n)
		{
		TranDb *database = databases + n;
		if (database->handle)
			if (database->subsystem->subsystem->commitTransaction (statusVector, &database->handle))
				break;
			else
				database->subsystem->transactionEnded (this);
		}

	return statusVector.getCode();		
}

ISC_STATUS YTransaction::commitRetaining(StatusVector& statusVector)
{
	int n;

	for (n = 0; n < numberDatabases; ++n)
		{
		TranDb *database = databases + n;
		if (database->handle)
			if (database->subsystem->subsystem->commitRetaining (statusVector, &database->handle))
				break;
			else
				database->subsystem->transactionEnded (this);
		}

	return statusVector.getCode();		
}

ISC_STATUS YTransaction::rollbackRetaining(StatusVector& statusVector)
{
	for (int n = 0; n < numberDatabases; ++n)
		{
		TranDb *database = databases + n;
		if (database->handle &&
			!database->subsystem->subsystem->rollbackRetaining (statusVector, &database->handle))
			database->subsystem->transactionEnded (this);
		}

	return statusVector.getCode();		
}

void YTransaction::setDatabase(int index, SubsysHandle* handle, int tpbLength, UCHAR* tpb)
{
	TranDb *db = databases + index;
	db->element.tpbLength = tpbLength;
	db->element.tpb = tpb;
	db->subsystem = handle;
	db->element.dbHandle = &db->subsystem->handle;
	db->handle = NULL;
}

void YTransaction::setTransactionHandle(SubsysHandle* subsystem, TraHandle transactionHandle)
{
	TranDb *db = databases;
	db->subsystem = subsystem;
	db->handle = transactionHandle;
}

ISC_STATUS YTransaction::prepare(StatusVector& statusVector, int msgLength, const UCHAR* msg)
{
	if (inLimbo)
		return statusVector.getCode();
		
	for (int n = 0; n < numberDatabases; ++n)
		{
		TranDb *database = databases + n;
		if (database->handle &&
			database->subsystem->subsystem->prepareTransaction (statusVector, &database->handle, msgLength, msg))
			return statusVector.getCode();
		}
	
	inLimbo = true;
	
	return statusVector.getCode();		
}

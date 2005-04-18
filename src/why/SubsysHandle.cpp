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

// SubsysHandle.cpp: implementation of the SubsysHandle class.
//
//////////////////////////////////////////////////////////////////////

#include "firebird.h"
#include "common.h"
#include "../jrd/dsc.h"
#include "SubsysHandle.h"
#include "YRequest.h"
#include "YStatement.h"
#include "YBlob.h"
#include "StatusVector.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

SubsysHandle::SubsysHandle(Subsystem *sys, DbHandle orgHandle)
{
	subsystem = sys;
	handle = orgHandle;
	firstRequest = NULL;
	lastRequest = NULL;
	firstStatement = NULL;
	lastStatement = NULL;
	firstBlob = NULL;
	lastBlob = NULL;
	activeTransactions = 0;
}

SubsysHandle::~SubsysHandle()
{
	releaseRequests();
}

void SubsysHandle::addRequest(YRequest *request)
{
	if (firstRequest)
		{
		request->prior = lastRequest;
		lastRequest->next = request;
		}
	else
		{
		request->prior = NULL;
		firstRequest = request;
		}

	request->next = NULL;
	lastRequest = request;
}

void SubsysHandle::removeRequest(YRequest *request)
{
	if (request->prior)
		request->prior->next = request->next;
	else
		firstRequest = request->next;

	if (request->next)
		request->next->prior = request->prior;
	else
		lastRequest = request->prior;
}

void SubsysHandle::releaseRequests()
{
	StatusVector statusVector (NULL);

	while (firstRequest)
		firstRequest->releaseRequest(statusVector);
}

void SubsysHandle::transactionStarted(YTransaction *transaction)
{
	++activeTransactions;
}

void SubsysHandle::transactionEnded(YTransaction *transaction)
{
	--activeTransactions;
}

void SubsysHandle::addStatement(YStatement *statement)
{
	if (firstStatement)
		{
		statement->prior = lastStatement;
		lastStatement->next = statement;
		}
	else
		{
		statement->prior = NULL;
		firstStatement = statement;
		}

	statement->next = NULL;
	lastStatement = statement;
}

void SubsysHandle::removeStatement(YStatement *statement)
{
	if (statement->prior)
		statement->prior->next = statement->next;
	else
		firstStatement = statement->next;

	if (statement->next)
		statement->next->prior = statement->prior;
	else
		lastStatement = statement->prior;
}

void SubsysHandle::releaseStatements()
{
	StatusVector statusVector (NULL);

	while (firstStatement)
		firstStatement->releaseStatement(statusVector, DSQL_drop, true);
}

void SubsysHandle::addBlob(YBlob* blob)
{
	if (firstBlob)
		{
		blob->prior = lastBlob;
		lastBlob->next = blob;
		}
	else
		{
		blob->prior = NULL;
		firstBlob = blob;
		}

	blob->next = NULL;
	lastBlob = blob;
}

void SubsysHandle::removeBlob(YBlob* blob)
{
	if (blob->prior)
		blob->prior->next = blob->next;
	else
		firstBlob = blob->next;

	if (blob->next)
		blob->next->prior = blob->prior;
	else
		lastBlob = blob->prior;
}

void SubsysHandle::cancelBlobs(void)
{
	StatusVector statusVector (NULL);

	while (firstBlob)
		firstBlob->cancelBlob(statusVector);
}

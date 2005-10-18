/*
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
 *  Copyright (c) 2004 James A. Starkey
 *  All Rights Reserved.
 */
 
#include "fbdev.h"
#include "common.h"
#include "InternalSecurityContext.h"
#include "jrd.h"
#include "tra_proto.h"
#include "tdbb.h"
//#include "Connection.h"
#include "InternalConnection.h"
#include "Attachment.h"
#include ".\internalsecuritycontext.h"

InternalSecurityContext::InternalSecurityContext(thread_db* tdbb)
{
	threadData = tdbb;
	transaction = NULL;
	connection = NULL;
}

InternalSecurityContext::~InternalSecurityContext(void)
{
	if (transaction)
		commit();

	if (connection)
		connection->release();
}

Connection* InternalSecurityContext::getUserConnection(void)
{
	if (connection)
		return connection;
	
	if (!transaction)
		transaction = TRA_start(threadData, threadData->tdbb_attachment, 0, NULL);
	
	connection = threadData->tdbb_attachment->getUserConnection(transaction);
	connection->addRef();
	
	return connection;
}

void InternalSecurityContext::commit(void)
{
	TRA_commit(threadData, transaction, false);
}

void InternalSecurityContext::rollback(void)
{
	TRA_rollback(threadData, transaction, false);
}

const char* InternalSecurityContext::getDatabaseFilename(void)
{
	return threadData->tdbb_database->dbb_filename;
}

Connection* InternalSecurityContext::getNewConnection(void)
{
	return threadData->tdbb_database->getNewConnection(threadData);
}

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

// ThreadData.cpp: implementation of the ThreadData class.
//
//////////////////////////////////////////////////////////////////////

#include <memory.h>
#include "firebird.h"
#include "jrd.h"
#include "jrd_proto.h"
#include "inuse_proto.h"
#include "ThreadData.h"
#include "Database.h"
#include "Attachment.h"
#include "thd_proto.h"
#include "gds_proto.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


ThreadData::ThreadData(ISC_STATUS *statusVector)
{
	init (statusVector, NULL);
}

ThreadData::ThreadData(ISC_STATUS *statusVector, Attachment *attachment)
{
	init (statusVector, attachment->att_database);
	setAttachment (attachment);
}

ThreadData::ThreadData(ISC_STATUS *statusVector, Database *database)
{
	init(statusVector, database);
}

ThreadData::~ThreadData()
{
	BOOLEAN cleaned_up = false;

	//cleaned_up = INUSE_cleanup(&threadData->tdbb_mutexes, (FPTR_VOID) THD_mutex_unlock);
	
	/* Charlie will fill this in
	cleaned_up |= INUSE_cleanup (&tdbb->tdbb_pages, (FPTR_VOID) CCH_?);
	*/
	
	THD_restore_specific(THDD_TYPE_TDBB);

#ifdef DEV_BUILD
	if (threadData->tdbb_status_vector && !threadData->tdbb_status_vector[1] && cleaned_up)
		gds__log("mutexes or pages in use on successful return");
#endif
}

void ThreadData::init(ISC_STATUS *statusVector, Database *database)
{
	threadData = &thd_context;
	memset (threadData, 0, sizeof (tdbb));
	threadData->tdbb_status_vector = statusVector;
	setDatabase (database);
	JRD_set_context(threadData);
}


void ThreadData::setDatabase(Database *database)
{
	threadData->tdbb_database = database;
}

ThreadData::operator tdbb* ()
{
	return threadData;
}

void ThreadData::setAttachment(Attachment *attachment)
{
	threadData->tdbb_attachment = attachment;

	if (attachment)
		setDatabase (attachment->att_database);
}


void ThreadData::setStatusVector(ISC_STATUS *statusVector)
{
	threadData->tdbb_status_vector = statusVector;
}

void ThreadData::setPool(JrdMemoryPool *pool)
{
	threadData->tdbb_default = pool;
}

Database* ThreadData::getDatabase()
{
	return threadData->tdbb_database;
}

Attachment* ThreadData::getAttachment()
{
	return threadData->tdbb_attachment;
}

ISC_STATUS* ThreadData::getStatusVector()
{
	return threadData->tdbb_status_vector;
}

JrdMemoryPool* ThreadData::getPool()
{
	return threadData->tdbb_default;
}

void ThreadData::setDsqlPool(DsqlMemoryPool *pool)
{
	threadData->tsql_default = pool;
}

ISC_STATUS ThreadData::getStatus()
{
	return threadData->tdbb_status_vector [1];
}

void tdbb::registerBdb(Bdb* bdb)
{
	for (int n = 0; n < MAX_THREAD_BDBS; ++n)
		if (!tdbb_bdbs [n])
			{
			tdbb_bdbs [n] = bdb;
			return;
			}
	
	fb_assert (false);
}

void tdbb::clearBdb(Bdb* bdb)
{
	for (int n = 0; n < MAX_THREAD_BDBS; ++n)
		if (tdbb_bdbs [n] == bdb)
			{
			tdbb_bdbs [n] = NULL;
			return;
			}
	
	fb_assert (false);
}

void ThreadData::setTransaction(Transaction* transaction)
{
	threadData->tdbb_transaction = transaction;
}

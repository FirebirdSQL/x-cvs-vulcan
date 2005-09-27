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
 *  Copyright (c) 2004 James A. Starkey
 *  All Rights Reserved.
 */

#include "fbdev.h"
#include "common.h"
#include "CommitManager.h"
#include "Sync.h"
#include "Database.h"
#include "Parameters.h"
#include "ConfObject.h"
#include "jrd.h"
#include "tra.h"
#include "tra_proto.h"
#include "ods.h"
#include "PageCache.h"


CommitManager::CommitManager(Database *dbb)
{
	database = dbb;
	interval = database->configuration->getValue (CommitInterval, CommitIntervalValue);
	pageCache = database->pageCache;
	transactions = NULL;
}

CommitManager::~CommitManager(void)
{
}

void CommitManager::headerWritten(SLONG nextTransaction)
{
}

void CommitManager::commit(thread_db* tdbb, Transaction* transaction)
{
	if (transaction->tra_flags & TRA_write)
		pageCache->flush(tdbb, FLUSH_TRAN, transaction->tra_number);
	else if (transaction->tra_flags & (TRA_prepare2 | TRA_reconnected)) 
		/* If the transaction only read data but is a member of a
		   multi-database transaction with a transaction description
		   message then flush RDB$TRANSACTIONS. */

		pageCache->flush(tdbb, FLUSH_SYSTEM, 0);

	TRA_set_state(tdbb, transaction, transaction->tra_number, tra_committed);
}

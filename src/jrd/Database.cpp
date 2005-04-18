/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		Database.cpp
 *	DESCRIPTION:	Database class implementation
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 *
 * December 27, 2003	Created by James A. Starkey
 *
 */
// Database.cpp: implementation of the Database class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "firebird.h"
#include "jrd.h"
#include "Database.h"
#include "all.h"
#include "ods.h"
#include "dialect.h"
#include "thd_proto.h"
#include "err_proto.h"
#include "met_proto.h"
#include "ConfObject.h"
#include "ConfObj.h"
#include "Parameters.h"
#include "cch.h"
#include "CharSetManager.h"
#include "ModuleManager.h"
#include "PathName.h"
#include "PageCache.h"
#include "Sync.h"
#include "Relation.h"
#include "../jrd/lck.h"
#include "../jrd/lck_proto.h"
#include "ProcManager.h"
#include "Procedure.h"
#include "Request.h"
#include "irq.h"
#include "drq.h"
#include "DirectoryList.h"
#include "InternalConnection.h"
#include "SecurityRoot.h"
#include "SecurityDb.h"
#include "InternalSecurityContext.h"
#include "Interlock.h"
#include "TipCache.h"
#include "CommitManager.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//Database::Database (MemoryPool& p, ConfObject *configObject) : dbb_pools(1, p, type_dbb)
Database::Database (const char *expandedFilename, ConfObject *configObject)
	{
	init();
	configObject->addRef();
	configuration = configObject;
	dbb_filename = expandedFilename;
	maxUnflushedWrites = configuration->getValue (MaxUnflushedWrites,MaxUnflushedWritesValue);
	maxUnflushedWriteTime = configuration->getValue (MaxUnflushedWriteTime,MaxUnflushedWriteTimeValue);
	priorityBoost = configuration->getValue (PriorityBoost, PriorityBoostValue);
	cacheDefault = configuration->getValue (DefaultDbCachePages, DefaultDbCachePagesValue);
	cacheDefault = MAX (cacheDefault, MIN_PAGE_BUFFERS);
	cacheDefault = MIN (cacheDefault, MAX_PAGE_BUFFERS);
	temporaryBlobIds = 1;
	dbb_ast_flags = 0;
	pageCache = new PageCache (this);
	charSetManager = new CharSetManager (this);
	tipCache = new TipCache(this);
	sweeperCount = 0;
#ifdef SHARED_CACHE
	syncReady.lock (NULL, Exclusive);
#endif
	int cur_perm = 0, max_perm = 0;
	dbb_permanent = JrdMemoryPool::createPool(this, &cur_perm, &max_perm);
	dbb_internal.resize(irq_MAX);
	dbb_dyn_req.resize(drq_MAX);
	
#ifndef MEMMGR
	dbb_permanent->moveStats(&dbb_current_memory, &dbb_max_memory);
#endif

	procManager = new ProcManager(this);
	commitManager = new CommitManager(this);
#ifdef SHARED_CACHE
	fileShared = configuration->getValue (DatabaseFileShared,DatabaseFileSharedValue);
#else
	fileShared = true; 
#endif
	
	securityPlugin = new SecurityRoot(this);
	const char *policyName = configuration->getValue(SecurityManager, SecurityManagerValue);
	
	if (policyName)
		{
		ConfObject *config = configuration->findObject("SecurityManager", policyName);
		if (config)
			{
			config->setChain(configuration->getChain());
			configuration->setChain (config);
			}
		}
	
	securityPlugin = new SecurityDb(securityPlugin);
	intlModuleManager = new ModuleManager;
	intlModuleManager->addSearchPath(configuration->expand("$(ROOT)/intl"));
	intlModuleManager->addSearchPath(configuration->expand("$(ROOT)/intl64"));
	intlModuleManager->addSearchPath(configuration->expand("$(ROOT)/udf"));
	intlModuleManager->addSearchPath(configuration->expand("$(ROOT)/udf64"));
	intlModuleManager->addSearchPath(configuration->expand("$(ROOT)/bin"));
	intlModuleManager->addSearchPath(configuration->expand("$(ROOT)/bin64"));
	
	JString directories = configuration->getConcatenatedValues (UdfAccess);
	
	if (!directories.IsEmpty())
		{
		DirectoryList directoryList(directories);
		
		for (int n = 0; n < directoryList.numberDirectories; ++n)
			intlModuleManager->addSearchPath(directoryList.directories[n]);
		}
	}


Database::~Database()
{
	delete charSetManager;
	delete pageCache;
	delete procManager;
	delete commitManager;
	delete intlModuleManager;
	delete securityPlugin;
	delete tipCache;
	
	FOR_OBJECTS (JrdMemoryPool*, pool, &dbb_pools)
		if (pool && pool != dbb_permanent)
			JrdMemoryPool::deletePool(pool);
	END_FOR;
	
	if (dbb_permanent)
		JrdMemoryPool::deletePool (dbb_permanent);
}


int Database::getSqlDialect()
{
	if (ENCODE_ODS (dbb_ods_version, dbb_minor_original) >= ODS_10_0)
		if (dbb_flags & DBB_DB_SQL_dialect_3)
			return SQL_DIALECT_V6;	// DB created in IB V6.0 by client SQL dialect 3
		
	return SQL_DIALECT_V5; // old DB was gbaked in IB V6.0
}

void Database::init()
{
	initialized = false;
	dbb_attachments = NULL;	/* Active attachments */
	dbb_lock = NULL;		/* granddaddy lock */
	dbb_sys_trans = NULL;	/* system transaction */
	dbb_file = NULL;		/* files for I/O operations */
	dbb_shadow = NULL;		/* shadow control block */
	dbb_shadow_lock = NULL;	/* lock for synchronizing addition of shadows */
	dbb_shadow_sync_count = 0;	/* to synchronize changes to shadows */
	dbb_retaining_lock = NULL;	/* lock for preserving commit retaining snapshot */
	dbb_connection = NULL;	/* connection block */
	dbb_pcontrol = NULL;	/* page control */
	dbb_blob_filters = NULL;	/* known blob filters */
	dbb_modules = NULL;	/* external function/filter modules */
	//dbb_mutexes = NULL;		/* DBB block mutexes */
	//dbb_rw_locks = NULL;		/* DBB block read/write locks */
	dbb_sort_size = 0;		/* Size of sort space per sort */

	maxUnflushedWrites = 0;
	maxUnflushedWriteTime = 0;
	priorityBoost = 0;
	cacheDefault = 0;
	temporaryBlobIds = 0;
	charSetManager = NULL;
	fileShared = false;
			
	dbb_ast_flags = 0;		/* flags modified at AST level */
	dbb_flags = 0;
	dbb_ods_version = 0;		/* major ODS version number */
	dbb_minor_version = 0;	/* minor ODS version number */
	dbb_minor_original = 0;	/* minor ODS version at creation */
	dbb_page_size = 0;		/* page size */
	dbb_dp_per_pp = 0;		/* data pages per pointer page */
	dbb_max_records = 0;		/* max record per data page */
	dbb_max_idx = 0;			/* max number of indexes on a root page */
	dbb_use_count = 0;		/* active count of threads */
	dbb_shutdown_delay = 0;	/* seconds until forced shutdown */
	dbb_refresh_ranges = 0;	/* active count of refresh ranges */
	dbb_prefetch_sequence = 0;	/* sequence to pace frequency of prefetch requests */
	dbb_prefetch_pages = 0;	/* prefetch pages per request */
	dbb_spare_string = 0;	/* random buffer */
	dbb_encrypt_key = NULL;	/* encryption key */
	dbb_decrypt = NULL;
	dbb_encrypt = NULL;
	
	JrdMemoryPool* dbb_permanent = NULL;
	JrdMemoryPool* dbb_bufferpool = NULL;

    dbb_next_pool_id = 0;

	dbb_oldest_active = 0;			/* Cached "oldest active" transaction */
	dbb_oldest_transaction = 0;		/* Cached "oldest interesting" transaction */
	dbb_oldest_snapshot = 0;		/* Cached "oldest snapshot" of all active transactions */
	dbb_next_transaction = 0;		/* Next transaction id used by NETWARE */
	dbb_attachment_id = 0;			/* Next attachment id for ReadOnly DB's */
	dbb_page_incarnation = 0;		/* Cache page incarnation counter */
	dbb_page_buffers = 0;			/* Page buffers from header page */

	dbb_update_attachment = NULL;	/* Attachment with update in process */
	dbb_update_que = NULL;			/* Attachments waiting for update */
	dbb_free_btbs = NULL;			/* Unused btb blocks */

	dbb_current_memory = 0;
	dbb_max_memory = 0;
	dbb_reads = 0;
	dbb_writes = 0;
	dbb_fetches = 0;
	dbb_marks = 0;
	dbb_last_header_write = 0;			/* Transaction id of last header page physical write */
	dbb_flush_cycle = 0;				/* Current flush cycle */
	dbb_sweep_interval = 0;				/* Transactions between sweep */
	dbb_lock_owner_handle = 0;			/* Handle for the lock manager */

	unflushed_writes = 0;				/* unflushed writes */
	last_flushed_write = 0;				/* last flushed write time */

	//crypt_routine dbb_encrypt;		/* External encryption routine */
	//crypt_routine dbb_decrypt;		/* External decryption routine */

	dbb_blob_map = NULL;				/* mapping of blobs for REPLAY */
	dbb_log = NULL;						/* log file for REPLAY */
	
	pageCache = NULL;
	//dbb_tip_cache = NULL;				/* cache of latest known state of all transactions in system */
	//dbb_pc_transactions = NULL;		/* active precommitted transactions */
	backup_manager = NULL;				/* physical backup manager */
	defaultCharSet = NULL;
	systemConnection = NULL;
	
	memset (dbb_hash_table, 0, sizeof (dbb_hash_table));
}

ISC_STATUS Database::executeDDL(ISC_STATUS *statusVector, Transaction *transaction, int length, const UCHAR *ddl)
{
	return statusVector [1];
}

Relation* Database::findRelation(thread_db* tdbb, const char *relationName)
{
	return MET_lookup_relation (tdbb, relationName);
}

bool Database::isFilename(const char* filename)
{
	return PathName::pathsEquivalent (dbb_filename, filename);
}

CharSetContainer* Database::findCharset(thread_db* tdbb, int ttype)
{
	return charSetManager->findCharset (tdbb, ttype);
}

CharSetContainer* Database::findCharset(thread_db* tdbb, const char* name)
{
	return charSetManager->findCharset (tdbb, name);
}

CharSetContainer* Database::findCollation(thread_db* tdbb, const char* name)
{
	return charSetManager->findCollation (tdbb, name);
}

void Database::addPool(JrdMemoryPool* pool)
{
#ifdef SHARED_CACHE
	Sync sync (&syncObject, "Database::addPool");
	sync.lock (Exclusive);
#endif
	dbb_pools.append (pool);
}

void Database::removePool(JrdMemoryPool* pool)
{
#ifdef SHARED_CACHE
	Sync sync (&syncObject, "Database::removePool");
	sync.lock (Exclusive);
#endif
	dbb_pools.deleteItem (pool);
}

Attachment* Database::createAttachment(void)
{
#ifdef SHARED_CACHE
	Sync sync (&syncAttachments, "Database::createAttachment");
	sync.lock (Exclusive);
#endif
	Attachment *attachment = FB_NEW(*dbb_permanent) Attachment (this);

	attachment->att_next = dbb_attachments;
	dbb_attachments = attachment;
	
	return attachment;
}

void Database::deleteAttachment(Attachment* attachment)
{
#ifdef SHARED_CACHE
	Sync sync (&syncAttachments, "Database::deleteAttachment");
	sync.lock (Exclusive);
#endif
	
	for (Attachment **ptr = &dbb_attachments; *ptr; ptr = &(*ptr)->att_next)
		if (*ptr == attachment)
			{
			*ptr = attachment->att_next;
			return;
			}
}

void Database::makeReady(void)
{
#ifdef SHARED_CACHE
	syncReady.unlock(NULL, Exclusive);
#endif
}

bool Database::isReady(bool waitFlag)
{
#ifdef SHARED_CACHE
	if (!waitFlag)
		return syncReady.isLocked();
		
	Sync sync (&syncReady, "Database::isReady");
	sync.lock(Shared);
#endif
	
	return true;
}


Relation* Database::lookupRelation(thread_db* tdbb, const char* relationName)
{
	int length = strlen(relationName);
#ifdef SHARED_CACHE
	Sync sync (&syncRelations, "Database::lookupRelation");
	sync.lock(Shared);
#endif

	//for (vec::iterator ptr = dbb_relations->begin(), end = dbb_relations->end(); ptr < end; ptr++)
	for (int n = 0; n < dbb_relations.size(); ++n)
		{
		Relation *relation = dbb_relations[n];
		if (relation && relation->rel_length == length &&
			!(relation->rel_flags & REL_deleted) && !relation->rel_name.IsEmpty())
			{
			/* dimitr: for non-system relations we should also check
					   REL_scanned and REL_being_scanned flags. Look
					   at MET_lookup_procedure for example. */
					   
			if (!(relation->rel_flags & REL_system) &&
				(!(relation->rel_flags & REL_scanned) || (relation->rel_flags & REL_being_scanned))) 
				continue;
			
			if (strcmp (relationName, relation->rel_name) == 0)
				return relation;
			}
		}
	
	return NULL;
}

Relation* Database::getRelation(thread_db* tdbb, int id)
{
	if (id < 0)
		return new Relation(this, id);
		//return FB_NEW(*dbb_permanent) Relation (this, id);

#ifdef SHARED_CACHE
	Sync sync (&syncRelations, "Database::getRelation");
	sync.lock(Exclusive);
#endif

	dbb_relations.checkSize(id, id + 10);
	Relation *relation = dbb_relations[id];
	
	if (relation)
		return relation;
	
	if (relation = dbb_relations[id])
		return relation;
	
	/* From ODS 9 onwards, the first 128 relation IDS have been 
	   reserved for system relations */

	USHORT max_sys_rel;
	
	if (ENCODE_ODS(dbb_ods_version, dbb_minor_original) < ODS_9_0)
		max_sys_rel = (USHORT) USER_REL_INIT_ID_ODS8 - 1;
	else
		max_sys_rel = (USHORT) USER_DEF_REL_INIT_ID - 1;

	relation = FB_NEW(*dbb_permanent) Relation (this, id);
	dbb_relations[id] = relation;

	if (relation->rel_id <= max_sys_rel)
		return relation;

	relation->createExistenceLock(tdbb);
	relation->rel_flags |= (REL_check_existence | REL_check_partners);
	
	return relation;
}

void Database::validate(void)
{
}

Procedure* Database::findProcedure(thread_db* tdbb, int id)
{
	return procManager->findProcedure (tdbb, id);
}

Procedure* Database::findProcedure(thread_db* tdbb, const TEXT* name, bool noscan)
{
	return procManager->findProcedure (tdbb, name, noscan);
}

InternalConnection* Database::getSystemConnection(void)
{
	if (!systemConnection)
		systemConnection = new InternalConnection(NULL, dbb_sys_trans);
	
	return systemConnection;
}

void Database::updateAccountInfo(thread_db* tdbb, int apbLength, const UCHAR* apb)
{
	InternalSecurityContext securityContext (tdbb);
	securityPlugin->updateAccountInfo(&securityContext, apbLength, apb);
}

void Database::authenticateUser(thread_db* tdbb, int dpbLength, const UCHAR* dpb, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	InternalSecurityContext securityContext (tdbb);
	securityPlugin->authenticateUser(&securityContext, dpbLength, dpb, itemsLength, items, bufferLength, buffer);
}

void Database::incrementUseCount(void)
{
	INTERLOCKED_INCREMENT(dbb_use_count);
}

void Database::decrementUseCount(void)
{
	INTERLOCKED_DECREMENT(dbb_use_count);
}

Relation* Database::findRelation(int relationId)
{
	if (relationId < 0 || relationId >= dbb_relations.size())
		return NULL;
	
	return dbb_relations[relationId];
}

Relation* Database::findRelation(thread_db* tdbb, int relationId)
{
	return NULL;
}

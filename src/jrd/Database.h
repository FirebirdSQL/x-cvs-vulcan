/* $Id$ */
/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		Database.h
 *	DESCRIPTION:	Database Class
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
// Database.h: interface for the Database class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DATABASE_H__BE654FF4_17AA_4D04_B13D_B8569CF885F2__INCLUDED_)
#define AFX_DATABASE_H__BE654FF4_17AA_4D04_B13D_B8569CF885F2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <time.h>
#include "thd.h"
#include "isc.h"
#include "AsyncEvent.h"
#include "JVector.h"
#include "JString.h"
#include "ConfObj.h"
#include "RefObject.h"
#include "LinkedList.h"
#include "SyncObject.h"
#include "DVector.h"
#include "SVector.h"
#include "SIVector.h"
#include "../lock/LockMgr.h"
#include "DatabaseManager.h"

static const int HASH_SIZE = 101;

//
// bit values for dbb_flags
//
const ULONG DBB_no_garbage_collect	= 0x1L;
const ULONG DBB_damaged				= 0x2L;
const ULONG DBB_exclusive			= 0x4L;	/* Database is accessed in exclusive mode */
const ULONG DBB_bugcheck			= 0x8L;	/* Bugcheck has occurred */
#ifdef GARBAGE_THREAD
const ULONG DBB_garbage_collector	= 0x10L;	/* garbage collector thread exists */
const ULONG DBB_gc_active			= 0x20L;	/* ... and is actively working. */
const ULONG DBB_gc_pending			= 0x40L;	/* garbage collection requested */
#endif
const ULONG DBB_force_write			= 0x80L;	/* Database is forced write */
const ULONG DBB_no_reserve			= 0x100L;	/* No reserve space for versions */
//const ULONG DBB_add_log				= 0x200L;	// write ahead log has been added
//const ULONG DBB_delete_log			= 0x400L;	// write ahead log has been deleted
const ULONG DBB_cache_manager		= 0x800L;	// Shared cache manager
const ULONG DBB_DB_SQL_dialect_3	= 0x1000L;	/* database SQL dialect 3 */
const ULONG DBB_read_only			= 0x2000L;	/* DB is ReadOnly (RO). If not set, DB is RW */
const ULONG DBB_being_opened_read_only	= 0x4000L;	/* DB is being opened RO. If unset, opened as RW */
const ULONG DBB_not_in_use			= 0x8000L;	/* Database to be ignored while attaching */
const ULONG DBB_lck_init_done		= 0x10000L;	/* LCK_init() called for the database */
const ULONG DBB_sp_rec_mutex_init	= 0x20000L;	/* Stored procedure mutex initialized */
const ULONG DBB_sweep_in_progress	= 0x40000L;	/* A database sweep operation is in progress */
const ULONG DBB_security_db			= 0x80000L;	/* ISC security database */
const ULONG DBB_sweep_thread_started	= 0x100000L;	/* A database sweep thread has been started */
const ULONG DBB_suspend_bgio		= 0x200000L;	/* Suspend I/O by background threads */
const ULONG DBB_being_opened		= 0x400000L;	/* database is being attached to */
const ULONG DBB_gc_cooperative		= 0x0800000L;	/* cooperative garbage collection */
const ULONG DBB_gc_background		= 0x1000000L;	/* background garbage collection by gc_thread */

//
// dbb_ast_flags
//
const UATOM DBB_blocking			= 0x1L;		// Exclusive mode is blocking
//const UATOM DBB_get_shadows			= 0x2L;		// Signal received to check for new shadows
const UATOM DBB_assert_locks		= 0x4L;		// Locks are to be asserted
const UATOM DBB_shutdown			= 0x8L;		// Database is shutdown
const UATOM DBB_shut_attach			= 0x10L;	// no new attachments accepted
const UATOM DBB_shut_tran			= 0x20L;	// no new transactions accepted
const UATOM DBB_shut_force			= 0x40L;	// forced shutdown in progress
const UATOM DBB_shutdown_locks		= 0x80L;	// Database locks release by shutdown
const UATOM DBB_shutdown_full		= 0x100L;	// Database fully shut down
const UATOM DBB_shutdown_single		= 0x200L;	// Database is in single-user maintenance mode

//
// Database mutexes and read/write locks
//
const int DBB_MUTX_init_fini		= 0;	// During startup and shutdown
const int DBB_MUTX_statistics		= 1;	// Memory size and counts
const int DBB_MUTX_replay			= 2;	// Replay logging
const int DBB_MUTX_dyn				= 3;	// Dynamic ddl
const int DBB_MUTX_cache			= 4;	// Process-private cache management
const int DBB_MUTX_clone			= 5;	// Request cloning
const int DBB_MUTX_cmp_clone		= 6;	// Compiled request cloning
const int DBB_MUTX_flush_count		= 7;	// flush count/time
const int DBB_MUTX_max				= 8;

const int DBB_WLCK_pools			= 0;	// Pool manipulation
const int DBB_WLCK_files			= 1;	// DB and shadow file manipulation
const int DBB_WLCK_max				= 2;

CLASS (ModuleManager);

class JrdMemoryPool;
class Attachment;
class Relation;
class Transaction;
class Request;
class str;
class BlockingThread;	/* Attachments waiting for update */
//class BlockingThread;	/* Unused btb blocks */
class map;	/* mapping of blobs for REPLAY */
class Symbol;
class log;
class Lock;
class BackupManager;
class PageCache;
class CharSetManager;
class CharSetContainer;
class Procedure;
class ProcManager;
class InternalConnection;
class SecurityPlugin;
class TipCache;
class CommitManager;

struct fil;		/* files for I/O operations */
class sdw;		/* shadow control block */
struct bcb;		/* Buffer control block */
struct plc;		/* connection block */
struct PageControl;	/* page control */
struct blf;		/* known blob filters */
struct lls;		/* external function/filter modules */
struct blf;
struct lls;
struct jrn;		/* journal block */
//struct tpc;		/* cache of latest known state of all transactions in system */
struct thread_db;

class Database : public RefObject //public pool_alloc<type_dbb>
{
public:
	//Database();
	virtual ~Database();

public:
	Relation* findRelation (thread_db* tdbb, const char *relationName);
	ISC_STATUS executeDDL (ISC_STATUS *statusVector, Transaction *transaction, int length, const UCHAR *ddl);
	void init();
	int getSqlDialect();
	typedef int (*crypt_routine) (char*, void*, int, void*);

	Database			*dbb_next;				/* Next database block in system */
	Attachment			*dbb_attachments;		/* Active attachments */
	SVector<Relation*>	dbb_relations;			/* relation vector */
	Lock					*dbb_lock;				/* granddaddy lock */
	Transaction			*dbb_sys_trans;			/* system transaction */
	fil					*dbb_file;				/* files for I/O operations */
	sdw					*dbb_shadow;			/* shadow control block */
	Lock					*dbb_shadow_lock;		/* lock for synchronizing addition of shadows */
	SLONG				dbb_shadow_sync_count;	/* to synchronize changes to shadows */
	Lock					*dbb_retaining_lock;	/* lock for preserving commit retaining snapshot */
	plc					*dbb_connection;		/* connection block */
	PageControl			*dbb_pcontrol;			/* page control */
#ifdef SHARED_CACHE
	SIVector<SLONG>		dbb_t_pages;			/* pages number for transactions */
	SIVector<SLONG>		dbb_gen_id_pages;		/* known pages for gen_id */
#else
	SVector<SLONG>		dbb_t_pages;			/* pages number for transactions */
	SVector<SLONG>		dbb_gen_id_pages;		/* known pages for gen_id */
#endif
	struct blf			*dbb_blob_filters;		/* known blob filters */
	struct lls			*dbb_modules;			/* external function/filter modules */
	SLONG				dbb_sort_size;			/* Size of sort space per sort */

	ConfObj				configuration;
	int					maxUnflushedWrites;
	int					maxUnflushedWriteTime;
	int					priorityBoost;
	int					cacheDefault;
	SLONG				temporaryBlobIds;
	CharSetManager		*charSetManager;
	ModuleManager		*intlModuleManager;
	CharSetContainer	*defaultCharSet;
	CommitManager		*commitManager;
	ProcManager			*procManager;
	SecurityPlugin		*securityPlugin;
	bool				fileShared;
	bool				initialized;
		
	UATOM				dbb_ast_flags;			/* flags modified at AST level */
	ULONG				dbb_flags;
	USHORT				dbb_ods_version;		/* major ODS version number */
	USHORT				dbb_minor_version;		/* minor ODS version number */
	USHORT				dbb_minor_original;		/* minor ODS version at creation */
	USHORT				dbb_page_size;			/* page size */
	USHORT				dbb_dp_per_pp;			/* data pages per pointer page */
	USHORT				dbb_max_records;		/* max record per data page */
	USHORT				dbb_max_idx;			/* max number of indexes on a root page */
	volatile INTERLOCK_TYPE		dbb_use_count;			/* active count of threads */
	USHORT				dbb_shutdown_delay;		/* seconds until forced shutdown */
	USHORT				dbb_refresh_ranges;		/* active count of refresh ranges */
	USHORT				dbb_prefetch_sequence;	/* sequence to pace frequency of prefetch requests */
	USHORT				dbb_prefetch_pages;		/* prefetch pages per request */
	str					*dbb_spare_string;		/* random buffer */
	JString				dbb_filename;			/* filename string */
	str					*dbb_encrypt_key;		/* encryption key */

	JrdMemoryPool		*dbb_permanent;
	JrdMemoryPool		*dbb_bufferpool;

	LinkedList			dbb_pools;
    USHORT				dbb_next_pool_id;
#ifdef SHARED_CACHE
	SIVector<Request*>	dbb_internal;			/* internal requests */
	SIVector<Request*>	dbb_dyn_req;			/* internal dyn requests */
#else
	SVector<Request*>	dbb_internal;			/* internal requests */
	SVector<Request*>	dbb_dyn_req;			/* internal dyn requests */
#endif

	SLONG				dbb_oldest_active;		/* Cached "oldest active" transaction */
	SLONG				dbb_oldest_transaction;	/* Cached "oldest interesting" transaction */
	SLONG 				dbb_oldest_snapshot;	/* Cached "oldest snapshot" of all active transactions */
	SLONG 				dbb_next_transaction;	/* Next transaction id used by NETWARE */
	SLONG 				dbb_attachment_id;		/* Next attachment id for ReadOnly DB's */
	SLONG 				dbb_page_incarnation;	/* Cache page incarnation counter */
	ULONG 				dbb_page_buffers;		/* Page buffers from header page */

	AsyncEvent			dbb_writer_event[1];	/* Event to wake up cache writer */
	AsyncEvent 			dbb_writer_event_init[1];	/* Event for initialization cache writer */
	AsyncEvent 			dbb_writer_event_fini[1];	/* Event for finalization cache writer */
	AsyncEvent 			dbb_reader_event[1];	/* Event to wake up cache reader */
	
#ifdef GARBAGE_THREAD
	AsyncEvent			dbb_gc_event[1];		/* Event to wake up garbage collector */
	AsyncEvent			dbb_gc_event_init[1];	/* Event for initialization garbage collector */
	AsyncEvent			dbb_gc_event_fini[1];	/* Event for finalization garbage collector */
#endif

	Attachment			*dbb_update_attachment;	/* Attachment with update in process */
	BlockingThread*		dbb_update_que;		/* Attachments waiting for update */
	BlockingThread*		dbb_free_btbs;			/* Unused btb blocks */

	int					dbb_current_memory;
	int 				dbb_max_memory;
	SLONG				dbb_reads;
	SLONG 				dbb_writes;
	SLONG 				dbb_fetches;
	SLONG 				dbb_marks;
	SLONG 				dbb_last_header_write;	/* Transaction id of last header page physical write */
	SLONG 				dbb_flush_cycle;		/* Current flush cycle */
	SLONG 				dbb_sweep_interval;		/* Transactions between sweep */
	SLONG 				dbb_lock_owner_handle;	/* Handle for the lock manager */

	USHORT 				unflushed_writes;		/* unflushed writes */
	time_t 				last_flushed_write;		/* last flushed write time */

	crypt_routine 		dbb_encrypt;			/* External encryption routine */
	crypt_routine 		dbb_decrypt;			/* External decryption routine */

	map 				*dbb_blob_map;			/* mapping of blobs for REPLAY */
	class log *dbb_log;							/* log file for REPLAY */
	
	JVector				dbb_text_objects;
	JVector				dbb_charsets;

	PageCache			*pageCache;

#ifdef SHARED_CACHE
	SyncObject			syncObject;
	SyncObject			syncAttachments;
	SyncObject			syncRelations;
	SyncObject			syncProcedures;
	SyncObject			syncReady;
	SyncObject			syncSymbols;
	SyncObject			syncExistence;
	SyncObject			syncStoredProc;
	SyncObject			syncScanPages;
	SyncObject			syncSecurityClass;
	SyncObject			syncSysTrans;
	SyncObject			syncCreateIndex;
	SyncObject			syncDyn;
	SyncObject			syncCmpClone;
	Mutex				syncClone;

	Mutex				syncFlushCount;
	SIVector<SLONG>		dbb_pc_transactions;		/* active precommitted transactions */
#else
	SyncObject			syncAst;
	SyncObject			syncConnection;				/* Enforce 1 thread per connection */
	DatabaseManager		*databaseManager;
	SVector<SLONG>		dbb_pc_transactions;		/* active precommitted transactions */
	LockMgr				lockMgr;
#endif
	
	InternalConnection	*systemConnection;
			
	//tpc					*dbb_tip_cache;				/* cache of latest known state of all transactions in system */
	TipCache			*tipCache;
	INTERLOCK_TYPE		sweeperCount;
	BackupManager		*backup_manager;			/* physical backup manager */
	Symbol				*dbb_hash_table[HASH_SIZE];	/* keep this at the end */

	Database (const char *expandedFilename, ConfObject *configObject);
	
	// The delete operators are no-oped because the dbb memory is allocated from the
	// dbb's own permanent pool.  That pool has already been released by the dbb
	// destructor, so the memory has already been released.  Hence the operator
	// delete no-op.
	
	//void operator delete(void *mem) {}
	//void operator delete[](void *mem) {}

	Database(const Database&);	// no impl.
	const Database& operator =(const Database&) { return *this; }
	bool isFilename(const char* filename);
	CharSetContainer* findCharset(thread_db* tdbb, int ttype);
	CharSetContainer* findCharset(thread_db* tdbb, const char* name);
	CharSetContainer* findCollation(thread_db* tdbb, const char* name);
	void addPool(JrdMemoryPool* pool);
	void removePool(JrdMemoryPool* pool);
	Attachment* createAttachment(void);
	void deleteAttachment(Attachment* attachment);
	void makeReady(void);
	bool isReady(bool waitFlag);
	//bool isInitialized(void);
	Relation* lookupRelation(thread_db* tdbb, const char* relationName);
	Relation* getRelation(thread_db* tdbb, int id);
	void validate(void);
	Procedure* findProcedure(thread_db* tdbb, int id);
	Procedure* findProcedure(thread_db* tdbb, const TEXT* name, bool noscan);
	InternalConnection* getSystemConnection(void);
	void updateAccountInfo(thread_db* tdbb, int apbLength, const UCHAR* apb);
	void authenticateUser(thread_db* tdbb, int dpbLength, const UCHAR* dpb, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer);
	void incrementUseCount(void);
	void decrementUseCount(void);
	Relation* findRelation(int relationId);
	Relation* findRelation(thread_db* tdbb, int relationId);
};

#endif // !defined(AFX_DATABASE_H__BE654FF4_17AA_4D04_B13D_B8569CF885F2__INCLUDED_)

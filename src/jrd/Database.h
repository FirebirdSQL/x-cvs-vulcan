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

static const int HASH_SIZE = 101;

//
// bit values for dbb_flags
//
#define DBB_no_garbage_collect 	0x1L
#define DBB_damaged         	0x2L
#define DBB_exclusive       	0x4L	/* Database is accessed in exclusive mode */
#define DBB_bugcheck        	0x8L	/* Bugcheck has occurred */
#ifdef GARBAGE_THREAD
#define DBB_garbage_collector	0x10L	/* garbage collector thread exists */
#define DBB_gc_active			0x20L	/* ... and is actively working. */
#define DBB_gc_pending			0x40L	/* garbage collection requested */
#endif
#define DBB_force_write     	0x80L	/* Database is forced write */
#define DBB_no_reserve     		0x100L	/* No reserve space for versions */
#define DBB_add_log         	0x200L	/* write ahead log has been added */
#define DBB_delete_log      	0x400L	/* write ahead log has been deleted */
#define DBB_cache_manager   	0x800L	/* Shared cache manager */
#define DBB_DB_SQL_dialect_3 	0x1000L	/* database SQL dialect 3 */
#define DBB_read_only    		0x2000L	/* DB is ReadOnly (RO). If not set, DB is RW */
#define DBB_being_opened_read_only 0x4000L	/* DB is being opened RO. If unset, opened as RW */
#define DBB_not_in_use      	0x8000L	/* DBB to be ignored while attaching */
#define DBB_lck_init_done   	0x10000L	/* LCK_init() called for the database */
#define DBB_sp_rec_mutex_init 	0x20000L	/* Stored procedure mutex initialized */
#define DBB_sweep_in_progress 	0x40000L	/* A database sweep operation is in progress */
#define DBB_security_db     	0x80000L	/* ISC security database */
#define DBB_sweep_thread_started 0x100000L	/* A database sweep thread has been started */
#define DBB_suspend_bgio		0x200000L	/* Suspend I/O by background threads */
#define DBB_being_opened    	0x400000L	/* database is being attached to */

//
// dbb_ast_flags
//
#define DBB_blocking		0x1L	// Exclusive mode is blocking
//#define DBB_get_shadows		0x2L	// Signal received to check for new shadows
#define DBB_assert_locks	0x4L	// Locks are to be asserted
#define DBB_shutdown		0x8L	// Database is shutdown
#define DBB_shut_attach		0x10L	// no new attachments accepted
#define DBB_shut_tran		0x20L	// no new transactions accepted
#define DBB_shut_force		0x40L	// forced shutdown in progress
#define DBB_shutdown_locks	0x80L	// Database locks release by shutdown

//
// Database mutexes and read/write locks
//
#define DBB_MUTX_init_fini      0	// During startup and shutdown
#define DBB_MUTX_statistics     1	// Memory size and counts
#define DBB_MUTX_replay         2	// Replay logging
#define DBB_MUTX_dyn            3	// Dynamic ddl
#define DBB_MUTX_cache          4	// Process-private cache management
#define DBB_MUTX_clone          5	// Request cloning
#define DBB_MUTX_cmp_clone      6	// Compiled request cloning
#define DBB_MUTX_flush_count    7	// flush count/time
#define DBB_MUTX_max            8

#define DBB_WLCK_pools          0	// Pool manipulation
#define DBB_WLCK_files          1	// DB and shadow file manipulation
#define DBB_WLCK_max            2


class JrdMemoryPool;
class Attachment;
class Relation;
class Transaction;
class Request;
class str;
class btb;	/* Attachments waiting for update */
class btb;	/* Unused btb blocks */
class map;	/* mapping of blobs for REPLAY */
class Sym;
class log;
class lck;
class BackupManager;
class PageCache;
class CharSetManager;
class ModuleManager;
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
struct pgc;		/* page control */
struct blf;		/* known blob filters */
struct lls;		/* external function/filter modules */
struct blf;
struct lls;
struct jrn;		/* journal block */
//struct tpc;		/* cache of latest known state of all transactions in system */
struct tdbb;

class Database : public RefObject //public pool_alloc<type_dbb>
{
public:
	//Database();
	virtual ~Database();

public:
	Relation* findRelation (tdbb *tdbb, const char *relationName);
	ISC_STATUS executeDDL (ISC_STATUS *statusVector, Transaction *transaction, int length, const UCHAR *ddl);
	void init();
	int getSqlDialect();
	typedef int (*crypt_routine) (char*, void*, int, void*);

	Database			*dbb_next;				/* Next database block in system */
	Attachment			*dbb_attachments;		/* Active attachments */
	SVector<Relation*>	dbb_relations;			/* relation vector */
	lck					*dbb_lock;				/* granddaddy lock */
	Transaction			*dbb_sys_trans;			/* system transaction */
	fil					*dbb_file;				/* files for I/O operations */
	sdw					*dbb_shadow;			/* shadow control block */
	lck					*dbb_shadow_lock;		/* lock for synchronizing addition of shadows */
	SLONG				dbb_shadow_sync_count;	/* to synchronize changes to shadows */
	lck					*dbb_retaining_lock;	/* lock for preserving commit retaining snapshot */
	plc					*dbb_connection;		/* connection block */
	pgc					*dbb_pcontrol;			/* page control */
	SIVector<SLONG>		dbb_t_pages;			/* pages number for transactions */
	SIVector<SLONG>		dbb_gen_id_pages;		/* known pages for gen_id */
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
	SIVector<Request*>	dbb_internal;			/* internal requests */
	SIVector<Request*>	dbb_dyn_req;			/* internal dyn requests */

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
	btb					*dbb_update_que;		/* Attachments waiting for update */
	btb					*dbb_free_btbs;			/* Unused btb blocks */

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
	
	Mutex				syncDyn;
	Mutex				syncClone;
	Mutex				syncCmpClone;
	Mutex				syncFlushCount;
	
	InternalConnection	*systemConnection;
			
	//tpc					*dbb_tip_cache;				/* cache of latest known state of all transactions in system */
	TipCache			*tipCache;
	SIVector<SLONG>		dbb_pc_transactions;		/* active precommitted transactions */
	BackupManager		*backup_manager;			/* physical backup manager */
	Sym					*dbb_hash_table[HASH_SIZE];	/* keep this at the end */

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
	CharSetContainer* findCharset(tdbb* tdbb, int ttype);
	CharSetContainer* findCharset(tdbb* tdbb, const char* name);
	CharSetContainer* findCollation(tdbb* tdbb, const char* name);
	void addPool(JrdMemoryPool* pool);
	void removePool(JrdMemoryPool* pool);
	Attachment* createAttachment(void);
	void deleteAttachment(Attachment* attachment);
	void makeReady(void);
	bool isReady(bool waitFlag);
	//bool isInitialized(void);
	Relation* lookupRelation(tdbb* tdbb, const char* relationName);
	Relation* getRelation(tdbb* tdbb, int id);
	void validate(void);
	Procedure* findProcedure(tdbb* tdbb, int id);
	Procedure* findProcedure(tdbb* tdbb, const TEXT* name, bool noscan);
	InternalConnection* getSystemConnection(void);
	void updateAccountInfo(tdbb* tdbb, int apbLength, const UCHAR* apb);
	void authenticateUser(tdbb* tdbb, int dpbLength, const UCHAR* dpb, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer);
	void incrementUseCount(void);
	void decrementUseCount(void);
	Relation* findRelation(int relationId);
	Relation* findRelation(tdbb* tdbb, int relationId);
};

#endif // !defined(AFX_DATABASE_H__BE654FF4_17AA_4D04_B13D_B8569CF885F2__INCLUDED_)

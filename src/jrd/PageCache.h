/*
 *	PROGRAM:		JRD access method
 *	MODULE:			PageCache.h
 *	DESCRIPTION:	Cache manager definitions
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
 */


#ifndef _PageCache_h_
#define _PageCache_h_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "que.h"
#include "SyncObject.h"
#include "Mutex.h"
//#include "LinkedList.h"

#ifndef BCB_keep_pages
static const int BCB_keep_pages		= 1;	/* set during btc_flush(), pages not removed from dirty binary tree */
static const int BCB_cache_writer	= 2;	/* cache writer thread has been started */
static const int BCB_checkpoint_db	= 4;	/* WAL has requested a database checkpoint */
static const int BCB_writer_active	= 8;	/* no need to post writer event count */
static const int BCB_cache_reader	= 16;	/* cache reader thread has been started */
static const int BCB_reader_active	= 32;	/* cache reader not blocked on event */
static const int BCB_free_pending	= 64;	/* request cache writer to free pages */
#endif

#ifndef LATCH_TYPE
#define LATCH_TYPE
enum LatchType
	{
	LATCH_none,
	LATCH_shared,
	LATCH_io,
	LATCH_exclusive,
	LATCH_mark
	};
#endif

/* Flush flags */

#ifndef FLUSH_FLAGS
#define FLUSH_FLAGS
#define FLUSH_ALL	1			/* flush all dirty buffers in cache */
//#define FLUSH_RLSE	2			/* release page locks after flush */
#define FLUSH_TRAN	4			/* flush transaction dirty buffers from dirty btree */
#define FLUSH_SWEEP	8			/* flush dirty buffers from garbage collection */
#define FLUSH_SYSTEM	16		/* flush system transaction only from dirty btree */
//#define FLUSH_FINI	(FLUSH_ALL | FLUSH_RLSE)
#define FLUSH_FINI	(FLUSH_ALL)
#endif

enum LockState {
	lsLatchTimeout,		// was -2		*** now unused ***
	lsLockTimeout,		// was -1
	lsLockedHavePage,	// was 0
	lsLocked,			// was 1
	lsError
	};
	
class Database;
class bdb;
class lls;
class pre;
class sbm;
class Bdb;
struct fil;
class sdw;
struct Que;
struct thread_db;
struct pag;
struct win;

class PageCache
{
public:
	PageCache(Database *dbb);
	~PageCache(void);

	void		*bcb_memory;		/* Large block partitioned into buffers */
	Que			bcb_in_use;			/* Que of buffers in use */
	Que			bcb_pending;		/* Que of buffers in use */
	Que			bcb_empty;			/* Que of empty buffers */
	Que			bcb_free_lwt;		/* Free latch wait blocks */
	Que			bcb_dirty;
	Que			*hashTable;
	int			bcb_flags;			/* see below */
	SSHORT		bcb_free_minimum;	/* Threshold to activate cache writer */
	ULONG		bcb_count;			/* Number of buffers allocated */
	ULONG		bcb_checkpoint;		/* Count of buffers to checkpoint */
	sbm*		bcb_prefetch;		/* Bitmap of pages to prefetch */
	Database	*database;
	
	SyncObject	syncObject;
	SyncObject	syncDirtyBdbs;
	SyncObject	syncPageWrite;
	SyncObject	syncPrecedence;
	SyncObject	syncFlushDirtyBdbs;
	
	Mutex		syncLRU;
	Bdb			**bdbs;
	volatile Bdb *bcb_lru_chain;
	int			markSequence;
	bool		getShadows;
	
	void initialize(thread_db* tdbb, int numberBuffers);
	bool writePage(thread_db* tdbb, Bdb* bdb, bool write_thru, bool inAst);
	void journalBuffer(Bdb* bdb);
	bool setWriteDirection(thread_db* tdbb, Bdb* bdb, int direction);
	bool rolloverToShadow(thread_db* tdbb, fil* file, bool inAst);
	bool writeAllShadows(thread_db* tdbb, sdw* shadow, Bdb* bdb, int checksum, bool inAst);
	void removeDirtyPage(Bdb* bdb);
	void unwind(thread_db* tdbb, bool punt);
	void releaseLatch(thread_db* tdbb, Bdb* bdb, bool repost, bool downgrade_latch, bool rel_mark_latch);
	void bugcheck(int msgNumber);
	void flushDatabase(thread_db* tdbb);
	void updateWriteDirection(thread_db* tdbb, Bdb* bdb);
	void invalidateAndRelease(thread_db* tdbb, Bdb* bdb);
	int downGradeDatabase(void);
	void insertDirtyPage(Bdb* bdb);
	pag* fake(thread_db* tdbb, win* window);
	Bdb* getBuffer(thread_db* tdbb, SLONG page, int lock_type);
	void clearPrecedence(Bdb* bdb);
	int writeBuffer(thread_db* tdbb, Bdb* bdb, SLONG page, bool write_thru, ISC_STATUS* status, bool write_this_page);
	LockState lockBuffer(thread_db* tdbb, Bdb* bdb, int wait, int page_type);
	void mark(thread_db* tdbb, win* window, int mark_system);
	int latchBdb(thread_db* tdbb, LatchType type, Bdb* bdb, SLONG page);
	bool writeable(Bdb* bdblock);
	void expandBuffers(thread_db* tdbb, int expandedSize);
	int blockingAst(Bdb* bdb);
	void downGrade(thread_db* tdbb, Bdb* bdb);
	bool getExclusive(thread_db* tdbb, int level, int wait_flag);
	bool getExclusiveAttachment(thread_db* tdbb, int level, int wait_flag);
	pag* fetch(thread_db* tdbb, win* window, int lock_type, int page_type, int checksum, int latch_wait, int read_shadow);
	LockState fetchLock(thread_db* tdbb, win* window, int lock_type, int wait, int page_type);
	void fetchPage(thread_db* tdbb, win* window, int compute_checksum, bool read_shadow);
	void pageValidationError(thread_db* tdbb, win* window, int type);
	pag* handoff(thread_db* tdbb, win* window, SLONG page, int lock, int page_type, int latch_wait, int release_tail);
	void unmark(thread_db* tdbb, win* window);
	void release(thread_db* tdbb, win* window, bool release_tail = false);
	void declarePrecedence(thread_db* tdbb, win* window, SLONG page);
	//void checkPrecedence(thread_db* tdbb, win* window, SLONG page);
	int related(Bdb* low, Bdb* high, int limit);
	void prefetch(thread_db* tdbb, SLONG* pages, int count);
	void markMustWrite(thread_db* tdbb, win* window);
	void flush(thread_db* tdbb, int flush_flag, int tra_number);
	void flushDirtyPages(thread_db* tdbb, int transaction_mask, bool sys_only, ISC_STATUS* status);
	void releaseExclusive(thread_db* tdbb);
	void shutdownDatabase(thread_db* tdbb);
	int checksum(Bdb *bdb);
	bool validate(win* window);
	void recoverShadow(thread_db* tdbb, sbm* sbm_rec);
	void fini(thread_db* tdbb);

	static int bdbBlockingAst(void* ast_argument);
	static int downGradeDbb(void* database);
	Bdb* findBuffer(SLONG pageNumber);
	void validate(void);
	void validate(que* que);
	void reorderDirty(void);
	void recentlyUsed(Bdb* bdb);
	void requeueRecentlyUsed(void);
	void deletePrecedence(pre* precedence);
	void clearPrecedenceSync(Bdb* bdb);
};

#endif

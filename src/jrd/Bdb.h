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

#ifndef _Bdb_h_
#define _Bdb_h_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "que.h"
#include "SyncObject.h"

#ifndef DUMMY_CHECKSUM
#define DUMMY_CHECKSUM        12345
#endif

/* bdb_flags */

#ifndef BDB_dirty
static const int BDB_dirty			= 1;		/* page has been updated but not written yet */
static const int BDB_garbage_collect= 2;		/* left by scan for garbage collector */
static const int BDB_writer			= 4;		/* someone is updating the page */
static const int BDB_marked			= 8;		/* page has been updated */
static const int BDB_must_write		= 16;		/* forces a write as soon as the page is released */
static const int BDB_faked			= 32;		/* page was just allocated */
static const int BDB_journal		= 64;		/* Journal buffer */
static const int BDB_system_dirty 	= 128;		/* system transaction has marked dirty */
static const int BDB_io_error	 	= 256;		/* page i/o error */
static const int BDB_read_pending 	= 512;		/* read is pending */
static const int BDB_free_pending 	= 1024;		/* buffer being freed for reuse */
static const int BDB_not_valid		= 2048;		/* i/o error invalidated buffer */
static const int BDB_db_dirty 		= 4096;		/* page must be written to database */
static const int BDB_checkpoint		= 8192;		/* page must be written by next checkpoint */
static const int BDB_prefetch		= 16384;	/* page has been prefetched but not yet referenced */
static const int BDB_no_blocking_ast = 32768;	/* No blocking AST registered with page lock */
static const int BDB_lru_chained	= 65536;	/* Bdb is in linked list for lru fixup */

/* bdb_ast_flags */

static const int BDB_blocking 		= 1;		/* a blocking ast was sent while page locked */

/* bdb_write_direction values */

static const int BDB_write_undefined	= 0;
static const int BDB_write_normal		= 1;
static const int BDB_write_diff			= 2;
static const int BDB_write_both			= 3;

static const int BDB_max_shared			= 20;	/* maximum number of shared latch owners per bdb */
#endif

class Database;
class lck;
class btb;
struct pag;
struct exp_index_buf;
struct thread_db;

class Bdb
{
public:
	Bdb(void);
	Bdb(thread_db* tdbb, pag* memory);
	~Bdb(void);

public:
	Database*		bdb_dbb;				/* Database block (for ASTs) */
	lck*			bdb_lock;				/* Lock block for buffer */
	Que				bdb_que;				/* Buffer que */
	Que				bdb_in_use;				/* queue of buffers in use */
	pag*			bdb_buffer;				/* Actual buffer */
	exp_index_buf*	bdb_expanded_buffer;	/* expanded index buffer */
	Bdb*			bdb_jrn_bdb;			/* BDB containing journal records */
	Bdb				*bdb_lru_chain;
	btb*			bdb_blocked;			/* Blocked attachments block */
	SLONG			bdb_page;				/* Database page number in buffer */
	SLONG			bdb_pending_page;		/* Database page number to be */
	ULONG			bdb_sequence;
	ULONG			bdb_mark_sequence;
	SLONG			bdb_incarnation;
	ULONG			bdb_transactions;		/* vector of dirty flags to reduce commit overhead */
	SLONG			bdb_mark_transaction;	/* hi-water mark transaction to defer header page I/O */
	//Bdb*			bdb_left;				/* dirty page binary tree link */
	//Bdb*			bdb_right;				/* dirty page binary tree link */
	//Bdb*			bdb_parent;				/* dirty page binary tree link */
	Que				bdb_lower;				/* lower precedence que */
	Que				bdb_higher;				/* higher precedence que */
	Que				bdb_waiters;			/* latch wait que */
	Que				bdb_dirty;				/* dirty buffer que */
	thread_db*		bdb_exclusive;			/* thread holding exclusive latch */
	thread_db*		bdb_io;					/* thread holding io latch */
	UATOM			bdb_ast_flags;			/* flags manipulated at AST level */
	volatile INTERLOCK_TYPE	bdb_flags;
	USHORT			bdb_length;				/* Length of journal records */
	SSHORT			bdb_scan_count;			/* concurrent sequential scans */
	USHORT			bdb_write_direction;    /* Where to write buffer */
	ULONG			bdb_difference_page;    /* Number of page in difference file */
	SLONG			bdb_diff_generation;    /* Number of backup/restore cycle for 
											   this database in current process.
											   Used in CS only. */
	thread_db*		bdb_shared[BDB_max_shared];	/* threads holding shared latches */
	bool			exclusive;
	//bool			bdb_lru_chained;
	SyncObject		syncPage;
	//SyncObject		syncBdb;
	int				writers;
	
	volatile INTERLOCK_TYPE bdb_use_count;			/* Number of active users */
	
	void		init(thread_db* tdbb, lck *lock, pag* buffer);
	USHORT		computeChecksum(void);
	static int	blockingAstBdb(void* argument);
	void		addRef(thread_db* tdbb, LockType lockType);
	void		release(thread_db* tdbb);
	void		downGrade(LockType lockType);
	void		bugcheck(void);
	bool		isLocked(void);
	void		validate(void);
	bool		ourExclusiveLock(void);
	void		init(void);
	int			clearFlags(int flags);
	int			setFlags(int flags);
	INTERLOCK_TYPE incrementUseCount(void);
	INTERLOCK_TYPE decrementUseCount(void);
	int setFlags(int setBits, int clearBits);
	void printPage(void);
	bool addRefConditional(thread_db* tdbb, LockType lType);
};

#endif


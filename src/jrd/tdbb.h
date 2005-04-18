/* $Id$ */
#ifndef __TDBB_H_
#define __TDBB_H_

#ifdef MVS
#include <setjmp.h>
#endif

#include "thd.h"

const USHORT TDBB_sweeper				= 1;	/* Thread sweeper or garbage collector */
const USHORT TDBB_no_cache_unwind		= 2;	/* Don't unwind page buffer cache */
const USHORT TDBB_prc_being_dropped		= 4;	/* Dropping a procedure  */
const USHORT TDBB_set_backup_state		= 8;	/* Setting state for backup lock */
const USHORT TDBB_backup_merge			= 16;	/* Merging changes from difference file */
//const USHORT TDBB_stack_trace_done		= 32;	/* PSQL stack trase is added into status-vector */

#define MAX_THREAD_BDBS		10

class Database;
class Attachment;
class JrdMemoryPool;
class DsqlMemoryPool;
class Transaction;
class Request;
class Bdb;

struct iuo;

/* Thread specific database block */

struct thread_db
{
	struct thdd	tdbb_thd_data;
	Database*	tdbb_database;
	Attachment*	tdbb_attachment;
	class Transaction*	tdbb_transaction;
	Request*	tdbb_request;
	JrdMemoryPool*	tdbb_default;
	DsqlMemoryPool*	tsql_default;
	ISC_STATUS*	tdbb_status_vector;
	void*		tdbb_setjmp;
	USHORT		tdbb_inhibit;		/* Inhibit context switch if non-zero */
	USHORT		tdbb_flags;
	//iuo		tdbb_mutexes;
	//iuo		tdbb_rw_locks;
	iuo			tdbb_pages;
	Bdb			*tdbb_bdbs [MAX_THREAD_BDBS];
	
#if defined(UNIX) && defined(SUPERSERVER)
#if defined(FREEBSD)
    sigjmp_buf tdbb_sigsetjmp;
#else
    //jmp_buf tdbb_sigsetjmp;
    sigjmp_buf tdbb_sigsetjmp;
#endif
#endif
	void registerBdb(Bdb* bdb);
	void clearBdb(Bdb* bdb);
};


#endif //__TDBB_H_

/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		lck.cpp
 *	DESCRIPTION:	Lock handler for JRD (not lock manager!)
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
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#include <memory.h>
#include "fbdev.h"
#include "../jrd/ib_stdio.h"
#include "../jrd/jrd.h"
#include "../jrd/lck.h"
#include "gen/iberror.h"
#include "../jrd/iberr.h"
//#include "../jrd/all_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/jrd_proto.h"
#include "../jrd/lck_proto.h"

#ifdef VMS
#include "../jrd/vmslo_proto.h"
#else
#include "../lock/LockMgr.h"
#endif

//#include "../jrd/sch_proto.h"
//#include "../jrd/thd_proto.h"
#include "../jrd/gdsassert.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef WIN_NT
#include <process.h>
#endif


static void bug_lck(const TEXT*);

#ifdef MULTI_THREAD
static Lock*		find_block(Lock*, USHORT);
static void		check_lock(Lock*, USHORT);
#endif

//static BOOLEAN	compatible(Lock*, Lock*, USHORT);
static void		enqueue(thread_db*, Lock*, USHORT, SSHORT);
static int		external_ast(void *arg);

static USHORT	hash_func(UCHAR *, USHORT);
static void		hash_allocate(Lock*);
static Lock*		hash_get_lock(Lock*, USHORT *, Lock* **);
static void		hash_insert_lock(Lock*);
static BOOLEAN	hash_remove_lock(Lock*, Lock* *);
static void		internal_ast(Lock*);
static BOOLEAN internal_compatible(Lock*, Lock*, USHORT);
static void		internal_dequeue(ISC_STATUS *statusVector, Lock*);
static USHORT	internal_downgrade(ISC_STATUS *statusVector, Lock*);
static BOOLEAN	internal_enqueue(thread_db*, Lock*, USHORT, SSHORT, BOOLEAN);


/* globals and macros */

static SLONG process_lck_owner_handle = 0;

//#ifdef SUPERSERVER
#ifdef SUPERSERVERxxx

#define LCK_OWNER_ID_PROCESS		(long) getpid()
#define LCK_OWNER_ID_DBB			(long) dbb
#define LCK_OWNER_ID_ATT			(long) attachment

#define LCK_OWNER_TYPE_PROCESS		LCK_OWNER_process
#define LCK_OWNER_TYPE_DBB			LCK_OWNER_dbb
#define LCK_OWNER_TYPE_ATT			LCK_OWNER_attachment

#define	LCK_OWNER_HANDLE_PROCESS	process_lck_owner_handle
#define	LCK_OWNER_HANDLE_DBB		dbb->dbb_lock_owner_handle
#define	LCK_OWNER_HANDLE_ATT		attachment->att_lock_owner_handle

#else	/* SUPERSERVER */

/* This is not a SUPERSERVER build */

#define LCK_OWNER_ID_PROCESS		(SLONG) getpid()
#define LCK_OWNER_ID_DBB			(SLONG) getpid()
#define LCK_OWNER_ID_ATT			(SLONG) getpid()

#define LCK_OWNER_TYPE_PROCESS		LCK_OWNER_process
#define LCK_OWNER_TYPE_DBB			LCK_OWNER_process
#define LCK_OWNER_TYPE_ATT			LCK_OWNER_process

#ifdef SHARED_CACHE
#define	LCK_OWNER_HANDLE_PROCESS	process_lck_owner_handle
#define	LCK_OWNER_HANDLE_DBB		process_lck_owner_handle
#define LCK_OWNER_HANDLE_ATT		process_lck_owner_handle
#else
#define	LCK_OWNER_HANDLE_PROCESS	dbb->dbb_lock_owner_handle
#define	LCK_OWNER_HANDLE_DBB		dbb->dbb_lock_owner_handle
#define LCK_OWNER_HANDLE_ATT		dbb->dbb_lock_owner_handle
#endif

#endif	/* SUPERSERVER */


static const UCHAR compatibility[] = {

/*				Shared	Prot	Shared	Prot
		none	null	 Read	Read	Write	Write	Exclusive */

/* none */ 1, 1, 1, 1, 1, 1, 1,
/* null */ 1, 1, 1, 1, 1, 1, 1,
/* SR */ 1, 1, 1, 1, 1, 1, 0,
/* PR */ 1, 1, 1, 1, 0, 0, 0,
/* SW */ 1, 1, 1, 0, 1, 0, 0,
/* PW */ 1, 1, 1, 0, 0, 0, 0,
/* EX */ 1, 1, 0, 0, 0, 0, 0
};

//#define LCK_max			(LCK_EX + 1)
#define COMPATIBLE(st1, st2)	compatibility [st1 * LCK_max + st2]
#define LOCK_HASH_SIZE		19

#define	ENQUEUE(lock,level,wait) if (lock->lck_compatible) \
				     internal_enqueue (tdbb, lock, level, wait, FALSE); \
		                 else enqueue (tdbb, lock, level, wait);

#ifdef SHARED_CACHE
#define	CONVERT(lock,level,wait) (lock->lck_compatible) ? \
				 internal_enqueue (tdbb, lock, level, wait, TRUE) : \
                                 LockMgr::LOCK_convert (lock->lck_id, level, wait, lock->lck_ast, lock->lck_object, status)
#else
#define	CONVERT(lock,level,wait) (lock->lck_compatible) ? \
				 internal_enqueue (tdbb, lock, level, wait, TRUE) : \
                                 tdbb->tdbb_database->lockMgr.LOCK_convert (lock->lck_id, level, wait, lock->lck_ast, lock->lck_object, status)
#endif
/***
#define DEQUEUE(lock)		 if (lock->lck_compatible) \
				     internal_dequeue (tdbb, lock); \
				 else LockMgr::LOCK_deq (lock->lck_id);

#define DOWNGRADE(lock,status)	 (lock->lck_compatible) ? \
				 internal_downgrade (tdbb, lock) : \
			 	 LockMgr::LOCK_downgrade (lock->lck_id, status);
***/

#ifdef DEV_BUILD
/* Valid locks are not NULL, 
	of the right memory block type, 
	are attached to a dbb.
   	If we have the dbb in non-exclusive mode, 
	    then we must have a physical lock of at least the same level
	    as the logical lock.
   	If we don't have a lock ID, 
	    then we better not have a physical lock at any level.

JMB: As part of the c++ conversion I removed the check for lck block type.
 There is no more blk_type field in the lck structure, and some stack allocated
 lck's are passed into lock functions, so we can't do the check.
 Here is the line I removed from the macro:
				 (l->blk_type == type_lck) && \
 */
#define LCK_CHECK_LOCK(l)	(((l) != NULL) && \
				 (l->lck_dbb != NULL) && \
				 (l->lck_test_field == 666) && \
				 (l->lck_id || (l->lck_physical == LCK_none)))

/* The following check should be part of LCK_CHECK_LOCK, but it fails
   when the exclusive attachment to a database is changed to a shared
   attachment.  When that occurs we fb_assert all our internal locks, but
   while we are in the process of asserting DBB_assert_locks is set, but
   we haven't gotten physical locks yet.
 
			         (!(l->lck_dbb->dbb_ast_flags & DBB_assert_locks) \
				    || (l->lck_physical >= l->lck_logical)) && \
*/
#endif

#ifndef LCK_CHECK_LOCK
#define LCK_CHECK_LOCK(x)		(TRUE)	/* nothing */
#endif

void LCK_ast_inhibit() {
#ifdef SHARED_CACHE
	LockMgr::LOCK_ast_inhibit();
#endif
}

void LCK_ast_enable() {
#ifdef SHARED_CACHE
	LockMgr::LOCK_ast_enable();
#endif
}

void LCK_assert(thread_db* tdbb, Lock* lock)
{
/**************************************
 *
 *	L C K _ a s s e r t
 *
 **************************************
 *
 * Functional description
 *	Assert a logical lock.
 *
 **************************************/

	//SET_TDBB(tdbb);
	fb_assert(LCK_CHECK_LOCK(lock));

	if (lock->lck_logical == lock->lck_physical || lock->lck_logical == LCK_none) 
		return;

	if (!LCK_lock(tdbb, lock, lock->lck_logical, LCK_WAIT))
		BUGCHECK(159);			/* msg 159 cannot fb_assert logical lock */
		
	fb_assert(LCK_CHECK_LOCK(lock));
}


int LCK_convert(thread_db* tdbb, Lock* lock, USHORT level, SSHORT wait)
{
/**************************************
 *
 *	L C K _ c o n v e r t
 *
 **************************************
 *
 * Functional description
 *	Convert an existing lock to a new level.
 *
 **************************************/
#ifdef SHARED_CACHE
	Sync sync(&lock->syncObject, "LCK_convert");
	sync.lock(Exclusive);
#endif

	fb_assert(LCK_CHECK_LOCK(lock));
	//SET_TDBB(tdbb);
	DBB dbb = lock->lck_dbb;
	ISC_STATUS *status = tdbb->tdbb_status_vector;
	
	if (lock->lck_attachment != tdbb->tdbb_attachment)
		{
		if (lock->lck_attachment && lock->lck_long_lock)
			lock->lck_attachment->removeLongLock(lock);

		if (tdbb->tdbb_attachment)
			tdbb->tdbb_attachment->addLongLock(lock);
		}
	lock->lck_attachment = tdbb->tdbb_attachment;
	
	//int result = CONVERT(lock, level, wait);
	int result = (lock->lck_compatible) ? 
					internal_enqueue (tdbb, lock, level, wait, TRUE) :
#ifdef SHARED_CACHE
                    LockMgr::LOCK_convert (lock->lck_id, level, wait, lock->lck_ast, lock->lck_object, status);
#else
                    tdbb->tdbb_database->lockMgr.LOCK_convert (lock->lck_id, level, wait, lock->lck_ast, lock->lck_object, status);
#endif

	if (!result) 
		{
		if (lock->lck_attachment)
			{
			if (lock->lck_long_lock) lock->lck_attachment->removeLongLock(lock);
			}
		
		switch (status[1])
			{
			case isc_deadlock:
			case isc_lock_conflict:
			case isc_lock_timeout:
				return false;
			
			case isc_lockmanerr:
				dbb->dbb_flags |= DBB_bugcheck;
			}
			
		ERR_punt();
		}

	if (!lock->lck_compatible)
		lock->lck_physical = lock->lck_logical = level;

	fb_assert(LCK_CHECK_LOCK(lock));
	
	return TRUE;
}


int LCK_convert_non_blocking(thread_db* tdbb, Lock* lock, USHORT level, SSHORT wait)
{
/**************************************
 *
 *	L C K _ c o n v e r t _ n o n _ b l o c k i n g		( m u l t i _ t h r e a d )
 *
 **************************************
 *
 * Functional description
 *	Convert an existing lock.
 *
 **************************************/

	if (!wait)	// || !gds__thread_enable(FALSE))
		return LCK_convert(tdbb, lock, level, wait);
	
#ifdef SHARED_CACHE
	Sync sync(&lock->syncObject, "LCK_convert_non_blocking");
	sync.lock(Exclusive);
#endif

	fb_assert(LCK_CHECK_LOCK(lock));
	//SET_TDBB(tdbb);

	DBB dbb = lock->lck_dbb;

	if (lock->lck_attachment != tdbb->tdbb_attachment)
		{
		if (lock->lck_attachment && lock->lck_long_lock)
			lock->lck_attachment->removeLongLock(lock);

		if (tdbb->tdbb_attachment)
			tdbb->tdbb_attachment->addLongLock(lock);
		}
	lock->lck_attachment = tdbb->tdbb_attachment;


	/* Save context and checkout from the scheduler */

	check_lock(lock, level);
	ISC_STATUS *status = tdbb->tdbb_status_vector;
	Attachment* attachment = tdbb->tdbb_attachment;
	AST_DISABLE;

	/* SuperServer: Do Not release engine here, it creates a race
	   condition - more than one thread RUNNING in the engine.
	   We check out of the scheduler later - in wait_for_request()
	   in lock/lock.c - when we are going to wait on wakeup event.
	*/

	//int result = CONVERT(lock, level, wait);
	int result = (lock->lck_compatible) ? 
					internal_enqueue (tdbb, lock, level, wait, TRUE) :
#ifdef SHARED_CACHE
                    LockMgr::LOCK_convert (lock->lck_id, level, wait, lock->lck_ast, lock->lck_object, status)
#else
                    tdbb->tdbb_database->lockMgr.LOCK_convert (lock->lck_id, level, wait, lock->lck_ast, lock->lck_object, status)
#endif
	AST_ENABLE;

	if (!result) 
		{
		if (lock->lck_attachment)
			{
			if (lock->lck_long_lock) lock->lck_attachment->removeLongLock(lock);
			}
		if (status[1] == isc_deadlock ||
			status[1] == isc_lock_conflict || status[1] == isc_lock_timeout)
			return FALSE;
		if (status[1] == isc_lockmanerr)
			dbb->dbb_flags |= DBB_bugcheck;
		ERR_punt();
		}

	if (!lock->lck_compatible)
		lock->lck_physical = lock->lck_logical = level;

	fb_assert(LCK_CHECK_LOCK(lock));
	return TRUE;
}



int LCK_convert_opt(thread_db* tdbb, Lock* lock, USHORT level)
{
/**************************************
 *
 *	L C K _ c o n v e r t _ o p t
 *
 **************************************
 *
 * Functional description
 *	Assert a lock if the parent is not locked in exclusive mode.
 *
 **************************************/
	//SET_TDBB(tdbb);
#ifdef SHARED_CACHE
	Sync sync(&lock->syncObject, "LCK_convert_opt");
	sync.lock(Exclusive);
#endif

	fb_assert(LCK_CHECK_LOCK(lock));

	USHORT old_level = lock->lck_logical;
	lock->lck_logical = level;
	DBB dbb = lock->lck_dbb;

	if (dbb->dbb_ast_flags & DBB_assert_locks) 
		{
		lock->lck_logical = old_level;
		return LCK_convert(tdbb, lock, level, LCK_NO_WAIT);
		}

	fb_assert(LCK_CHECK_LOCK(lock));
	
	return TRUE;
}


#ifndef VMS
int LCK_downgrade(thread_db* tdbb, Lock* lock)
{
/**************************************
 *
 *	L C K _ d o w n g r a d e
 *
 **************************************
 *
 * Functional description
 *	Downgrade a lock.
 *
 **************************************/

#ifdef SHARED_CACHE
	Sync sync(&lock->syncObject, "LCK_downgrade");
	sync.lock(Exclusive);
#endif

	fb_assert(LCK_CHECK_LOCK(lock));
	//SET_TDBB(tdbb);
	ISC_STATUS *status = tdbb->tdbb_status_vector;
	fb_assert(LCK_CHECK_LOCK(lock));

	if (lock->lck_id && lock->lck_physical != LCK_none) 
		{
		USHORT level = (lock->lck_compatible) ?
						internal_downgrade (status, lock) :
#ifdef SHARED_CACHE
			 			LockMgr::LOCK_downgrade (lock->lck_id, status);
#else
			 			tdbb->tdbb_database->lockMgr.LOCK_downgrade (lock->lck_id, status);
#endif
		if (!lock->lck_compatible)
			lock->lck_physical = lock->lck_logical = level;
		}

	if (lock->lck_physical == LCK_none)
		lock->lck_id = lock->lck_data = 0;

	fb_assert(LCK_CHECK_LOCK(lock));
	
	return TRUE;
}
#endif


void LCK_fini(thread_db* tdbb, enum lck_owner_t owner_type)
{
/**************************************
 *
 *	L C K _ f i n i
 *
 **************************************
 *
 * Functional description
 *	Check out with lock manager.
 *
 **************************************/
	SLONG *owner_handle_ptr;
	//SET_TDBB(tdbb);
	DBB dbb = tdbb->tdbb_database;
	Attachment* attachment = tdbb->tdbb_attachment;

	switch (owner_type) 
		{
		case LCK_OWNER_process:
			owner_handle_ptr = &(LCK_OWNER_HANDLE_PROCESS);
			break;

		case LCK_OWNER_database:
			owner_handle_ptr = &(LCK_OWNER_HANDLE_DBB);
			break;

		case LCK_OWNER_attachment:
			owner_handle_ptr = &(LCK_OWNER_HANDLE_ATT);
			break;

		default:
			bug_lck("Invalid lock owner type in LCK_fini ()");
			break;
		}

#ifdef SHARED_CACHE
	LockMgr::LOCK_fini(tdbb->tdbb_status_vector, owner_handle_ptr);
#else
	tdbb->tdbb_database->lockMgr.LOCK_fini(tdbb->tdbb_status_vector, owner_handle_ptr);
#endif
}


SLONG LCK_get_owner_handle(thread_db* tdbb, enum lck_t lock_type)
//SLONG LCK_get_owner_handle()
{
/**************************************
 *
 *	L C K _ g e t _ l o c k _ o w n e r
 *
 **************************************
 *
 * Functional description
 *	return the right kind of lock owner given a lock type.
 *
 **************************************/

	DBB dbb = tdbb->tdbb_database;
	Attachment* attachment = tdbb->tdbb_attachment;

	switch (lock_type) 
		{
		case LCK_database:
		case LCK_bdb:
		case LCK_rel_exist:
		case LCK_idx_exist:
		case LCK_shadow:
		case LCK_retaining:
		case LCK_expression:
		case LCK_record_locking:
		case LCK_prc_exist:
		case LCK_range_relation:
		case LCK_backup_state:
		case LCK_backup_alloc:
		case LCK_backup_database:
			return LCK_OWNER_HANDLE_DBB;
			
		case LCK_attachment:
		case LCK_relation:
		case LCK_file_extend:
		case LCK_tra:
		case LCK_sweep:
		case LCK_record:
		case LCK_update_shadow:
			return LCK_OWNER_HANDLE_ATT;
			
		default:
			bug_lck("Invalid lock type in LCK_get_owner_handle ()");
			// Not Reached - bug_lck calls ERR_post
			return 0;
		}

	//return process_lck_owner_handle;
}


void LCK_init(thread_db* tdbb, enum lck_owner_t owner_type)
{
/**************************************
 *
 *	L C K _ i n i t
 *
 **************************************
 *
 * Functional description
 *	Initialize the locking stuff for the given owner.
 *
 **************************************/

	DBB dbb = tdbb->tdbb_database;
	Attachment* attachment = tdbb->tdbb_attachment;
	long owner_id;
	SLONG *owner_handle_ptr;

	switch (owner_type) 
		{
		case LCK_OWNER_process:
			owner_id = LCK_OWNER_ID_PROCESS;
			owner_handle_ptr = &(LCK_OWNER_HANDLE_PROCESS);
			break;

		case LCK_OWNER_database:
			owner_id = LCK_OWNER_ID_DBB;
			owner_handle_ptr = &(LCK_OWNER_HANDLE_DBB);
			break;

		case LCK_OWNER_attachment:
			owner_id = LCK_OWNER_ID_ATT;
			owner_handle_ptr = &(LCK_OWNER_HANDLE_ATT);
			break;

		default:
			bug_lck("Invalid lock owner type in LCK_init ()");
			break;
		}

#ifdef SHARED_CACHE
	if (LockMgr::LOCK_init(tdbb->tdbb_status_vector, dbb->configuration, 
#else
	if (tdbb->tdbb_database->lockMgr.LOCK_init(dbb, tdbb->tdbb_status_vector, dbb->configuration, 
#endif
						   owner_id, owner_type, owner_handle_ptr)) 
		{
		if (tdbb->tdbb_status_vector[1] == isc_lockmanerr)
			dbb->dbb_flags |= DBB_bugcheck;
		ERR_punt();
		}
}


int LCK_lock(thread_db* tdbb, Lock* lock, USHORT level, SSHORT wait)
{
/**************************************
 *
 *	L C K _ l o c k
 *
 **************************************
 *
 * Functional description
 *	Lock a block.  There had better not have been a lock there.
 *
 **************************************/
#ifdef SHARED_CACHE
	Sync sync(&lock->syncObject, "LCK_lock");
	sync.lock(Exclusive);
#endif

	fb_assert(LCK_CHECK_LOCK(lock));

	DBB dbb = lock->lck_dbb;
	ISC_STATUS *status = tdbb->tdbb_status_vector;
	lock->lck_blocked_threads = NULL;
	
	if (!lock->lck_long_lock)
	    lock->lck_next = lock->lck_prior = NULL;
	    
	if (lock->lck_attachment != tdbb->tdbb_attachment)
		{
		if (lock->lck_attachment && lock->lck_long_lock)
			lock->lck_attachment->removeLongLock(lock);

		if (tdbb->tdbb_attachment)
			tdbb->tdbb_attachment->addLongLock(lock);
		}
		
	lock->lck_attachment = tdbb->tdbb_attachment;

	if (lock->lck_compatible)
		internal_enqueue (tdbb, lock, level, wait, FALSE);
	else 
		enqueue (tdbb, lock, level, wait);
		
	fb_assert(LCK_CHECK_LOCK(lock));
	
	if (!lock->lck_id)
		{
		if (lock->lck_attachment && lock->lck_long_lock) 
			lock->lck_attachment->removeLongLock(lock);
			
		if (!wait || status[1] == isc_deadlock || 
			 status[1] == isc_lock_conflict || status[1] == isc_lock_timeout) 
			return FALSE;
			
		if (status[1] == isc_lockmanerr)
			dbb->dbb_flags |= DBB_bugcheck;
			
		ERR_punt();
		}

	if (!lock->lck_compatible)
		lock->lck_physical = lock->lck_logical = level;

	fb_assert(LCK_CHECK_LOCK(lock));
	
	return TRUE;
}


int LCK_lock_non_blocking(thread_db* tdbb, Lock* lock, USHORT level, SSHORT wait)
{
/**************************************
 *
 *	L C K _ l o c k _ n o n _ b l o c k i n g
 *
 **************************************
 *
 * Functional description
 *	Lock an object in a manner that allows other 
 *	threads to be scheduled.
 *
 **************************************/

	/* Don't bother for the non-wait or non-multi-threading case */

	if (!wait)	// || !gds__thread_enable(FALSE))
		return LCK_lock(tdbb, lock, level, wait);

#ifdef SHARED_CACHE
	Sync sync(&lock->syncObject, "LCK_lock_non_blocking");
	sync.lock(Exclusive);
#endif

	//LockMgr::validate();
	fb_assert(LCK_CHECK_LOCK(lock));
	//SET_TDBB(tdbb);

	DBB dbb = lock->lck_dbb;
	Attachment* attachment = tdbb->tdbb_attachment;

	if (!lock->lck_long_lock)
		lock->lck_next = lock->lck_prior = NULL;
	if (lock->lck_attachment != tdbb->tdbb_attachment)
		{
		if (lock->lck_attachment && lock->lck_long_lock)
			lock->lck_attachment->removeLongLock(lock);

		if (tdbb->tdbb_attachment)
			tdbb->tdbb_attachment->addLongLock(lock);
		}
	lock->lck_attachment = tdbb->tdbb_attachment;


	/* Make sure we're not about to wait for ourselves */

	lock->lck_blocked_threads = NULL;
	check_lock(lock, level);
	ISC_STATUS *status = tdbb->tdbb_status_vector;
	INIT_STATUS(status);

	//ENQUEUE(lock, level, wait);
	if (lock->lck_compatible)
		internal_enqueue (tdbb, lock, level, wait, FALSE);
	else 
		enqueue (tdbb, lock, level, wait);
		
	fb_assert(LCK_CHECK_LOCK(lock));
	
	/* If lock was rejected, there's trouble */

	if (!lock->lck_id)
		{
		if (lock->lck_attachment)
			{
			if (lock->lck_long_lock) lock->lck_attachment->removeLongLock(lock);
			}
		if (status[1] == isc_deadlock ||
			status[1] == isc_lock_conflict ||
			status[1] == isc_lock_timeout) 
			return FALSE;
			
		if (status[1] == isc_lockmanerr)
			throw OSRIBugcheck(status);
			
		throw OSRIException(status);
		}

	if (!lock->lck_compatible)
		lock->lck_physical = lock->lck_logical = level;

	/* Link into list of two phased locks */

	/***
	lock->lck_next = attachment->att_long_locks;
	lock->lck_prior = NULL;
	attachment->att_long_locks = lock;
	Lock* next = lock->lck_next;

	if (next)
		next->lck_prior = lock;
	***/

	//attachment->addLongLock (lock);
	
	return TRUE;
}


int LCK_lock_opt(thread_db* tdbb, Lock* lock, USHORT level, SSHORT wait)
{
/**************************************
 *
 *	L C K _ l o c k _ o p t
 *
 **************************************
 *
 * Functional description
 *	Assert a lock if the parent is not locked in exclusive mode.
 *
 **************************************/

#ifdef SHARED_CACHE
	Sync sync(&lock->syncObject, "LCK_lock_opt");
	sync.lock(Exclusive);
#endif

	//SET_TDBB(tdbb);
	fb_assert(LCK_CHECK_LOCK(lock));
	lock->lck_logical = level;
	DBB dbb = lock->lck_dbb;

	if (dbb->dbb_ast_flags & DBB_assert_locks) 
		{
		lock->lck_logical = LCK_none;
		return LCK_lock(tdbb, lock, level, wait);
		}

	fb_assert(LCK_CHECK_LOCK(lock));
	
	return TRUE;
}


#ifndef VMS
SLONG LCK_query_data(Lock* parent, enum lck_t lock_type, USHORT aggregate)
{
/**************************************
 *
 *	L C K _ q u e r y _ d a t a
 *
 **************************************
 *
 * Functional description
 *	Perform aggregate operations on data associated
 *	with a lock series for a lock hierarchy rooted
 *	at a parent lock.
 *
 **************************************/

	fb_assert(LCK_CHECK_LOCK(parent));
#ifdef SHARED_CACHE
	return LockMgr::LOCK_query_data(parent->lck_id, lock_type, aggregate);
#else
	return parent->lck_dbb->lockMgr.LOCK_query_data(parent->lck_id, lock_type, aggregate);
#endif
}
#endif


SLONG LCK_read_data(Lock* lock)
{
/**************************************
 *
 *	L C K _ r e a d _ d a t a
 *
 **************************************
 *
 * Functional description
 *	Read the data associated with a lock.
 *
 **************************************/

	fb_assert(LCK_CHECK_LOCK(lock));
	
#ifdef SHARED_CACHE
	Sync sync(&lock->syncObject, "LCK_read_data");
	sync.lock(Shared);
#endif
	
#ifdef VMS
	if (!LCK_lock(NULL, lock, LCK_null, LCK_NO_WAIT))
		return 0;

#ifdef SHARED_CACHE
	SLONG data = LockMgr::LOCK_read_data(lock->lck_id);
#else
	SLONG data = tdbb->tdbb_database->lockMgr.LOCK_read_data(lock->lck_id);
#endif
	LCK_release(lock);
#else
	Lock* parent = lock->lck_parent;
#ifdef SHARED_CACHE
	SLONG data = LockMgr::LOCK_read_data2(
#else
	SLONG data = lock->lck_dbb->lockMgr.LOCK_read_data2(
#endif
						   (parent) ? parent->lck_id : 0,
						   lock->lck_type,
						   (UCHAR*) &lock->lck_key,
						   lock->lck_length, lock->lck_owner_handle);
#endif

	fb_assert(LCK_CHECK_LOCK(lock));
	return data;
}


void LCK_release(Lock* lock)
{
/**************************************
 *
 *	L C K _ r e l e a s e
 *
 **************************************
 *
 * Functional description
 *	Release an existing lock.
 *
 **************************************/

	//SET_TDBB(tdbb);
#ifdef SHARED_CACHE
	Sync sync(&lock->syncObject, "LCK_release");
	sync.lock(Exclusive);
#endif

	fb_assert(LCK_CHECK_LOCK(lock));
	ISC_STATUS statusVector [20];
	
	if (lock->lck_physical != LCK_none) 
		if (lock->lck_compatible)
			internal_dequeue (statusVector, lock);
		else 
#ifdef SHARED_CACHE
			LockMgr::LOCK_deq (lock->lck_id);
#else
			lock->lck_dbb->lockMgr.LOCK_deq (lock->lck_id);
#endif

	lock->lck_physical = lock->lck_logical = LCK_none;
	lock->lck_id = lock->lck_data = 0;
	Attachment* attachment = lock->lck_attachment;

	if (lock->lck_long_lock && attachment)
		attachment->removeLongLock(lock);

	/***
	Lock* next = lock->lck_next;
	Lock* prior = lock->lck_prior;

	if (prior) 
		{
		fb_assert(prior->lck_next == lock);
		prior->lck_next = next;
		}
	else 
		{
		Attachment *attachment = lock->lck_attachment;
		if (attachment && attachment->att_long_locks == lock)
			attachment->att_long_locks = lock->lck_next;
		}

	if (next) 
		{
		fb_assert(next->lck_prior == lock);
		next->lck_prior = prior;
		}
	***/
	
	if (lock->lck_blocked_threads)
		JRD_unblock(&lock->lck_blocked_threads);

	fb_assert(LCK_CHECK_LOCK(lock));
}


void LCK_re_post(Lock* lock)
{
/**************************************
 *
 *	L C K _ r e _ p o s t
 *
 **************************************
 *
 * Functional description
 *	Re-post an ast when the original 
 *	deliver resulted in blockage.
 *
 **************************************/

	fb_assert(LCK_CHECK_LOCK(lock));
	
	if (lock->lck_compatible) 
		{
		if (lock->lck_ast) 
			(*lock->lck_ast)(lock->lck_object);
		return;
		}

#ifdef SHARED_CACHE
	LockMgr::LOCK_re_post(lock->lck_ast, lock->lck_object, lock->lck_owner_handle);
#else
	lock->lck_dbb->lockMgr.LOCK_re_post(lock->lck_ast, lock->lck_object, lock->lck_owner_handle);
#endif
	fb_assert(LCK_CHECK_LOCK(lock));
}


void LCK_write_data(Lock* lock, SLONG data)
{
/**************************************
 *
 *	L C K _ w r i t e _ d a t a
 *
 **************************************
 *
 * Functional description
 *	Write a longword into an existing lock.
 *
 **************************************/

#ifdef SHARED_CACHE
	Sync sync(&lock->syncObject, "LCK_lock_write_data");
	sync.lock(Exclusive);
#endif

	fb_assert(LCK_CHECK_LOCK(lock));
#ifdef SHARED_CACHE
	LockMgr::LOCK_write_data(lock->lck_id, data);
#else
	lock->lck_dbb->lockMgr.LOCK_write_data(lock->lck_id, data);
#endif
	lock->lck_data = data;
	fb_assert(LCK_CHECK_LOCK(lock));
}


static void bug_lck(const TEXT* string)
{
/**************************************
 *
 *	b u g _ l c k
 *
 **************************************
 *
 * Functional description
 *	Log the bug message, initialize the status vector
 *	and get out.
 *
 **************************************/
	TEXT s[128];

	sprintf(s, "Fatal lock interface error: %s", string);
	gds__log(s);
	ERR_post(isc_db_corrupt, isc_arg_string, string, 0);
}

#ifdef MULTI_THREAD
static void check_lock(Lock* lock, USHORT level)
{
/**************************************
 *
 *	c h e c k _ l o c k
 *
 **************************************
 *
 * Functional description
 *	If a lock is already held by this process, 
 *	wait for it to be released before asking 
 *	the lock manager for it.
 *
 **************************************/

	fb_assert(LCK_CHECK_LOCK(lock));
	
	for (Lock* next; next = find_block(lock, level);)
		JRD_blocked(next->lck_attachment, &next->lck_blocked_threads);
}
#endif

#ifdef OBSOLETE
static BOOLEAN compatible(Lock* lock1, Lock* lock2, USHORT level2)
{
/**************************************
 *
 *	c o m p a t i b l e
 *
 **************************************
 *
 * Functional description
 *	Given two locks, and a desired level for the
 *	second lock, determine whether the two locks
 *	would be compatible.
 *
 **************************************/

	fb_assert(LCK_CHECK_LOCK(lock1));
	fb_assert(LCK_CHECK_LOCK(lock2));

/* if the locks have the same compatibility block,
   they are always compatible regardless of level */

	if (lock1->lck_compatible &&
		lock2->lck_compatible &&
		lock1->lck_compatible == lock2->lck_compatible) {
		/* check for a second level of compatibility as well:
		   if a second level was specified, the locks must 
		   also be compatible at the second level */

		if (!lock1->lck_compatible2 ||
			!lock2->lck_compatible2 ||
			lock1->lck_compatible2 == lock2->lck_compatible2) return TRUE;
	}

	if (COMPATIBLE(lock1->lck_logical, level2))
		return TRUE;

	return FALSE;
}
#endif

static void enqueue(thread_db* tdbb, Lock* lock, USHORT level, SSHORT wait)
{
/**************************************
 *
 *	e n q u e u e
 *
 **************************************
 *
 * Functional description
 *	Submit a lock to the lock manager.
 *
 **************************************/

	Lock* parent;
	fb_assert(LCK_CHECK_LOCK(lock));

#ifdef SHARED_CACHE
	lock->lck_id = LockMgr::LOCK_enq(lock->lck_id,
#else
	lock->lck_id = lock->lck_dbb->lockMgr.LOCK_enq(lock->lck_id,
#endif
							(parent = lock->lck_parent) ? parent->lck_id : 0,
							lock->lck_type,
							(UCHAR *) & lock->lck_key,
							lock->lck_length,
							level,
							lock->lck_ast,
							lock->lck_object, lock->lck_data, wait,
							tdbb->tdbb_status_vector, lock->lck_owner_handle);
							
	if (!lock->lck_id)
		lock->lck_physical = lock->lck_logical = LCK_none;
		
	fb_assert(LCK_CHECK_LOCK(lock));
}


static int external_ast(void *arg)
{
/**************************************
 *
 *	e x t e r n a l _ a s t
 *
 **************************************
 *
 * Functional description
 *	Deliver blocking asts to all locks identical to 
 *	the passed lock.  This routine is called when 
 *	we are blocking a lock from another process.
 *
 **************************************/
 
	Lock *lock = (Lock*) arg;
	fb_assert(LCK_CHECK_LOCK(lock));

	/* go through the list, saving the next lock in the list
	   in case the current one gets deleted in the ast */

	for (Lock* next, *match = hash_get_lock(lock, 0, 0); match; match = next) 
		{
		next = match->lck_identical;
		if (match->lck_ast)
			(*match->lck_ast)(match->lck_object);
		}
	
	return 0;
}



#ifdef MULTI_THREAD
static Lock* find_block(Lock* lock, USHORT level)
{
/**************************************
 *
 *	f i n d _ b l o c k
 *
 **************************************
 *
 * Functional description
 *	Given a prospective lock, see if the database already holds
 *	the lock for a different attachment.  If so, return the
 *	locking lock, otherwise return NULL.
 *
 **************************************/
	fb_assert(LCK_CHECK_LOCK(lock));
	DBB dbb = lock->lck_dbb;
	Attachment* attachment = lock->lck_attachment;

	if (!attachment)
		return NULL;

	return attachment->findBlock (lock, level);
	
	#ifdef OBSOLETE
	//SCHAR *end = lock->lck_key.lck_string + lock->lck_length;
	
	for (Lock* next = attachment->att_long_locks; next; next = next->lck_next)
		/***
		if (lock->lck_type == next->lck_type &&
			lock->lck_parent && next->lck_parent &&
			(lock->lck_parent->lck_id == next->lck_parent->lck_id) &&
			lock->lck_length == next->lck_length &&
			lock->lck_attachment != next->lck_attachment &&
			!next->compatible(lock,level))
			//!compatible(next, lock, level)) 
			{
			for (SCHAR *p1 = lock->lck_key.lck_string, *p2 = next->lck_key.lck_string;
				 p1 < end && *p1++ == *p2++;)
				 ;
			if (p1[-1] == p2[-1])
				return next;
			}
		***/
		if (lock->lck_attachment != next->lck_attachment &&
			 next->equiv(lock) && !next->compatible(lock, level))
			return next;

	return NULL;
	#endif
}
#endif


static USHORT hash_func(UCHAR * value, USHORT length)
{
/**************************************
 *
 *	h a s h
 *
 **************************************
 *
 * Functional description
 *	Provide a repeatable hash value based
 *	on the passed key.
 *
 **************************************/
	//ULONG hash_value;
	//UCHAR *p, *q;
	//USHORT l;

	/* Hash the value, preserving its distribution 
	   as much as possible */

	ULONG hash_value = 0;
	USHORT l = 0;
	
	for (UCHAR *p, *q = value; l < length; l++) 
		{
		if (!(l & 3))
			p = (UCHAR*) &hash_value;
		*p++ = *q++;
		}

	return (USHORT) (hash_value % LOCK_HASH_SIZE);
}


static void hash_allocate(Lock* lock)
{
/**************************************
 *
 *	h a s h _ a l l o c a t e
 *
 **************************************
 *
 * Functional description
 *	Allocate the hash table for handling
 *	compatible locks.
 *
 **************************************/

	fb_assert(LCK_CHECK_LOCK(lock));
	DBB dbb = lock->lck_dbb;
	Attachment* att = lock->lck_attachment;

	if (att) 
		att->att_compatibility_table = vec::newVector(*dbb->dbb_permanent, LOCK_HASH_SIZE);
}


static Lock* hash_get_lock(Lock* lock, USHORT * hash_slot, Lock* ** prior)
{
/**************************************
 *
 *	h a s h _ g e t _ l o c k
 *
 **************************************
 *
 * Functional description
 *	Return the first matching identical 
 *	lock to the passed lock.  To minimize
 *	code for searching through the hash 
 *	table, return hash_slot or prior lock
 *	if requested.
 *
 **************************************/
	Lock *match, *collision;
	SCHAR *p, *q;
	SSHORT l;

	fb_assert(LCK_CHECK_LOCK(lock));
	Attachment* att = lock->lck_attachment;

	if (!att)
		return NULL;

	if (!att->att_compatibility_table)
		hash_allocate(lock);

	USHORT hash_value = hash_func((UCHAR *) & lock->lck_key, lock->lck_length);
	
	if (hash_slot)
		*hash_slot = hash_value;

	/* if no collisions found, we're done */

	if (!(match = (Lock*) (*att->att_compatibility_table)[hash_value]))
		return NULL;
		
	if (prior)
		*prior =(Lock**) &(*att->att_compatibility_table)[hash_value];

	/* look for an identical lock */

	fb_assert(LCK_CHECK_LOCK(match));
	
	for (collision = match; collision; collision = collision->lck_collision) 
		{
		fb_assert(LCK_CHECK_LOCK(collision));
		
		if (collision->lck_parent && lock->lck_parent &&
			collision->lck_parent->lck_id == lock->lck_parent->lck_id &&
			collision->lck_type == lock->lck_type &&
			collision->lck_length == lock->lck_length) {
			/* check that the keys are the same */

			p = lock->lck_key.lck_string;
			q = collision->lck_key.lck_string;
			
			for (l = lock->lck_length; l; l--)
				if (*p++ != *q++)
					break;
			if (!l)
				return collision;
		}

		if (prior)
			*prior = &collision->lck_collision;
	}

	return NULL;
}


static void hash_insert_lock(Lock* lock)
{
/**************************************
 *
 *	h a s h _ i n s e r t _ l o c k
 *
 **************************************
 *
 * Functional description
 *	Insert the provided lock into the 
 *	compatibility lock table.
 *
 **************************************/
	Lock* identical;
	USHORT hash_slot;
	Attachment* att;

	fb_assert(LCK_CHECK_LOCK(lock));

	if (!(att = lock->lck_attachment))
		return;

/* if no identical is returned, place it in the collision list */

	if (!(identical = hash_get_lock(lock, &hash_slot, 0))) {
		lock->lck_collision =
			(Lock*) (*att->att_compatibility_table)[hash_slot];
		(*att->att_compatibility_table)[hash_slot] = (BLK) lock;
		return;
	}

/* place it second in the list, out of pure laziness */

	lock->lck_identical = identical->lck_identical;
	identical->lck_identical = lock;
}


static BOOLEAN hash_remove_lock(Lock* lock, Lock* * match)
{
/**************************************
 *
 *	h a s h _ r e m o v e _ l o c k
 *
 **************************************
 *
 * Functional description
 *	Remove the passed lock from the hash table.
 *	Return TRUE if this is the last such identical 
 *	lock removed.   Also return the first matching 
 *	locking found.
 *
 **************************************/
	Lock *next, *last, **prior;

	fb_assert(LCK_CHECK_LOCK(lock));

	if (!(next = hash_get_lock(lock, 0, &prior))) 
		{
		/* set lck_compatible to NULL to make sure we don't
		   try to release the lock again in bugchecking */

		lock->lck_compatible = NULL;
		BUGCHECK(285);			/* lock not found in internal lock manager */
		}

	if (match)
		*match = next;

	/* special case if our lock is the first one in the identical list */

	if (next == lock)
		if (lock->lck_identical) 
			{
			lock->lck_identical->lck_collision = lock->lck_collision;
			*prior = lock->lck_identical;
			return FALSE;
			}
		else 
			{
			*prior = lock->lck_collision;
			return TRUE;
			}

	for (; next; last = next, next = next->lck_identical)
		if (next == lock)
			break;

	if (!next) 
		{
		lock->lck_compatible = NULL;
		BUGCHECK(285);			/* lock not found in internal lock manager */
		}

	last->lck_identical = next->lck_identical;
	
	return FALSE;
}


static void internal_ast(Lock* lock)
{
/**************************************
 *
 *	i n t e r n a l _ a s t
 *
 **************************************
 *
 * Functional description
 *	Deliver blocking asts to all locks identical to 
 *	the passed lock.  This routine is called to downgrade
 *	all other locks in the same process which do not have
 *	the lck_compatible field set.  
 *	Note that if this field were set, the internal lock manager 
 *	should not be able to generate a lock request which blocks 
 *	on our own process.
 *
 **************************************/
	//Lock* match, next;

	fb_assert(LCK_CHECK_LOCK(lock));

	/* go through the list, saving the next lock in the list
	   in case the current one gets deleted in the ast */

	for (Lock* next, *match = hash_get_lock(lock, 0, 0); match; match = next) 
		{
		next = match->lck_identical;

		/* don't deliver the ast to any locks which are already compatible */

		if ((match != lock) && !match->compatible(lock, lock->lck_logical) && match->lck_ast)
			(*match->lck_ast)(match->lck_object);
		}
}



static BOOLEAN internal_compatible(Lock* match, Lock* lock, USHORT level)
{
/**************************************
 *
 *	i n t e r n a l _ c o m p a t i b l e
 *
 **************************************
 *
 * Functional description
 *	See if there are any incompatible locks
 *	in the list of locks held by this process.
 *	If there are none, return TRUE to indicate 
 *	that the lock is compatible.
 *
 **************************************/
	Lock* next;

	fb_assert(LCK_CHECK_LOCK(match));
	fb_assert(LCK_CHECK_LOCK(lock));

	/* first check if there are any locks which are 
	   incompatible which do not have blocking asts;
	   if so, there is no chance of getting a compatible 
	   lock */

	for (next = match; next; next = next->lck_identical)
		if (!next->lck_ast && !next->compatible(lock, level))
			return FALSE;

	/* now deliver the blocking asts, attempting to gain
	   compatibility by getting everybody to downgrade */

	internal_ast(match);

	/* make one more pass to see if all locks were downgraded */

	for (next = match; next; next = next->lck_identical)
		if (!next->compatible(match, level))
			return FALSE;

	return TRUE;
}


static void internal_dequeue(ISC_STATUS *statusVector, Lock* lock)
{
/**************************************
 *
 *	i n t e r n a l _ d e q u e u e
 *
 **************************************
 *
 * Functional description
 *	Dequeue a lock.  If there are identical 
 *	compatible locks, check to see whether 
 *	the lock needs to be downgraded.
 *
 **************************************/
	Lock* match;

	//SET_TDBB(tdbb);
	fb_assert(LCK_CHECK_LOCK(lock));
	fb_assert(lock->lck_compatible);

	/* if this is the last identical lock in the hash table, release it */

	if (hash_remove_lock(lock, &match)) 
		{
#ifdef SHARED_CACHE
		if (!LockMgr::LOCK_deq(lock->lck_id))
#else
		if (!lock->lck_dbb->lockMgr.LOCK_deq(lock->lck_id))
#endif
			bug_lck("LOCK_deq() failed in lck:internal_dequeue");
			
		lock->lck_id = 0;
		lock->lck_physical = lock->lck_logical = LCK_none;
		return;
		}

	/* check for a potential downgrade */

	internal_downgrade(statusVector, match);
}


static USHORT internal_downgrade(ISC_STATUS *statusVector, Lock* first)
{
/**************************************
 *
 *	i n t e r n a l _ d o w n g r a d e 
 *
 **************************************
 *
 * Functional description
 *	Set the physical lock value of all locks identical
 *	to the passed lock.  It should be the same as the
 *	highest logical level.
 *
 **************************************/
	Lock* lock;
	USHORT level = LCK_none;

	//SET_TDBB(tdbb);

	fb_assert(LCK_CHECK_LOCK(first));
	fb_assert(first->lck_compatible);

	/* find the highest required lock level */

	for (lock = first; lock; lock = lock->lck_identical)
		level = MAX(level, lock->lck_logical);

	/* if we can convert to that level, set all identical 
	   locks as having that level */

	if ((level < first->lck_physical) &&
#ifdef SHARED_CACHE
		LockMgr::LOCK_convert(first->lck_id,
#else
		first->lck_dbb->lockMgr.LOCK_convert(first->lck_id,
#endif
					 level,
					 LCK_NO_WAIT,
					 external_ast,
					 (int *) first, 
					 statusVector)) 
		{
		for (lock = first; lock; lock = lock->lck_identical)
			lock->lck_physical = level;
			
		return level;
		}

	return first->lck_physical;
}


static BOOLEAN internal_enqueue(
								thread_db* tdbb,
								Lock* lock,
								USHORT level,
								SSHORT wait, BOOLEAN convert_flg)
{
/**************************************
 *
 *	i n t e r n a l _ e n q u e u e
 *
 **************************************
 *
 * Functional description
 *	See if there is a compatible lock already held
 *	by this process; if not, go ahead and submit the
 *	lock to the real lock manager.
 *	NOTE: This routine handles both enqueueing
 *	and converting existing locks, since the convert
 *	will find itself in the hash table and convert 
 *	itself upward.
 *
 **************************************/
	Lock *match, *update;
	ISC_STATUS *status;

	fb_assert(LCK_CHECK_LOCK(lock));
	fb_assert(lock->lck_compatible);

	//SET_TDBB(tdbb);
	status = tdbb->tdbb_status_vector;

	/* look for an identical lock */

	if ( (match = hash_get_lock(lock, 0, 0)) ) 
		{
		/* if there are incompatible locks for which
		   there are no blocking asts defined, give up */

		if (!internal_compatible(match, lock, level)) 
			{
			/* for now return a lock conflict; it would be better if we were to 
			   do a wait on the other lock by setting some flag bit or some such */

			status[0] = isc_arg_gds;
			status[1] = isc_lock_conflict;
			status[2] = isc_arg_end;
			return FALSE;
			}

		/* if there is still an identical lock,
		   convert the lock, otherwise fall
		   through and enqueue a new one */

		if ( (match = hash_get_lock(lock, 0, 0)) ) 
			{
			/* if a conversion is necessary, update all identical 
			   locks to reflect the new physical lock level */

			if (level > match->lck_physical) 
				{
#ifdef SHARED_CACHE
				if (!LockMgr::LOCK_convert(match->lck_id,
#else
				if (!lock->lck_dbb->lockMgr.LOCK_convert(match->lck_id,
#endif
								  level,
								  wait,
								  external_ast,
								  (int *) lock, status))
					  return FALSE;
				for (update = match; update; update = update->lck_identical)
					update->lck_physical = level;
				}

			lock->lck_id = match->lck_id;
			lock->lck_logical = level;
			lock->lck_physical = match->lck_physical;

			/* When converting a lock (from the callers point of view), 
			   then no new lock needs to be inserted. */

			if (!convert_flg)
				hash_insert_lock(lock);

			return TRUE;
			}
		}

	/* enqueue the lock, but swap out the ast and the ast argument
	  with the local ast handler, passing it the lock block itself */

#ifdef SHARED_CACHE
	lock->lck_id = LockMgr::LOCK_enq(lock->lck_id,
#else
	lock->lck_id = lock->lck_dbb->lockMgr.LOCK_enq(lock->lck_id,
#endif
							lock->lck_parent ? lock->lck_parent->lck_id : 0,
							lock->lck_type,
							(UCHAR *) & lock->lck_key,
							lock->lck_length,
							level,
							external_ast,
							(int *)lock,
							lock->lck_data,
							wait, status, lock->lck_owner_handle);

	/* If the lock exchange failed, set the lock levels appropriately */
	
	if (lock->lck_id == 0)
		lock->lck_physical = lock->lck_logical = LCK_none;

	fb_assert(LCK_CHECK_LOCK(lock));

	if (lock->lck_id) 
		{
		hash_insert_lock(lock);
		lock->lck_logical = lock->lck_physical = level;
		}

	fb_assert(LCK_CHECK_LOCK(lock));
	
	return lock->lck_id ? TRUE : FALSE;
}



bool Lock::equiv(Lock* lock)
{
	if (lck_type != lock->lck_type ||
		!lck_parent || !lock->lck_parent ||
		lck_parent->lck_id != lock->lck_parent->lck_id ||
		lck_length != lock->lck_length)
		return false;

	return memcmp(lck_key.lck_string, lock->lck_key.lck_string, lock->lck_length) == 0;
}

bool Lock::compatible(Lock* lock, int level)
{
	fb_assert(LCK_CHECK_LOCK(this));
	fb_assert(LCK_CHECK_LOCK(lock));

	/* if the locks have the same compatibility block,
	   they are always compatible regardless of level */

	if (lck_compatible && lock->lck_compatible && lck_compatible == lock->lck_compatible) 
		{
		/* check for a second level of compatibility as well:
		   if a second level was specified, the locks must 
		   also be compatible at the second level */

		if (!lck_compatible2 ||!lock->lck_compatible2 || lck_compatible2 == lock->lck_compatible2) 
			return TRUE;
		}

	if (COMPATIBLE(lck_logical, level))
		return TRUE;

	return FALSE;
}

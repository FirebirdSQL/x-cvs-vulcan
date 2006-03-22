/*
 *	PROGRAM:	JRD Lock Manager
 *	MODULE:		LockMgr.cpp
 *	DESCRIPTION:	Generic ISC Lock Manager
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
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "IMP" port
 *
 * 2002.10.27 Sean Leyne - Completed removal of obsolete "DELTA" port
 * 2002.10.27 Sean Leyne - Completed removal of obsolete "IMP" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 * 2003.03.24 Nickolay Samofatov
 *	- cleanup #define's,
 *  - shutdown blocking thread cleanly on Windows CS
 *  - fix Windows CS lock-ups (make wakeup event manual-reset)
 *  - detect deadlocks instantly in most cases (if blocking owner 
 *     dies during AST processing deadlock scan timeout still applies)
 * 2003.04.29 Nickolay Samofatov - fix broken lock table resizing code in CS builds
 * 2003.08.11 Nickolay Samofatov - finally and correctly fix Windows CS lock-ups. 
 *            Roll back earlier workarounds on this subject.
 *
 */

/*
$Id$
*/

#include <stdarg.h>
#include "fbdev.h"
#include "../jrd/jrd_time.h"
#include "../jrd/ib_stdio.h"
#include "../jrd/common.h"
#include "../jrd/isc.h"
#include "LockMgr.h"
#include "gen/iberror.h"
#include "../jrd/gds_proto.h"
#include "../jrd/gdsassert.h"
#include "../jrd/isc_proto.h"
#include "../jrd/os/isc_i_proto.h"
#include "../jrd/isc_s_proto.h"
#include "../jrd/thd_proto.h"
#include <errno.h>
#include "Configuration.h"
#include "ConfObject.h"
#include "Parameters.h"
#include "AsyncEvent.h"
#include "Sync.h"
#include "Mutex.h"
#include "LockAcquire.h"
#include "Thread.h"
#include "Database.h"

#undef SUPERSERVER

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#ifndef INFINITE
#define INFINITE	-1
#endif

/***
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
***/

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef WIN_NT
#include <windows.h>
#include <process.h>
#define ERRNO		GetLastError()
#define MUTEX		lock_manager_mutex
#endif

#ifdef SHARED_CACHE
THREAD_ID LOCK_thread;
#endif

#ifdef DEV_BUILD
#define ASSERT_ACQUIRED current_is_active_owner (true,  __LINE__)
#define ASSERT_RELEASED current_is_active_owner (false, __LINE__)
//#define VALIDATE_LOCK_TABLE
//#define DEBUG_TRACE
#endif


#ifndef ASSERT_ACQUIRED
#define	ASSERT_ACQUIRED			/* nothing */
#endif

#ifndef ASSERT_RELEASED
#define	ASSERT_RELEASED			/* nothing */
#endif


#ifdef DEV_BUILD
#define CHECK(x)	{ if (!(x)) bug_assert ("consistency check", __LINE__); }
#endif

#ifndef CHECK
#define CHECK(x)				/* nothing */
#endif

#ifdef DEBUG
#define DEBUG_MANAGER "manager"
#define DEBUG_TRACE
#endif

//#define DEBUG_TRACE

#ifdef DEBUG_TRACE
#define LOCK_TRACE(x)	lockTrace x
//#define DUMP_LOCK_COUNTS(x)	dumpLockCounts(x, __LINE__)
//#define PRINT_QUE(x,y)	print_que(x,y,__LINE__)
#endif


#ifdef DEBUG
SSHORT LOCK_debug_level = 0;
#define DEBUG_MSG(l,x)	if ((l) <= LOCK_debug_level) { time_t t; time (&t); ib_printf ("%s", ctime(&t) ); ib_printf x ; ib_fflush (ib_stdout); gds__log x ; }
#endif

#ifndef DEBUG_MSG
#define DEBUG_MSG(l,x)			/* nothing */
#endif

#ifndef LOCK_TRACE
#define LOCK_TRACE(x)			/* nothing */
#endif
#ifndef DUMP_LOCK_COUNTS
#define DUMP_LOCK_COUNTS(x)		/* nothing */
#endif
#ifndef PRINT_QUE
#define PRINT_QUE(x,y)			/* nothing */
#endif

/* Debug delay is used to create nice big windows for signals or other
 * events to occur in - eg: slow down the code to try and make
 * timing race conditions show up
 */

#ifdef DEBUG
#define	DEBUG_DELAY	debug_delay (__LINE__)
#endif

#ifndef DEBUG_DELAY
#define DEBUG_DELAY				/* nothing */
#endif

#ifndef ERRNO
#define ERRNO		errno
#endif

#ifndef MUTEX
#if defined SHARED_CACHE || defined ONE_LOCK_TABLE
#define MUTEX		LOCK_table->lhb_mutex
#else
#define MUTEX		((Thread::getCurrentThreadId() == blocking_action_thread_id) ? \
						LOCK_table2->lhb_mutex : LOCK_table->lhb_mutex)
#endif
#endif


#ifdef hpux
/* can't map the same file to two locations on HP */
#undef HAS_MAP_OBJECT
#else
#define HAS_MAP_OBJECT 1
#endif


/***
#ifndef SV_INTERRUPT
#define SV_INTERRUPT	0
#endif
***/

#undef BASE
#define BASE                    ((UCHAR*) LOCK_header)
#define REL_PTR(item)           (PTR) ((UCHAR*) item - BASE)
#define ABS_PTR(item)           (BASE + item)


#define LOCK_BLOCK(ptr)		((LockBlock*) ABS_PTR(ptr))
#define LOCK_REQUEST(ptr)	((LockRequest*) ABS_PTR(ptr))
#define LOCK_OWNER(ptr)		((LockOwner*) ABS_PTR(ptr))
#define LOCK_EVENT(ptr)		((LockEvent*) ABS_PTR(ptr))

#define DUMMY_OWNER_CREATE		((PTR) -1)
#define DUMMY_OWNER_DELETE	(	(PTR) -2)
#define DUMMY_OWNER_SHUTDOWN	((PTR) -3)

#if 0
#if (defined HAVE_MMAP)
#define GET_OWNER(owner_offset)		LOCK_owner
#else
#define GET_OWNER(owner_offset)		(LOCK_OWNER(owner_offset))
#endif
#endif

#ifdef SHARED_CACHE
//static LockOwner	LOCK_process_owner;	/* Place holder */
static SSHORT		LOCK_bugcheck = 0;
static LHB volatile LOCK_header = NULL;
static LHB volatile LOCK_table = NULL;
static LHB volatile LOCK_table_original = NULL;
static PTR			LOCK_owner_offset = 0;
static OWN			LOCK_owner = 0;

static int			LOCK_pid = 0;
static int			LOCK_version = 0;
static SLONG		LOCK_shm_size;
static SLONG		LOCK_sem_count;
static SLONG		LOCK_solaris_stall;
static int			LOCK_trace_stop = 80;

static bool			shutdown;
static AsyncEvent	shutdownComplete;
static AsyncEvent	blockingThreadStarted;
static int			shutdownCount;
static int			startupCount;

//static SLONG		LOCK_block_signal;

static LRQ			debugRequest;

#ifdef WAKEUP_SIGNAL
//static SLONG		LOCK_wakeup_signal = 0;
#endif

static SLONG		LOCK_ordering;
static SLONG		LOCK_hash_slots;
static SLONG		LOCK_scan_interval;
static SH_MEM_T		LOCK_data;
//static TEXT		LOCK_bug_buffer[128];
static Mutex		lockManagerMutex;
static Mutex 		LOCK_table_init_mutex;

#ifdef WIN_NT
static MTX_T		lock_manager_mutex[1];
//static HANDLE		blocking_event[1];
//static HANDLE		wakeup_event[1];
#endif


#ifdef WIN_NT
static HANDLE	blocking_action_thread_handle;
#else
static pthread_t blocking_action_thread_handle;
#endif

static bool blocking_action_thread_initialized;
#endif


#define GET_TIME	time (NULL)

#define HASH_MIN_SLOTS	101
#define HASH_MAX_SLOTS	2048
#define HISTORY_BLOCKS	256
#define LOCKMANTIMEOUT	300

#define STARVATION_THRESHHOLD	500	/* acquires of lock table */
#define SOLARIS_MIN_STALL       0
#define SOLARIS_MAX_STALL       200
#define SOLARIS_MAX_STALL       200	/* Seconds */

#define OWN_BLOCK_new		1
#define OWN_BLOCK_reused	2
#define OWN_BLOCK_dummy		3

#ifndef LOCK_MANAGER
#define LOCK_MANAGER	"bin/vulcan_lock_mgr"
#endif

#define QUE_INIT(que)   {que.srq_forward = que.srq_backward = REL_PTR (&que);}
#define QUE_EMPTY(que)  (que.srq_forward == REL_PTR (&que))
#define QUE_NEXT(que)   (PSRQ) ABS_PTR (que.srq_forward)

#define SRQ_LOOP(header,que)    for (que = QUE_NEXT (header);\
	que != &header; que = QUE_NEXT ((*que)))

/* Lock block types */

#define type_null       0
#define type_lhb        1
#define type_prb        2
#define type_lrq        3
#define type_lbl        4
#define type_his        5
#define type_smb        6
#define type_shb        7
#define type_own        8

#define type_MAX        type_own

const int	LCK_OWNER_dummy_process = 255;
INTERLOCK_TYPE lock_file_count;

#ifndef SHARED_CACHE
#ifdef ONE_LOCK_TABLE
LHB			LockMgr::LOCK_table_static;
SH_MEM_T	LockMgr::LOCK_data;
Mutex 		LockMgr::LOCK_table_init_mutex;
#ifdef WIN_NT
MTX_T		LockMgr::lock_manager_mutex[1];
#endif
#endif
#endif


#if 0
int num_blocking_wakeups;
int num_long_waits;
int num_acquires;
extern int sync_locks, long_sync_locks;
#endif

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

#define COMPATIBLE(st1, st2)	compatibility [st1 * LCK_max + st2]

void dumpLockCounts(LBL lock, int line)
{
	int i;
	printf("line %d: lock %p\n", line, lock);
	for (i = 0; i < LCK_max; i++)
	{
		printf("count[%d] = %d\n", i, lock->lbl_counts[i]);
		if (lock->lbl_counts[i] == 2)
			printf("*********\n");
	}
	printf("\n");
}


LockMgr::LockMgr(void)
{
#ifndef SHARED_CACHE
	LOCK_header = NULL;
	LOCK_table = NULL;
	LOCK_bugcheck = 0;
	LOCK_owner_offset = 0;
	LOCK_owner = 0;

	LOCK_pid = 0;
	LOCK_version = 0;
	LOCK_shm_size;
	LOCK_sem_count;
	LOCK_solaris_stall;
	LOCK_trace_stop = 80;

	shutdown = false;
	shutdownCount = 0;

	debugRequest = 0;

	LOCK_ordering = 0;
	LOCK_hash_slots = 0;
	LOCK_scan_interval = 0;

#ifndef ONE_LOCK_TABLE
	memset(&LOCK_data, 0, sizeof(LOCK_data));
	memset(&LOCK_data2, 0, sizeof(LOCK_data2));
#ifdef POSIX_THREADS
	LOCK_data.sh_file_count = LOCK_data2.sh_file_count = &lock_file_count;
#endif
#endif
	
	blocking_action_thread_initialized = false;
	memset(&blocking_action_thread_id, 0xff, sizeof(blocking_action_thread_id));
#endif
}

LockMgr::~LockMgr(void)
{
#if 0
	printf("Num acquires = %d, Num long waits = %d, num blocking wakeups = %d\n", 
			num_acquires, num_long_waits, num_blocking_wakeups);
			
	printf("sync_locks = %d, long_sync_locks = %d\n", sync_locks, long_sync_locks);
#endif
#if !defined SHARED_CACHE && !defined ONE_LOCK_TABLE
	LOCK_owner_offset = 0;
	exit_handler();
#endif
	ISC_mutex_delete(MUTEX);
}

void LockMgr::LOCK_ast_inhibit() 
{
	AST_DISABLE;
}

void LockMgr::LOCK_ast_enable() 
{
	AST_ENABLE;
}

int LockMgr::LOCK_convert(PTR		request_offset,
				 UCHAR		type,
				 SSHORT		lck_wait,
				 lock_ast_t ast_routine,
				 void*		ast_argument,
				 ISC_STATUS*	status_vector)
{
/**************************************
 *
 *	L O C K _ c o n v e r t
 *
 **************************************
 *
 * Functional description
 *	Perform a lock conversion, if possible.
 *
 **************************************/
	LOCK_TRACE(("LOCK_convert owner: %ld (%d, %d)\n", request_offset, type, lck_wait));

	LRQ request = get_request(request_offset);
	acquire(request->lrq_owner);
	request = get_request(request_offset); /* in case of remap */
	OWN owner = LOCK_OWNER(request->lrq_owner);
	
	if (!owner->own_count)
		{
		release(request->lrq_owner);
		return FALSE;
		}

	owner = NULL;				/* remap */
	++LOCK_header->lhb_converts;
	request = LOCK_REQUEST(request_offset);	/* remap */
	LBL lock = LOCK_BLOCK(request->lrq_lock);
	
	if (lock->lbl_series < LCK_MAX_SERIES)
		++LOCK_header->lhb_operations[lock->lbl_series];
	else
		++LOCK_header->lhb_operations[0];

	int ret = convert(request_offset, type, lck_wait, ast_routine, ast_argument, status_vector);
	ASSERT_RELEASED;
	
	return ret;
}


int LockMgr::LOCK_deq( PTR request_offset)
{
/**************************************
 *
 *	L O C K _ d e q
 *
 **************************************
 *
 * Functional description
 *	Release an outstanding lock.
 *
 **************************************/
	LOCK_TRACE(("LOCK_deq (%ld)\n", request_offset));

	LRQ request = get_request(request_offset);
#ifdef SHARED_CACHE
	LockAcquire lockAcquire(request->lrq_owner);
#else
	LockAcquire lockAcquire(request->lrq_owner, this);
#endif
	request = get_request(request_offset); /* in case of remap */
	PTR owner_offset = request->lrq_owner;
	OWN owner = LOCK_OWNER(owner_offset);
	
	if (!owner->own_count)
		return FALSE;

	++LOCK_header->lhb_deqs;
	LBL lock = LOCK_BLOCK(request->lrq_lock);
	
	if (lock->lbl_series < LCK_MAX_SERIES)
		++LOCK_header->lhb_operations[lock->lbl_series];
	else
		++LOCK_header->lhb_operations[0];

	dequeue(request_offset);
	
	return TRUE;
}


UCHAR LockMgr::LOCK_downgrade(PTR request_offset, ISC_STATUS * status_vector)
{
/**************************************
 *
 *	L O C K _ d o w n g r a d e
 *
 **************************************
 *
 * Functional description
 *	Downgrade an existing lock returning
 *	its new state.
 *
 **************************************/
	LOCK_TRACE(("LOCK_downgrade (%ld)\n", request_offset));

	LRQ request = get_request(request_offset);
#ifdef SHARED_CACHE
	LockAcquire lockAcquire(request->lrq_owner);
#else
	LockAcquire lockAcquire(request->lrq_owner, this);
#endif
	request = get_request(request_offset); /* in case of remap */
	PTR owner_offset = request->lrq_owner;
	OWN owner = LOCK_OWNER(owner_offset);
	
	if (!owner->own_count)
		return FALSE;

	//acquire(owner_offset);
	owner = NULL;				/* remap */
	++LOCK_header->lhb_downgrades;

	request = LOCK_REQUEST(request_offset);	/* Re-init after a potential remap */
	LBL lock = LOCK_BLOCK(request->lrq_lock);
	UCHAR pending_state = LCK_none;

	/* Loop thru requests looking for pending conversions
	   and find the highest requested state */

	PSRQ que;

	SRQ_LOOP(lock->lbl_requests, que) 
		{
		LRQ pending = (LRQ) ((UCHAR *) que - OFFSET(LRQ, lrq_lbl_requests));
		
		if (pending->lrq_flags & LRQ_pending && pending != request) 
			{
			pending_state = MAX(pending->lrq_requested, pending_state);
			
			if (pending_state == LCK_EX)
				break;
			}
		}

	UCHAR state;
	
	for (state = request->lrq_state;  state > LCK_none && !COMPATIBLE(pending_state, state); --state)
		;

	if (state == LCK_none || state == LCK_null) 
		{
		dequeue(request_offset);
		//release(owner_offset);
		state = LCK_none;
		}
	else
		{
		convert(request_offset, state, FALSE,
				request->lrq_ast_routine, request->lrq_ast_argument,
				status_vector);
		lockAcquire.released();
		}

	return state;
}


SLONG LockMgr::LOCK_enq(	PTR		prior_request,
				PTR		parent_request,
				USHORT	series,
				UCHAR*	value,
				USHORT	length,
				UCHAR	type,
				lock_ast_t ast_routine,
				void*	ast_argument,
				SLONG	data,
				SSHORT	lck_wait,
				ISC_STATUS*	status_vector,
				PTR		owner_offset)
{
/**************************************
 *
 *	L O C K _ e n q
 *
 **************************************
 *
 * Functional description
 *	Enque on a lock.  If the lock can't be granted immediately,
 *	return an event count on which to wait.  If the lock can't
 *	be granted because of deadlock, return NULL.
 *
 **************************************/
	//LRQ request;
	PTR parent, request_offset;
	UCHAR *p;
	SLONG lock_id;
	USHORT hash_slot, *ps;
	SSHORT l;

	LOCK_TRACE(("LOCK_enq (%ld)\n", parent_request));
	
	if (!owner_offset)
		return 0;
		
#ifdef SHARED_CACHE
	LockAcquire lockAcquire(owner_offset);
#else
	LockAcquire lockAcquire(owner_offset, this);
#endif
	OWN owner = LOCK_OWNER(owner_offset);
	
	if (!owner->own_count)
		return 0;

	//acquire(owner_offset);
	owner = NULL;				/* remap */
	ASSERT_ACQUIRED;
	++LOCK_header->lhb_enqs;

#ifdef VALIDATE_LOCK_TABLE
	if ((LOCK_header->lhb_enqs % 50) == 0)
		validate_lhb(LOCK_header);
#endif

	if (prior_request)
		dequeue(prior_request);

	if (parent_request) 
		{
		LockRequest *request = get_request(parent_request);
		parent = request->lrq_lock;
		}
	else
		parent = 0;

	/* Allocate or reuse a lock request block */

	LockRequest *request = allocRequest();
	owner = LOCK_OWNER(owner_offset);	/* Re-init after a potential remap */
	post_history(his_enq, owner_offset, (PTR)0, REL_PTR(request), true);

	request->lrq_requested = type;
	request->lrq_owner = owner_offset;
	request->lrq_ast_routine = ast_routine;
	request->lrq_ast_argument = ast_argument;
	insert_tail(&owner->own_requests, &request->lrq_own_requests);
	//QUE_INIT(request->lrq_own_pending);		// this makes no sense

	/* See if the lock already exits */

	LBL lock = find_lock(parent, series, value, length, &hash_slot);
	
	if (lock)
		{
		if (series < LCK_MAX_SERIES) 
			++LOCK_header->lhb_operations[series];
		else 
			++LOCK_header->lhb_operations[0];

		insert_tail(&lock->lbl_requests, &request->lrq_lbl_requests);
		request->lrq_data = data;
		
		if (!(lock_id = grant_or_que(request, lock, lck_wait))) 
			{
			*status_vector++ = isc_arg_gds;
			*status_vector++ = (lck_wait > 0) ? isc_deadlock :
				((lck_wait < 0) ? isc_lock_timeout : isc_lock_conflict);
			*status_vector++ = isc_arg_end;
			}
		
		ASSERT_ACQUIRED;
		//ASSERT_RELEASED;
		//lockAcquire.released();
		
		return lock_id;
		}

	/* Lock doesn't exist.  Allocate lock block and set it up. */

	request_offset = REL_PTR(request);

	if (!(lock = alloc_lock(length, status_vector))) 
		{
		/* lock table is exhausted */
		/* release request gracefully */
		
		remove_que(&request->lrq_own_requests);
		request->lrq_type = type_null;
		insert_tail(&LOCK_header->lhb_free_requests, &request->lrq_lbl_requests);
		//release(owner_offset);
		
		return 0;
		}
		
	lock->lbl_state = type;
	lock->lbl_parent = parent;
	fb_assert(series <= MAX_UCHAR);
	lock->lbl_series = (UCHAR)series;

	/* Maintain lock series data queue */

	QUE_INIT(lock->lbl_lhb_data);
	
	if (lock->lbl_data = data)
		insert_data_que(lock);

	if (series < LCK_MAX_SERIES)
		++LOCK_header->lhb_operations[series];
	else
		++LOCK_header->lhb_operations[0];

	lock->lbl_flags = 0;
	lock->lbl_pending_lrq_count = 0;

	for (l = LCK_max, ps = lock->lbl_counts; l--;)
		*ps++ = 0;

	if (lock->lbl_length = length)
		{
		p = lock->lbl_key;
		do 
			*p++ = *value++; 
		while (--length);
		}

	request = LOCK_REQUEST(request_offset);

	QUE_INIT(lock->lbl_requests);
	ASSERT_ACQUIRED;
	insert_tail(&LOCK_header->lhb_hash[hash_slot], &lock->lbl_lhb_hash);
	insert_tail(&lock->lbl_requests, &request->lrq_lbl_requests);
	request->lrq_lock = REL_PTR(lock);
	grant(request, lock);
	lock_id = REL_PTR(request);
	//release(request->lrq_owner);

	return lock_id;
}


void LockMgr::LOCK_fini( ISC_STATUS *status_vector, PTR *owner_offset)
{
/**************************************
 *
 *	L O C K _ f i n i
 *
 **************************************
 *
 * Functional description
 *	Release the owner block and any outstanding locks.
 *	The exit handler will unmap the shared memory.
 *
 **************************************/
	LOCK_TRACE(("LOCK_fini(%ld)\n", *owner_offset));
	PTR offset = *owner_offset;
	
	if (!LOCK_table || !offset)
		return;

	if (startupCount)
		blockingThreadStarted.wait(startupCount, INFINITE);

	//if (LOCK_table->lhb_active_owner != offset)
		acquire(offset);

	OWN owner = LOCK_OWNER(offset);
	
	if (!owner->own_count)
		{
		release(offset);
		return;
		}

	if (--owner->own_count > 0)
		{
		release(offset);
		return;
		}

	if (LOCK_pid == owner->own_process_id)
		{
		shutdown_blocking_thread(owner);
		purge_owner(offset, owner);
		}
		
	LOCK_header->lhb_active_owner = DUMMY_OWNER_DELETE;
	int count = 0;
	PSRQ que;
	
	SRQ_LOOP(LOCK_header->lhb_owners, que)
		{
		OWN obj = (OWN) ((UCHAR*) que - OFFSET(OWN, own_lhb_owners));
		
		if (obj->own_process_id == LOCK_pid)
			++count;
		}
	
	if (count)
		{
		release(0);
		return;
		}
	

#if defined HAVE_MMAP

	if (LOCK_owner) 
		{
#ifdef HAS_MAP_OBJECT
		ISC_unmap_object(status_vector, &LOCK_data,(UCHAR**)&LOCK_owner, sizeof(LockOwner));
#endif
		LOCK_owner_offset = 0;
		}
#endif

	LOCK_owner = 0;

	/***
	if (LOCK_header->lhb_active_owner != offset)
		{
		acquire(offset);
		owner = LOCK_OWNER(offset);	// Re-init after a potential remap
		}

	if (LOCK_pid == owner->own_process_id)
		purge_owner(offset, owner);
	***/

	release_mutex();
	*owner_offset = (PTR)0;
}


#ifdef SHARED_CACHE
int LockMgr::LOCK_init(ISC_STATUS * status_vector,
#else
int LockMgr::LOCK_init(Database *dbb, ISC_STATUS * status_vector,
#endif
			  ConfObject *configuration, 
			  long owner_id, UCHAR owner_type, SLONG * owner_handle)
{
/**************************************
 *
 *	L O C K _ i n i t
 *
 **************************************
 *
 * Functional description
 *	Initialize lock manager for the given owner, if not already done.
 *
 *	Initialize an owner block in the lock manager, if not already
 *	initialized.
 *
 *	Return the offset of the owner block through owner_handle.
 *
 *	Return FB_SUCCESS or FB_FAILURE.
 *
 **************************************/
#ifdef SHARED_CACHE
	Sync sync (&lockManagerMutex, "LOCK_init");
	sync.lock (Exclusive);
#endif
	LOCK_TRACE(("LOCK_init (ownerid=%ld)\n", owner_id));

	/* If everything is already initialized, just bump the use count. */
	if (*owner_handle) 
		{
		acquire(*owner_handle);
		OWN owner = LOCK_OWNER(*owner_handle);
		owner->own_count++;
		release(*owner_handle);
		
		return FB_SUCCESS;
		}

#ifndef SHARED_CACHE
	LOCK_dbb = dbb;
#endif

#if defined ONE_LOCK_TABLE || defined SHARED_CACHE
	Sync sync2(&LOCK_table_init_mutex, "LOCK_init");
	sync2.lock(Exclusive);
#endif

	if (!LOCK_table) 
		{
		/* We haven't yet mapped the shared region.  Do so now. */

		if (init_lock_table(status_vector, configuration) != FB_SUCCESS)
			return FB_FAILURE;
		}

	acquire(DUMMY_OWNER_CREATE);	/* acquiring owner is being created */
	LockOwner *owner = create_owner(status_vector, owner_id, owner_type);
	
	if (!owner)
		return FB_FAILURE;

	LOCK_owner = owner;
	PTR ownerOffset = LOCK_owner_offset = *owner_handle = REL_PTR(owner);

	/* Initialize process level stuffs for different platforms.
	   This should be done after the call to create_owner() that 
	   initializes owner_handle. */


#if defined HAVE_MMAP

	/* Map the owner block separately so that threads waiting
	   on synchronization variables embedded in the owner block
	   don't have to coordinate during lock table unmapping. */
	   
#ifdef HAS_MAP_OBJECT
	if (LOCK_owner_offset &&
		!(LOCK_owner = (OWN) ISC_map_object(status_vector, &LOCK_data, LOCK_owner_offset, sizeof(LockOwner)))) 
		{
		release(*owner_handle);
		return FB_FAILURE;
		}
#else

	if (LOCK_owner_offset &&
		!(LOCK_owner = (OWN) (((char *)LOCK_table) +  sizeof(LockOwner)))) 
		{
		release(*owner_handle);
		return FB_FAILURE;
		}
	

#endif


#endif

	AST_ALLOC;
	
	if (!blocking_action_thread_initialized)
		{
		shutdown = false;
		shutdownComplete.init();
		shutdownCount = shutdownComplete.clear();
		LockEvent *event = allocEvent(owner);
		owner->own_blocking_event = REL_PTR(event);
		blockingThreadStarted.init();
		startupCount = blockingThreadStarted.clear();

#ifdef SHARED_CACHE
		ULONG status = THD_start_thread(blocking_action_thread, &LOCK_owner_offset, THREAD_high, 0, 
										&blocking_action_thread_handle);
#else
		ULONG status = THD_start_thread(blocking_action_thread, (void *)this, THREAD_high, 0, 
										&blocking_action_thread_handle);
#endif

		if (status) 
			{
			release(*owner_handle);
			*status_vector++ = isc_arg_gds;
			*status_vector++ = isc_lockmanerr;
			*status_vector++ = isc_arg_gds;
			*status_vector++ = isc_sys_request;
			*status_vector++ = isc_arg_string;
			*status_vector++ = (ISC_STATUS) "thr_create";
			*status_vector++ = isc_arg_unix;
			*status_vector++ = status;
			*status_vector++ = isc_arg_end;
			return FB_FAILURE;
			}
		
		blocking_action_thread_initialized = true;
		}
	else
		bug(NULL, "blocking thread already running");

	release(*owner_handle);

	return FB_SUCCESS;
}



SLONG LockMgr::LOCK_query_data(PTR parent_request, USHORT series, USHORT aggregate)
{
/**************************************
 *
 *	L O C K _ q u e r y _ d a t a
 *
 **************************************
 *
 * Functional description
 *	Query lock series data with respect to a rooted
 *	lock hierarchy calculating aggregates as we go.
 *
 **************************************/
	LRQ parent;
	PSRQ que;

	LOCK_TRACE(("LOCK_query_data (%ld)\n", parent_request));

	/* Get root of lock hierarchy */

	if (parent_request && series < LCK_MAX_SERIES)
		parent = get_request(parent_request);
	else 
		{
		CHECK(false);
		return 0;
		}

#ifdef SHARED_CACHE
	LockAcquire lockAcquire(parent->lrq_owner);
#else
	LockAcquire lockAcquire(parent->lrq_owner, this);
#endif
	parent = LOCK_REQUEST(parent_request);	/* remap */

	++LOCK_header->lhb_query_data;
	PSRQ data_header = &LOCK_header->lhb_data[series];
	SLONG data;
	SLONG count = 0;

	/* Simply walk the lock series data queue forward for the minimum
	   and backward for the maximum -- it's maintained in sorted order. */

	switch (aggregate) 
		{
		case LCK_MIN:
		case LCK_CNT:
		case LCK_AVG:
		case LCK_SUM:
		case LCK_ANY:
			for (que = (PSRQ) ABS_PTR(data_header->srq_forward);
				que != data_header; que = (PSRQ) ABS_PTR(que->srq_forward)) 
				{
				LBL lock = (LBL) ((UCHAR *) que - OFFSET(LBL, lbl_lhb_data));
				CHECK(lock->lbl_series == series);
				
				if (lock->lbl_parent != parent->lrq_lock)
					continue;

				switch (aggregate) 
					{
					case LCK_MIN:
						data = lock->lbl_data;
						break;

					case LCK_ANY:
					case LCK_CNT:
						++count;
						break;

					case LCK_AVG:
						++count;

					case LCK_SUM:
						data += lock->lbl_data;
						break;
					}

				if (aggregate == LCK_MIN || aggregate == LCK_ANY)
					break;
				}

			if (aggregate == LCK_CNT || aggregate == LCK_ANY)
				data = count;
			else if (aggregate == LCK_AVG)
				data = (count) ? data / count : 0;
			break;

		case LCK_MAX:
			for (que = (PSRQ) ABS_PTR(data_header->srq_backward);
				que != data_header; que = (PSRQ) ABS_PTR(que->srq_backward)) 
				{
				LBL lock = (LBL) ((UCHAR *) que - OFFSET(LBL, lbl_lhb_data));
				CHECK(lock->lbl_series == series);
				if (lock->lbl_parent != parent->lrq_lock)
					continue;

				data = lock->lbl_data;
				break;
				}
			break;

		default:
			CHECK(false);
		}

	//release(parent->lrq_owner);
	
	return data;
}


SLONG LockMgr::LOCK_read_data(PTR request_offset)
{
/**************************************
 *
 *	L O C K _ r e a d _ d a t a
 *
 **************************************
 *
 * Functional description
 *	Read data associated with a lock.
 *
 **************************************/
	LOCK_TRACE(("LOCK_read_data(%ld)\n", request_offset));

	LRQ request = get_request(request_offset);
#ifdef SHARED_CACHE
	LockAcquire lockAcquire(request->lrq_owner);
#else
	LockAcquire lockAcquire(request->lrq_owner, this);
#endif
	request = get_request(request_offset); /* in case of remap */
	//acquire(request->lrq_owner);
	++LOCK_header->lhb_read_data;
	request = LOCK_REQUEST(request_offset);	/* Re-init after a potential remap */
	LBL lock = LOCK_BLOCK(request->lrq_lock);
	SLONG data = lock->lbl_data;
	
	if (lock->lbl_series < LCK_MAX_SERIES)
		++LOCK_header->lhb_operations[lock->lbl_series];
	else
		++LOCK_header->lhb_operations[0];

	//release(request->lrq_owner);

	return data;
}


SLONG LockMgr::LOCK_read_data2(PTR parent_request,
					  USHORT series,
					  UCHAR * value, USHORT length, PTR owner_offset)
{
/**************************************
 *
 *	L O C K _ r e a d _ d a t a 2
 *
 **************************************
 *
 * Functional description
 *	Read data associated with transient locks.
 *
 **************************************/
	PTR parent;
	LRQ request;
	LBL lock;
	SLONG data;
	USHORT junk;

	LOCK_TRACE(("LOCK_read_data2(%ld)\n", parent_request));
#ifdef SHARED_CACHE
	LockAcquire lockAcquire(owner_offset);
#else
	LockAcquire lockAcquire(owner_offset, this);
#endif
	//acquire(owner_offset);
	++LOCK_header->lhb_read_data;
	
	if (series < LCK_MAX_SERIES)
		++LOCK_header->lhb_operations[series];
	else
		++LOCK_header->lhb_operations[0];

	if (parent_request) 
		{
		request = get_request(parent_request);
		parent = request->lrq_lock;
		}
	else
		parent = 0;

	if (lock = find_lock(parent, series, value, length, &junk))
		data = lock->lbl_data;
	else
		data = 0;

	//release(owner_offset);

	return data;
}


void LockMgr::LOCK_re_post( lock_ast_t ast, void *arg, PTR owner_offset)
{
/**************************************
 *
 *	L O C K _ r e _ p o s t
 *
 **************************************
 *
 * Functional description
 *	Re-post an AST that was previously blocked.
 *	It is assumed that the routines that look
 *	at the re-post list only test the ast element.
 *
 **************************************/

	//printf ("LOCK_re_post\n");
	LOCK_TRACE(("LOCK_re_post(%ld)\n", owner_offset));
#ifdef SHARED_CACHE
	LockAcquire lockAcquire(owner_offset);
#else
	LockAcquire lockAcquire(owner_offset, this);
#endif
	//acquire(owner_offset);

	/* Allocate or reuse a lock request block */

	LockRequest *request = allocRequest();
	OWN owner = LOCK_OWNER(owner_offset);
	request->lrq_flags = LRQ_repost;
	request->lrq_ast_routine = ast;
	request->lrq_ast_argument = arg;
	request->lrq_requested = LCK_none;
	request->lrq_owner = owner_offset;
	insert_tail(&owner->own_blocks, &request->lrq_own_blocks);

	DEBUG_DELAY;
	signal_owner(LOCK_OWNER(owner_offset), (PTR) NULL);
	//postWakeup(request);
	//release(owner_offset);
}


bool LockMgr::LOCK_shut_manager(void)
{
/**************************************
 *
 *	L O C K _ s h u t _ m a n a g e r
 *
 **************************************
 *
 * Functional description
 *	Set a shutdown flag and post the lock
 *	manager process to exit.
 *
 **************************************/
 
	return true;
}


SLONG LockMgr::LOCK_write_data(PTR request_offset, SLONG data)
{
/**************************************
 *
 *	L O C K _ w r i t e _ d a t a
 *
 **************************************
 *
 * Functional description
 *	Write a longword into the lock block.
 *
 **************************************/

	LOCK_TRACE(("LOCK_write_data (%ld)\n", request_offset));

	LRQ request = get_request(request_offset);
#ifdef SHARED_CACHE
	LockAcquire lockAcquire(request->lrq_owner);
#else
	LockAcquire lockAcquire(request->lrq_owner, this);
#endif
	request = get_request(request_offset); /* in case of remap */
	//acquire(request->lrq_owner);
	++LOCK_header->lhb_write_data;
	request = LOCK_REQUEST(request_offset);	/* Re-init after a potential remap */
	LBL lock = LOCK_BLOCK(request->lrq_lock);
	remove_que(&lock->lbl_lhb_data);
	
	if (lock->lbl_data = data)
		insert_data_que(lock);

	if (lock->lbl_series < LCK_MAX_SERIES)
		++LOCK_header->lhb_operations[lock->lbl_series];
	else
		++LOCK_header->lhb_operations[0];

	//release(request->lrq_owner);

	return data;
}


void LockMgr::acquire( PTR owner_offset)
{
/**************************************
 *
 *	a c q u i r e
 *
 **************************************
 *
 * Functional description
 *	Acquire the lock file.  It it's busy, wait for it.
 *
 **************************************/
	SLONG length;
	LHB header;
	SH_MEM_T *ld;

//num_acquires++;
	/***
	if (LOCK_table == LOCK_header)
		printf ("lock table in use\n");
	***/
		
	acquire_retry:
	
	/* Check that we aren't trying to acquire when we already LockOwner it! */
	/* ASSERT_RELEASED; This will not work, when the current active owner
	   of the lock table falls in the remapped portion of the map
	   file, which we are yet to expand (remap) to */


	/* Measure the impact of the lock table resource as an overall
	   system bottleneck. This will be useful metric for lock
	   improvements and as a limiting factor for SMP. A conditional
	   mutex would probably be more accurate but isn't worth the
	   effort. */

	PTR prior_active = LOCK_table->lhb_active_owner;

	if (LOCK_owner && !shutdown) 
		{
		/* Record a "timestamp" of when this owner requested the lock table */
		LOCK_owner->own_acquire_time = LOCK_table->lhb_acquires;
		
#ifdef DEV_BUILD
		/* Due to the system overhead of acquiring the actual time-of-day,
		   we only get the realtime for DEV_BUILD */
		LOCK_owner->own_acquire_realtime = GET_TIME;
#endif

		LOCK_owner->own_ast_hung_flags |= OWN_hung;
		}


	if (ISC_mutex_lock(MUTEX)) 
		bug(NULL, "semop failed (acquire)");

	LOCK_thread = Thread::getCurrentThreadId(); 

	LOCK_header = LOCK_table;
	ld = &LOCK_data;

#if !defined ONE_LOCK_TABLE && !defined SHARED_CACHE
	if (LOCK_thread == blocking_action_thread_id)
	{
		LOCK_header = LOCK_table2;
		ld = &LOCK_data2;
	}
#endif

#if defined WIN_NT || !defined HAS_MAP_OBJECT
	// Make sure we have the right version of the LOCK_owner
	// Other platforms use directly mapped objects
	if (LOCK_owner) LOCK_owner = LOCK_OWNER(LOCK_owner_offset);
#endif

	++LOCK_header->lhb_acquires;
	
	if (prior_active) 
		++LOCK_header->lhb_acquire_blocks;

	prior_active = LOCK_header->lhb_active_owner;
	LOCK_header->lhb_active_owner = owner_offset;

	if (LOCK_owner) 
		LOCK_owner->own_ast_hung_flags &= ~OWN_hung;	/* Can't be hung by OS if we got here */

	if (LOCK_header->lhb_length > ld->sh_mem_length_mapped) 
		{
		length = LOCK_header->lhb_length;
		
		/* We do not do Lock table remapping here for SuperServer because 
		we have only one address space and we do not need to adjust our
		mapping because another process has changed size of the lock table.
		*/
		
#if (defined HAVE_MMAP || defined WIN_NT)
		ISC_STATUS_ARRAY status_vector;
		header = (LHB) ISC_remap_file(status_vector, ld, length, FALSE);
		
		if (!header)
#endif
			{
			bug(NULL, "remap failed");
			return;
			}
			
		LOCK_header = header;
#if !defined ONE_LOCK_TABLE && !defined SHARED_CACHE
		if (LOCK_thread == blocking_action_thread_id)
			LOCK_table2 = LOCK_header;
		else
#endif
			LOCK_table = LOCK_header;
		
#if defined WIN_NT || !defined HAS_MAP_OBJECT
		// Other platforms use directly mapped objects
		LOCK_owner = LOCK_OWNER(LOCK_owner_offset);
#endif
		}

	/* If we were able to acquire the MUTEX, but there is an prior owner marked
	 * in the the lock table, it means that someone died while owning
	 * the lock mutex.  In that event, lets see if there is any unfinished work
	 * left around that we need to finish up.
	 */
	 
	if (prior_active) 
		{
		post_history(his_active, owner_offset, prior_active, (PTR) 0, false);
		SHB recover = (SHB) ABS_PTR(LOCK_header->lhb_secondary);
		
		if (recover->shb_remove_node) 
			{
			/* There was a remove_que operation in progress when the prior_owner died */
			DEBUG_MSG(0, ("Got to the funky shb_remove_node code\n"));
			remove_que((PSRQ) ABS_PTR(recover->shb_remove_node));
			}
		else if (recover->shb_insert_que && recover->shb_insert_prior) 
			{
			/* There was a insert_que operation in progress when the prior_owner died */
			DEBUG_MSG(0, ("Got to the funky shb_insert_que code\n"));

			PSRQ que = (PSRQ) ABS_PTR(recover->shb_insert_que);
			que->srq_backward = recover->shb_insert_prior;
			que = (PSRQ) ABS_PTR(recover->shb_insert_prior);
			que->srq_forward = recover->shb_insert_que;
			recover->shb_insert_que = 0;
			recover->shb_insert_prior = 0;
			}
		}
	
	if (LOCK_solaris_stall) 
		{
		if (owner_offset > 0) 
			{
			// Can't be hung by OS if we got here
			
			OWN owner = LOCK_OWNER(owner_offset);
			owner->own_ast_hung_flags &= ~OWN_hung;
			OWN first_owner = (OWN) ((UCHAR *) QUE_NEXT(LOCK_header->lhb_owners) - OFFSET(OWN, own_lhb_owners));
								 
			if (first_owner->own_ast_hung_flags & OWN_hung &&
				((LOCK_header->lhb_acquires - first_owner->own_acquire_time) > STARVATION_THRESHHOLD)) 
				{
				first_owner->own_flags |= OWN_starved;
				
				if (owner->own_flags & OWN_blocking) 
					{
					probe_owners(owner_offset);
					owner->own_flags &= ~OWN_blocking;
					release_mutex();
					}
				else 
					{
					owner->own_flags |= OWN_blocking;
					owner->own_flags &= ~OWN_wakeup;
					owner->own_semaphore = 1;
					AsyncEvent* event_ptr = owner->own_stall;
					SLONG value = ISC_event_clear(event_ptr);
					release_mutex();
					SLONG ret = ISC_event_wait(1, &event_ptr, &value,
								LOCK_solaris_stall * 1000000,
								lock_alarm_handler, event_ptr);
#ifdef DEV_BUILD
					if (ret != FB_SUCCESS)
						gds__log ("LOCK: owner %d timed out while stalling for benefit of owner %d",
								 owner_offset, REL_PTR(first_owner));
#endif
					}

				goto acquire_retry;
				}

			owner->own_flags &= ~OWN_blocking;
			}
		}
	
}


UCHAR *LockMgr::alloc( SSHORT size, ISC_STATUS * status_vector)
{
/**************************************
 *
 *	a l l o c
 *
 **************************************
 *
 * Functional description
 *	Allocate a block of given size.
 *
 **************************************/
	ULONG block;
	SH_MEM_T *ld;

	size = FB_ALIGN(size, ALIGNMENT);
	ASSERT_ACQUIRED;
	block = LOCK_header->lhb_used;
	LOCK_header->lhb_used += size;

	/* Make sure we haven't overflowed the lock table.  If so, bump the size of
	   the table */

	if (LOCK_header->lhb_used > LOCK_header->lhb_length) 
		{
		LOCK_header->lhb_used -= size;
		
	/* We do not do Lock table remapping for SuperServer on non-Windows platforms
	   mainly because it is not tested and is not really needed as long SS builds
	   do not use lock manager for page locks. On all other platforms we grow
	   lock table automatically.
	   Do not remap file for Win32 CS because we use mapped objects for 
	   synchronization and there is no (documented) way to have multiple 
	   coherent memory mappings of different size on Windows to apply
	   ISC_map_object approach
	*/

		LOCK_thread = Thread::getCurrentThreadId(); 
		ld = &LOCK_data;
#if !defined ONE_LOCK_TABLE && !defined SHARED_CACHE
		if (LOCK_thread == blocking_action_thread_id)
		{
			ld = &LOCK_data2;
		}
#endif
	
#if defined WIN_NT || (defined HAVE_MMAP)
		ULONG length = ld->sh_mem_length_mapped + EXTEND_SIZE;
		LHB header;
		header = (LHB) ISC_remap_file(status_vector, ld, length, TRUE);
		
		if (header) 
			{
			LOCK_header = header;
#if !defined ONE_LOCK_TABLE && !defined SHARED_CACHE
			if (LOCK_thread == blocking_action_thread_id)
				LOCK_table2 = LOCK_header;
			else
#endif
				LOCK_table = LOCK_header;
				
#if defined WIN_NT || !defined HAS_MAP_OBJECT
			// Other platforms use directly mapped objects
			LOCK_owner = LOCK_OWNER(LOCK_owner_offset);
#endif
			ASSERT_ACQUIRED;
			LOCK_header->lhb_length = ld->sh_mem_length_mapped;
			LOCK_header->lhb_used += size;
			}
		else
#endif
			{
			/* Do not do abort in case if there is not enough room -- just 
			   return an error */

			if (status_vector) 
				{
				*status_vector++ = isc_arg_gds;
				*status_vector++ = isc_random;
				*status_vector++ = isc_arg_string;
				*status_vector++ = (ISC_STATUS) "lock manager out of room";
				*status_vector++ = isc_arg_end;
				}

			return NULL;
			}
		}

#ifdef DEV_BUILD
/* This version of alloc() doesn't initialize memory.  To shake out
   any bugs, in DEV_BUILD we initialize it to a "funny" pattern */
	memset(ABS_PTR(block), 0xFD, size);
#endif

	return ABS_PTR(block);
}


LBL LockMgr::alloc_lock( USHORT length, ISC_STATUS * status_vector)
{
/**************************************
 *
 *	a l l o c _ l o c k
 *
 **************************************
 *
 * Functional description
 *	Allocate a lock for a key of a given length.  Look first to see
 *	if a spare of the right size is sitting around.  If not, allocate
 *	one.
 *
 **************************************/
	LBL lock;
	PSRQ que;

	length = (length + 3) & ~3;
	ASSERT_ACQUIRED;
	
	SRQ_LOOP(LOCK_header->lhb_free_locks, que) 
		{
		lock = (LBL) ((UCHAR *) que - OFFSET(LBL, lbl_lhb_hash));
		
		if (lock->lbl_size == length) 
			{
			remove_que(&lock->lbl_lhb_hash);
			lock->lbl_type = type_lbl;
			return lock;
			}
		}

	if (lock = (LBL) alloc(sizeof(LockBlock) + length, status_vector))
		{
		lock->lbl_size = length;
		lock->lbl_type = type_lbl;
		}

	/* NOTE: if the above alloc() fails do not release mutex here but rather
         release it in LOCK_enq() (as of now it is the only function that
         calls alloc_lock()). We need to hold mutex to be able
         to release a lock request block */


	return lock;
}



void LockMgr::blocking_action(PTR owner_offset)
{
/**************************************
 *
 *	b l o c k i n g _ a c t i o n
 *
 **************************************
 *
 * Functional description
 *	Fault hander for a blocking signal.  A blocking signal
 *	is an indication (albeit a strong one) that a blocking
 *	AST is pending for the owner.  Check in with the data
 *	structure for details.
 *	The re-post code in this routine assumes that no more
 *	than one thread of execution can be running in this
 *	routine at any time.
 *
 *      NOTE: This is a wrapper for calling blocking_action2() where 
 *		   the real action is.  This routine would be called 
 *		   from a signal_handler or blocking_action_thread()
 *		   or LOCK_re_post() where acquire() would not have 
 *		   been done.
 *
 **************************************/

	/* Ignore signals that occur when executing in lock manager
	   or when there is no owner block set up */

	//printf ("blocking_action %d\n", owner_offset);

	if (!owner_offset)
		return;

	DEBUG_DELAY;

	//acquire(owner_offset);
	OWN owner = LOCK_OWNER(owner_offset);
	owner->own_ast_flags &= ~OWN_signaled;
	blocking_action2(owner_offset, 0);
	//release(owner_offset);

}

void LockMgr::blocking_action2( PTR blocking_owner_offset, PTR blocked_owner_offset)
{
/**************************************
 *
 *	b l o c k i n g _ a c t i o n 2
 *
 **************************************
 *
 * Functional description
 *	Fault hander for a blocking signal.  A blocking signal
 *	is an indication (albeit a strong one) that a blocking
 *	AST is pending for the owner.  Check in with the data
 *	structure for details.
 *	The re-post code in this routine assumes that no more
 *	than one thread of execution can be running in this
 *	routine at any time.
 *
 *      IMPORTANT: Before calling this routine, acquire() should
 *	           have already been done.
 *
 *      Note that both a blocking owner offset and blocked owner
 *      offset are passed to this function.   This is for those
 *      cases where the owners are not the same.   If they are
 *      the same, then the blocked owner offset will be NULL.
 *
 **************************************/
	ASSERT_ACQUIRED;
	OWN owner = LOCK_OWNER(blocking_owner_offset);

	if (!blocked_owner_offset)
		blocked_owner_offset = blocking_owner_offset;

	while (owner->own_count) 
		{
		PSRQ que = QUE_NEXT(owner->own_blocks);
		
		if (que == &owner->own_blocks) 
			{
			/* We've processed the own_blocks queue, reset the "we've been
			 * signaled" flag and start winding out of here
			 */
			owner->own_ast_flags &= ~OWN_signaled;
			/*post_history (his_leave_ast, blocking_owner_offset, 0, 0, true); */
			break;
			}
			
		LRQ request = (LRQ) ((UCHAR *) que - OFFSET(LRQ, lrq_own_blocks));
		lock_ast_t routine = request->lrq_ast_routine;
		void *arg = request->lrq_ast_argument;
		remove_que(&request->lrq_own_blocks);
		
		if (request->lrq_flags & LRQ_blocking) 
			{
			request->lrq_flags &= ~LRQ_blocking;
			request->lrq_flags |= LRQ_blocking_seen;
			++LOCK_header->lhb_blocks;
			post_history(his_post_ast, blocking_owner_offset, request->lrq_lock, REL_PTR(request), true);
			}
		else if (request->lrq_flags & LRQ_repost) 
			{
			request->lrq_type = type_null;
			insert_tail(&LOCK_header->lhb_free_requests, &request->lrq_lbl_requests);
			}

		if (routine) 
			{
			release(blocked_owner_offset);
			(*routine)(arg);
			acquire(blocked_owner_offset);
			owner = LOCK_OWNER(blocking_owner_offset);
			}
		}
}


#ifndef SHARED_CACHE
int LockMgr::blocking_action_thread(void *arg)
{
	LockMgr *lockMgr = (LockMgr *)arg;
	return lockMgr->blocking_action_thread();
}

int LockMgr::blocking_action_thread()
#else
int LockMgr::blocking_action_thread(void *arg)
#endif
{
/**************************************
 *
 *	b l o c k i n g _ a c t i o n _ t h r e a d	
 *
 **************************************
 *
 * Functional description
 *	Thread to handle blocking signals that a process
 *	will send to itself.
 *
 **************************************/

#ifndef SHARED_CACHE
	blocking_action_thread_id = Thread::getCurrentThreadId(); 

#if !defined ONE_LOCK_TABLE
	ISC_STATUS_ARRAY status_vector;

	if (!(LOCK_header2 = (LHB) ISC_map_file(status_vector, lock_buffer,
										   lock_initialize2, this,
										   LOCK_shm_size, &LOCK_data2))) 
		{
		blockingThreadStarted.post();
		return status_vector[1];
		}

	LOCK_table2 = LOCK_header2;
#endif
#endif

	try
		{
#ifdef SHARED_CACHE
		PTR ownerOffset = *(PTR*)arg;
#else
		PTR ownerOffset = LOCK_owner_offset;
#endif
		acquire(ownerOffset);
		blockingThreadStarted.post();
		LockOwner *owner = LOCK_OWNER(ownerOffset);

		if (!owner->own_blocking_event)
			{
			release(ownerOffset);
			return 0;
			}
		
		LockEvent *event = LOCK_EVENT(owner->own_blocking_event);
		//PTR eventOffset = owner->own_blocking_event = REL_PTR(event);
		
		while(!shutdown)
			{
			SLONG eventCount = event->event.clear();
			
			release(ownerOffset);
			
			if (!(owner->own_ast_flags & OWN_signaled))
				event->event.wait(eventCount, INFINITE);
			
//			num_blocking_wakeups++;
			
#ifndef SHARED_CACHE
			if (!shutdown)
				LOCK_dbb->syncAst.lock(NULL, Exclusive);
#endif

			//printf ("blocking action wakeup on event %d\n", owner->own_blocking_event);
			acquire(ownerOffset);
			
			if (shutdown)
			{
#ifndef SHARED_CACHE
				if (LOCK_dbb->syncAst.ourExclusiveLock())
					LOCK_dbb->syncAst.unlock();
#endif
				break;
			}
			
			/* this will release and reacquire, potentially remapping the lock table */
			blocking_action(ownerOffset);

			owner = LOCK_OWNER(ownerOffset);
			event = LOCK_EVENT(owner->own_blocking_event);

#ifndef SHARED_CACHE
			LOCK_dbb->syncAst.unlock();
#endif
			}
		
		owner = LOCK_OWNER(ownerOffset);
		event = LOCK_EVENT(owner->own_blocking_event);
		owner->own_blocking_event = 0;
		releaseEvent(event);
		release(ownerOffset);
#if !defined ONE_LOCK_TABLE && !defined SHARED_CACHE
		ISC_unmap_file(status_vector, &LOCK_data2, 0);
#endif
		shutdownComplete.post();
		return 0;
		}
	catch (...)
		{
		shutdownComplete.post();
		throw;
		}
}


#ifdef DEV_BUILD
void LockMgr::bug_assert( const TEXT * string, ULONG line)
{
/**************************************
 *
 *      b u g _ a s s e r t
 *
 **************************************
 *
 * Functional description
 *	Disasterous lock manager bug.  Issue message and abort process.
 *
 **************************************/
	TEXT buffer[100];
	LockHeader LOCK_header_copy;

#ifdef WIN_NT
	DebugBreak();
	return;
#endif
	sprintf((char *) buffer, "%s %ld: lock assertion failure: %s\n",
			__FILE__, line, string);

/* Copy the shared memory so we can examine its state when we crashed */
	LOCK_header_copy = *LOCK_header;

	bug(NULL, buffer);			/* Never returns */
}
#endif


void LockMgr::bug( ISC_STATUS * status_vector, const TEXT * string)
{
/**************************************
 *
 *	b u g
 *
 **************************************
 *
 * Functional description
 *	Disasterous lock manager bug.  Issue message and abort process.
 *
 **************************************/
	TEXT s[512];
	OWN owner;

	sprintf(s, "Fatal lock manager error: %s, errno: %d", string, ERRNO);
	gds__log(s);
	ib_fprintf(ib_stderr, "%s\n", s);

#if defined DEV_BUILD && defined WIN_NT
	DebugBreak();
	return;
#endif

#if !(defined WIN_NT)
	/* The  strerror()  function  returns  the appropriate description string,
	   or an unknown error message if the error code is unknown. */
	ib_fprintf(ib_stderr, "--%s\n", strerror(errno));
#endif

	LOCK_header = LOCK_table;
	
	if (!LOCK_bugcheck++) 
		{

#ifdef DEV_BUILD
#if !defined(WIN_NT)
		/* The lock file has some problem - copy it for later analysis */
		{
			TEXT *lock_file;
			TEXT buffer[2 * MAXPATHLEN];
			TEXT buffer2[2 * MAXPATHLEN];
			TEXT hostname[64];
			gds__prefix_lock(buffer, LOCK_FILE);
			lock_file = buffer;
			sprintf(buffer2, lock_file,
					ISC_get_host(hostname, sizeof(hostname)));
			sprintf(buffer, "cp %s isc_lock1.%d", buffer2, getpid());
			system(buffer);
		}
#endif /* WIN_NT */
#endif /* DEV_BUILD */

		/* If the current mutex acquirer is in the same process, 
		   release the mutex */

		if (LOCK_header && (LOCK_header->lhb_active_owner > 0)) 
			{
			owner = LOCK_OWNER(LOCK_header->lhb_active_owner);
			
			if (owner->own_process_id == LOCK_pid)
				release(LOCK_header->lhb_active_owner);
			}

		if (status_vector) 
			{
			*status_vector++ = isc_arg_gds;
			*status_vector++ = isc_lockmanerr;
			*status_vector++ = isc_arg_gds;
			*status_vector++ = isc_random;
			*status_vector++ = isc_arg_string;
			*status_vector++ = (ISC_STATUS) string;
			*status_vector++ = isc_arg_end;
			return;
			}
		}


#ifdef DEV_BUILD
/* Make a core drop - we want to LOOK at this failure! */
	abort();
#endif

	exit(FINI_ERROR);
}


bool LockMgr::convert(PTR		request_offset,
					UCHAR	type,
					SSHORT	lck_wait,
					lock_ast_t ast_routine,
					void*		ast_argument,
					ISC_STATUS*	status_vector)
{
/**************************************
 *
 *	c o n v e r t
 *
 **************************************
 *
 * Functional description
 *	Perform a lock conversion, if possible.  If the lock cannot be
 *	granted immediately, either return immediately or wait depending
 *	on a wait flag.  If the lock is granted return true, otherwise
 *	return false.  Note: if the conversion would cause a deadlock,
 *	FALSE is returned even if wait was requested.
 *
 **************************************/

	LOCK_TRACE(("convert (%ld)\n", request_offset));

	ASSERT_ACQUIRED;
	LRQ request = get_request(request_offset);
	LBL lock = LOCK_BLOCK(request->lrq_lock);
	PTR owner_offset = request->lrq_owner;
	post_history(his_convert, owner_offset, request->lrq_lock, request_offset, true);
	request->lrq_requested = type;
	request->lrq_flags &= ~LRQ_blocking_seen;

	/* Compute the state of the lock without the request. */
	DUMP_LOCK_COUNTS(lock);

	--lock->lbl_counts[request->lrq_state];
	UCHAR temp = lock_state(lock);

	/* If the requested lock level is compatible with the current state
	   of the lock, just grant the request.  Easy enough. */

	if (COMPATIBLE(type, temp))
		{
		request->lrq_ast_routine = ast_routine;
		request->lrq_ast_argument = ast_argument;
		grant(request, lock);
		post_pending(lock);
		release(owner_offset);
		DUMP_LOCK_COUNTS(lock);
		return true;
		}

	++lock->lbl_counts[request->lrq_state];
	DUMP_LOCK_COUNTS(lock);

	/* If we weren't requested to wait, just forget about the whole thing. 
	   Otherwise wait for the request to be granted or rejected */

	if (lck_wait) 
		{
		bool new_ast = request->lrq_ast_routine != ast_routine || 
					   request->lrq_ast_argument != ast_argument;;

		if (lock->lbl_flags & LBL_converting)
			{
			/* someone else is already trying to convert this lock */
			/* return deadlock */
			goto done;
			}

		lock->lbl_flags |= LBL_converting;

		if (wait_for_request(request, lck_wait, status_vector, NULL)) 
			{
			ASSERT_RELEASED;
			return false;
			}
			
		acquire(owner_offset);
		request = LOCK_REQUEST(request_offset);
		lock = LOCK_BLOCK(request->lrq_lock);
		lock->lbl_flags &= ~LBL_converting;
		
		if (!(request->lrq_flags & LRQ_rejected)) 
			{
			if (new_ast) 
				{
//				acquire(owner_offset);
//				request = LOCK_REQUEST(request_offset);
				request->lrq_ast_routine = ast_routine;
				request->lrq_ast_argument = ast_argument;
//				release(owner_offset);
				}
			release(owner_offset);
			ASSERT_RELEASED;
			return true;
			}
			
//		request = get_request(request_offset);
		post_pending(lock);
		}

	request = LOCK_REQUEST(request_offset);
	request->lrq_requested = request->lrq_state;
	ASSERT_ACQUIRED;
	++LOCK_header->lhb_denies;
	
	if (lck_wait < 0)
		++LOCK_header->lhb_timeouts;
		
done:
	release(owner_offset);
	*status_vector++ = isc_arg_gds;
	*status_vector++ = (lck_wait > 0) ? isc_deadlock :
		((lck_wait < 0) ? isc_lock_timeout : isc_lock_conflict);
	*status_vector++ = isc_arg_end;

	return false;
}


LockOwner* LockMgr::create_owner(ISC_STATUS*	status_vector,
						   SLONG	owner_id,
						   UCHAR	owner_type)
{
/**************************************
 *
 *	c r e a t e _ o w n e r 
 *
 **************************************
 *
 * Functional description
 *	Create an owner block.
 *
 **************************************/
	PSRQ que;
	OWN owner;
	USHORT new_block;

	LOCK_version = LOCK_table->lhb_version;
	
	if (LOCK_version != LHB_VERSION)
		{
		JString bugBuffer;
		bugBuffer.Format("inconsistent lock table version number; found %d, expected %d",
						 LOCK_version, LHB_VERSION);
		bug(status_vector, bugBuffer);
		
		return NULL;
		}

	//acquire(DUMMY_OWNER_CREATE);	/* acquiring owner is being created */

	/* Look for a previous instance of owner.  If we find one, get rid of it. */

	SRQ_LOOP(LOCK_header->lhb_owners, que)
		{
		owner = (OWN) ((UCHAR *) que - OFFSET(OWN, own_lhb_owners));
		
		if (owner->own_owner_id == (ULONG) owner_id && (UCHAR)owner->own_owner_type == owner_type
#ifndef SHARED_CACHE
		    && owner->own_dbb == LOCK_dbb
#endif
			)
			{
			purge_owner(DUMMY_OWNER_CREATE, owner);	/* purging owner_offset has not been set yet */
			break;
			}
		}

	/* Allocate an owner block */

	if (QUE_EMPTY(LOCK_header->lhb_free_owners))
		{
		if (!(owner = (OWN) alloc(sizeof(LockOwner), status_vector)))
			{
			release_mutex();
			return NULL;
			}
			
		new_block = OWN_BLOCK_new;
		}
	else
		{
		owner = (OWN) ((UCHAR*) QUE_NEXT(LOCK_header->lhb_free_owners) -  OFFSET(OWN, own_lhb_owners));
		remove_que(&owner->own_lhb_owners);
		new_block = OWN_BLOCK_reused;
		}

	init_owner_block(owner, owner_type, owner_id, new_block);

	/* cannot ASSERT_ACQUIRED; here - owner not setup */
	
	insert_tail(&LOCK_header->lhb_owners, &owner->own_lhb_owners);
	probe_owners(REL_PTR(owner));
	LOCK_header->lhb_active_owner = REL_PTR(owner);

#ifdef VALIDATE_LOCK_TABLE
	validate_lhb(LOCK_header);
#endif

	//release(*owner_handle);

	return owner;
}


#ifdef DEV_BUILD
void LockMgr::current_is_active_owner(bool expect_acquired, ULONG line)
{
/**************************************
 *
 *	c u r r e n t _ i s _ a c t i v e _ o w n e r
 *
 **************************************
 *
 * Functional description
 *	Decide if the current process is the active owner
 *	for the lock table.  Used in assertion checks.
 *
 **************************************/

	/* Do not ASSERT_ACQUIRED in this routine */

	/* If there's no header, we must be setting up in init somewhere */
	
	if (!LOCK_header)
		return;

	/* Use a local copy of lhb_active_owner.  We're viewing the lock table
	   without being acquired in the "expect_acquired false" case, so it
	   can change out from under us.  We don't care that it changes, but
	   if it gets set to DUMMY_OWNER_SHUTDOWN, DUMMY_OWNER_CREATE or 
	   DUMMY_OWNER_DELETE it can lead to a core drop when we try to map 
	   the owner pointer */

	PTR owner_ptr = LOCK_table->lhb_active_owner;

	/* If no active owner, then we certainly aren't the active owner */
	
	if (!owner_ptr) 
		{
		if (!expect_acquired)
			return;
			
		bug_assert("not acquired", line);
		}

	/* When creating or deleting an owner the owner offset is set such that
	 * we can't be sure if WE have the lock table acquired, or someone else 
	 * has it, and they just happen to be doing the same operation.  So, we 
	 * don't try to report any problems when the lock table is in that state
	 */

	/* If active owner is DUMMY_OWNER_SHUTDOWN, then we're in process of shutting
	   down the lock system. */
	   
	if (owner_ptr == DUMMY_OWNER_SHUTDOWN)
		return;

	/* If active owner is DUMMY_OWNER_CREATE, then we're creating a new owner */
	
	if (owner_ptr == DUMMY_OWNER_CREATE)
		return;

	/* If active owner is DUMMY_OWNER_DELETE, then we're deleting an owner */

	if (owner_ptr == DUMMY_OWNER_DELETE)
		return;

	/* Find the active owner, and see if it is us */
	OWN owner;
	if (!expect_acquired)
	{
		/* LOCK_header may be -1 if not acquired, so get the owner from */
		/* LOCK_table instead */
		owner = (OWN) ((UCHAR*) LOCK_table + owner_ptr);
	}
	else
		owner = LOCK_OWNER(owner_ptr);
	

	/* SUPERSERVER uses the same pid for all threads, so the tests
	   below are of limited utility and can cause bogus errors */

	LockOwner owner_copy;

	if (owner->own_process_id == LOCK_pid) 
		{
		if (expect_acquired)
			{
			if (Thread::getCurrentThreadId() == LOCK_thread)
				return;
			}
		
		if (Thread::getCurrentThreadId() != LOCK_thread)
			return;
			
		/* Save a copy for the debugger before we abort */
		
		memcpy(&owner_copy, owner, sizeof(owner_copy));
		bug_assert("not acquired", line);
		}
	else 
		{
		if (!expect_acquired)
			return;
			
		/* Save a copy for the debugger before we abort */
		
		memcpy(&owner_copy, owner, sizeof(owner_copy));
		bug_assert("not released", line);
		}
}

#endif // DEV_BUILD


void LockMgr::deadlock_clear(void)
{
/**************************************
 *
 *	d e a d l o c k _ c l e a r
 *
 **************************************
 *
 * Functional description
 *	Clear deadlock and scanned bits for pending requests
 *	in preparation for a deadlock scan.
 *
 **************************************/
	PSRQ que;

	LOCK_TRACE(("deadlock_clear\n"));
	ASSERT_ACQUIRED;
	
	SRQ_LOOP(LOCK_header->lhb_owners, que) 
		{
		OWN owner = (OWN) ((UCHAR *) que - OFFSET(OWN, own_lhb_owners));
		//PTR pending_offset = owner->own_pending_request;
		PSRQ inner;
		
		SRQ_LOOP(owner->own_pending, inner)
			{
			LRQ pending = (LRQ) ((UCHAR*) inner - OFFSET(LRQ, lrq_own_pending));			
			pending->lrq_flags &= ~(LRQ_deadlock | LRQ_scanned);
			}
		}
}


LRQ LockMgr::deadlock_scan(OWN owner,
						 LRQ request)
{
/**************************************
 *
 *	d e a d l o c k _ s c a n
 *
 **************************************
 *
 * Functional description
 *	Given an owner block that has been stalled for some time, find
 *	a deadlock cycle if there is one.  If a deadlock is found, return
 *	the address of a pending lock request in the deadlock request.
 *	If no deadlock is found, return null.
 *
 **************************************/
 
	bool maybe_deadlock = false;
	LOCK_TRACE(("deadlock_scan: owner %ld request %ld\n", REL_PTR(owner), REL_PTR(request)));
	ASSERT_ACQUIRED;
	++LOCK_header->lhb_scans;
	post_history(his_scan, request->lrq_owner, request->lrq_lock, REL_PTR(request), true);
	deadlock_clear();

#ifdef VALIDATE_LOCK_TABLE
	validate_lhb(LOCK_header);
#endif

	LRQ victim = deadlock_walk(request, &maybe_deadlock);

/* Only when it is certain that this request is not part of a deadlock do we
   mark this request as 'scanned' so that we will not check this request again. 
   Note that this request might be part of multiple deadlocks. */

	if (!victim && !maybe_deadlock)
		owner->own_flags |= OWN_scanned;
#ifdef DEBUG
	else if (!victim && maybe_deadlock)
		DEBUG_MSG(0, ("deadlock_scan: not marking due to maybe_deadlock\n"));
#endif

	return victim;
}


LRQ LockMgr::deadlock_walk(LRQ request, bool * maybe_deadlock)
{
/**************************************
 *
 *	d e a d l o c k _ w a l k
 *
 **************************************
 *
 * Functional description
 *	Given a request that is waiting, determine whether a deadlock has
 *	occured.
 *
 **************************************/
	PSRQ que;

	/* If this request was scanned for deadlock earlier than don't
	   visit it again. */

	if (request->lrq_flags & LRQ_scanned)
		return NULL;

	/* If this request has been seen already during this deadlock-walk, then we
	   detected a circle in the wait-for graph.  Return "deadlock". */

	if (request->lrq_flags & LRQ_deadlock)
		return request;

	/* Remember that this request is part of the wait-for graph. */

	request->lrq_flags |= LRQ_deadlock;

	/* Check if this is a conversion request. */

	bool conversion = (request->lrq_state > LCK_null);

	/* Find the parent lock of the request */

	LBL lock = LOCK_BLOCK(request->lrq_lock);

	/* Loop thru the requests granted against the lock.  If any granted request is
	   blocking the request we're handling, recurse to find what's blocking him. */

	SRQ_LOOP(lock->lbl_requests, que) 
		{
		LRQ block = (LRQ) ((UCHAR *) que - OFFSET(LRQ, lrq_lbl_requests));

		/* Note that the default for LOCK_ordering is 1, and the default can be
		   changed with the isc_config modifier 'V4_LOCK_GRANT_ORDER'. */

		if (!LOCK_ordering || conversion) 
			{
			/* Don't pursue our LockOwner lock-request again. */

			if (request == block)
				continue;

			/* Since lock conversions can't follow the fairness rules (to avoid
			   deadlocks), only granted lock requests need to be examined. */
			/* If lock-ordering is turned off (opening the door for starvation),
			   only granted requests can block our request. */

			if (COMPATIBLE(request->lrq_requested, block->lrq_state))
				continue;
			}
		else 
			{
			/* Don't pursue our LockOwner lock-request again.  In addition, don't look
			   at requests that arrived after our request because lock-ordering
			   is in effect. */

			if (request == block)
				break;

			/* Since lock ordering is in effect, granted locks and waiting
			   requests that arrived before our request could block us. */

			if (COMPATIBLE (request->lrq_requested, MAX(block->lrq_state, block->lrq_requested))) 
				continue;
			}

		/* Don't pursue lock owners that are not blocked themselves 
		   (they can't cause a deadlock). */

		OWN owner = LOCK_OWNER(block->lrq_owner);

		/* Don't pursue lock owners that still have to finish processing their AST.
		   If the blocking queue is not empty, then the owner still has some
		   AST's to process (or lock reposts).
		   Remember this fact because they still might be part of a deadlock. */

		if (owner->own_ast_flags & OWN_signaled || !QUE_EMPTY((owner->own_blocks))) 
			{
			*maybe_deadlock = true;
			continue;
			}

		/* YYY: Note: can the below code be moved to the
		   start of this block?  Before the OWN_signaled check?
		 */

		/* Get pointer to the waiting request whose owner also owns a lock
		   that blocks the input request. */

		/***
		PTR pending_offset = owner->own_pending_request;
		
		if (!pending_offset)
			continue;

		LRQ target = LOCK_REQUEST(pending_offset);
		***/
		PSRQ que2;
		
		SRQ_LOOP(owner->own_pending, que2)
			{
			LRQ target = (LRQ) ((UCHAR*) que2 - OFFSET(LRQ, lrq_own_pending));	
			
			/* If this waiting request is not pending anymore, then things are changing
			   and this request cannot be part of a deadlock. */

			if (!(target->lrq_flags & LRQ_pending))
				continue;

			/* Check who is blocking the request whose owner is blocking the input
			   request. */

			if (target = deadlock_walk(target, maybe_deadlock))
				return target;
			}
		}

	/* This branch of the wait-for graph is exhausted, the current waiting
	   request is not part of a deadlock. */

	request->lrq_flags &= ~LRQ_deadlock;
	request->lrq_flags |= LRQ_scanned;
	
	return NULL;
}




void LockMgr::dequeue( PTR request_offset)
{
/**************************************
 *
 *	d e q u e u e
 *
 **************************************
 *
 * Functional description
 *	Release an outstanding lock.
 *
 **************************************/

	/* Acquire the data structure, and compute addresses of both lock
	   request and lock */

	LockRequest *request = get_request(request_offset);
	post_history(his_deq, request->lrq_owner, request->lrq_lock, request_offset, true);
	request->lrq_ast_routine = NULL;
	release_request(request);
}


#ifdef DEBUG

static ULONG delay_count = 0;
static ULONG last_signal_line = 0;
static ULONG last_delay_line = 0;

void LockMgr::debug_delay( ULONG lineno)
{
/**************************************
 *
 *	d e b u g _ d e l a y
 *
 **************************************
 *
 * Functional description
 *	This is a debugging routine, the purpose of which is to slow
 *	down operations in order to expose windows of critical 
 *	sections.
 *
 **************************************/

	ULONG i;

/* Delay for a while */

	last_delay_line = lineno;
	for (i = 0; i < 10000; i++)
		/* Nothing */ ;


/* Occasionally post a signal to ourselves, provided we aren't
 * in the signal handler already and we've gotten past the initialization code.
 */
#if !defined WIN_NT
	if (LOCK_asts < 1)
		if (((delay_count++ % 20) == 0) && LOCK_block_signal
			&& LOCK_owner_offset) {
			last_signal_line = lineno;
			kill(getpid(), LOCK_block_signal);
		}
#endif

/* Occasionally crash for robustness testing */
/*
if ((delay_count % 500) == 0)
    exit (-1);
*/

	for (i = 0; i < 10000; i++)
		/* Nothing */ ;

}
#endif


#ifndef SHARED_CACHE
void LockMgr::exit_handler( void *arg)
{
	LockMgr *lockMgr = (LockMgr *)arg;
	lockMgr->exit_handler();
}
void LockMgr::exit_handler()
#else
void LockMgr::exit_handler(void *arg)
#endif
{
/**************************************
 *
 *	e x i t _ h a n d l e r
 *
 **************************************
 *
 * Functional description
 *	Release the process block, any outstanding locks,
 *	and unmap the lock manager.  This is usually called
 *	by the cleanup handler.
 *
 **************************************/
	ISC_STATUS_ARRAY local_status;

	
	if (!LOCK_table) 
		return;

	LOCK_header = LOCK_table;
	
	/* For a superserver (e.g. Netware), since the server is going away, 
	   the semaphores cleanups are also happening; things are in a flux; 
	   all the threads are going away -- so don't get into any trouble 
	   by doing purge_owner() below. */

	PSRQ que;
	PTR owner_offset;

	/* Get rid of all the owners belonging to the current process */

	if (owner_offset = LOCK_owner_offset) 
		{
		if (LOCK_owner_offset != LOCK_header->lhb_active_owner)
			{
			ISC_mutex_lock(MUTEX);
			LOCK_header->lhb_active_owner = owner_offset;
			}
			
		shutdown_blocking_thread(LOCK_OWNER(owner_offset));
		
		if (owner_offset != LOCK_header->lhb_active_owner)
			acquire(DUMMY_OWNER_DELETE);

		SRQ_LOOP(LOCK_header->lhb_owners, que) 
			{
			OWN owner = (OWN) ((UCHAR *) que - OFFSET(OWN, own_lhb_owners));
			if (owner->own_process_id == LOCK_pid) 
				{
				que = (PSRQ) ABS_PTR(que->srq_backward);
				purge_owner(REL_PTR(owner), owner);
				break;
				}
			}

		release_mutex();
		LOCK_owner_offset = 0;
		}

	ISC_unmap_file(local_status, &LOCK_data, 0);
#if defined ONE_LOCK_TABLE && !defined SHARED_CACHE
	LOCK_table_static = NULL;
#endif
}


LBL LockMgr::find_lock(PTR parent,
					 USHORT series,
					 UCHAR * value, USHORT length, USHORT * slot)
{
/**************************************
 *
 *	f i n d _ l o c k
 *
 **************************************
 *
 * Functional description
 *	Find a lock block given a resource
 *	name. If it doesn't exist, the hash
 *	slot will be useful for enqueing a
 *	lock.
 *
 **************************************/
	UCHAR *p;
	USHORT l;

	/* Hash the value preserving its distribution as much as possible */

	ULONG hash_value = 0;
	UCHAR *q = value;

	for (l = 0, q = value; l < length; l++) 
		{
		if (!(l & 3))
			p = (UCHAR *) & hash_value;
		*p++ = *q++;
		}

	/* See if the lock already exists */

	USHORT hash_slot = *slot = (USHORT) (hash_value % LOCK_header->lhb_hash_slots);
	ASSERT_ACQUIRED;
	PSRQ hash_header = &LOCK_header->lhb_hash[hash_slot];

	for (PSRQ que = (PSRQ) ABS_PTR(hash_header->srq_forward);
		 que != hash_header; que = (PSRQ) ABS_PTR(que->srq_forward)) 
		{
		LBL lock = (LBL) ((UCHAR *) que - OFFSET(LBL, lbl_lhb_hash));
		if (lock->lbl_series != series ||
			lock->lbl_length != length || lock->lbl_parent != parent)
			continue;
		if (l = length) 
			{
			p = value;
			q = lock->lbl_key;
			do
				if (*p++ != *q++)
					break;
			while (--l);
			}
		if (!l)
			return lock;
		}

	return NULL;
}

LRQ LockMgr::get_request( PTR offset)
{
/**************************************
 *
 *	g e t _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Locate and validate user supplied request offset.
 *
 **************************************/
	//TEXT s[32];
	
	//LRQ request = LOCK_REQUEST(offset);
	LRQ request;
	

#if !defined ONE_LOCK_TABLE && !defined SHARED_CACHE
	if (blocking_action_thread_id == Thread::getCurrentThreadId())
	{
		request = (LRQ) ((UCHAR*) LOCK_table2 + offset);
	}
	else
#endif
	{
		request = (LRQ) ((UCHAR*) LOCK_table + offset);
	}
	
	
	if ((SLONG) offset == -1 || request->lrq_type != type_lrq) 
		{
		JString text;
		text.Format("invalid lock id (%"SLONGFORMAT")", offset);
		bug(NULL, text);
		}

	/***
	LBL lock = LOCK_BLOCK(request->lrq_lock);
	
	if (lock->lbl_type != type_lbl) 
		{
		sprintf(s, "invalid lock (%"SLONGFORMAT")", offset);
		bug(NULL, s);
		}
	***/
	
	return request;
}

void LockMgr::grant( LRQ request, LBL lock)
{
/**************************************
 *
 *	g r a n t
 *
 **************************************
 *
 * Functional description
 *	Grant a lock request.  If the lock is a conversion, assume the caller
 *	has already decremented the former lock type count in the lock block.
 *
 **************************************/

	/* Request must be for THIS lock */
	
	LOCK_TRACE(("grant (%ld)\n", request->lrq_lock));
	
	CHECK(REL_PTR(lock) == request->lrq_lock);
	post_history(his_grant, request->lrq_owner, request->lrq_lock, REL_PTR(request), true);
	++lock->lbl_counts[request->lrq_requested];
	request->lrq_state = request->lrq_requested;
	
	if (request->lrq_data) 
		{
		remove_que(&lock->lbl_lhb_data);
		
		if (lock->lbl_data = request->lrq_data)
			insert_data_que(lock);
			
		request->lrq_data = 0;
		}

	lock->lbl_state = lock_state(lock);
	
	if (request->lrq_flags & LRQ_pending)
		clearPending(request);
		
	//post_wakeup(LOCK_OWNER(request->lrq_owner), REL_PTR(request));
	postWakeup(request);
	DUMP_LOCK_COUNTS(lock);
}


PTR LockMgr::grant_or_que( LRQ request, LBL lock, SSHORT lck_wait)
{
/**************************************
 *
 *	g r a n t _ o r _ q u e
 *
 **************************************
 *
 * Functional description
 *	There is a request against an existing lock.  If the request
 *	is compatible with the lock, grant it.  Otherwise que it.
 *	If the lock is que-ed, set up the machinery to do a deadlock
 *	scan in awhile.
 *
 **************************************/

	PTR request_offset = REL_PTR(request);
	request->lrq_lock = REL_PTR(lock);

	LOCK_TRACE(("grant (%ld) (%ld)\n", request_offset, request->lrq_lock));

	/* Compatible requests are easy to satify.  Just post the request
	   to the lock, update the lock state, release the data structure,
	   and we're done. */

	if (COMPATIBLE(request->lrq_requested, lock->lbl_state))
		if (!LOCK_ordering || request->lrq_requested == LCK_null || (lock->lbl_pending_lrq_count == 0)) 
			{
			grant(request, lock);
			post_pending(lock);
			//release(request->lrq_owner);
			
			return request_offset;
			}

	/* The request isn't compatible with the current state of the lock.
	 * If we haven't be asked to wait for the lock, return now. 
	 */

	if (lck_wait)
		{
		int lrq_owner;
		wait_for_request(request, lck_wait, NULL, &lrq_owner);

		acquire(lrq_owner);

		/* For performance reasons, we're going to look at the 
		 * request's status without re-acquiring the lock table.
		 * This is safe as no-one can take away the request, once
		 * granted, and we're doing a read-only access
		 */
		 
		request = LOCK_REQUEST(request_offset);

		/* Request HAS been resolved */
		
		CHECK(!(request->lrq_flags & LRQ_pending));

		if (!(request->lrq_flags & LRQ_rejected))
			return request_offset;
			
		}

	request = LOCK_REQUEST(request_offset);
	post_history(his_deny, request->lrq_owner, request->lrq_lock, REL_PTR(request), true);
	ASSERT_ACQUIRED;
	++LOCK_header->lhb_denies;
	
	if (lck_wait < 0)
		++LOCK_header->lhb_timeouts;
		
	PTR owner_offset = request->lrq_owner;
	release_request(request);
	//release(owner_offset);

	return NULL;
}


ISC_STATUS LockMgr::init_lock_table( ISC_STATUS * status_vector, ConfObject *configuration)
{
/**************************************
 *
 *	i n i t _ l o c k _ t a b l e
 *
 **************************************
 *
 * Functional description
 *	Initialize the global lock table for the first time.
 *	Read the config file, map the shared region, etc. 
 *
 **************************************/
	const TEXT *lock_file;
	TEXT buffer[MAXPATHLEN];

	LOCK_shm_size = configuration->getValue (LockMemSize, LockMemSizeValue); //Config::getLockMemSize();
	LOCK_sem_count = configuration->getValue (LockSemCount, LockSemCountValue); //Config::getLockSemCount();
	//LOCK_block_signal = configuration->getValue (LockSignal, LockSignalValue); //Config::getLockSignal();
	//LOCK_wakeup_signal = WAKEUP_SIGNAL;

	LOCK_hash_slots = configuration->getValue (LockHashSlots, LockHashSlotsValue); //Config::getLockHashSlots();
	LOCK_scan_interval = configuration->getValue (DeadlockTimeout, DeadlockTimeoutValue); //Config::getDeadlockTimeout();

	
	/* LOCK_ordering is TRUE (ON) by default.  It may be changed 
	   by the V4_LOCK_GRANT_ORDER option in the configuration file.
	   A value of 0 for that option turns lock ordering off */

	LOCK_ordering = configuration->getValue (LockGrantOrder, LockGrantOrderValue); //Config::getLockGrantOrder();

	if (LOCK_hash_slots < HASH_MIN_SLOTS)
		LOCK_hash_slots = HASH_MIN_SLOTS;
		
	if (LOCK_hash_slots > HASH_MAX_SLOTS)
		LOCK_hash_slots = HASH_MAX_SLOTS;

	LOCK_solaris_stall = configuration->getValue (SolarisStallValue, SolarisStallValueValue);

	if (LOCK_solaris_stall < SOLARIS_MIN_STALL)
		LOCK_solaris_stall = SOLARIS_MIN_STALL;
		
	if (LOCK_solaris_stall > SOLARIS_MAX_STALL)
		LOCK_solaris_stall = SOLARIS_MAX_STALL;

	LOCK_pid = getpid();
	JString lockFileName = configuration->getValue (LockFileName, LockFileNameValue);
	
	if (lockFileName.IsEmpty())
		{
		gds__prefix_lock(buffer, LOCK_FILE);
		lock_file = buffer;
		}
	else
		lock_file = lockFileName;
		
#ifdef UNIX
	LOCK_data.sh_mem_semaphores = LOCK_sem_count;
#endif

#if defined ONE_LOCK_TABLE && !defined SHARED_CACHE
	if (LOCK_table_static)
		LOCK_header = LOCK_table_static;
	else			
#endif
		if (!(LOCK_header = (LHB) ISC_map_file(status_vector, lock_file,
#ifdef SHARED_CACHE
										   lock_initialize, 0,
#else
										   lock_initialize, this,
#endif
										   LOCK_shm_size, &LOCK_data))) 
			return status_vector[1];

#ifndef SHARED_CACHE
	strcpy(lock_buffer, lock_file);
#endif
	
	LOCK_table = LOCK_header;

#if defined ONE_LOCK_TABLE && !defined SHARED_CACHE
	LOCK_table_static = LOCK_table;
#endif

	/* Now get the globally consistent value of LOCK_ordering from the 
	   shared lock table. */

	LOCK_ordering = (LOCK_header->lhb_flags & LHB_lock_ordering) ? TRUE : FALSE;

// BUG BUG SEK
// Doesn't work now, as the LockMgr class is most likely already freed */
// Need and alternative.
#ifdef SHARED_CACHE
	gds__register_cleanup(exit_handler, 0);
#endif

	/* Define a place holder process owner block */

	/***
	init_owner_block(&LOCK_process_owner, LCK_OWNER_dummy_process,
					 LOCK_header->lhb_process_count++, OWN_BLOCK_dummy);
	***/
	
	LOCK_header = (LHB) -1;
	
	return FB_SUCCESS;
}


void LockMgr::init_owner_block(OWN owner, UCHAR owner_type, ULONG owner_id, USHORT new_block)
{
/**************************************
 *
 *	i n i t _ o w n e r _ b l o c k
 *
 **************************************
 *
 * Functional description
 *	Initialize the passed owner block nice and new. 
 *
 **************************************/

	owner->own_type = type_own;
	owner->own_owner_type = owner_type;
	owner->own_flags = 0;
	owner->own_ast_flags = 0;
	owner->own_ast_hung_flags = 0;
	owner->own_count = 1;
	owner->own_owner_id = owner_id;
	owner->own_events = 0;
	//QUE_INIT(owner->own_lhb_owners);
	QUE_INIT(owner->own_requests);
	QUE_INIT(owner->own_blocks);
	QUE_INIT(owner->own_pending);
	//owner->own_pending_request = 0;
	owner->own_process_id = LOCK_pid;
	owner->own_blocking_event = 0;
	
#ifdef UNIX
	owner->own_process_uid = getuid();
#endif

#ifdef WIN_NT
	owner->own_process_uid = 0;
#endif

	owner->own_acquire_time = 0;
	owner->own_acquire_realtime = 0;
	owner->own_semaphore = 0;
	
#ifndef SHARED_CACHE
	owner->own_dbb = LOCK_dbb;
#endif

#if defined(WIN_NT)
	// Skidder: This Win32 EVENT is deleted when our process is closing
	/***
	if (new_block != OWN_BLOCK_dummy)
		owner->own_wakeup_hndl = ISC_make_signal(TRUE, TRUE, LOCK_pid, LOCK_wakeup_signal);
	***/
#endif

#ifdef SHARED_CACHE
	if (new_block == OWN_BLOCK_new)
#endif
	{
		//ISC_event_init(owner->own_blocking, 0, 1);
		ISC_event_init(owner->own_stall, 0, 1);

#ifdef OBSOLETE
#ifdef USE_WAKEUP_EVENTS

#ifdef SOLARIS
		ISC_event_init(owner->own_wakeup, 0, 1);
#endif

#ifdef POSIX_THREADS
		ISC_event_init(owner->own_wakeup, 0, 0);
#endif

#ifdef UNIX
#if !(defined SOLARIS || defined POSIX_THREADS)

		owner->own_wakeup[0].event_semid = LOCK_data.sh_mem_mutex_arg;
		owner->own_wakeup[0].event_semnum = 0;
		owner->own_wakeup[0].event_count = 0;
#endif
#endif
#endif
#endif
	}
}


void LockMgr::lock_alarm_handler(void* event)
{
/**************************************
 *
 *      l o c k _ a l a r m _ h a n d l e r
 *
 **************************************
 *
 * Functional description
 *      This handler would just post the passed event.
 *      This is to take care of the situation when an alarm handler
 *      expires before the wait on the samaphore starts.  This way, we can
 *      avoid an indefinite wait when there is nobody to poke the
 *      semaphore and the timer also expired becuase of some asynchronous
 *      event (signal) handling.
 *
 **************************************/

	//ISC_event_post(reinterpret_cast<EVENT>(event));
	ISC_event_post((AsyncEvent*) event);
}


#ifndef SHARED_CACHE
void LockMgr::lock_initialize2(void* arg, SH_MEM shmem_data, bool initialize)
{
	/* nothing to do in the blocking thread's version */
}


void LockMgr::lock_initialize(void* arg, SH_MEM shmem_data, bool initialize)
{
	LockMgr *lockMgr = (LockMgr *)arg;
	lockMgr->lock_initialize(shmem_data, initialize);
}

void LockMgr::lock_initialize(SH_MEM shmem_data, bool initialize)
#else
void LockMgr::lock_initialize(void *arg, SH_MEM shmem_data, bool initialize)
#endif
{
/**************************************
 *
 *	l o c k _ i n i t i a l i z e
 *
 **************************************
 *
 * Functional description
 *	Initialize the lock header block.  The caller is assumed
 *	to have an exclusive lock on the lock file.
 *
 **************************************/
	SSHORT length;
	USHORT i, j;
	PSRQ que;
	LockHistory *history;
	PTR *prior;

#ifdef WIN_NT
	char buffer[MAXPATHLEN];
	gds__prefix_lock(buffer, LOCK_FILE);
	
	if (ISC_mutex_init(MUTEX, buffer)) 
		bug(NULL, "mutex init failed");
#endif

	LOCK_header = LOCK_table = (LHB) shmem_data->sh_mem_address;
	// Reflect real number of semaphores available
	
#ifdef UNIX
	LOCK_sem_count = shmem_data->sh_mem_semaphores;
#endif

	if (!initialize) 
		return;

	memset(LOCK_header, 0, sizeof(LockHeader));
	LOCK_header->lhb_type = type_lhb;
	LOCK_header->lhb_version = LHB_VERSION;

	/* Mark ourselves as active owner to prevent fb_assert() checks */
	
	LOCK_header->lhb_active_owner = DUMMY_OWNER_CREATE;	/* In init of lock system */

	QUE_INIT(LOCK_header->lhb_owners);
	QUE_INIT(LOCK_header->lhb_free_owners);
	QUE_INIT(LOCK_header->lhb_free_locks);
	QUE_INIT(LOCK_header->lhb_free_requests);

#ifndef WIN_NT
	LOCK_header->lhb_semid = LOCK_data.sh_mem_mutex_arg;
	LOCK_header->lhb_event_number = 1;
	
	if (ISC_mutex_init(MUTEX, LOCK_header->lhb_semid)) 
		bug(NULL, "mutex init failed");
#endif

	LOCK_header->lhb_sequence = 0;
	LOCK_header->lhb_hash_slots = (USHORT) LOCK_hash_slots;
	LOCK_header->lhb_scan_interval = LOCK_scan_interval;
	
	/* Initialize lock series data queues and lock hash chains. */

	for (i = 0, que = LOCK_header->lhb_data; i < LCK_MAX_SERIES; i++, que++)
		QUE_INIT((*que));
		
	for (i = 0, que = LOCK_header->lhb_hash; i < LOCK_header->lhb_hash_slots; i++, que++)
		QUE_INIT((*que));

	/* Set lock_ordering flag for the first time */

	if (LOCK_ordering)
		LOCK_header->lhb_flags |= LHB_lock_ordering;

	length = sizeof(LockHeader) + (LOCK_header->lhb_hash_slots * sizeof(LOCK_header->lhb_hash[0]));
	LOCK_header->lhb_length = shmem_data->sh_mem_length_mapped;
	LOCK_header->lhb_used = FB_ALIGN(length, ALIGNMENT);
	SHB secondary_header = (SHB) alloc(sizeof(shb), NULL);

	if (!secondary_header)
		{
		gds__log("Fatal lock manager error: lock manager out of room");
		exit(STARTUP_ERROR);
		}
	
	LOCK_header->lhb_secondary = REL_PTR(secondary_header);
	secondary_header->shb_type = type_shb;
	secondary_header->shb_flags = 0;
	secondary_header->shb_remove_node = 0;
	secondary_header->shb_insert_que = 0;
	secondary_header->shb_insert_prior = 0;
	
	for (i = FB_NELEM(secondary_header->shb_misc); i--;)
		secondary_header->shb_misc[i] = 0;

	/* Allocate a sufficiency of history blocks */

	for (j = 0; j < 2; j++)
		{
		prior = (j == 0) ? &LOCK_header->lhb_history : &secondary_header->shb_history;
		
		for (i = 0; i < HISTORY_BLOCKS; i++)
			{
			if (!(history = (LockHistory*) alloc(sizeof(LockHistory), NULL)))
				{
				gds__log("Fatal lock manager error: lock manager out of room");
				exit(STARTUP_ERROR);
				}
			*prior = REL_PTR(history);
			history->his_type = type_his;
			history->his_operation = 0;
			prior = &history->his_next;
			}

		history->his_next = (j == 0) ? LOCK_header->lhb_history : secondary_header->shb_history;
		}

	/* Done initializing, unmark owner information */
	
	LOCK_header->lhb_active_owner = 0;
}


void LockMgr::insert_data_que( LBL lock)
{
/**************************************
 *
 *	i n s e r t _ d a t a _ q u e
 *
 **************************************
 *
 * Functional description
 *	Insert a node in the lock series data queue
 *	in sorted (ascending) order by lock data.
 *
 **************************************/
	LBL lock2;
	PSRQ data_header, que;

	if (lock->lbl_series < LCK_MAX_SERIES && lock->lbl_parent
		&& lock->lbl_data) {
		data_header = &LOCK_header->lhb_data[lock->lbl_series];

		for (que = (PSRQ) ABS_PTR(data_header->srq_forward);
			 que != data_header; que = (PSRQ) ABS_PTR(que->srq_forward)) {
			lock2 = (LBL) ((UCHAR *) que - OFFSET(LBL, lbl_lhb_data));
			CHECK(lock2->lbl_series == lock->lbl_series);
			if (lock2->lbl_parent != lock->lbl_parent)
				continue;

			if (lock->lbl_data <= lock2->lbl_data)
				break;
		}

		insert_tail(que, &lock->lbl_lhb_data);
	}
}


void LockMgr::insert_tail( PSRQ que, PSRQ node)
{
/**************************************
 *
 *	i n s e r t _ t a i l
 *
 **************************************
 *
 * Functional description
 *	Insert a node at the tail of a que.
 *
 *	To handle the event of the process terminating during
 *	the insertion of the node, we set values in the shb to
 *	indicate the node being inserted.
 *	Then, should we be unable to complete
 *	the node insert, the next process into the lock table
 *	will notice the uncompleted work and undo it,
 *	eg: it will put the queue back to the state
 *	prior to the insertion being started.
 *
 **************************************/
	PSRQ prior;
	SHB recover;

	PRINT_QUE(que, node);

	ASSERT_ACQUIRED;
	recover = (SHB) ABS_PTR(LOCK_header->lhb_secondary);
	DEBUG_DELAY;
	recover->shb_insert_que = REL_PTR(que);
	DEBUG_DELAY;
	recover->shb_insert_prior = que->srq_backward;
	DEBUG_DELAY;

	node->srq_forward = REL_PTR(que);
	DEBUG_DELAY;
	node->srq_backward = que->srq_backward;

	DEBUG_DELAY;
	prior = (PSRQ) ABS_PTR(que->srq_backward);
	DEBUG_DELAY;
	prior->srq_forward = REL_PTR(node);
	DEBUG_DELAY;
	que->srq_backward = REL_PTR(node);
	DEBUG_DELAY;

	recover->shb_insert_que = 0;
	DEBUG_DELAY;
	recover->shb_insert_prior = 0;
	DEBUG_DELAY;
	
	PRINT_QUE(que, node);
	
}

USHORT LockMgr::lock_state( LBL lock)
{
/**************************************
 *
 *	l o c k _ s t a t e
 *
 **************************************
 *
 * Functional description
 *	Compute the current state of a lock.
 *
 **************************************/

	for (int level = LCK_EX; level >= LCK_null; --level)
		if (lock->lbl_counts[level])
			return level;
	
	/***		
	if (lock->lbl_counts[LCK_EX])
		return LCK_EX;
	
	if (lock->lbl_counts[LCK_PW])
		return LCK_PW;
	
	if (lock->lbl_counts[LCK_SW])
		return LCK_SW;
	
	if (lock->lbl_counts[LCK_PR])
		return LCK_PR;
	
	if (lock->lbl_counts[LCK_SR])
		return LCK_SR;
	
	if (lock->lbl_counts[LCK_null])
		return LCK_null;
	***/
	
	return LCK_none;
}


LockRequest* LockMgr::post_blockage(LRQ request, LBL lock, bool force)
{
/**************************************
 *
 *	p o s t _ b l o c k a g e
 *
 **************************************
 *
 * Functional description
 *	The current request is blocked.  Post blocking notices to
 *	any process blocking the request.
 *
 **************************************/

	LOCK_TRACE(("post_blockage (%ld)\n", request->lrq_lock));

	OWN owner = LOCK_OWNER(request->lrq_owner);
	ASSERT_ACQUIRED;
	//CHECK(owner->own_pending_request == REL_PTR(request));
	CHECK(request->lrq_flags & LRQ_pending);
	PTR next_que_offset;
	LockRequest *blockingRequest = NULL;
	
	for (PSRQ que = QUE_NEXT(lock->lbl_requests); que != &lock->lbl_requests; que = (PSRQ) ABS_PTR(next_que_offset)) 
		{
		PSRQ next_que = QUE_NEXT((*que));
		next_que_offset = REL_PTR(next_que);
		LockRequest *block = (LockRequest*) ((UCHAR *) que - OFFSET(LRQ, lrq_lbl_requests));

		/* Figure out if this lock request is blocking our LockOwner lock request.
		   Of course, our LockOwner request cannot block ourselves.  Compatible
		   requests don't block us, and if there is no AST routine for the
		   request the block doesn't matter as we can't notify anyone.
		   If the owner has marked the request with "LRQ_blocking_seen 
		   then the blocking AST has been delivered and the owner promises
		   to release the lock as soon as possible (so don't bug the owner) */
		   
		if (block == request)
			continue;
		
		if (COMPATIBLE(request->lrq_requested, block->lrq_state))
			continue;

		blockingRequest = block;
		
		if (!block->lrq_ast_routine || ((block->lrq_flags & LRQ_blocking_seen) && !force))
			continue;

		OWN blocking_owner = LOCK_OWNER(block->lrq_owner);
		
		/* Add the blocking request to the list of blocks if it's not
		   there already (LRQ_blocking) */

		if (!(block->lrq_flags & LRQ_blocking)) 
			{
			insert_tail(&blocking_owner->own_blocks, &block->lrq_own_blocks);
			block->lrq_flags |= LRQ_blocking;
			block->lrq_flags &= ~LRQ_blocking_seen;
			}

		if (force) 
			blocking_owner->own_ast_flags &= ~OWN_signaled;

		if ( 
#ifdef SHARED_CACHE
		     blocking_owner != owner && 
#endif
		     signal_owner(blocking_owner, REL_PTR(owner))) 
			{
			/* We can't signal the blocking_owner, assume he has died
			   and purge him out */

			que = (PSRQ) ABS_PTR(que->srq_backward);
			purge_owner(REL_PTR(owner), blocking_owner);
			}
			
		if (block->lrq_state == LCK_EX)
			break;
		}
	
	return blockingRequest;
}


void LockMgr::post_history(USHORT operation,
						 PTR process,
						 PTR lock,
						 PTR request,
						 bool old_version)
{
/**************************************
 *
 *	p o s t _ h i s t o r y
 *
 **************************************
 *
 * Functional description
 *	Post a history item.
 *
 **************************************/
	LockHistory *history;

	LOCK_TRACE(("post_history (%ld)\n", request));

	if (old_version) {
		history = (LockHistory*) ABS_PTR(LOCK_header->lhb_history);
		ASSERT_ACQUIRED;
		LOCK_header->lhb_history = history->his_next;
	}
	else {
		SHB shb;

		ASSERT_ACQUIRED;
		shb = (SHB) ABS_PTR(LOCK_header->lhb_secondary);
		history = (LockHistory*) ABS_PTR(shb->shb_history);
		shb->shb_history = history->his_next;
	}

	history->his_operation = operation;
	history->his_process = process;
	history->his_lock = lock;
	history->his_request = request;
}

void LockMgr::post_pending( LBL lock)
{
/**************************************
 *
 *	p o s t _ p e n d i n g
 *
 **************************************
 *
 * Functional description
 *	There has been a change in state of a lock.  Check pending
 *	requests to see if something can be granted.  If so, do it.
 *
 **************************************/
	OWN owner;
#ifdef DEV_BUILD
	USHORT pending_counter = 0;
#endif

	if (lock->lbl_pending_lrq_count == 0)
		return;

	/* Loop thru granted requests looking for pending conversions.  If one
	   is found, check to see if it can be granted.  Even if a request cannot
	   be granted for compatibility reason, post_wakeup () that owner so that
	   it can post_blockage() to the newly granted owner of the lock. */

	PSRQ que;

	SRQ_LOOP(lock->lbl_requests, que) 
		{
		LRQ request = (LRQ) ((UCHAR *) que - OFFSET(LRQ, lrq_lbl_requests));
		
		if (!(request->lrq_flags & LRQ_pending))
			continue;
			
		if (request->lrq_state)
			{
			DUMP_LOCK_COUNTS(lock);
			--lock->lbl_counts[request->lrq_state];
			UCHAR  temp_state = lock_state(lock);
			
			if (COMPATIBLE(request->lrq_requested, temp_state))
				grant(request, lock);
			else 
				{
#ifdef DEV_BUILD
				pending_counter++;
#endif
				++lock->lbl_counts[request->lrq_state];
				owner = LOCK_OWNER(request->lrq_owner);
				//post_wakeup(owner, REL_PTR(request));
				postWakeup(request);
				
				if (LOCK_ordering) 
					{
					CHECK(lock->lbl_pending_lrq_count >= pending_counter);
					return;
					}
				}
			}
		else if (COMPATIBLE(request->lrq_requested, lock->lbl_state))
			grant(request, lock);
		else 
			{
#ifdef DEV_BUILD
			pending_counter++;
#endif
			owner = LOCK_OWNER(request->lrq_owner);
			//post_wakeup(owner, REL_PTR(request));
			postWakeup(request);
			
			if (LOCK_ordering) 
				{
				CHECK(lock->lbl_pending_lrq_count >= pending_counter);
				return;
				}
			}
		}

	CHECK(lock->lbl_pending_lrq_count == pending_counter);
}

#ifdef OBSOLETE
#ifdef USE_WAKEUP_EVENTS
void LockMgr::post_wakeup( OWN owner, int id)
{
/**************************************
 *
 *	p o s t _ w a k e u p		( U S E _ E V E N T S )
 *
 **************************************
 *
 * Functional description
 *	Wakeup whoever is waiting on a lock.
 *
 **************************************/
	USHORT semaphore;

	/* Note: own_semaphore is set to 1 if we are in wait_for_request() */

	if (!(semaphore = owner->own_semaphore) || (semaphore & OWN_semavail))
		return;

	++LOCK_header->lhb_wakeups;
	owner->own_flags |= OWN_wakeup;
	LOCK_TRACE(("post_wakeup %d for %d\n", owner->own_process_id, id));
	ISC_event_post(owner->own_wakeup);
}
#endif


#ifndef USE_WAKEUP_EVENTS
void LockMgr::post_wakeup( OWN owner, int id)
{
/**************************************
 *
 *	p o s t _ w a k e u p		( n o n - U S E _ E V E N T S )
 *
 **************************************
 *
 * Functional description
 *	Wakeup whoever is waiting on a lock.
 *
 **************************************/

	if (!owner->own_semaphore)
		return;

	if (owner->own_flags & OWN_wakeup)
		return;

	++LOCK_header->lhb_wakeups;
	owner->own_flags |= OWN_wakeup;
	LOCK_TRACE(("post_wakeup %d for %d\n", owner->own_process_id, id));

#ifdef WIN_NT
	ISC_kill(owner->own_process_id, LOCK_wakeup_signal, owner->own_wakeup_hndl);
#else
	ISC_kill(owner->own_process_id, LOCK_wakeup_signal);
#endif
}
#endif
#endif

bool LockMgr::probe_owners( PTR probing_owner_offset)
{
/**************************************
 *
 *	p r o b e _ o w n e r s
 *
 **************************************
 *
 * Functional description
 *	Probe an owner to see if it still exists.  If it doesn't, get
 *	rid of it.
 *
 **************************************/
	bool purged = false;
	ASSERT_ACQUIRED;
	PSRQ que;
	
	SRQ_LOOP(LOCK_header->lhb_owners, que) 
		{
		OWN owner = (OWN) ((UCHAR *) que - OFFSET(OWN, own_lhb_owners));
		
		if (owner->own_flags & OWN_signal)
			signal_owner(owner, (PTR) NULL);
			
		if (owner->own_process_id != LOCK_pid &&
			!ISC_check_process_existence(owner->own_process_id, owner->own_process_uid, FALSE)) 
			{
			que = (PSRQ) ABS_PTR(que->srq_backward);
			purge_owner(probing_owner_offset, owner);
			purged = true;
			}
		}

	return purged;
}


void LockMgr::purge_owner(PTR purging_owner_offset, OWN owner)
{
/**************************************
 *
 *	p u r g e _ o w n e r    
 *
 **************************************
 *
 * Functional description
 *	Purge an owner and all of its associated locks.
 *
 **************************************/
	PSRQ que;
	LRQ request;

	ASSERT_ACQUIRED;

	LOCK_TRACE(("purge_owner (%ld)\n", purging_owner_offset));
	post_history(his_del_owner, purging_owner_offset, REL_PTR(owner), 0, false);

	/* Release any locks that are active. */

	while ((que = QUE_NEXT(owner->own_requests)) != &owner->own_requests) 
		{
		request = (LRQ) ((UCHAR*) que - OFFSET(LRQ, lrq_own_requests));
		release_request(request);
		}

	/* Release any repost requests left dangling on blocking queue. */

	while ((que = QUE_NEXT(owner->own_blocks)) != &owner->own_blocks) 
		{
		request = (LRQ) ((UCHAR *) que - OFFSET(LRQ, lrq_own_blocks));
		remove_que(&request->lrq_own_blocks);
		request->lrq_type = type_null;
		insert_tail(&LOCK_header->lhb_free_requests, &request->lrq_lbl_requests);
		}

	shutdown_blocking_thread(owner);
	
	if (owner->own_blocking_event)
		{
		LockEvent *event = LOCK_EVENT(owner->own_blocking_event);
		owner->own_blocking_event = 0;
		releaseEvent(event);
		}

	/* Release any available events */
	
	for (PTR eventOffset; eventOffset = owner->own_events;)
		{
		LockEvent *event = LOCK_EVENT(eventOffset);
		owner->own_events = event->next;
		releaseEvent(event);
		}

	/* Release owner block */

	remove_que(&owner->own_lhb_owners);
	insert_tail(&LOCK_header->lhb_free_owners, &owner->own_lhb_owners);

	owner->own_owner_type = 0;
	owner->own_owner_id = 0;
	owner->own_process_id = 0;
	owner->own_flags = 0;
	
#ifndef SHARED_CACHE
	/* this is resued in SHARED_CACHE mode so we can't release it */
	ISC_event_fini(owner->own_stall);
#endif
}


void LockMgr::remove_que( PSRQ node)
{
/**************************************
 *
 *	r e m o v e _ q u e
 *
 **************************************
 *
 * Functional description
 *	Remove a node from a self-relative que.
 *
 *	To handle the event of the process terminating during
 *	the removal of the node, we set shb_remove_node to the
 *	node to be removed.  Then, should we be unsuccessful
 *	in the node removal, the next process into the lock table
 *	will notice the uncompleted work and complete it.
 *
 *	Note that the work is completed by again calling this routine,
 *	specifing the node to be removed.  As the prior and following
 *	nodes may have changed prior to the crash, we need to redo the
 *	work only based on what is in <node>.  
 *
 **************************************/
	PSRQ que;
	SHB recover;

	ASSERT_ACQUIRED;
	recover = (SHB) ABS_PTR(LOCK_header->lhb_secondary);
	DEBUG_DELAY;
	recover->shb_remove_node = REL_PTR(node);
	DEBUG_DELAY;

	que = (PSRQ) ABS_PTR(node->srq_forward);

	PRINT_QUE(que, node);

/* The next link might point back to us, or our prior link should
 * the backlink change occur before a crash
 */
	CHECK(que->srq_backward == REL_PTR(node) ||
		  que->srq_backward == node->srq_backward);
	DEBUG_DELAY;
	que->srq_backward = node->srq_backward;

	DEBUG_DELAY;
	que = (PSRQ) ABS_PTR(node->srq_backward);

/* The prior link might point forward to us, or our following link should
 * the change occur before a crash
 */
	CHECK(que->srq_forward == REL_PTR(node) ||
		  que->srq_forward == node->srq_forward);
	DEBUG_DELAY;
	que->srq_forward = node->srq_forward;

	DEBUG_DELAY;
	recover->shb_remove_node = 0;
	DEBUG_DELAY;

	PRINT_QUE(que, NULL);

/* To prevent trying to remove this entry a second time, which could occur
 * for instance, when we're removing an owner, and crash while removing
 * the owner's blocking requests, reset the que entry in this node.
 * Note that if we get here, shb_remove_node has been cleared, so we
 * no longer need the queue information.
 */

	QUE_INIT((*node));
}


void LockMgr::release( PTR owner_offset)
{
/**************************************
 *
 *	r e l e a s e
 *
 **************************************
 *
 * Functional description
 *	Release the mapped lock file.  Advance the event count to wake up
 *	anyone waiting for it.  If there appear to be blocking items
 *	posted.
 *
 **************************************/

	OWN owner;
	
	LOCK_TRACE(("release (%ld)\n", owner_offset));

	if (owner_offset && LOCK_header->lhb_active_owner != owner_offset)
		bug(NULL, "release when not owner");

	if (LOCK_solaris_stall) 
		{
		OWN first_owner;

		if (owner_offset)
			owner = LOCK_OWNER(owner_offset);

		/* Rotate first owner to tail of active owners' queue
		   in search of mutex-starved owners. */

		first_owner = (OWN) ((UCHAR *) QUE_NEXT(LOCK_header->lhb_owners) -
							 OFFSET(OWN, own_lhb_owners));
		remove_que(&first_owner->own_lhb_owners);
		insert_tail(&LOCK_header->lhb_owners, &first_owner->own_lhb_owners);

		/* If others stepped aside to let us run then wake them up now. */

		if (owner_offset && owner->own_flags & OWN_starved) 
			{
			PSRQ que;
			owner->own_flags &= ~(OWN_starved | OWN_blocking);

			SRQ_LOOP(LOCK_header->lhb_owners, que) 
				{
				owner = (OWN) ((UCHAR *) que - OFFSET(OWN, own_lhb_owners));
				if (owner->own_flags & OWN_blocking) 
					{
					owner->own_flags &= ~OWN_blocking;
					ISC_event_post(owner->own_stall);
					}
				}
			}

	}

#ifdef VALIDATE_LOCK_TABLE
	/* Validate the lock table occasionally (every 500 releases) */

	if ((LOCK_header->lhb_acquires % (HISTORY_BLOCKS / 2)) == 0)
		validate_lhb(LOCK_header);
#endif

#ifdef MVS
	memset(&LOCK_thread, 0xff, sizeof(LOCK_thread));
#else
	LOCK_thread = (THREAD_ID)-1;
#endif
	LOCK_header = (LHB) -1;
	release_mutex();
}


void LockMgr::release_mutex(void)
{
/**************************************
 *
 *	r e l e a s e _ m u t e x
 *
 **************************************
 *
 * Functional description
 *	Release the mapped lock file.  Advance the event count to wake up
 *	anyone waiting for it.  If there appear to be blocking items
 *	posted.
 *
 **************************************/

	if (!LOCK_table->lhb_active_owner)
		bug(NULL, "release when not active");

	LOCK_table->lhb_active_owner = 0;

	if (ISC_mutex_unlock(MUTEX))
		bug(NULL, "semop failed (release)");

}


void LockMgr::release_request( LRQ request)
{
/**************************************
 *
 *	r e l e a s e _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Release a request.  This is called both by release lock
 *	and by the cleanup handler.
 *
 **************************************/

	ASSERT_ACQUIRED;
	LockBlock *lock = LOCK_BLOCK(request->lrq_lock);

	/* Start by disconnecting request from both lock and process */

	remove_que(&request->lrq_lbl_requests);
	remove_que(&request->lrq_own_requests);

	request->lrq_type = type_null;
	insert_tail(&LOCK_header->lhb_free_requests, &request->lrq_lbl_requests);

	/* If the request is marked as blocking, clean it up. */

	if (request->lrq_flags & LRQ_blocking)
		{
		remove_que(&request->lrq_own_blocks);
		request->lrq_flags &= ~LRQ_blocking;
		}

	request->lrq_flags &= ~LRQ_blocking_seen;

	/* If it has an outstanding event, release it now */
	
	if (request->lrq_wakeup_event)
		{
		LockOwner *owner = LOCK_OWNER(request->lrq_owner);
		LockEvent *event = LOCK_EVENT(request->lrq_wakeup_event);
		releaseEvent(event, owner);
		request->lrq_wakeup_event = 0;
		}

	/* Update counts if we are cleaning up something we're waiting on!
	   This should only happen if we are purging out an owner that died */
	   
	if (request->lrq_flags & LRQ_pending) 
		clearPending(request);

	/* If there are no outstanding requests, release the lock */

	if (QUE_EMPTY(lock->lbl_requests))
		{
		CHECK(lock->lbl_pending_lrq_count == 0);
		remove_que(&lock->lbl_lhb_hash);
		remove_que(&lock->lbl_lhb_data);
		lock->lbl_type = type_null;
		insert_tail(&LOCK_header->lhb_free_locks, &lock->lbl_lhb_hash);
		
		return;
		}

	/* Re-compute the state of the lock and post any compatible pending
	   requests. */

	DUMP_LOCK_COUNTS(lock);
	if ((request->lrq_state != LCK_none) && !(--lock->lbl_counts[request->lrq_state]))
		{
		lock->lbl_state = lock_state(lock);
		
		if (request->lrq_state != LCK_null)
			{
			post_pending(lock);
			return;
			}
		}

	/* If a lock enqueue failed because of a deadlock or timeout, the pending
	   request may be holding up compatible, pending lock requests queued
	   behind it. 
	   Or, even if this request had been granted, it's now being released,
	   so we might be holding up requests or conversions queued up behind. */

	post_pending(lock);
}


void LockMgr::shutdown_blocking_thread(LockOwner *owner)
{
/**************************************
 *
 *	s h u t d o w n _ b l o c k i n g _ t h r e a d
 *
 **************************************
 *
 * Functional description
 *	Perform a controlled shutdown of blocking thread
 *	to avoid tragic misunderstandings when unmapping
 *	memory.
 *
 **************************************/

#ifdef SHARED_CACHE
	Sync sync (&lockManagerMutex, "LockMgr::shutdown_blocking_thread");
	sync.lock (Exclusive);
#endif

	if (!blocking_action_thread_initialized || !owner->own_blocking_event)
		return;
	
	PTR ownerOffset = REL_PTR(owner);
	shutdown = true;
	LockEvent *event = LOCK_EVENT(owner->own_blocking_event);
	release(ownerOffset);
	
#ifndef SHARED_CACHE
	if (LOCK_dbb->syncAst.ourExclusiveLock())
		LOCK_dbb->syncAst.unlock();

	if (LOCK_dbb->syncConnection.ourExclusiveLock())
		LOCK_dbb->syncConnection.unlock();
#endif
	
	event->event.post();
	THD_thread_wait(&blocking_action_thread_handle);
	blocking_action_thread_initialized = false;
	shutdownComplete.wait(shutdownCount, 2000000);
	shutdownComplete.fini();
	acquire(ownerOffset);
}

int LockMgr::signal_owner( OWN blocking_owner, PTR blocked_owner_offset)
{
/**************************************
 *
 *	s i g n a l _ o w n e r
 *
 **************************************
 *
 * Functional description
 *	Send a signal to a process.
 *
 *      The second parameter is a possible offset to the
 *      blocked owner (or NULL), which is passed on to
 *      blocking_action2().
 *
 **************************************/

	/*post_history (his_signal, LOCK_header->lhb_iactive_owner, REL_PTR (blocking_owner), 0, true);*/

	ASSERT_ACQUIRED;

	/* If a process, other than ourselves, hasn't yet seen a signal
	   that was sent, don't bother to send another one. */

	DEBUG_DELAY;

	if (blocking_owner->own_ast_flags & OWN_signaled)
		{
		DEBUG_MSG(1, ("signal_owner (%ld) - skipped OWN_signaled\n", blocking_owner->own_process_id));
		return FB_SUCCESS;
		}

	blocking_owner->own_ast_flags |= OWN_signaled;
	DEBUG_DELAY;
	blocking_owner->own_flags &= ~OWN_signal;
	DEBUG_DELAY;
	LOCK_TRACE(("signal_owner (%d)\n", REL_PTR(blocking_owner)));
	
	if (blocking_owner->own_blocking_event)
		{
		LockEvent *event = LOCK_EVENT(blocking_owner->own_blocking_event);
		//printf ("waking up for event %d\n", blocking_owner->own_blocking_event);
		event->event.post();
		return FB_SUCCESS;
		}
	
	DEBUG_MSG(1, ("signal_owner - direct delivery failed\n"));
	blocking_owner->own_ast_flags &= ~OWN_signaled;
	DEBUG_DELAY;
	blocking_owner->own_flags |= OWN_signal;

	/* Conclude that the owning process is dead */
	
	return FB_FAILURE;
}

#ifdef VALIDATE_LOCK_TABLE


#define	EXPECT_inuse	0
#define	EXPECT_freed	1

#define	RECURSE_yes	0
#define	RECURSE_not	1

void LockMgr::validate_history( PTR history_header)
{
/**************************************
 *
 *	v a l i d a t e _ h i s t o r y
 *
 **************************************
 *
 * Functional description
 *	Validate a circular list of history blocks.
 *
 **************************************/
	LockHistory *history;
	USHORT count = 0;

	LOCK_TRACE(("validate_history: %ld\n", history_header));

	for (history = (LockHistory*) ABS_PTR(history_header); true;
		 history = (LockHistory*) ABS_PTR(history->his_next)) {
		count++;
		CHECK(history->his_type == type_his);
// The following condition is always true because UCHAR >= 0
//		CHECK(history->his_operation >= 0);
		CHECK(history->his_operation <= his_MAX);
		if (history->his_next == history_header)
			break;
		CHECK(count <= HISTORY_BLOCKS);
	}
}
#endif


#ifdef VALIDATE_LOCK_TABLE
void LockMgr::validate_lhb( LHB LockHeader)
{
/**************************************
 *
 *	v a l i d a t e _ l h b
 *
 **************************************
 *
 * Functional description
 *	Validate the LHB structure and everything that hangs off of it.
 *
 **************************************/

	PSRQ que, que_next;
	OWN owner;
	LBL lock;
	LRQ request;

	LOCK_TRACE(("validate_lhb:\n"));

	/* Prevent recursive reporting of validation errors */
	
	if (LOCK_bugcheck)
		return;

	CHECK(LockHeader != NULL);
	CHECK(LockHeader->lhb_type == type_lhb);
	CHECK(LockHeader->lhb_version == LHB_VERSION);

	validate_shb(LockHeader->lhb_secondary);
	if (LockHeader->lhb_active_owner > 0)
		validate_owner(LockHeader->lhb_active_owner, EXPECT_inuse);

	SRQ_LOOP(LockHeader->lhb_owners, que) {
		/* Validate that the next backpointer points back to us */
		que_next = QUE_NEXT((*que));
		CHECK(que_next->srq_backward == REL_PTR(que));

		owner = (OWN) ((UCHAR *) que - OFFSET(OWN, own_lhb_owners));
		validate_owner(REL_PTR(owner), EXPECT_inuse);
	}

	SRQ_LOOP(LockHeader->lhb_free_owners, que) {
		/* Validate that the next backpointer points back to us */
		que_next = QUE_NEXT((*que));
		CHECK(que_next->srq_backward == REL_PTR(que));

		owner = (OWN) ((UCHAR *) que - OFFSET(OWN, own_lhb_owners));
		validate_owner(REL_PTR(owner), EXPECT_freed);
	}

	SRQ_LOOP(LockHeader->lhb_free_locks, que) {
		/* Validate that the next backpointer points back to us */
		que_next = QUE_NEXT((*que));
		CHECK(que_next->srq_backward == REL_PTR(que));

		lock = (LBL) ((UCHAR *) que - OFFSET(LBL, lbl_lhb_hash));
		validate_lock(REL_PTR(lock), EXPECT_freed, (PTR) 0);
	}

	SRQ_LOOP(LockHeader->lhb_free_requests, que) 
		{
		/* Validate that the next backpointer points back to us */
		
		que_next = QUE_NEXT((*que));
		CHECK(que_next->srq_backward == REL_PTR(que));

		request = (LRQ) ((UCHAR *) que - OFFSET(LRQ, lrq_lbl_requests));
		validate_request(REL_PTR(request), EXPECT_freed, RECURSE_not);
		}

	CHECK(LockHeader->lhb_used <= LockHeader->lhb_length);

	validate_history(LockHeader->lhb_history);
/* validate_semaphore_mask (LockHeader->lhb_mask); */

	CHECK(LockHeader->lhb_reserved[0] == 0);
	CHECK(LockHeader->lhb_reserved[1] == 0);

	DEBUG_MSG(0, ("validate_lhb completed:\n"));

}
#endif


#ifdef VALIDATE_LOCK_TABLE
void LockMgr::validate_lock( PTR lock_ptr, USHORT freed, PTR lrq_ptr)
{
/**************************************
 *
 *	v a l i d a t e _ l o c k
 *
 **************************************
 *
 * Functional description
 *	Validate the lock structure and everything that hangs off of it.
 *
 **************************************/
	LBL lock;
	PSRQ que, que_next;
	LRQ request;
	USHORT found;
	UCHAR highest_request;
	USHORT found_pending;
	USHORT i;
	USHORT direct_counts[LCK_max];
	LockBlock lock_copy;

	LOCK_TRACE(("validate_lock: %ld\n", lock_ptr));

	lock = LOCK_BLOCK(lock_ptr);
	lock_copy = *lock;

	if (freed == EXPECT_freed)
		CHECK(lock->lbl_type == type_null)
			else
		CHECK(lock->lbl_type == type_lbl);

// The following condition is always true because UCHAR >= 0
//	CHECK(lock->lbl_state >= LCK_none);
	CHECK(lock->lbl_state < LCK_max);

	CHECK(lock->lbl_length <= lock->lbl_size);

/* The lbl_count's should never roll over to be negative */
	for (i = 0; i < FB_NELEM(lock->lbl_counts); i++)
		CHECK(!(lock->lbl_counts[i] & 0x8000))

/* The count of pending locks should never roll over to be negative */
			CHECK(!(lock->lbl_pending_lrq_count & 0x8000));

	memset(direct_counts, 0, sizeof(direct_counts));

	found = 0;
	found_pending = 0;
	highest_request = LCK_none;
	SRQ_LOOP(lock->lbl_requests, que) {
		/* Validate that the next backpointer points back to us */
		que_next = QUE_NEXT((*que));
		CHECK(que_next->srq_backward == REL_PTR(que));

		/* Any requests of a freed lock should also be free */
		CHECK(freed == EXPECT_inuse);

		request = (LRQ) ((UCHAR *) que - OFFSET(LRQ, lrq_lbl_requests));
		/* Note: Don't try to validate_request here, it leads to recursion */

		if (REL_PTR(request) == lrq_ptr)
			found++;

		CHECK(found <= 1);		/* check for a loop in the queue */

		/* Request must be for this lock */
		CHECK(request->lrq_lock == lock_ptr);

		if (request->lrq_requested > highest_request)
			highest_request = request->lrq_requested;

		/* If the request is pending, then it must be incompatible with current
		   state of the lock - OR lock_ordering is enabled and there is at 
		   least one pending request in the queue (before this request 
		   but not including it). */

		if (request->lrq_flags & LRQ_pending) 
			{
			CHECK(!COMPATIBLE(request->lrq_requested, lock->lbl_state) ||
				  (LOCK_ordering && found_pending));

			/* The above condition would probably be more clear if we 
			   wrote it as the following:

			   CHECK (!COMPATIBLE (request->lrq_requested, lock->lbl_state) ||
			   (LOCK_ordering && found_pending &&
			   COMPATIBLE (request->lrq_requested, lock->lbl_state)));

			   but that would be redundant
			 */

			found_pending++;
			}

		/* If the request is NOT pending, then it must be rejected or 
		   compatible with the current state of the lock */
		   
		if (!(request->lrq_flags & LRQ_pending)) 
			{
			CHECK((request->lrq_flags & LRQ_rejected) ||
				  (request->lrq_requested == lock->lbl_state) ||
				  COMPATIBLE(request->lrq_requested, lock->lbl_state));

			}

		direct_counts[request->lrq_state]++;
		}

	if ((freed == EXPECT_inuse) && (lrq_ptr != 0))
		CHECK(found == 1);		/* request is in lock's queue */


	if (freed == EXPECT_inuse) {
		CHECK(found_pending == lock->lbl_pending_lrq_count);

		/* The counter in the lock header should match the actual counts
		   lock->lbl_counts [LCK_null] isn't maintained, so don't check it */
		for (i = LCK_null; i < LCK_max; i++)
			CHECK(direct_counts[i] == lock->lbl_counts[i]);
	}

	if (lock->lbl_parent && (freed == EXPECT_inuse))
		validate_lock(lock->lbl_parent, EXPECT_inuse, (PTR) 0);

}
#endif


#ifdef VALIDATE_LOCK_TABLE
void LockMgr::validate_owner( PTR own_ptr, USHORT freed)
{
/**************************************
 *
 *	v a l i d a t e _ o w n e r
 *
 **************************************
 *
 * Functional description
 *	Validate the owner structure and everything that hangs off of it.
 *
 **************************************/
	PSRQ que, que_next;
	PSRQ que2, que2_next;
	LRQ request;
	LRQ request2;
	USHORT found;

	LOCK_TRACE(("validate_owner: %ld\n", own_ptr));

	OWN owner = LOCK_OWNER(own_ptr);
	LockOwner owner_copy = *owner;

	/* Note that owner->own_pending_request can be reset without the lock
	 * table being acquired - eg: by another process.  That being the case,
	 * we need to stash away a copy of it for validation.
	 */
	 
	PTR owner_own_pending_request = owner->own_pending_request;
	CHECK(owner->own_type == type_own);
	
	if (freed == EXPECT_freed)
		CHECK(owner->own_owner_type == 0)
	else 
		{
		CHECK(owner->own_owner_type > 0);
		CHECK(owner->own_owner_type <= 4	/* LCK_OWNER_transaction */
			  || owner->own_owner_type == LCK_OWNER_dummy_process);	
		}

	CHECK(owner->own_acquire_time <= LOCK_header->lhb_acquires);
	CHECK(owner->own_acquire_realtime == 0 || static_cast<time_t>(owner->own_acquire_realtime) <= GET_TIME);

	/* Check that no invalid flag bit is set */
	
	CHECK(!(owner-> own_flags & 
			~(OWN_blocking | OWN_scanned | OWN_manager | OWN_signal | OWN_wakeup | OWN_starved)));

	/* Check that no invalid flag bit is set */
	
	CHECK(!(owner->own_ast_flags & ~(OWN_signaled)));

	/* Check that no invalid flag bit is set */
	
	CHECK(!(owner->own_ast_hung_flags & ~(OWN_hung)));

	/* Can't both be signal & signaled */
	
	if (owner->own_flags & OWN_signal)
		CHECK(!(owner->own_ast_flags & OWN_signaled));

	if (owner->own_ast_flags & OWN_signaled)
		CHECK(!(owner->own_flags & OWN_signal));

	SRQ_LOOP(owner->own_requests, que) 
		{
		/* Validate that the next backpointer points back to us */
		PSRQ que_next = QUE_NEXT((*que));
		CHECK(que_next->srq_backward == REL_PTR(que));

		CHECK(freed == EXPECT_inuse);	/* should not be in loop for freed owner */

		request = (LRQ) ((UCHAR *) que - OFFSET(LRQ, lrq_own_requests));
		validate_request(REL_PTR(request), EXPECT_inuse, RECURSE_not);
		CHECK(request->lrq_owner == own_ptr);

		/* Make sure that request marked as blocking also exists in the blocking list */

		if (request->lrq_flags & LRQ_blocking) 
			{
			found = 0;
			SRQ_LOOP(owner->own_blocks, que2) 
				{
				/* Validate that the next backpointer points back to us */
				
				que2_next = QUE_NEXT((*que2));
				CHECK(que2_next->srq_backward == REL_PTR(que2));
				request2 = (LRQ) ((UCHAR *) que2 - OFFSET(LRQ, lrq_own_blocks));
				CHECK(request2->lrq_owner == own_ptr);

				if (REL_PTR(request2) == REL_PTR(request))
					found++;

				CHECK(found <= 1);	/* watch for loops in queue */
				}
			CHECK(found == 1);	/* request marked as blocking must be in blocking queue */
			}
		}

	/* Check each item in the blocking queue */

	SRQ_LOOP(owner->own_blocks, que) 
		{
		/* Validate that the next backpointer points back to us */
		que_next = QUE_NEXT((*que));
		CHECK(que_next->srq_backward == REL_PTR(que));

		CHECK(freed == EXPECT_inuse);	/* should not be in loop for freed owner */

		request = (LRQ) ((UCHAR *) que - OFFSET(LRQ, lrq_own_blocks));
		validate_request(REL_PTR(request), EXPECT_inuse, RECURSE_not);

		LOCK_TRACE(("Validate own_block: %ld\n", REL_PTR(request)));

		CHECK(request->lrq_owner == own_ptr);

		/* A repost won't be in the request list */

		if (request->lrq_flags & LRQ_repost)
			continue;

		/* Make sure that each block also exists in the request list */

		found = 0;
		
		SRQ_LOOP(owner->own_requests, que2) 
			{
			/* Validate that the next backpointer points back to us */
			que2_next = QUE_NEXT((*que2));
			CHECK(que2_next->srq_backward == REL_PTR(que2));

			request2 = (LRQ) ((UCHAR *) que2 - OFFSET(LRQ, lrq_own_requests));
			CHECK(request2->lrq_owner == own_ptr);

			if (REL_PTR(request2) == REL_PTR(request))
				found++;

			CHECK(found <= 1);	/* watch for loops in queue */
			}
		CHECK(found == 1);		/* blocking request must be in request queue */
		}

	/* If there is a pending request, make sure it is valid, that it
	  * exists in the queue for the lock.
	  */
	  
	if (owner_own_pending_request && (freed == EXPECT_inuse)) 
		{
		bool found_pending;
		LRQ request;
		LBL lock;
		LRQ pending;
		PSRQ que_of_lbl_requests;

		/* Make sure pending request is valid, and we LockOwner it */
		
		request = LOCK_REQUEST(owner_own_pending_request);
		validate_request(REL_PTR(request), EXPECT_inuse, RECURSE_not);
		CHECK(request->lrq_owner == own_ptr);

		/* Make sure the lock the request is for is valid */
		
		lock = LOCK_BLOCK(request->lrq_lock);
		validate_lock(REL_PTR(lock), EXPECT_inuse, (PTR) 0);

		/* Make sure the pending request is on the list of requests for the lock */

		found_pending = false;
		
		SRQ_LOOP(lock->lbl_requests, que_of_lbl_requests) 
			{
			pending = (LRQ) ((UCHAR *) que_of_lbl_requests - OFFSET(LRQ, lrq_lbl_requests));
			
			if (REL_PTR(pending) == owner_own_pending_request) 
				{
				found_pending = true;
				break;
				}
			}

		/* pending request must exist in the lock's request queue */
		
		CHECK(found_pending);

		/* Either the pending request is the same as what we stashed away, or it's
		 * been cleared by another process without the lock table acquired.  */
		 
		CHECK((owner_own_pending_request == owner->own_pending_request) ||
			  !owner->own_pending_request);
		}
}
#endif


#ifdef VALIDATE_LOCK_TABLE
void LockMgr::validate_request( PTR lrq_ptr, USHORT freed, USHORT recurse)
{
/**************************************
 *
 *	v a l i d a t e _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Validate the request structure and everything that hangs off of it.
 *
 **************************************/
	LRQ request;
	LockRequest request_copy;

	LOCK_TRACE(("validate_request: %ld\n", lrq_ptr));

	request = LOCK_REQUEST(lrq_ptr);
	request_copy = *request;

	if (freed == EXPECT_freed)
		CHECK(request->lrq_type == type_null)
			else
		CHECK(request->lrq_type == type_lrq);

/* Check that no invalid flag bit is set */
	CHECK(!
		  (request->
		   lrq_flags & ~(LRQ_blocking | LRQ_pending | LRQ_converting |
						 LRQ_rejected | LRQ_timed_out | LRQ_deadlock |
						 LRQ_repost | LRQ_scanned | LRQ_blocking_seen)));

/* LRQ_converting & LRQ_timed_out are defined, but never actually used */
	CHECK(!(request->lrq_flags & (LRQ_converting | LRQ_timed_out)));

/* Once a request is rejected, it CAN'T be pending any longer */
	if (request->lrq_flags & LRQ_rejected)
		CHECK(!(request->lrq_flags & LRQ_pending));

/* Can't both be scanned & marked for deadlock walk */
	CHECK((request->lrq_flags & (LRQ_deadlock | LRQ_scanned)) !=
		  (LRQ_deadlock | LRQ_scanned));

	CHECK(request->lrq_requested < LCK_max);
	CHECK(request->lrq_state < LCK_max);

	if (freed == EXPECT_inuse) {
		if (recurse == RECURSE_yes)
			validate_owner(request->lrq_owner, EXPECT_inuse);

		/* Reposted request are pseudo requests, not attached to any real lock */
		if (!(request->lrq_flags & LRQ_repost))
			validate_lock(request->lrq_lock, EXPECT_inuse, REL_PTR(request));
	}
}
#endif


#ifdef VALIDATE_LOCK_TABLE
void LockMgr::validate_shb( PTR shb_ptr)
{
/**************************************
 *
 *	v a l i d a t e _ s h b
 *
 **************************************
 *
 * Functional description
 *	Validate the SHB structure and everything that hangs off of
 *	it.
 *	Of course, it would have been a VERY good thing if someone
 *	had moved this into LockHeader when we made a unique v4 lock
 *	manager....
 *	1995-April-13 David Schnepper 
 *
 **************************************/

	LOCK_TRACE(("validate_shb: %ld\n", shb_ptr));
	SHB secondary_header = (SHB) ABS_PTR(shb_ptr);
	CHECK(secondary_header->shb_type == type_shb);
	validate_history(secondary_header->shb_history);

	for (int i = 0; i < FB_NELEM(secondary_header->shb_misc); i++)
		CHECK(secondary_header->shb_misc[i] == 0);
}
#endif


USHORT LockMgr::wait_for_request(LRQ request,
							   SSHORT lck_wait, ISC_STATUS * status_vector, int *req_owner)
{
/**************************************
 *
 *	w a i t _ f o r _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	There is a request that needs satisfaction, but is waiting for
 *	somebody else.  Mark the request as pending and go to sleep until
 *	the lock gets poked.  When we wake up, see if somebody else has
 *	cleared the pending flag.  If not, go back to sleep.
 * Returns
 *	FB_SUCCESS	- we waited until the request was granted or rejected
 * 	FB_FAILURE - Insufficient resouces to wait (eg: no semaphores)
 *
 **************************************/

	LRQ blocking_request;
	int ret;
	SLONG timeout, current_time, deadlock_timeout, lock_timeout;
#ifdef DEV_BUILD
	ULONG repost_counter = 0;
#endif

	ASSERT_ACQUIRED;
	++LOCK_header->lhb_waits;
	SLONG scan_interval = LOCK_header->lhb_scan_interval;

	/* lrq_count will be off if we wait for a pending request */

	CHECK(!(request->lrq_flags & LRQ_pending));

	request->lrq_flags &= ~LRQ_rejected;
	request->lrq_flags |= LRQ_pending;
	PTR owner_offset = request->lrq_owner;
	PTR lock_offset = request->lrq_lock;
	LBL lock = LOCK_BLOCK(lock_offset);
	lock->lbl_pending_lrq_count++;

	LOCK_TRACE(("wait_for_request (%ld)\n", owner_offset));

	
	/* If ordering is in effect, and this is a conversion of
		an existing lock in LCK_none state - put the lock to the
		end of the list so it's not taking cuts in the lineup */
		
	if (LOCK_ordering && !request->lrq_state) 
		{
		remove_que(&request->lrq_lbl_requests);
		insert_tail(&lock->lbl_requests, &request->lrq_lbl_requests);
		}

	OWN owner = LOCK_OWNER(owner_offset);
	//CHECK(owner->own_pending_request == 0);
	//PTR request_offset = owner->own_pending_request = REL_PTR(request);
	PTR request_offset = REL_PTR(request);
	insert_tail(&owner->own_pending, &request->lrq_own_pending);
	owner->own_flags &= ~(OWN_scanned | OWN_wakeup);


	/* Post blockage.  If the blocking owner has disappeared, the blockage
	   may clear spontaneously. */

	LockRequest *blockingRequest = post_blockage(request, lock, false);
	post_history(his_wait, owner_offset, lock_offset, REL_PTR(request), true);
	//release(owner_offset);
	current_time = GET_TIME;

	/* If a lock timeout was requested (wait < 0) then figure
	   out the time when the lock request will timeout */

	if (lck_wait < 0)
		lock_timeout = current_time + (-lck_wait);
		
	deadlock_timeout = current_time + scan_interval;

	/* Wait in a loop until the lock becomes available */

	//while (true)
	for (bool first = true;; first = false)
		{
		/* NOTE: Many operations in this loop are done without having
		 *       the lock table acquired - for performance reasons
		 */

		/* Before starting to wait - look to see if someone resolved
		   the request for us - if so we're out easy! */

		//acquire(owner_offset);
		request = LOCK_REQUEST(request_offset);
		
		if (!(request->lrq_flags & LRQ_pending)) 
			break;
			
		/* recalculate when we next want to wake up, the lesser of a
		   deadlock scan interval or when the lock request wanted a timeout */

		timeout = deadlock_timeout;
		
		if (lck_wait < 0 && lock_timeout < deadlock_timeout)
			timeout = lock_timeout;

		/* Prepare to wait for a timeout or a wakeup from somebody else.  */

#ifdef OBSOLETE //USE_WAKEUP_EVENTS
		SLONG value;
		AsyncEvent* event_ptr;
		//owner = LOCK_OWNER(owner_offset);
		owner = GET_OWNER(offset_offset);
		
		if (!(owner->own_flags & OWN_wakeup))
			{
			/* Re-initialize value each time thru the loop to make sure that the
			   semaphore looks 'un-poked'. */

			/* YYY: NOTE - couldn't there be "missing events" here? */
			/* We don't LockOwner the lock_table at this point, but we modify it! */

			event_ptr = owner->own_wakeup;
			value = ISC_event_clear(event_ptr);
			event_ptr = owner->own_wakeup;

			if (!(owner->own_flags & OWN_wakeup))
				{
				/* Until here we've been the only thread in the engine (We no longer
				release engine in LCK.C module to avoid problems with more than
				one thread running in out not-yet-MT-safe engine). We already
				tried to execute AST routine provided (sometimes) by blocking
				owner hoping our lock request would be satisfied. Did not help!
				The only thing we could do now is to wait. But let's do it without
				monopolizing the engine
				*/

				release(owner_offset);
				AST_ENABLE;
				ret = ISC_event_wait(1, &event_ptr, &value,
									 (timeout - current_time) * 1000000,
									 lock_alarm_handler, event_ptr);
				AST_DISABLE;
				LOCK_TRACE(("lock wait wakeup for %d\n", request_offset));
				acquire(owner_offset);
				}
			}

#endif

		CHECK(request->lrq_requested != request->lrq_state);

		ret = wait(owner, request, LOCK_scan_interval * 1000000);		
//		ret = FB_SUCCESS;
		LOCK_TRACE(("lock wait wakeup for %d\n", request_offset));
		

		/* We've worken up from the wait - now look around and see
		   why we wokeup */

		/* If somebody else has resolved the lock, we're done */

		request = LOCK_REQUEST(request_offset);
		
		if (!(request->lrq_flags & LRQ_pending))
			{
			LOCK_TRACE(("lock grant wakeup for %d\n", request_offset));
			break;
			}

		/* See if we wokeup due to another owner deliberately waking us up
		   ret==FB_SUCCESS --> we were deliberately worken up
		   ret==FB_FAILURE --> we still don't know why we work up */

		/* Only if we came out of the ISC_event_wait() because of a post_wakeup()
		   by another owner is OWN_wakeup set.  This is the only FB_SUCCESS case. */

#ifdef OBSOLETE
		owner = GET_OWNER(owner_offset);
		
		if (owner->own_flags & OWN_wakeup)
			ret = FB_SUCCESS;
#ifdef USE_WAKEUP_EVENTS
		else
			ret = FB_FAILURE;
#endif
#endif

		current_time = GET_TIME;

		/* See if we workup for a bogus reason - if so 
		   go right back to sleep.  We wokeup bogus unless
		   - we weren't deliberatly woken up
		   - it's not time for a deadlock scan.
		   - it's not time for the lock timeout to expire.
		   Bogus reasons for wakeups include signal reception on some
		   platforms (eg: SUN4)
		   Note: we allow a 1 second leaway on declaring a bogus
		   wakeup due to timing differences (we use seconds here,
		   ISC_event_wait() uses finer granularity) */

		if ((ret != FB_SUCCESS) && (current_time + 1 < timeout)) 
			continue;

		/* We apparently woke up for some real reason.  
		   Make sure everyone is still with us.  Then see if we're still
		   blocked. */

		//acquire(owner_offset);
		request = LOCK_REQUEST(request_offset);	/* Re-init after potential remap */
		lock = LOCK_BLOCK(lock_offset);
		owner = LOCK_OWNER(owner_offset);
		owner->own_flags &= ~OWN_wakeup;

		/* Now that we LockOwner the lock table, see if the request was resolved
		   while we were waiting for the lock table */

		if (!(request->lrq_flags & LRQ_pending)) 
			{
			LOCK_TRACE(("lock grant wakeup(2) for %d\n", request_offset));
			//release(owner_offset);
			break;
			}

		current_time = GET_TIME;	/* refetch due to wait in acquire() */

		/* See if we've waited beyond the lock timeout - if so we
		   mark our LockOwner request as rejected */

		if (lck_wait < 0 && lock_timeout <= current_time)
			{
			/* We're going to reject our lock - it's the callers responsibility
			   to do cleanup and make sure post_pending() is called to wakeup
			   other owners we might be blocking */
			
			request->lrq_flags |= LRQ_rejected;
			clearPending(request);
			//release(owner_offset);
			break;
			}

		/* We're going to do some real work - reset when we next want to
		   do a deadlock scan */
		   
		deadlock_timeout = current_time + scan_interval;


#if 0
		if (status_vector)
		{
			/* deadlock condition, return an error */
			request->lrq_flags |= LRQ_rejected;
			clearPending(request);

			*status_vector++ = isc_arg_gds;
			*status_vector++ = isc_deadlock;
			*status_vector++ = isc_arg_end;
			release(owner_offset);
			return FB_FAILURE;
		}
#endif

		/* Handle lock event first */
		
		if (ret == FB_SUCCESS)
			{
			/* Someone posted our wakeup event, but DIDN'T grant our request.
			   Re-post what we're blocking on and continue to wait.
			   This could happen if the lock was granted to a different request,
			   we have to tell the new owner of the lock that they are blocking us. */

			LOCK_TRACE(("spurious wakeup for %d\n", request_offset));
			post_blockage(request, lock, false);
			//release(owner_offset);
			continue;
			}

		/* See if all the other owners are still alive.  Dead ones will be purged,
		   purging one might resolve our lock request. */
		/* Do not do rescan of owners if we received notification that
		   blocking ASTs have completed - will do it next time if needed */
		   
		else if (probe_owners(owner_offset) && !(request->lrq_flags & LRQ_pending))
			{
			LOCK_TRACE(("owner %d disappear while waiting for %d\n", owner_offset, request_offset));
			//release(owner_offset);
			break;
			}

		/* If we've not previously been scanned for a deadlock, go do a
		   deadlock scan */

		else if (!(owner->own_flags & OWN_scanned) && (blocking_request = deadlock_scan(owner, request)))
			{
			/* Something has been selected for rejection to prevent a
			   deadlock.  Clean things up and go on.  We still have to
			   wait for our request to be resolved. */

			DEBUG_MSG(0, ("wait_for_request: selecting something for deadlock kill\n"));

			++LOCK_header->lhb_deadlocks;
			blocking_request->lrq_flags |= LRQ_rejected;
			
			if (blocking_request->lrq_flags & LRQ_pending)
				clearPending(blocking_request);
				
			OWN blocking_owner = LOCK_OWNER(blocking_request->lrq_owner);
			//blocking_owner->own_pending_request = 0;
			if (blocking_request->lrq_flags & LRQ_blocking)
			{
				remove_que(&blocking_request->lrq_own_blocks);
				blocking_request->lrq_flags &= ~LRQ_blocking;
			}
			blocking_owner->own_flags &= ~OWN_scanned;
			
			if (blocking_request != request)
				//post_wakeup(blocking_owner, REL_PTR(request));
				postWakeup(request);
				
			/* else
			   We rejected our LockOwner request to avoid a deadlock.
			   When we get back to the top of the master loop we
			   fall out and start cleaning up */
			}
		else
			{
			/* Our request is not resolved, all the owners are alive, there's
			   no deadlock -- there's nothing else to do.  Let's
			   make sure our request hasn't been forgotten by reminding
			   all the owners we're waiting - some plaforms under CLASSIC
			   architecture had problems with "missing signals" - which is
			   another reason to repost the blockage.
			   Also, the ownership of the lock could have changed, and we
			   weren't woken up because we weren't next in line for the lock.
			   We need to inform the new owner. */

			DEBUG_MSG(0, ("wait_for_request: forcing a resignal of blockers\n"));
			post_blockage(request, lock, false);
			
#ifdef DEV_BUILD
			repost_counter++;
			
			if (repost_counter % 50 == 0) 
				{
				gds__log("wait_for_request: owner %d reposted %ld times for lock %d",
						owner_offset, repost_counter, lock_offset);
				DEBUG_MSG(0, ("wait_for_request: reposted %ld times for this lock!\n", repost_counter));
				}
#endif /* DEV_BUILD */
			}
			
		//release(owner_offset);
		}

/* NOTE: lock table is not acquired at this point */

	request = LOCK_REQUEST(request_offset);
	if (req_owner)
		*req_owner = request->lrq_owner;
		
#ifdef DEV_BUILD
	CHECK(!(request->lrq_flags & LRQ_pending));
#endif /* DEV_BUILD */

	owner = LOCK_OWNER(owner_offset);
	//owner->own_pending_request = 0;
	release(owner_offset);

	LOCK_TRACE(("wait_for_request returning, granted = %d\n", !(request->lrq_flags & LRQ_rejected)));


	return FB_SUCCESS;
}

void LockMgr::lockTrace(const char* text, ...)
{
	va_list	args;
	va_start (args, text);
	char temp [1024];
	char *p = temp;
	/***
	time_t t; 
	time (&t); 
	strcpy (p, ctime(&t));
	***/
	
	if (LOCK_table)
		sprintf (p, "%d %d:", Thread::getCurrentThreadId(), ++LOCK_table->lhb_sequence);
	else
		sprintf (p, "%d ?:", Thread::getCurrentThreadId() );
		
	while (*p && *p != '\n')
		++p;
	
	*p++ = ' ';	
	vsprintf (p, text, args);
	va_end(args);
	ib_printf ("%s", temp); 
	ib_fflush (ib_stdout); 
	gds__log (temp);

	if (LOCK_table && LOCK_table->lhb_sequence == LOCK_trace_stop)
		lockBreak();
}


void LockMgr::lockBreak(void)
{
}

void LockMgr::validate(void)
{
	if (debugRequest && debugRequest->lrq_type != type_lrq) 
		bug(NULL, "debug corruption");
}

void LockMgr::checkReleased(PTR ownerOffset)
{
#if 0
	/* this check does not appear valid, as a second thread with the */
	/* same ownerOffset can acquire just as the first has released */
	if (LOCK_table->lhb_active_owner == ownerOffset)
		bug(NULL, "missing lock table release");
#endif
}

void LockMgr::clearPending(LRQ request)
{
	remove_que(&request->lrq_own_pending);
	request->lrq_flags &= ~LRQ_pending;
	LBL lock = LOCK_BLOCK(request->lrq_lock);
	lock->lbl_pending_lrq_count--;
}

LockEvent* LockMgr::allocEvent(LockOwner* lockOwner)
{
	ASSERT_ACQUIRED;
	LockEvent *event;
	PTR ptr = lockOwner->own_events;
	
	if (ptr)
		{
		event = LOCK_EVENT(ptr);
		lockOwner->own_events = event->next;
		
		return event;
		}
	
	if (ptr = LOCK_header->lhb_free_events)
		{
		event = LOCK_EVENT(ptr);
		LOCK_header->lhb_free_events = event->next;
		}
	else if (!(event = (LockEvent*) alloc(sizeof(LockEvent), NULL)))
		bug(NULL, "space exhausted");

#ifdef WIN_NT
	event->event.init(WAKEUP_SIGNAL + ++LOCK_header->lhb_event_number, 0);
#else
	event->event.init(LOCK_header->lhb_semid, ++LOCK_header->lhb_event_number);
#endif
	
	return event;		
}

void LockMgr::releaseEvent(LockEvent* event, LockOwner* owner)
{
	ASSERT_ACQUIRED;
	event->next = owner->own_events;
	owner->own_events = REL_PTR(event);
}

void LockMgr::releaseEvent(LockEvent* event)
{
	ASSERT_ACQUIRED;
	event->event.fini();
	event->next = LOCK_header->lhb_free_events;
	LOCK_header->lhb_free_events = REL_PTR(event);
}

void LockMgr::postWakeup(LockRequest* request)
{
	if (request->lrq_wakeup_event)
		{
		LockEvent *event = LOCK_EVENT(request->lrq_wakeup_event);
		//printf ("waking up event %d\n", request->lrq_wakeup_event);
		event->event.post();
		}
}

int LockMgr::wait(LockOwner *owner, LRQ request, int waitTime)
{
	int rc;
	int num_locks = 0;
	
	PTR ownerOffset = REL_PTR(owner);
	PTR requestOffset = REL_PTR(request);

	LockEvent *event = allocEvent(owner);
	PTR eventOffset = REL_PTR(event);
	request->lrq_wakeup_event = REL_PTR(event);
	int eventCount = event->event.clear();

	LOCK_TRACE(("Going into long wait for thread = %x, owner = %d, request = %d, event = %d\n", 
			Thread::getCurrentThreadId(), ownerOffset, requestOffset, eventOffset));

	release(ownerOffset);
#ifndef SHARED_CACHE
	while(LOCK_dbb->syncAst.ourExclusiveLock())
	{
		LOCK_dbb->syncAst.unlock();
		num_locks++;
	}
#endif

//num_long_waits++;

	rc = event->event.wait(eventCount, waitTime);   /****** NOT THREAD SAFE, as the mem for event may be remapped *****/
	
	LOCK_TRACE(("Back from wait for thread = %x, owner = %d, request = %d, event = %d\n", 
			Thread::getCurrentThreadId(),ownerOffset, requestOffset, eventOffset));

#ifndef SHARED_CACHE
	while(num_locks)
	{
		LOCK_dbb->syncAst.lock(NULL, Exclusive);
		num_locks--;
	}
#endif

	acquire(ownerOffset);

	LOCK_TRACE(("Back from acquire for thread = %x, owner = %d, request = %d, event = %d\n", 
		Thread::getCurrentThreadId(),ownerOffset, requestOffset, eventOffset));

	event = LOCK_EVENT(eventOffset);
	request = LOCK_REQUEST(requestOffset);
	owner = LOCK_OWNER(ownerOffset);	
	request->lrq_wakeup_event = 0;
	releaseEvent(event, owner);
	
	return rc;
}

LockRequest* LockMgr::allocRequest(void)
{
	ASSERT_ACQUIRED;
	LockRequest *request;
	
	if (QUE_EMPTY(LOCK_header->lhb_free_requests)) 
		request = (LockRequest*) alloc(sizeof(LockRequest), NULL);
	else
		{
		request = (LRQ) ((UCHAR*) QUE_NEXT(LOCK_header->lhb_free_requests) - OFFSET(LRQ, lrq_lbl_requests));
		remove_que(&request->lrq_lbl_requests);
		}
	
	request->lrq_type = type_lrq;
	request->lrq_flags = 0;
	request->lrq_state = LCK_none;
	request->lrq_data = 0;
	request->lrq_ast_routine = NULL;
	request->lrq_ast_argument = NULL;
#ifndef SHARED_CACHE
	request->lrq_dbb = LOCK_dbb;
#endif
	request->lrq_wakeup_event = (PTR) 0;
	request->lrq_lock = (PTR) 0;
	return request;
}

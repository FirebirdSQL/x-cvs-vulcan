/*
 *      PROGRAM:        JRD Lock Manager
 *      MODULE:         LockMgr.h
 *      DESCRIPTION:    Lock manager structure definitions
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
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "EPSON" port
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "DELTA" port
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "IMP" port
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "NCR3000" port
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "M88K" port
 *
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "UNIXWARE" port
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "Ultrix" port
 *
 * 2002.10.28 Sean Leyne - Completed removal of obsolete "DGUX" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "MPEXL" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "DecOSF" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "SGI" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 * 2003.03.24 Nickolay Samofatov - clean up defines, improve deadlock detection logic
 *
 */

#ifndef _LOCK_MGR_H_
#define _LOCK_MGR_H_

#include <stdio.h>
#include <sys/types.h>

#if !defined(WIN_NT)
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#endif

#include "../jrd/common.h"
#include "../jrd/file_params.h"
#include "AsyncEvent.h"

#ifdef WIN_NT
#define DEFAULT_SIZE    32768
#include "../jrd/isc_signal.h"
#endif


#ifndef DEFAULT_SIZE
#define DEFAULT_SIZE    98304
#endif

#ifndef EXTEND_SIZE
#define EXTEND_SIZE     32768
#endif

#ifdef SCO_UNIX
#define SEMAPHORES      25
#endif

#ifdef SINIXZ
#define SEMAPHORES      25
#endif

#ifndef SEMAPHORES
#define SEMAPHORES      32
#endif

#undef BASE
#define BASE                    ((UCHAR*) LOCK_header)
#define REL_PTR(item)           (PTR) ((UCHAR*) item - BASE)
#define ABS_PTR(item)           (BASE + item)

typedef SLONG	PTR;

/* Maximum lock series for gathering statistics
   and querying data */

#define LCK_MAX_SERIES	7

/* Lock query data aggregates */

#define LCK_MIN		1
#define LCK_MAX		2
#define LCK_CNT		3
#define LCK_SUM		4
#define LCK_AVG		5
#define LCK_ANY		6

/* Lock states */

#define LCK_none        0
#define LCK_null        1
#define LCK_SR          2		/* Shared Read */
#define LCK_PR          3		/* Protected Read */
#define LCK_SW          4		/* Shared Write */
#define LCK_PW          5		/* Protected Write */
#define LCK_EX          6		/* Exclusive */
#define LCK_max         7

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



/* Self-relative que block.  Offsets are from the block itself. */

typedef struct srq {
	PTR srq_forward;			/* Forward offset */
	PTR srq_backward;			/* Backward offset */
} *SRQ;


#define CLASSIC_LHB_VERSION	16 // Vulcan 1.0
#define SS_LHB_VERSION		(CLASSIC_LHB_VERSION + 100)

//#ifdef SUPERSERVER
//#define LHB_VERSION	SS_LHB_VERSION
//#else
#define LHB_VERSION	CLASSIC_LHB_VERSION
//#endif

#define LHB_PATTERN     123454321

//
// Define methods that lock manager uses to handle events
// USE_WAKEUP_EVENTS - use UNIX events to handle owner wakeup.
//    makes no sense on Windows because signals are simulated VIA events
//    otherwise signals are used to wakeup owners

#ifdef UNIX
#define USE_WAKEUP_EVENTS
#endif

// STATIC_SEMAPHORES - preallocate semaphores for UNIX events
// if undefined UNIX events allocate semaphores dynamically

#if defined UNIX && (!defined SUPERSERVER && !defined MULTI_THREAD)
#define USE_STATIC_SEMAPHORES
#endif

/* Lock header block -- one per lock file, lives up front */

struct LockHeader  {
	UCHAR	lhb_type;			/* memory tag - always type_lbh */
	UCHAR	lhb_version;		/* Version of lock table */
	PTR		lhb_secondary;		/* Secondary lock header block */
	PTR		lhb_active_owner;	/* Active owner, if any */
	srq		lhb_owners;			/* Que of active owners */
	srq		lhb_free_owners;	/* Free owners blocks */
	srq		lhb_free_locks;		/* Free lock blocks */
	srq		lhb_free_requests;	/* Free lock requests */
	PTR		lhb_free_events;
	SLONG	lhb_length;			/* Size of lock table */
	SLONG	lhb_used;			/* Bytes of lock table in use */
	SLONG	lhb_sequence;		/* Sequence for debugging */
	USHORT	lhb_hash_slots;		/* Number of hash slots allocated */
	USHORT	lhb_flags;			/* Miscellaneous info */
	MTX_T	lhb_mutex[1];		/* Mutex controlling access */
	ULONG	lhb_semid;			/* Semaphores
	PTR		lhb_manager;		/* Lock manager owner block */
	PTR		lhb_history;
	ULONG	lhb_process_count;	/* To give a unique id to each process attachment to the lock table */
	//PTR	lhb_mask;			/* Semaphore mask block */
	ULONG	lhb_event_number;
	ULONG	lhb_scan_interval;	/* Deadlock scan interval (secs) */
	ULONG	lhb_acquire_spins;
	ULONG	lhb_acquires;
	ULONG	lhb_acquire_blocks;
	ULONG	lhb_acquire_retries;
	ULONG	lhb_retry_success;
	ULONG	lhb_enqs;
	ULONG	lhb_converts;
	ULONG	lhb_downgrades;
	ULONG	lhb_deqs;
	ULONG	lhb_read_data;
	ULONG	lhb_write_data;
	ULONG	lhb_query_data;
	ULONG	lhb_operations[LCK_MAX_SERIES];
	ULONG	lhb_waits;
	ULONG	lhb_denies;
	ULONG	lhb_timeouts;
	ULONG	lhb_blocks;
	ULONG	lhb_direct_sigs;
	ULONG	lhb_indirect_sigs;
	ULONG	lhb_wakeups;
	ULONG	lhb_scans;
	ULONG	lhb_deadlocks;
	ULONG	lhb_wait_time;
	ULONG	lhb_reserved[2];		/* For future use */
	srq lhb_data[LCK_MAX_SERIES];
	srq lhb_hash[1];			/* Hash table */
};

typedef LockHeader *LHB;

#define LHB_lock_ordering       1	/* Lock ordering is enabled */
#define LHB_shut_manager	2	/* Lock manager shutdown flag */

/* Secondary header block -- exists only in V3.3 and later lock
   managers.  It is pointed to by the word in the lhb that used to contain
   a pattern. */

typedef struct shb {
	UCHAR shb_type;				/* memory tag - always type_shb */
	UCHAR shb_flags;
	PTR shb_history;
	PTR shb_remove_node;		/* Node removing itself */
	PTR shb_insert_que;			/* Queue inserting into */
	PTR shb_insert_prior;		/* Prior of inserting queue */
	SLONG shb_misc[10];			/* Unused space */
} *SHB;


/* Lock block */

struct LockBlock
{
	UCHAR		lbl_type;				/* mem tag: type_lbl=in use, type_null=free */
	UCHAR		lbl_state;				/* High state granted */
	UCHAR		lbl_size;				/* Key bytes allocated */
	UCHAR		lbl_length;				/* Key bytes used */
	srq			lbl_requests;			/* Requests granted */
	srq			lbl_lhb_hash;			/* Collision que for hash table */
	srq			lbl_lhb_data;			/* Lock data que by series */
	SLONG		lbl_data;				/* user data */
	PTR			lbl_parent;				/* Parent */
	UCHAR		lbl_series;				/* Lock series */
	UCHAR		lbl_flags;				/* Misc flags */
	USHORT		lbl_pending_lrq_count;	/* count of lbl_requests with LRQ_pending */
	USHORT		lbl_counts[LCK_max];	/* Counts of granted locks */
	UCHAR		lbl_key[1];				/* Key value */
};

typedef LockBlock *LBL;

/* No flags are defined for LBL at this time */

/* Lock requests */

struct LockRequest {
	UCHAR		lrq_type;			/* mem tag: type_lrq=in use, type_null=free */
	UCHAR		lrq_requested;		/* Level requested  */
	UCHAR		lrq_state;			/* State of lock request */
	USHORT		lrq_flags;			/* Misc crud */
	PTR			lrq_owner;			/* Owner making request */
	PTR			lrq_lock;			/* Lock requested */
	SLONG		lrq_data;			/* Lock data requested */
	srq			lrq_own_requests;	/* Locks granted for owner */
	srq			lrq_lbl_requests;	/* Que of requests (active, pending) */
	//srq		lrq_own_blocks;		/* Owner block que */
	srq 		lrq_own_pending;	/* Owner block or pending que */
	lock_ast_t	lrq_ast_routine;	/* Block ast routine */
	void		*lrq_ast_argument;	/* Ast argument */
	PTR			lrq_wakeup_event;
	int			lrq_thread_id;
};

typedef LockRequest *LRQ;

#define LRQ_blocking    1		/* Request is blocking */
#define LRQ_pending     2		/* Request is pending */
#define LRQ_converting  4		/* Request is pending conversion */
#define LRQ_rejected    8		/* Request is rejected */
#define LRQ_timed_out   16		/* Wait timed out */
#define LRQ_deadlock    32		/* Request has been seen by the deadlock-walk */
#define LRQ_repost      64		/* Request block used for repost */
#define LRQ_scanned     128		/* Request already scanned for deadlock */
#define LRQ_blocking_seen 256	/* Blocking notification received by owner */

/* Owner block */

struct LockOwner
{
	UCHAR	own_type;				/* memory tag - always type_own */
	UCHAR	own_owner_type;			/* type of owner */
	SSHORT	own_count;				/* init count for the owner */
	ULONG	own_owner_id;			/* Owner ID */
	UATOM	own_ast_flags;			/* flags shared by main and ast codes */
	UATOM	own_ast_hung_flags;		/* unprotected - OWN_hung flag */
	srq		own_lhb_owners;			/* Owner que */
	srq		own_requests;			/* Lock requests granted */
	srq		own_blocks;				/* Lock requests blocking */
	srq		own_pending;			/* Lock requests blocking */
	//PTR	own_pending_request;	/* Request we're waiting on */
	int		own_process_id;			/* Owner's process ID */
	int		own_process_uid;		/* Owner's process UID */
	ULONG	own_acquire_time;		/* lhb_acquires when owner last tried acquire() */
	ULONG	own_acquire_realtime;	/* GET_TIME when owner last tried acquire() */
	PTR		own_events;				/* Free event blocks */
	PTR		own_blocking_event;		/* blocking event */
	
	//AsyncEvent own_blocking[1];		/* Blocking event block */
	AsyncEvent own_stall[1];		/* Owner is stalling for other owner */
	//AsyncEvent own_wakeup[1];		/* Wakeup event block */

	USHORT own_semaphore;			/* Owner semaphore -- see note below */
	USHORT own_flags;				/* Misc stuff */
};

typedef LockOwner *OWN;

/* Flags in own_flags */

#define OWN_blocking		1		// Owner is blocking
#define OWN_scanned			2		// Owner has been deadlock scanned
#define OWN_manager			4		// Owner is privileged manager
#define OWN_signal			8		// Owner needs signal delivered
#define OWN_wakeup			32		// Owner has been awoken
#define OWN_starved			128		// This thread may be starved

/* Flags in own_ast_flags */
#define OWN_signaled    16		/* Signal is thought to be delivered */

/* Flags in own_semaphore */
//#define OWN_semavail    0x8000	/* Process semaphore is available */

/* Flags in own_ast_hung_flag */
#define OWN_hung        64		/* Owner may be hung by OS-level bug */

/* NOTE: own_semaphore, when USE_WAKEUP_EVENTS is set, is used to indicate when a 
   owner is waiting inside wait_for_request().  post_wakeup() will only
   attempt to wakeup owners that are waiting.  The reason for this is
   likely historical - a flag bit could be used for this instead. */

/* Lock Event Block */

struct LockEvent {
	PTR			next;
	AsyncEvent	event;		/* Wakeup event block */
};


/* Semaphore mask block */

typedef struct smb {
	UCHAR smb_type;				/* memory tag - always type_smb */
	ULONG smb_mask[1];			/* Mask of available semaphores */
} *SMB;

/* Lock manager history block */

typedef struct his {
	UCHAR his_type;				/* memory tag - always type_his */
	UCHAR his_operation;		/* operation that occured */
	PTR his_next;				/* PTR to next item in history list */
	PTR his_process;			/* owner to record for this operation */
	PTR his_lock;				/* lock to record for operation */
	PTR his_request;			/* request to record for operation */
} *HIS;

/* his_operation definitions */
#define his_enq         1
#define his_deq         2
#define his_convert     3
#define his_signal      4
#define his_post_ast    5
#define his_wait        6
#define his_del_process 7
#define his_del_lock    8
#define his_del_request 9
#define his_deny        10
#define his_grant       11
#define his_leave_ast   12
#define his_scan        13
#define his_dead        14
#define his_enter       15
#define his_bug         16
#define his_active      17
#define his_cleanup     18
#define his_del_owner   19
#define his_MAX         his_del_owner

CLASS(ConfObject);

class LockMgr
{
public:
	LockMgr(void);
	~LockMgr(void);

	static int		LOCK_convert(SLONG, UCHAR, SSHORT, int (*)(void *), void *, ISC_STATUS *);
	static int		LOCK_deq(PTR);
	static UCHAR	LOCK_downgrade(SLONG, ISC_STATUS *);
	static SLONG	LOCK_enq(PTR, PTR, USHORT, UCHAR *, USHORT, UCHAR,
						lock_ast_t, void *, SLONG, SSHORT, ISC_STATUS *,
						PTR);
	static void		LOCK_fini(ISC_STATUS*, PTR*);
	static int		LOCK_init(ISC_STATUS *, ConfObject *configuration, long, UCHAR, SLONG *);
	static void		LOCK_manager(SLONG);
	static SLONG	LOCK_query_data(SLONG, USHORT, USHORT);
	static SLONG	LOCK_read_data(SLONG);
	static SLONG	LOCK_read_data2(SLONG, USHORT, UCHAR *, USHORT, SLONG);
	static void		LOCK_re_post(int (*)(void *), void *, SLONG);
	static bool		LOCK_shut_manager(void);
	static SLONG	LOCK_write_data(SLONG, SLONG);
	static void		LOCK_ast_inhibit();
	static void		LOCK_ast_enable();

protected:
	static void acquire(PTR);
	static UCHAR *alloc(SSHORT, ISC_STATUS *);
	static LBL alloc_lock(USHORT, ISC_STATUS *);

#ifdef USE_STATIC_SEMAPHORES
	static USHORT alloc_semaphore(OWN, ISC_STATUS *);
	static void release_semaphore(OWN);
#endif

	static void blocking_action(PTR owner_offset);
	static void blocking_action2(PTR, PTR);
	static int blocking_action_thread(void*);
	static void bug(ISC_STATUS *, const TEXT *);
	static bool convert(PTR, UCHAR, SSHORT, lock_ast_t, void *, ISC_STATUS *);
	static LockOwner* create_owner(ISC_STATUS *, SLONG, UCHAR);

#ifdef DEV_BUILD
	static void bug_assert(const TEXT *, ULONG);
	static void current_is_active_owner(bool, ULONG);
	static void debug_delay(ULONG);
#endif

	static void deadlock_clear(void);
	static LRQ deadlock_scan(OWN, LRQ);
	static LRQ deadlock_walk(LRQ, bool *);
	static void dequeue(PTR);
	static void exit_handler(void *);
	static LBL find_lock(PTR, USHORT, UCHAR *, USHORT, USHORT *);
	static LRQ get_request(PTR);
	static void grant(LRQ, LBL);
	static PTR grant_or_que(LRQ, LBL, SSHORT);
	static ISC_STATUS init_lock_table(ISC_STATUS *, ConfObject *configuration);
	static void init_owner_block(OWN, UCHAR, ULONG, USHORT);
	static void lock_alarm_handler(void *event);
	static void lock_initialize(void*, SH_MEM, bool);
	static void insert_data_que(LBL);
	static void insert_tail(SRQ, SRQ);
	static USHORT lock_state(LBL);
	static LockRequest* post_blockage(LRQ, LBL, bool);
	static void post_history(USHORT, PTR, PTR, PTR, bool);
	static void post_pending(LBL);
	//static void post_wakeup(OWN, int id);
	static bool probe_owners(PTR);
	static void purge_owner(PTR, OWN);
	static void remove_que(SRQ);
	static void release(PTR);
	static void release_mutex(void);
	static void release_request(LRQ);

	static void shutdown_blocking_thread(LockOwner *owner);
	static int signal_owner(OWN, PTR);

#ifdef VALIDATE_LOCK_TABLE
	static void validate_lhb(LHB);
	static void validate_shb(PTR);
	static void validate_owner(PTR, USHORT);
	static void validate_lock(PTR, USHORT, PTR);
	static void validate_request(PTR, USHORT, USHORT);
	//static void validate_block(PTR);
#endif

	static USHORT wait_for_request(LRQ, SSHORT, ISC_STATUS *);
public:
	static void lockTrace(const char* text, ...);
	static void lockBreak(void);
	static void validate(void);
	static void checkReleased(PTR ownerOffset);
	static void clearPending(LRQ request);
	static LockEvent* allocEvent(LockOwner* lockOwner);
	static void releaseEvent(LockEvent* event, LockOwner* owner);
	static void releaseEvent(LockEvent* event);
	static void postWakeup(LockRequest* request);
	static void wait(LockOwner *owner, PTR *wakeup, int wait);
	static LockRequest* allocRequest(void);
	
	friend class LockAcquire;
};

#endif


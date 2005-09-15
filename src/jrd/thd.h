/*
 *	PROGRAM:	JRD access method
 *	MODULE:		thd.h
 *	DESCRIPTION:	Thread support definitions
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
 * 2002.10.28 Sean Leyne - Completed removal of obsolete "DGUX" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */
/*
$Id$
*/

#ifndef JRD_THD_H
#define JRD_THD_H

#ifdef SOLARIS_MT
#include <thread.h>
#define THD_MUTEX_STRUCT	mutex_t
#define THD_COND_STRUCT		cond_t
#define THD_RWLOCK_STRUCT	rwlock_t
#endif
 

#ifdef HAVE_POSIX_THREADS
#ifdef SUPERSERVER
#define POSIX_THREADS		1
#endif
#ifdef SUPERCLIENT
#if defined(LINUX) || defined(FREEBSD)
/* The following ifdef was added to build thread safe gds shared
   library on linux platform. It seems the gdslib works now (20020220)
   with thread enabled applications. Anyway, more tests should be 
   done as I don't have deep knowledge of the interbase/firebird 
   engine and this change may imply side effect I haven't known 
   about yet. Tomas Nejedlik (tomas@nejedlik.cz) */
#define POSIX_THREADS		1
#endif
#endif
#endif

#ifdef VMS
/*
#define POSIX_THREADS		1
*/

#include "../jrd/isc.h"
#define THD_MUTEX_STRUCT	MTX_T
#endif


#ifdef MULTI_THREAD
#define ANY_THREADING		1
#endif

#ifdef V4_THREADING
#define ANY_THREADING		1
#endif

#ifdef POSIX_THREADS
#include <pthread.h>
#define THD_MUTEX_STRUCT	pthread_mutex_t
#define THD_COND_STRUCT		pthread_cond_t

#ifndef PTHREAD_PROCESS_SHARED
//#define PTHREAD_PROCESS_SHARED	1
#endif

#ifndef PTHREAD_CREATE_DETACHED
//#define PTHREAD_CREATE_DETACHED	1
#endif

#ifdef HAVE_PTHREAD_KEYCREATE
#define pthread_key_create	pthread_keycreate
#endif
#endif

#ifndef THD_MUTEX_STRUCT
#define THD_MUTEX_STRUCT	SCHAR
#endif
#ifndef THD_COND_STRUCT
#define THD_COND_STRUCT		SCHAR
#endif

#ifdef ANY_THREADING
#define GET_THREAD_DATA		((THDD) THD_get_specific(THDD_TYPE_TDBB))
#else
#define GET_THREAD_DATA		gdbb
#endif

#ifdef SUPERSERVER
#ifdef SHARED_CACHE
#define SWEEP_THREAD
#endif
#else
#define AST_THREAD
#endif

#define THREAD_ENTER
#define THREAD_EXIT
#define THREAD_VALIDATE
#define THREAD_SLEEP(msecs)	THD_sleep (msecs)
#define THREAD_YIELD

/* Thread priorities (may be ignored) */

#define THREAD_high		1
#define THREAD_medium_high	2
#define THREAD_medium		3
#define THREAD_medium_low	4
#define THREAD_low		5
#define THREAD_critical		6

/* Thread option flags */

#define THREAD_ast		1	/* Thread can/should run at ast level */
#define THREAD_blast		2	/* Blow away thread during exit handler */
#define THREAD_wait		4	/* Somebody will wait for thread exit */

/* Thread quanta */

#define QUANTUM			100	/* Default quantum */
#define SWEEP_QUANTUM		10	/* Make sweeps less disruptive */

/* Thread specific data */

#if defined(WIN_NT)
#define THREAD_ENTRY_PARAM void*
#define THREAD_ENTRY_RETURN unsigned int
#define THREAD_ENTRY_CALL __stdcall
#elif defined(USE_POSIX_THREADS)
#define THREAD_ENTRY_PARAM void*
#define THREAD_ENTRY_RETURN void*
#define THREAD_ENTRY_CALL
#elif defined(SOLARIS_MT)
#define THREAD_ENTRY_PARAM void*
#define THREAD_ENTRY_RETURN void *
#define THREAD_ENTRY_CALL
#else
// Define correct types for other platforms
#define THREAD_ENTRY_PARAM void*
#define THREAD_ENTRY_RETURN int
#define THREAD_ENTRY_CALL
#endif

#define THREAD_ENTRY_DECLARE THREAD_ENTRY_RETURN THREAD_ENTRY_CALL

typedef THREAD_ENTRY_DECLARE ThreadEntryPoint(THREAD_ENTRY_PARAM);

/* Thread specific data */

struct thdd 
	{
	void *thdd_prior_context;
	ULONG thdd_type;		/* what kind of structrue this is */
	};

typedef thdd *THDD;

/* Thread structure types */

#define THDD_TYPE_TGBL  1		/* used by backup/restore */
#define THDD_TYPE_TSQL  2		/* used by DSQL */
#define THDD_TYPE_TDBB  3		/* used in engine */
#define THDD_TYPE_TRDB  4		/* used in remote interface */
#define THDD_TYPE_TDBA  5		/* used in DBA utility */
#define THDD_TYPE_TIDB	6		/* used by interprocess server */
#define THDD_TYPE_TALICE 7		/* used by gfix */
#define THDD_TYPE_TSEC	8		/* used by gsec */
#define THDD_TYPE_SRVR	9		/* used by server */
#define THDD_TYPE_MAX	9

/* General purpose in use object structure */

typedef struct iuo {
	struct iuo *iuo_next;
	void *iuo_object[10];
	USHORT iuo_in_use_count;
} *IUO;

/* Mutex structure */

#ifdef OBSOLETE
typedef struct mutx_t {
	THD_MUTEX_STRUCT mutx_mutex;
} MUTX_T, *MUTX;

/* Recursive mutex structure */
typedef struct rec_mutx_t {
	MUTX_T rec_mutx_mtx[1];
	ULONG rec_mutx_id;
	SLONG rec_mutx_count;
} REC_MUTX_T, *REC_MUTX;
#endif

/* Combined mutex and condition variable structure */

#ifndef SOLARIS_MT
typedef struct Cond_t {
	THD_MUTEX_STRUCT cond_mutex;
	THD_COND_STRUCT cond_cond;
} COND_T, *COND;

#endif

/* Read/write lock structure */
#ifdef OBSOLETE
typedef struct wlck_t {
#ifdef THD_RWLOCK_STRUCT
	THD_RWLOCK_STRUCT wlck_wlock;
#else
	COND_T wlck_cond;
	int wlck_count;
	int wlck_waiters;
#endif
} WLCK_T, *WLCK;

#define WLCK_read	1
#define WLCK_write	2
#endif

/* Threading allocation size */

#define THREAD_STRUCT_SIZE(type,n)	(n * sizeof (type) + ALIGNMENT)
#define THREAD_STRUCT_ALIGN(blk)	FB_ALIGN((U_IPTR) blk, ALIGNMENT)

/***
#ifdef V4_THREADING
#define V4_INIT				THD_init()
#define V4_GLOBAL_MUTEX_LOCK		THD_mutex_lock_global()
#define V4_GLOBAL_MUTEX_UNLOCK		THD_mutex_unlock_global()
#define V4_MUTEX_INIT(mutx)		THD_mutex_init (mutx)
#define V4_MUTEX_LOCK(mutx)		THD_mutex_lock (mutx)
#define V4_MUTEX_UNLOCK(mutx)		THD_mutex_unlock (mutx)
#define V4_MUTEX_DESTROY(mutx)		THD_mutex_destroy (mutx)

// BRS. 03/23/2003
// Those empty defines was substituted with #ifdef V4_THREADING
//#else
//#define V4_INIT
//#define V4_GLOBAL_MUTEX_LOCK
//#define V4_GLOBAL_MUTEX_UNLOCK
//#define V4_MUTEX_INIT(mutx)
//#define V4_MUTEX_LOCK(mutx)
//#define V4_MUTEX_UNLOCK(mutx)
//#define V4_MUTEX_DESTROY(mutx)
//#define V4_RW_LOCK_INIT_N(wlck,n)
//#define V4_RW_LOCK_INIT(wlck)
//#define V4_RW_LOCK_LOCK(wlck,type)
//#define V4_RW_LOCK_UNLOCK(wlck)
//#define V4_RW_LOCK_DESTROY_N(wlck,n)
//#define V4_RW_LOCK_DESTROY(wlck)
#endif
***/

#ifdef ANY_THREADING
#define THD_INIT			THD_init()
#define THD_GLOBAL_MUTEX_LOCK		THD_mutex_lock_global()
#define THD_GLOBAL_MUTEX_UNLOCK		THD_mutex_unlock_global()
#define THD_MUTEX_INIT_N(mutx,n)	THD_mutex_init_n (mutx, n)
#define THD_MUTEX_DESTROY_N(mutx,n)	THD_mutex_destroy_n (mutx, n)
#define THD_MUTEX_INIT(mutx)		THD_mutex_init (mutx)
#define THD_MUTEX_LOCK(mutx)		THD_mutex_lock (mutx)
#define THD_MUTEX_UNLOCK(mutx)		THD_mutex_unlock (mutx)
#define THD_MUTEX_DESTROY(mutx)		THD_mutex_destroy (mutx)
#else
#define THD_INIT
#define THD_GLOBAL_MUTEX_LOCK
#define THD_GLOBAL_MUTEX_UNLOCK
#define THD_MUTEX_INIT_N(mutx,n)
#define THD_MUTEX_DESTROY_N(mutx,n)
#define THD_MUTEX_INIT(mutx)
#define THD_MUTEX_LOCK(mutx)
#define THD_MUTEX_UNLOCK(mutx)
#define THD_MUTEX_DESTROY(mutx)
#endif

#endif /* JRD_THD_H */

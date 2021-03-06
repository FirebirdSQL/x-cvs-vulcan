/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		thd.cpp
 *	DESCRIPTION:	Thread support routines
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

#include "fbdev.h"
#include "../jrd/ib_stdio.h"
#include <errno.h>
#include "../jrd/common.h"
#include "../jrd/thd.h"
#include "../jrd/isc.h"
#include "../jrd/thd_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/isc_s_proto.h"
#include "../jrd/gdsassert.h"
#include "Mutex.h"
#include <string.h>



#ifdef WIN_NT
#include <process.h>
#include <windows.h>
#ifdef THREAD_SCHEDULER
#include "os/thd_priority.h"
#endif
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


#if defined SOLARIS || defined _AIX
#include <sched.h>
#endif

#ifdef SOLARIS_MT
#include <thread.h>
#include <signal.h>
#endif


#ifndef ANY_THREADING
//THDD gdbb;
#endif

#ifdef VMS
/* THE SOLE PURPOSE OF THE FOLLOWING DECLARATION IS TO ALLOW THE VMS KIT TO
   COMPILE.  IT IS NOT CORRECT AND MUST BE REMOVED AT SOME POINT. */
THDD gdbb;
#endif

#ifdef MVS
extern "C"
{
static void init(void);
static void init_tkey(void);
typedef void *(*start_routine)(void*);
}
#endif



static void init(void);
static void init_tkey(void);
static void put_specific(THDD, int subSystem);
static int thread_start(int (*)(void *), void *, int, int, void *);

static USHORT initialized = FALSE;
static USHORT t_init = FALSE;

#ifdef ANY_THREADING
//static MUTX_T ib_mutex;

#ifdef POSIX_THREADS
static pthread_key_t specificKeys [THDD_TYPE_MAX];
static pthread_key_t t_key;
#endif

#ifdef SOLARIS_MT
static thread_key_t specificKeys [THDD_TYPE_MAX];
static thread_key_t t_key;
#endif

#ifdef WIN_NT
static DWORD specificKeys [THDD_TYPE_MAX];
static DWORD t_key;
#endif
#endif // ANY_THREADING


static Mutex	threadInitMutex;


int API_ROUTINE THD_start_thread(
								  FPTR_INT_VOID_PTR entrypoint,
								  void *arg,
								  int priority, int flags, void *thd_id)
{
/**************************************
 *
 *	g d s _ $ t h r e a d _ s t a r t
 *
 **************************************
 *
 * Functional description
 *	Start a thread.
 *
 **************************************/

	return thread_start(entrypoint, arg, priority, flags, thd_id);
}

int API_ROUTINE THD_thread_wait(void *thd_id)
{
#ifdef WIN_NT
	WaitForSingleObject(*(HANDLE*)thd_id, INFINITE );
	CloseHandle(*(HANDLE*)thd_id);
#endif	

#ifdef POSIX_THREADS
	void *value_ptr;
	pthread_t thread;
	memcpy(&thread, thd_id, sizeof(thread));
	pthread_join(thread, &value_ptr);
#endif	

	return 0;
}


#ifdef OBSOLETE
long THD_get_thread_id(void)
{
/**************************************
 *
 *	T H D _ g e t _ t h r e a d _ i d
 *
 **************************************
 *
 * Functional description
 *	Get platform's notion of a thread ID.
 *
 **************************************/
	long id = 1;
#ifdef WIN_NT
	id = GetCurrentThreadId();
#endif
#ifdef SOLARIS_MT
	id = thr_self();
#endif
#ifdef POSIX_THREADS

/* The following is just a temp. decision.
*/
#ifdef HP10

	id = (long) (pthread_self().field1);

#elif defined MVS
   pthread_t pid;
   pid = pthread_self();
   memcpy(&id, ((char *)&pid)+4, sizeof(id));

#else

	id = (long) pthread_self();

#endif /* HP10 */
#endif /* POSIX_THREADS */

	return id;
}
#endif


#ifdef ANY_THREADING
#ifdef POSIX_THREADS
#define GET_SPECIFIC_DEFINED
THDD THD_get_specific(int subSystem)
{
/**************************************
 *
 *	T H D _ g e t _ s p e c i f i c		( P O S I X )
 *
 **************************************
 *
 * Functional description
 * Gets thread specific data and returns
 * a pointer to it.
 *
 **************************************/
#ifdef HP10

	THDD current_context;

	pthread_getspecific(specificKeys [subSystem - 1], (pthread_addr_t *) & current_context);
	return current_context;

#elif defined MVS

	void* current_context;

	pthread_getspecific(specificKeys [subSystem - 1], &current_context);
	return (THDD)current_context;


#else

	return ((THDD) pthread_getspecific(specificKeys [subSystem - 1]));

#endif /* HP10 */
}
#endif /* POSIX_THREADS */
#endif /* ANY_THREADING */


#ifdef ANY_THREADING
#ifdef SOLARIS_MT
#define GET_SPECIFIC_DEFINED
THDD THD_get_specific(int subsystem)
{
/**************************************
 *
 *	T H D _ g e t _ s p e c i f i c		( S o l a r i s )
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	THDD current_context;

	if (thr_getspecific(specificKeys [subsystem - 1], (void **) &current_context)) {
		ib_perror("thr_getspecific");
		exit(1);
	}

	return current_context;
}
#endif
#endif


#ifdef ANY_THREADING
#ifdef WIN_NT
#define GET_SPECIFIC_DEFINED
THDD THD_get_specific(int subSystem)
{
/**************************************
 *
 *	T H D _ g e t _ s p e c i f i c		( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

#ifdef THREAD_SCHEDULER
	return THPS_GET((THDD)TlsGetValue(specificKeys [subSystem - 1]));
#else
	void *p = TlsGetValue(specificKeys [subSystem - 1]);
#ifdef DEV_BUILD
	if (p == NULL)
	{
	    long ret = GetLastError();
	    if (ret != NO_ERROR)
	    {
		fb_assert(0);
	    }
	}
#endif
	return (THDD)p;
#endif
}
#endif
#endif


#ifndef GET_SPECIFIC_DEFINED
THDD THD_get_specific(int subSystem)
{
/**************************************
 *
 *	T H D _ g e t _ s p e c i f i c		( G e n e r i c )
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	return gdbb;
}
#endif


void THD_getspecific_data(void **t_data)
{
/**************************************
 *
 *	T H D _ g e t s p e c i f i c _ d a t a
 *
 **************************************
 *
 * Functional description
 *	return the previously stored t_data.
 *
 **************************************/

#ifdef ANY_THREADING

/* There are some circumstances in which we do not call THD_putspecific_data(),
   such as services API, and local access on NT. As result of that, t_init
   does not get initialized. So don't use an fb_assert in here but rather do
   the work only if t_init is initialised */
	if (t_init) {
#ifdef POSIX_THREADS
#if defined HP10 || defined MVS
		pthread_getspecific(t_key, t_data);
#else
		*t_data = (void *) pthread_getspecific(t_key);
#endif /* HP10 */
#endif /* POSIX_THREADS */

#ifdef SOLARIS_MT
		thr_getspecific(t_key, t_data);
#endif

#ifdef WIN_NT
		*t_data = (void *) TlsGetValue(t_key);
#endif
	}
#endif
}


void THD_cleanup(void)
{
/**************************************
 *
 *	T H D _ c l e a n u p
 *
 **************************************
 *
 * Functional description
 * This is the cleanup function called from the DLL
 * cleanup function. This helps to remove the allocated
 * thread specific key.
 *
 **************************************/

	if (initialized) 
		{
		initialized = FALSE;
#ifdef POSIX_THREADS
#endif

#ifdef SOLARIS_MT
#endif

#ifdef ANY_THREADING
#ifdef WIN_NT
	for (int n = 0; n < THDD_TYPE_MAX; ++n)
		TlsFree(specificKeys [n]);
#endif
		/* destroy the mutex ib_mutex which was created */
	
#ifdef THREAD_SCHEDULER
#ifdef WIN_NT
		THPS_FINI();
#endif /* WIN_NT */
#endif

#endif /* ANY_THREADING */
	}
}


void THD_init(void)
{
/**************************************
 *
 *	T H D _ i n i t
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
#ifdef POSIX_THREADS

/* In case of Posix threads we take advantage of using function
   pthread_once. This function makes sure that init() routine
   will be called only once by the first thread to call pthread_once.
*/
#ifndef PTHREAD_ONCE_INIT
	static pthread_once_t once = pthread_once_init;
#else
	static pthread_once_t once = PTHREAD_ONCE_INIT;
#endif

	pthread_once(&once, init);

#else

	init();

#endif /* POSIX_THREADS */
}


void THD_init_data(void)
{
/**************************************
 *
 *	T H D _ i n i t _ d a t a
 *
 **************************************
 *
 * Functional description
 *	init function for t_key. This is called 
 *	to ensure that the key is created.
 *
 **************************************/
#ifdef POSIX_THREADS

/* In case of Posix threads we take advantage of using function
   pthread_once. This function makes sure that init_tkey() routine
   will be called only once by the first thread to call pthread_once.
*/
#ifndef PTHREAD_ONCE_INIT
	static pthread_once_t once = pthread_once_init;
#else
	static pthread_once_t once = PTHREAD_ONCE_INIT;
#endif

	pthread_once(&once, init_tkey);

#else

	init_tkey();

#endif /* POSIX_THREADS */
}


#ifdef ANY_THREADING
#ifdef POSIX_THREADS
#define THREAD_MUTEXES_DEFINED

#endif /* POSIX_THREADS */
#endif /* ANY_THREADING */


#ifdef ANY_THREADING
#ifdef SOLARIS_MT
#define THREAD_MUTEXES_DEFINED

#endif
#endif


#ifdef ANY_THREADING
#ifdef VMS
#define THREAD_MUTEXES_DEFINED


#endif
#endif



#ifdef ANY_THREADING
#ifdef WIN_NT
#define THREAD_MUTEXES_DEFINED



#endif // WIN_NT
#endif // ANY_THREADING


#ifndef THREAD_MUTEXES_DEFINED

#endif


#ifdef ANY_THREADING
#endif


void THD_put_specific(THDD new_context, int subSystem)
{
/**************************************
 *
 *	T H D _ p u t _ s p e c i f i c
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	if (!initialized)
		THD_init();

/* Save the current context */

	new_context->thdd_prior_context = THD_get_specific(subSystem);

	put_specific(new_context, subSystem);
}


void THD_putspecific_data(void *t_data)
{
/**************************************
 *
 *	T H D _ p u t s p e c i f i c _ d a t a
 *
 **************************************
 *
 * Functional description
 *	Store the passed t_data using the ket t_key
 *
 **************************************/

	if (!t_init)
		THD_init_data();
#ifdef ANY_THREADING

#ifdef POSIX_THREADS
	pthread_setspecific(t_key, t_data);
#endif

#ifdef SOLARIS_MT
	thr_setspecific(t_key, t_data);
#endif

#ifdef WIN_NT
	TlsSetValue(t_key, (LPVOID) t_data);
#endif

#endif
}


THDD THD_restore_specific(int subSystem)
{
/**************************************
 *
 *	T H D _ r e s t o r e _ s p e c i f i c
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	THDD current_context;

	current_context = THD_get_specific(subSystem);

#pragma FB_COMPILER_MESSAGE("Fix! Bad struct ptr type cast.")
	put_specific(reinterpret_cast < THDD >
				 (current_context->thdd_prior_context), subSystem);

	return reinterpret_cast < THDD > (current_context->thdd_prior_context);
}


#ifdef OBSOLETE

#endif /* OBSOLETE SUPERSERVER */


#ifdef WIN_NT
#define THREAD_SUSPEND_DEFINED
int THD_resume(THD_T thread)
{
/**************************************
 *
 *	T H D _ r e s u m e			( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Resume execution of a thread that has been
 *	suspended.
 *
 **************************************/

	if (ResumeThread(thread) == 0xFFFFFFFF)
		return GetLastError();

	return 0;
}


int THD_suspend(THD_T thread)
{
/**************************************
 *
 *	T H D _ s u s p e n d			( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Suspend execution of a thread.
 *
 **************************************/

	if (SuspendThread(thread) == 0xFFFFFFFF)
		return GetLastError();

	return 0;
}
#endif


#ifndef THREAD_SUSPEND_DEFINED
int THD_resume(THD_T thread)
{
/**************************************
 *
 *	T H D _ r e s u m e			( G e n e r i c )
 *
 **************************************
 *
 * Functional description
 *	Resume execution of a thread that has been
 *	suspended.
 *
 **************************************/

	return 0;
}


int THD_suspend(THD_T thread)
{
/**************************************
 *
 *	T H D _ s u s p e n d			( G e n e r i c )
 *
 **************************************
 *
 * Functional description
 *	Suspend execution of a thread.
 *
 **************************************/

	return 0;
}
#endif


void THD_sleep(ULONG milliseconds)
{
/**************************************
 *
 *	T H D _ s l e e p
 *
 **************************************
 *
 * Functional description
 *	Thread sleeps for requested number
 *	of milliseconds.
 *
 **************************************/
#ifdef WIN_NT
	SleepEx(milliseconds, FALSE);
#else

#ifdef ANY_THREADING
	AsyncEvent timer;
	AsyncEvent* timer_ptr = &timer;
	SLONG count;

	ISC_event_init(&timer, 0, 0);
	count = ISC_event_clear(&timer);

	ISC_event_wait(1, &timer_ptr, &count, milliseconds * 1000, NULL, 0);
	ISC_event_fini(&timer);
#else /* !ANY_THREADING */
	int seconds;

/* Insure that process sleeps some amount of time. */

	if (!(seconds = milliseconds / 1000))
		++seconds;

/* Feedback unslept time due to premature wakeup from signals. */

	while (seconds = sleep(seconds));

#endif /* !ANY_THREADING */

#endif /* !WIN_NT */
}


void THD_yield(void)
{
/**************************************
 *
 *	T H D _ y i e l d
 *
 **************************************
 *
 * Functional description
 *	Thread relinquishes the processor.
 *
 **************************************/
#ifdef ANY_THREADING

#ifdef POSIX_THREADS
/* use sched_yield() instead of pthread_yield(). Because pthread_yield() 
   is not part of the (final) POSIX 1003.1c standard. Several drafts of 
   the standard contained pthread_yield(), but then the POSIX guys 
   discovered it was redundant with sched_yield() and dropped it. 
   So, just use sched_yield() instead. POSIX systems on which  
   sched_yield() is available define _POSIX_PRIORITY_SCHEDULING 
   in <unistd.h>.  Darwin defined _POSIX_THREAD_PRIORITY_SCHEDULING
   instead of _POSIX_PRIORITY_SCHEDULING.
*/

#if (defined _POSIX_PRIORITY_SCHEDULING || defined _POSIX_THREAD_PRIORITY_SCHEDULING) || defined(_AIX)
	sched_yield();
#else
#ifdef MVS
	pthread_yield(NULL);
#else
	pthread_yield();
#endif
#endif /* _POSIX_PRIORITY_SCHEDULING */
#endif

#ifdef SOLARIS_MT
	thr_yield();
#endif

#ifdef WIN_NT
	SleepEx(0, FALSE);
#endif
#endif /* ANY_THREADING */
}


#ifdef V4_THREADING
#ifdef SOLARIS_MT
#define CONDITION_DEFINED
static int cond_broadcast(COND_T * cond)
{
/**************************************
 *
 *	c o n d _ b r o a d c a s t	( S o l a r i s )
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	return cond_broadcast(&cond->cond_cond);
}


static int cond_destroy(COND_T * cond)
{
/**************************************
 *
 *	c o n d _ d e s t r o y		( S o l a r i s )
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	THD_mutex_destroy(&cond->cond_mutex);

	return cond_destroy(&cond->cond_cond);
}


static int cond_init(COND_T * cond)
{
/**************************************
 *
 *	c o n d _ i n i t		( S o l a r i s )
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	THD_mutex_init(&cond->cond_mutex);

	return cond_init(&cond->cond_cond, USYNC_THREAD, NULL);
}


static int cond_wait(COND_T * cond, timestruc_t * time)
{
/**************************************
 *
 *	c o n d _ w a i t		( S o l a r i s )
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	if (time)
		return cond_timedwait(&cond->cond_cond, &cond->cond_mutex, time);
	else
		return cond_wait(&cond->cond_cond, &cond->cond_mutex);
}
#endif
#endif


static void init(void)
{
/**************************************
 *
 *	i n i t
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	threadInitMutex.lock();
	
	if (initialized)
	{
		threadInitMutex.release();
		return;
	}

	initialized = TRUE;

#ifdef ANY_THREADING

#ifdef POSIX_THREADS
	for (int n = 0; n < THDD_TYPE_MAX; ++n)
		pthread_key_create(&specificKeys [n], NULL);
#endif

#ifdef SOLARIS_MT
	for (int n = 0; n < THDD_TYPE_MAX; ++n)
		if (thr_keycreate(&specificKeys [n], NULL)) 
			{
			/* This call to thr_min_stack exists simply to force a link error
			 * for a client application that neglects to include -lthread.
			 * Solaris, for unknown reasons, includes stubs for all the
			 * thread functions in libC.  Should the stubs be linked in
			 * there is no compile error, no runtime error, and InterBase
			 * will core drop.
			 * Including this call gives an undefined symbol if -lthread is
			 * omitted, looking at the man page will inform the client programmer
			 * that -lthread is needed to resolve the symbol.
			 * Note that we don't care if this thr_min_stack() is called or
			 * not, we just need to have a reference to it to force a link error.
			 */
			thr_min_stack();
			ib_perror("thr_keycreate");
			exit(1);
			}
#endif /* SOLARIS _MT */

#ifdef WIN_NT
	for (int n = 0; n < THDD_TYPE_MAX; ++n)
		specificKeys [n] = TlsAlloc();
#endif /* WIN_NT */

#pragma FB_COMPILER_MESSAGE("Fix! Bad function ptr type cast.")
	gds__register_cleanup(reinterpret_cast < FPTR_VOID_PTR > (THD_cleanup),
						  0);
#ifdef THREAD_SCHEDULER
#ifdef WIN_NT
	THPS_INIT();
#endif
#endif
	threadInitMutex.release();

#endif /* ANY_THREADING */
}


static void init_tkey(void)
{
/**************************************
 *
 *	i n i t
 *
 **************************************
 *
 * Functional description
 *	Function which actually creates the key which
 *	can be used by the threads to store t_data
 *
 **************************************/

	if (t_init)
		return;

	t_init = TRUE;

#ifdef ANY_THREADING

#ifdef POSIX_THREADS
	pthread_key_create(&t_key, NULL);
#endif

#ifdef SOLARIS_MT
	thr_keycreate(&t_key, NULL);
#endif

#ifdef WIN_NT
	t_key = TlsAlloc();
#endif
#endif
}


#ifdef ANY_THREADING
#ifdef POSIX_THREADS
#define PUT_SPECIFIC_DEFINED
static void put_specific(THDD new_context, int subSystem)
{
/**************************************
 *
 *	p u t _ s p e c i f i c		( P O S I X )
 *
 **************************************
 *
 * Functional description
 * Puts new thread specific data
 *
 **************************************/

	pthread_setspecific(specificKeys [subSystem - 1], new_context);
}
#endif /* ANY_THREADING */
#endif /* POSIX_THREADS */


#ifdef ANY_THREADING
#ifdef SOLARIS_MT
#define PUT_SPECIFIC_DEFINED
static void put_specific(THDD new_context, int subsystem)
{
/**************************************
 *
 *	p u t _ s p e c i f i c		( S o l a r i s )
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	if (thr_setspecific(specificKeys [subsystem - 1], new_context)) {
		ib_perror("thr_setspecific");
		exit(1);
	}
}
#endif
#endif


#ifdef ANY_THREADING
#ifdef WIN_NT
#define PUT_SPECIFIC_DEFINED
static void put_specific(THDD new_context, int subSystem)
{
/**************************************
 *
 *	p u t _ s p e c i f i c		( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

#ifdef THREAD_SCHEDULER
	THPS_SET(TlsSetValue(specificKeys [subSystem - 1], (LPVOID) new_context), new_context);
#else
	TlsSetValue(specificKeys [subSystem - 1], (LPVOID) new_context);
#ifdef DEV_BUILD
	long ret = GetLastError();
	if (ret != NO_ERROR)
	{
		fb_assert(0);
	}
#endif
#endif
}
#endif
#endif


#ifndef PUT_SPECIFIC_DEFINED
static void put_specific(THDD new_context, int subSystem)
{
/**************************************
 *
 *	p u t _ s p e c i f i c		( G e n e r i c )
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	gdbb = new_context;
}
#endif


#ifdef ANY_THREADING
#ifdef POSIX_THREADS
#define START_THREAD
static int thread_start(
						int (*routine) (void *),
						void *arg, int priority_arg, int flags, void *thd_id)
{
/**************************************
 *
 *	t h r e a d _ s t a r t		( P O S I X )
 *
 **************************************
 *
 * Functional description
 *	Start a new thread.  Return 0 if successful,
 *	status if not.
 *
 **************************************/
	pthread_t thread;
	pthread_attr_t pattr;
	int state;

#if ( !defined HP10 && !defined LINUX && !defined FREEBSD && !defined MVS)
		state = pthread_attr_init(&pattr);
	if (state)
		return state;

/* Do not make thread bound for superserver/client */

#if (!defined (SUPERCLIENT) && !defined (SUPERSERVER))
	pthread_attr_setscope(&pattr, PTHREAD_SCOPE_SYSTEM);
#endif

	pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);
	state = pthread_create(&thread, &pattr, (void*(*)(void*))routine, arg);

	if (thd_id)
		memcpy(thd_id, &thread, sizeof(thread));

	pthread_attr_destroy(&pattr);
	return state;

#else
#if ( defined LINUX || defined FREEBSD || defined MVS)
		if (state = pthread_create(&thread, NULL, (void*(*)(void*))routine, arg))
		return state;

	if (thd_id)
		memcpy(thd_id, &thread, sizeof(thread));

#ifdef MVS
	return pthread_detach(&thread);
#else
	return pthread_detach(thread);
#endif
#else
		long stack_size;

	state = pthread_attr_create(&pattr);
	if (state) {
		fb_assert(state == -1);
		return errno;
	}

/* The default HP's stack size is too small. HP's documentation
   says it is "machine specific". My test showed it was less
   than 64K. We definitly need more stack to be able to execute
   concurrently many (at least 100) copies of the same request
   (like, for example in case of recursive stored prcedure).
   The following code sets threads stack size up to 256K if the
   default stack size is less than this number
*/
	stack_size = pthread_attr_getstacksize(pattr);
	if (stack_size == -1)
		return errno;

	if (stack_size < 0x40000L) {
		state = pthread_attr_setstacksize(&pattr, 0x40000L);
		if (state) {
			fb_assert(state == -1);
			return errno;
		}
	}

/* HP's Posix threads implementation does not support
   bound attribute. It just a user level library.
*/
	state = pthread_create(&thread, pattr, routine, arg);
	if (state) {
		fb_assert(state == -1);
		return errno;
	}
	state = pthread_detach(&thread);
	if (state) {
		fb_assert(state == -1);
		return errno;
	}
	state = pthread_attr_delete(&pattr);
	if (state) {
		fb_assert(state == -1);
		return errno;
	}
	memcpy(thd_id, &thread, sizeof(thread));
	return 0;

#endif /* linux */
#endif /* HP10 */
}
#endif /* POSIX_THREADS */
#endif /* ANY_THREADING */


#ifdef ANY_THREADING
#ifdef SOLARIS_MT
#define START_THREAD
static int thread_start(
						int (*routine) (void *),
						void *arg, int priority_arg, int flags, void *thd_id)
{
/**************************************
 *
 *	t h r e a d _ s t a r t		( S o l a r i s )
 *
 **************************************
 *
 * Functional description
 *	Start a new thread.  Return 0 if successful,
 *	status if not.
 *
 **************************************/
	int rval;
	thread_t thread_id;
	sigset_t new_mask, orig_mask;

	sigfillset(&new_mask);
	sigdelset(&new_mask, SIGALRM);
	if (rval = thr_sigsetmask(SIG_SETMASK, &new_mask, &orig_mask))
		return rval;
#if (defined SUPERCLIENT || defined SUPERSERVER)
	rval = thr_create(NULL, 0, (void* (*)(void*) )routine, arg, THR_DETACHED, &thread_id);
#else
	rval =
		thr_create(NULL, 0, (void* (*)(void*) ) routine, arg, (THR_BOUND | THR_DETACHED),
				   &thread_id);
#endif
	thr_sigsetmask(SIG_SETMASK, &orig_mask, NULL);

	return rval;
}
#endif
#endif


#ifdef WIN_NT
#define START_THREAD
static int thread_start(int (*routine) (void *),
						void *arg, int priority_arg, int flags, void *thd_id)
{
/**************************************
 *
 *	t h r e a d _ s t a r t		( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Start a new thread.  Return 0 if successful,
 *	status if not.
 *
 **************************************/

	DWORD thread_id;

	/* I have changed the CreateThread here to _beginthreadex() as using
	 * CreateThread() can lead to memory leaks caused by C-runtime library.
	 * Advanced Windows by Richter pg. # 109. */

	unsigned long real_handle = _beginthreadex(NULL,
											   0,
											   reinterpret_cast <unsigned (__stdcall *) (void *)>(routine),
											   arg,
											   CREATE_SUSPENDED,
											   reinterpret_cast <unsigned *>(&thread_id));

	if (!real_handle) 
		return GetLastError();

	HANDLE handle = reinterpret_cast < HANDLE > (real_handle);
	int priority;

	switch (priority_arg) 
		{
		case THREAD_critical:
			priority = THREAD_PRIORITY_TIME_CRITICAL;
			break;
			
		case THREAD_high:
			priority = THREAD_PRIORITY_HIGHEST;
			break;
			
		case THREAD_medium_high:
			priority = THREAD_PRIORITY_ABOVE_NORMAL;
			break;
			
		case THREAD_medium:
			priority = THREAD_PRIORITY_NORMAL;
			break;
			
		case THREAD_medium_low:
			priority = THREAD_PRIORITY_BELOW_NORMAL;
			break;
			
		case THREAD_low:
		default:
			priority = THREAD_PRIORITY_LOWEST;
			break;
		}

#ifdef THREAD_SCHEDULER
	THPS_ATTACH(handle, thread_id, priority);
	SetThreadPriority(handle, priority);
#endif

	if (! (flags & THREAD_wait))
		ResumeThread(handle);
		
	if (thd_id)
		*(HANDLE *) thd_id = handle;
	else
		CloseHandle(handle);

	return 0;
}
#endif


#ifdef ANY_THREADING
#ifdef VMS
#ifndef POSIX_THREADS
#define START_THREAD
/**************************************
 *
 *	t h r e a d _ s t a r t		( V M S )
 *
 **************************************
 *
 * Functional description
 *	Start a new thread.  Return 0 if successful,
 *	status if not.  This routine is coded in macro.
 *
 **************************************/
#endif
#endif
#endif


#ifndef START_THREAD
static int thread_start(int (*routine) (void *),
						void *arg, int priority_arg, int flags, void *thd_id)
{
/**************************************
 *
 *	t h r e a d _ s t a r t		( G e n e r i c )
 *
 **************************************
 *
 * Functional description
 *	Start a new thread.  Return 0 if successful,
 *	status if not.
 *
 **************************************/

	return 1;
}
#endif



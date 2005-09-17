/*	PROGRAM:	JRD Access Method
 *	MODULE:		AsyncEvent.h
 *	DESCRIPTION:	Asynchronous Event Waiting
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

// AsyncEvent.h: interface for the AsyncEvent class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ASYNCEVENT_H__3BA65948_FE95_4A41_B754_C8A554AAFDBC__INCLUDED_)
#define AFX_ASYNCEVENT_H__3BA65948_FE95_4A41_B754_C8A554AAFDBC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//#include "thd.h"

START_NAMESPACE

class AsyncEvent  
{
public:
	void fini();
	int wait (SLONG value, SLONG microSeconds, FPTR_VOID_PTR handler, void* arg);
	int wait (SLONG value, SLONG microSeconds);
	void post();
	SLONG clear();
	int init (int semid, int semnum);
	int init();
	AsyncEvent();
	//virtual ~AsyncEvent();

#ifdef WIN_NT
	SLONG			event_pid;
	SLONG			event_count;
	SLONG			event_type;
	void*			event_handle;
	AsyncEvent*		event_shared;
#endif /* WIN_NT */

#ifdef UNIX
#ifndef ANY_THREADING
#define ANY_THREADING 1
#endif
#ifdef ANY_THREADING
	SLONG event_semid;
	SLONG event_count;
	THD_MUTEX_STRUCT event_mutex[1];
	THD_COND_STRUCT event_semnum[1];
#else
	SLONG event_semid;
	SLONG event_count;
	SSHORT event_semnum;
#endif /* ANY_THREADING */
#endif /* UNIX */

#ifdef VMS
	SLONG event_pid;
	SLONG event_count;
#endif

};

END_NAMESPACE

#endif // !defined(AFX_ASYNCEVENT_H__3BA65948_FE95_4A41_B754_C8A554AAFDBC__INCLUDED_)

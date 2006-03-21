/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		Asynchronous Event Waiting
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

// Asynchronous Event Waiting: implementation of the AsyncEvent class.
//
//////////////////////////////////////////////////////////////////////

#include "fbdev.h"
#include "AsyncEvent.h"
#include "../jrd/isc_s_proto.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

AsyncEvent::AsyncEvent()
{

}

/***
AsyncEvent::~AsyncEvent()
{

}
***/

int AsyncEvent::init()
{
	return ISC_event_init (this, 0, 0);
}

int AsyncEvent::init(int semid, int semnum)
{
	return ISC_event_init (this, semid, semnum);
}

SLONG AsyncEvent::clear()
{
	return ISC_event_clear (this);
}

void AsyncEvent::post()
{
	ISC_event_post (this);
}

int AsyncEvent::wait(SLONG value, SLONG microSeconds)
{
	AsyncEvent *ptr = this;
	return ISC_event_wait (1, &ptr, &value, microSeconds, NULL, NULL); 
}

int AsyncEvent::wait(SLONG value, SLONG microSeconds, FPTR_VOID_PTR handler, void *arg)
{
	AsyncEvent *ptr = this;
	return ISC_event_wait (1, &ptr, &value, microSeconds, handler, arg); 
}

void AsyncEvent::fini()
{
	ISC_event_fini (this);
}

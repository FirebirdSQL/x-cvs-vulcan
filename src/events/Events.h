/*
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
 *
 *     Software distributed under the License is distributed on 
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *     express or implied.  See the License for the specific 
 *     language governing rights and limitations under the License.
 *
 *     The contents of this file or any work derived from this file
 *     may not be distributed under any other license whatsoever 
 *     without the express prior written permission of the original 
 *     author.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 2005 James A. Starkey
 *  All Rights Reserved.
 */

#ifndef _EVENTS_H_
#define _EVENTS_H_

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _PTHREADS
#include <pthread.h>
#include <errno.h>
#endif

#include <ibase.h>

typedef unsigned char	UCHAR;

class Events
{
public:
	Events::Events(void);
	Events(isc_db_handle handle, int argc, ...);
	virtual ~Events(void);

	ISC_STATUS		statusVector[20];
	isc_db_handle	dbHandle;	
	UCHAR			*eventBlock;
	UCHAR			*resultBlock;
	int				eventBlockLength;
	int				allocatedLength;
	int				eventId;
	int				numberEvents;
	bool			sleeping;
	bool			wakeup;
	
#ifdef _WIN32
	void	*event;
#endif

#ifdef _PTHREADS
	pthread_cond_t	condition;
	pthread_mutex_t	mutex;
#endif
	
	void		addEvent(const char* eventName);
	int			waitForEvents();
	void		wake(void);
	void		sleep(void);
	bool		queEvents(void);
	void		cancelEvents(void);

protected:
	int			getCount(const UCHAR **event);
	void		setEvents(int numberEvents, va_list args);
	void		init(void);
	void		alloc(int minLength, bool copy);
	static void eventCompleted(void* events, unsigned short length, UCHAR* eventBuffer);
	void		eventCompletion(int length, UCHAR* buffer);

public:
	void setDatabase(isc_db_handle handle);
};

#endif

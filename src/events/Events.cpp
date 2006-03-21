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

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "Events.h"

#ifndef va_copy
#define va_copy(to,from) to = from
#endif

Events::Events(void)
{
	init();
}

Events::~Events(void)
{
	delete [] eventBlock;
	delete [] resultBlock;
}

Events::Events(isc_db_handle handle, int argc, ...)
{
	init();
	dbHandle = handle;
	va_list args;
	va_start(args, argc);
	setEvents(argc, args);
	va_end(args);
}

void Events::setEvents(int numberEvents, va_list stuff)
{
	int length = 1;
	va_list args;
	int n;
	va_copy(args, stuff);
	
	for (n = 0; n < numberEvents; ++n)
		{
		const char *eventName = va_arg(args, const char*);
		length += (int) strlen(eventName) + 1 + 4;
		}
	
	va_end(args);
	alloc(length, false);
	va_copy(args, stuff);
	
	for (n = 0; n < numberEvents; ++n)
		addEvent(va_arg(args, const char*));
	
	va_end(args);
}

void Events::init(void)
{
	eventBlock = NULL;
	resultBlock = NULL;
	allocatedLength = 0;
	eventBlockLength = 0;
	eventId = 0;
	sleeping = false;
	wakeup = false;
	numberEvents = 0;
	
#ifdef _WIN32
	event = CreateEvent (NULL, false, false, NULL);
#endif

#ifdef _PTHREADS
	int ret = pthread_mutex_init (&mutex, NULL);
	pthread_cond_init (&condition, NULL);
#endif
}

void Events::alloc(int minLength, bool copy)
{
	if (minLength <= allocatedLength)
		return;
	
	allocatedLength = minLength;
	UCHAR *oldEvents = eventBlock;
	delete [] resultBlock;
	eventBlock = new UCHAR[allocatedLength];
	resultBlock = new UCHAR[allocatedLength];
	
	if (oldEvents && copy && eventBlockLength)
		memcpy (eventBlock, oldEvents, eventBlockLength);
	else
		{
		eventBlockLength = 0;
		numberEvents = 0;
		}
	
	delete [] oldEvents;
}

void Events::addEvent(const char* eventName)
{
	int nameLength = (int) strlen(eventName);	
	int length = eventBlockLength + 1 + nameLength + 4;
	
	if (!eventBlockLength)
		++length;

	if (length > allocatedLength)
		alloc(length + 100, true);
		
	UCHAR *start = eventBlock + eventBlockLength;
	UCHAR *p = start;
	
	if (!eventBlockLength)
		*p++ = isc_epb_version1;
	
	*p++ = (UCHAR) nameLength;
	
	while (*eventName)
		*p++ = *eventName++;
	
	for (int n = 0; n < 4; ++n)
		*p++ = 0;
		
	eventBlockLength += (int) (p - start);
	++numberEvents;
}

int Events::waitForEvents()
{
	if (!queEvents())
		return 0;
			
	sleep();
	int bits = 0;
	const UCHAR *p = eventBlock + 1;
	const UCHAR *q = resultBlock + 1;
	
	for (int n = 0; n < numberEvents; ++n)
		{
		int before = getCount(&p);
		int after = getCount(&q);
		
		if (before != after)
			bits |= 1 << n;
		}
	
	UCHAR *temp = eventBlock;
	eventBlock = resultBlock;
	resultBlock = temp;
	
	return bits;
}

void Events::eventCompleted(void* events, unsigned short length, UCHAR* eventBuffer)
{
	((Events*) events)->eventCompletion(length, eventBuffer);
}

void Events::eventCompletion(int length, UCHAR* buffer)
{
	memcpy(resultBlock, buffer, length);
	eventId = 0;
	wake();
}

void Events::wake(void)
{
#ifdef _WIN32
	SetEvent (event);
#endif

#ifdef _PTHREADS
	int ret = pthread_mutex_lock (&mutex);

	if (ret)
		{
		fprintf(stderr, "pthread_mutex_lock failed, errno %d\n", errno);
		return;
		}

	wakeup = true;
	pthread_cond_broadcast (&condition);

	if (ret = pthread_mutex_unlock (&mutex))
		{
		fprintf(stderr, "pthread_mutex_unlock failed, errno %d\n", errno);
		return;
		}
#endif
}

void Events::sleep(void)
{
	sleeping = true;
#ifdef _WIN32
#ifdef _DEBUG
	for (;;)
		{
		int n = WaitForSingleObject (event, 10000);
		sleeping = false;
		if (n != WAIT_TIMEOUT)
			return;
		}
#else
	sleep (INFINITE);
#endif
#endif

#ifdef _PTHREADS
	int ret = pthread_mutex_lock (&mutex);

	if (ret)
		{
		fprintf(stderr, "pthread_mutex_lock failed, errno %d\n", errno);
		return;
		}

	while (!wakeup)
		pthread_cond_wait (&condition, &mutex);

	wakeup = false;
	
	if (ret = pthread_mutex_unlock (&mutex))
		{
		fprintf(stderr, "pthread_mutex_unlock failed, errno %d\n", errno);
		return;
		}
#endif

	sleeping = false;
}

int Events::getCount(const UCHAR **event)
{
	const UCHAR *p = *event;
	int count = 0;
	int length = *p++;
	p += length;
	
	for (int n = 0; n < 4; ++n)
		count |= *p++ << (n * 4);
	
	*event = p;
	
	return count;
}

bool Events::queEvents(void)
{
	if (isc_que_events (statusVector, &dbHandle, &eventId, 
						eventBlockLength, (char*) eventBlock, (FPTR_EVENT_CALLBACK) eventCompleted, this))
		return false;
	
	return true;
}

void Events::cancelEvents(void)
{
	if (eventId)
		{
		isc_cancel_events(statusVector, &dbHandle, &eventId);
		eventId = 0;
		}
}

void Events::setDatabase(isc_db_handle handle)
{
	cancelEvents();
	dbHandle = handle;
	
	for (UCHAR *p = eventBlock + 1, *end = eventBlock + eventBlockLength; p < end;)
		{
		UCHAR length = *p++;
		p += length;
		
		for (int n = 0; n < 4; ++n)
			*p++ = 0;
		}
}

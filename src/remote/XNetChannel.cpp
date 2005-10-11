/*
 *	PROGRAM:	JRD Remote Interface/Server
 *      MODULE:         xnet.cpp
 *      DESCRIPTION:    Interprocess Server Communications module.
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
 * 2003.05.01 Victor Seryodkin, Dmitry Yemanov: Completed XNET implementation
 */

#ifdef _WIN32
#include <windows.h>
#endif /* WIN_NT */

#include <time.h>
#include <stdio.h>
#include "fbdev.h"
#include "common.h"
#include "XNetChannel.h"
#include "isc_proto.h"
#include "xnet.h"

#define XNET_PREFIX				"FirebirdXNET"

#define XNET_E_C2S_DATA_CHAN_FILLED	"%s_E_C2S_DATA_FILLED_%"ULONGFORMAT"_%"ULONGFORMAT"_%"ULONGFORMAT
#define XNET_E_CHAN	"%s_E_%s_%s_%s_%"ULONGFORMAT"_%"ULONGFORMAT"_%"ULONGFORMAT


XNetChannel::XNetChannel(void)
{
	channelFilled = 0;
	channelEmptied = 0;
}

XNetChannel::~XNetChannel(void)
{
	close();
}

void XNetChannel::close(void)
{
	closeEvent(&channelFilled);
	closeEvent(&channelEmptied);
}

void XNetChannel::closeEvent(HANDLE* handlePtr)
{
	if (*handlePtr) 
		{
#ifdef WIN_NT
		CloseHandle(*handlePtr);
#endif
		*handlePtr = 0;
		}
}

HANDLE XNetChannel::createEvent(const char* name)
{
#ifdef WIN_NT
	HANDLE handle = CreateEvent(ISC_get_security_desc(), FALSE, FALSE, name);
	
	if (!handle || GetLastError() == ERROR_ALREADY_EXISTS)
		error("CreateEvent");

	return handle;
#else

	return 0;

#endif
}

HANDLE XNetChannel::openEvent(const char* name)
{
#ifdef WIN_NT
	HANDLE handle = OpenEvent(EVENT_ALL_ACCESS, FALSE, name);
	
	if (!handle) 
		error("openEvent");
		
	return handle;
#else

	return 0;

#endif
}


void XNetChannel::error(const char* operation)
{
#ifdef WIN_NT
	int error = GetLastError();
#endif

	throw -1;
}

void XNetChannel::open(bool eventChannel, bool toServer, int mapNum, int slot, time_t timestamp)
{
	char name[128];
	channelFilled = openEvent(genName(eventChannel, toServer, true, slot, mapNum, timestamp, name));
	channelEmptied = openEvent(genName(eventChannel, toServer, false, slot, mapNum, timestamp, name));
}

void XNetChannel::create(bool eventChannel, bool toServer, int mapNum, int slot, time_t timestamp)
{
	char name[128];
	channelFilled = createEvent(genName(eventChannel, toServer, true, slot, mapNum, timestamp, name));
	channelEmptied = createEvent(genName(eventChannel, toServer, false, slot, mapNum, timestamp, name));
}

const char* XNetChannel::genName(bool eventChannel, bool toServer, bool filledEvent, int mapNum, int slot, time_t timestamp, char* buffer)
{
	//	"%s_E_C2S_DATA_FILLED_%"ULONGFORMAT"_%"ULONGFORMAT"_%"ULONGFORMAT
	sprintf(buffer, XNET_E_CHAN,
			XNET_PREFIX,
			(toServer) ? "C2S" : "S2C",
			(eventChannel) ? "EVENT" : "DATA",
			(filledEvent) ? "FILLED" : "EMPTIED",
			slot, mapNum, timestamp);
			
	return buffer;	
}

void* XNetChannel::preSend(int timeout)
{
	if (wait(channelEmptied, timeout))
		return data;

	return NULL;
}

void XNetChannel::send(int length)
{
	channelControl->xch_length = length;
	postEvent(channelFilled);
}

void* XNetChannel::receive(int timeout)
{
	if (wait(channelFilled, timeout))
		return data;
		
	return NULL;
}

void XNetChannel::postReceive(void)
{
	postEvent(channelEmptied);
}

int XNetChannel::getMsgLength(void)
{
	return channelControl->xch_length;
}

int XNetChannel::getMsgSize(void)
{
	return channelControl->xch_size;
}

bool XNetChannel::wait(HANDLE handle, int timeout)
{
#ifdef WIN_NT
	DWORD ret = WaitForSingleObject(handle, timeout);
	
	if (ret == WAIT_OBJECT_0)
		return true;
		
	if (ret != WAIT_TIMEOUT)
		error ("WaitForSingleObject");
#endif

	return false;
}

void XNetChannel::postEvent(HANDLE handle)
{
#ifdef WIN_NT
	if (!SetEvent(handle))
		error ("SetEvent");
#endif
}


HANDLE XNetChannel::createMutex(const char* pattern)
{
#ifdef WIN_NT
	char name_buffer[128];
	sprintf(name_buffer, pattern, XNET_PREFIX);
	HANDLE handle = CreateMutex(ISC_get_security_desc(), FALSE, name_buffer);
	
	if (!handle || GetLastError() == ERROR_ALREADY_EXISTS)
		error("CreateMutex");

	return handle;
#else
	
	return 0;

#endif
}

HANDLE XNetChannel::openMutex(const char* pattern)
{
#ifdef WIN_NT
	char name_buffer[128];
	sprintf(name_buffer, pattern, XNET_PREFIX);
	HANDLE handle = OpenMutex(MUTEX_ALL_ACCESS, TRUE, name_buffer);
	
	if (!handle) 
		error("OpenMutex");
		
	return handle;
#else

	return 0;

#endif
}

void XNetChannel::closeMutex(HANDLE* handlePtr)
{
	if (*handlePtr) 
		{
#ifdef WIN_NT
		CloseHandle(*handlePtr);
#endif // WIN_NT
		*handlePtr = 0;
		}
}

void XNetChannel::releaseMutex(HANDLE handle)
{
#ifdef WIN_NT
		ReleaseMutex(handle);
#endif // WIN_NT
}

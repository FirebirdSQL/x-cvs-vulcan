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
#include ".\xnetchannel.h"

#define XNET_PREFIX				"FirebirdXNET"

#define XNET_E_C2S_DATA_CHAN_FILLED	"%s_E_C2S_DATA_FILLED_%"ULONGFORMAT"_%"ULONGFORMAT"_%"ULONGFORMAT
#define XNET_E_CHAN	"%s_E_C2S_DATA_FILLED_%"ULONGFORMAT"_%"ULONGFORMAT"_%"ULONGFORMAT


XNetChannel::XNetChannel(void)
{
	channelFilled = 0;
	channelEmptied = 0;
}

XNetChannel::~XNetChannel(void)
{
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

HANDLE XNetChannel::openEvent(bool eventChannel, int mapNum, int slot, time_t timestamp, const char *pattern)
{
#ifdef WIN_NT
	TEXT name_buffer[128];
	const char *type = (eventChannel) ? "EVNT" : "DATA";
	sprintf(name_buffer, pattern, XNET_PREFIX, type, mapNum, slot, (ULONG) timestamp);
	HANDLE handle = OpenEvent(EVENT_ALL_ACCESS, FALSE, name_buffer);

	if (!handle)
		error("OpenEvent");
	
	return handle;
#else

	return 0;

#endif // WIN_NT
}

HANDLE XNetChannel::createEvent(bool eventChannel, int mapNum, int slot, time_t timestamp, const char* pattern)
{
#ifdef WIN_NT
	TEXT name_buffer[128];
	const char *type = (eventChannel) ? "EVNT" : "DATA";
	sprintf(name_buffer, pattern, XNET_PREFIX, type, mapNum, slot, (ULONG) timestamp);
	HANDLE handle = CreateEvent(ISC_get_security_desc(), FALSE, FALSE, name_buffer);

	if (!handle)
		error("CreateEvent");
	
	return handle;
#else

	return 0;

#endif
}

HANDLE XNetChannel::createEvent(const char* pattern)
{
#ifdef WIN_NT
	char name_buffer[128];
	sprintf(name_buffer, pattern, XNET_PREFIX);
	HANDLE handle = CreateEvent(ISC_get_security_desc(), FALSE, FALSE, name_buffer);
	
	if (!handle || GetLastError() == ERROR_ALREADY_EXISTS)
		error("CreateEvent");

	return handle;
#else

	return 0;

#endif
}

HANDLE XNetChannel::openEvent(const char* pattern)
{
#ifdef WIN_NT
	char name_buffer[128];
	sprintf(name_buffer, pattern, XNET_PREFIX);
	HANDLE handle = OpenEvent(EVENT_ALL_ACCESS, FALSE, name_buffer);
	
	if (!handle) 
		error("openEvent");
		
	return handle;
#else

	return 0;

#endif
}


void XNetChannel::error(const char* operation)
{
	throw -1;
}

void XNetChannel::open(bool eventChannel, bool sendChannel, int mapNum, int slot, time_t timestamp)
{
	
}

void XNetChannel::genName(bool eventChannel, bool toServer, bool filledEvent, int slot, int mapNum, time_t timestamp, char* buffer)
{
}

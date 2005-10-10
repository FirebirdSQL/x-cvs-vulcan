/*
 *	PROGRAM:	JRD Remote Interface/Server
 *      MODULE:         XNetConnection.cpp
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
#include "XNetConnection.h"
#include "isc_proto.h"
#include "XNetMappedFile.h"

#define XNET_PREFIX				"FirebirdXNET"

#define XNET_E_C2S_FILLED	"%s_E_C2S_%s_FILLED_%"ULONGFORMAT"_%"ULONGFORMAT"_%"ULONGFORMAT
#define XNET_E_C2S_EMPTED	"%s_E_C2S_%s_EMPTED_%"ULONGFORMAT"_%"ULONGFORMAT"_%"ULONGFORMAT
#define XNET_E_S2C_FILLED	"%s_E_S2C_%s_FILLED_%"ULONGFORMAT"_%"ULONGFORMAT"_%"ULONGFORMAT
#define XNET_E_S2C_EMPTED	"%s_E_S2C_%s_EMPTED_%"ULONGFORMAT"_%"ULONGFORMAT"_%"ULONGFORMAT

/***
#define XNET_E_C2S_DATA_CHAN_FILLED	"%s_E_C2S_DATA_FILLED_%"ULONGFORMAT"_%"ULONGFORMAT"_%"ULONGFORMAT
#define XNET_E_C2S_DATA_CHAN_EMPTED	"%s_E_C2S_DATA_EMPTED_%"ULONGFORMAT"_%"ULONGFORMAT"_%"ULONGFORMAT
#define XNET_E_S2C_DATA_CHAN_FILLED	"%s_E_S2C_DATA_FILLED_%"ULONGFORMAT"_%"ULONGFORMAT"_%"ULONGFORMAT
#define XNET_E_S2C_DATA_CHAN_EMPTED	"%s_E_S2C_DATA_EMPTED_%"ULONGFORMAT"_%"ULONGFORMAT"_%"ULONGFORMAT

#define XNET_E_C2S_EVNT_CHAN_FILLED	"%s_E_C2S_EVNT_FILLED_%"ULONGFORMAT"_%"ULONGFORMAT"_%"ULONGFORMAT
#define XNET_E_C2S_EVNT_CHAN_EMPTED	"%s_E_C2S_EVNT_EMPTED_%"ULONGFORMAT"_%"ULONGFORMAT"_%"ULONGFORMAT
#define XNET_E_S2C_EVNT_CHAN_FILLED	"%s_E_S2C_EVNT_FILLED_%"ULONGFORMAT"_%"ULONGFORMAT"_%"ULONGFORMAT
#define XNET_E_S2C_EVNT_CHAN_EMPTED	"%s_E_S2C_EVNT_EMPTED_%"ULONGFORMAT"_%"ULONGFORMAT"_%"ULONGFORMAT
***/

XNetConnection::XNetConnection(int mapNum, int slot)
{
	init();
	xcc_map_num = mapNum;
	xcc_slot = slot;
}


XNetConnection::XNetConnection(XNetConnection* parent)
{
	init();
	xcc_xpm = parent->xcc_xpm;
	xcc_map_num = parent->xcc_map_num;
	xcc_slot = parent->xcc_slot;
	xcc_proc_h = parent->xcc_proc_h;
	xcc_map_handle = parent->xcc_map_handle;
	xcc_mapped_addr = parent->xcc_mapped_addr;
	xcc_xpm->xpm_count++;
}

void XNetConnection::init()
{
	/***
	xcc_event_send_channel_filled = 0;
	xcc_event_send_channel_empted = 0;
	xcc_event_recv_channel_filled = 0;
	xcc_event_recv_channel_empted = 0;
	***/
	xcc_proc_h = 0;
	xcc_flags = 0;
	xcc_next = NULL;
	xcc_mapped_addr = NULL;
	xcc_xpm = NULL;
	xcc_slot = 0;
}

XNetConnection::~XNetConnection(void)
{
	close();
}

void XNetConnection::close(void)
{
	sendChannel.close();
	recvChannel.close();
	/***
	closeEvent(&xcc_event_send_channel_filled);
	closeEvent(&xcc_event_send_channel_empted);
	closeEvent(&xcc_event_recv_channel_filled);
	closeEvent(&xcc_event_recv_channel_empted);
	***/
	closeEvent(&xcc_proc_h);
}

void XNetConnection::open(bool eventChannel, time_t timestamp)
{
	sendChannel.open(eventChannel, true, xcc_map_num, xcc_slot, timestamp);
	recvChannel.open(eventChannel, false, xcc_map_num, xcc_slot, timestamp);
	/***
	xcc_event_send_channel_filled = openEvent(eventChannel, timestamp, XNET_E_C2S_FILLED);
	xcc_event_send_channel_empted = openEvent(eventChannel, timestamp, XNET_E_C2S_EMPTED);
	xcc_event_recv_channel_filled = openEvent(eventChannel, timestamp, XNET_E_S2C_FILLED);
	xcc_event_recv_channel_empted = openEvent(eventChannel, timestamp, XNET_E_S2C_EMPTED);
	***/
}

void XNetConnection::create(bool eventChannel, time_t timestamp)
{
	sendChannel.create(eventChannel, false, xcc_map_num, xcc_slot, timestamp);
	recvChannel.create(eventChannel, true, xcc_map_num, xcc_slot, timestamp);
	/***
	xcc_event_send_channel_filled = createEvent(eventChannel, timestamp, XNET_E_S2C_FILLED);
	xcc_event_send_channel_empted = createEvent(eventChannel, timestamp, XNET_E_S2C_EMPTED);
	xcc_event_recv_channel_filled = createEvent(eventChannel, timestamp, XNET_E_C2S_FILLED);
	xcc_event_recv_channel_empted = createEvent(eventChannel, timestamp, XNET_E_C2S_EMPTED);
	***/
}

void XNetConnection::closeEvent(HANDLE* handlePtr)
{
	if (*handlePtr) 
		{
#ifdef WIN_NT
		CloseHandle(*handlePtr);
#endif
		*handlePtr = 0;
		}
}


HANDLE XNetConnection::openMutex(const char* pattern)
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

HANDLE XNetConnection::createMutex(const char* pattern)
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

void XNetConnection::closeMutex(HANDLE* handlePtr)
{
	if (*handlePtr) 
		{
#ifdef WIN_NT
		CloseHandle(*handlePtr);
#endif // WIN_NT
		*handlePtr = 0;
		}
}

void XNetConnection::error(const char* operation)
{
	throw -1;
}

void* XNetConnection::preSend(int timeout)
{
	return sendChannel.preSend(timeout);
}

void XNetConnection::send(int length)
{
	sendChannel.send(length);
}

void* XNetConnection::receive(int timeout)
{
	return recvChannel.receive(timeout);
}

void XNetConnection::postReceive(void)
{
	return recvChannel.postReceive();
}

bool XNetConnection::stillAlive(void)
{
	return WaitForSingleObject(xcc_proc_h, 1) == WAIT_TIMEOUT;			
}

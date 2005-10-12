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
	clone = true;
	xcc_xpm->xpm_count++;
}

void XNetConnection::init()
{
	xcc_proc_h = 0;
	xcc_flags = 0;
	xcc_next = NULL;
	xcc_mapped_addr = NULL;
	xcc_xpm = NULL;
	xcc_slot = 0;
	clone = false;
}

XNetConnection::~XNetConnection(void)
{
	close();
}

void XNetConnection::close(void)
{
	sendChannel.close();
	recvChannel.close();
	
	if (!clone)
		XNetChannel::closeEvent(&xcc_proc_h);
}

void XNetConnection::open(bool eventChannel, time_t timestamp)
{
	sendChannel.open(eventChannel, true, xcc_map_num, xcc_slot, timestamp);
	recvChannel.open(eventChannel, false, xcc_map_num, xcc_slot, timestamp);
}

void XNetConnection::create(bool eventChannel, time_t timestamp)
{
	sendChannel.create(eventChannel, false, xcc_map_num, xcc_slot, timestamp);
	recvChannel.create(eventChannel, true, xcc_map_num, xcc_slot, timestamp);
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
	return !XNetChannel::wait(xcc_proc_h, 1);
}

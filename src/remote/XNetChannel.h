/*
 *	PROGRAM:	Interprocess Interface definitions
 *      MODULE:         XNetChannel.h
 *	DESCRIPTION:	Common descriptions
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

#ifndef XNET_CHANNEL_H
#define XNET_CHANNEL_H

#ifndef _WIN32
#ifndef HANDLE
#define HANDLE		int
#endif
#endif

struct ChannelControl;

class XNetChannel
{
public:
	XNetChannel(void);
	~XNetChannel(void);

    ChannelControl	*channelControl;
    UCHAR			*data;
    HANDLE			channelFilled;
    HANDLE			channelEmptied;
    
	void			close(void);
	static HANDLE	createEvent(const char* pattern);
	static HANDLE	openEvent(const char* pattern);
	static void		closeEvent(HANDLE* handlePtr);
	static void		error(const char* operation);
	void			open(bool eventChannel, bool sendChannel, int mapNum, int slot, time_t timestamp);
	void			create(bool eventChannel, bool sendChannel, int mapNum, int slot, time_t timestamp);
	static const char* genName(bool eventChannel, bool toServer, bool filledEvent, int mapNum, int slot, time_t timestamp, char* buffer);
	void*			preSend(int timeout);
	void			send(int length);
	void*			receive(int timeout);
	void			postReceive(void);
	int				getMsgLength(void);
	int				getMsgSize(void);
	static bool		wait(HANDLE handle, int timeout);
	static void		postEvent(HANDLE handle);
	static HANDLE	openMutex(const char* pattern);
	static HANDLE	createMutex(const char* pattern);
	static void		closeMutex(HANDLE* handlePtr);
	static void		releaseMutex(HANDLE handle);
};

#endif

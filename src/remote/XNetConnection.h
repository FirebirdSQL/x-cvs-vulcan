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

#ifndef XNET_CONNECTION_H
#define XNET_CONNECTION_H

#include "XNetChannel.h"

class XNetMappedFile;
struct xpm;

class XNetConnection
{
public:
	XNetConnection(XNetConnection* parent);
	XNetConnection(int mapNum, int slot);
	~XNetConnection(void);
	void			close(void);
	void			open(bool eventChannel, time_t timestamp);
	void			create(bool eventChannel, time_t timestamp);
	static void		error(const char* operation);
	void			init(void);
	void*			preSend(int timeout);
	void			send(int length);
	void*			receive(int timeout);
	void			postReceive(void);
	bool			stillAlive(void);

    XNetConnection  *xcc_next;              /* pointer to next thread */
    XNetMappedFile	*xcc_xpm;               /* pointer back to xpm */
    ULONG			xcc_map_num;            /* this thread's mapped file number */
    ULONG			xcc_slot;               /* this thread's slot number */
    
    HANDLE			xcc_map_handle;         /* mapped file's handle */
    HANDLE			xcc_proc_h;             /* for server client's process handle
		                                       for client server's process handle */
    XNetChannel		recvChannel;			/* receive channel structure */
    XNetChannel		sendChannel;			/* send channel structure */
    ULONG			xcc_flags;              /* status bits */
    UCHAR			*xcc_mapped_addr;       /* where the thread's mapped to */
    bool			clone;					/* this is clone; don't close process handle */
};

#endif

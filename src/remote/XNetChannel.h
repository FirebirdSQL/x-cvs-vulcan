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

/* xch comm channel structure - four per connection (client to server data,
   server to client data, client to server events, server to client events) */

class XNetChannel
{
public:
	XNetChannel(void);
	~XNetChannel(void);

    ULONG		xch_length;      /* message length */
    ULONG		xch_size;        /* channel data size */
    USHORT      xch_flags;       /* flags */
    UCHAR       *xch_buffer;     /* message */
    UCHAR 	    *xch_client_ptr; /* client pointer to xch buffers */
	void close(void);
};

#endif

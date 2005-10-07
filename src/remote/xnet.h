/*
 *	PROGRAM:	Interprocess Interface definitions
 *      MODULE:         xnet.h
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

#ifndef REMOTE_XNET_H
#define REMOTE_XNET_H

#ifndef WIN_NT
#include <sys/types.h>
#define PID_T	pid_t
#else
#define PID_T  	DWORD
#define CADDR_T	LPVOID
#endif

#define XNET_CONNECT_TIMEOUT 10000 /* client connect timeout (ms) */

#define XNET_RECV_WAIT_TIMEOUT 10000                  /* Receive wait timeout (ms) */
#define XNET_SEND_WAIT_TIMEOUT XNET_RECV_WAIT_TIMEOUT /* Send wait timeout (ms) */


#define XNET_INVALID_MAP_NUM 0xFFFFFFFF

#define XNET_EVENT_SPACE 100 /* half of space (bytes) for event handling per connection */


class XNetChannel;


//typedef ConnectionControl	*XCC;

/* xcc structure flags */
#define XCCF_SERVER_SHUTDOWN    2       /* server has shutdown detected */

#include "XNetChannel.h"

/* This structure (xps) is mapped to the start of the allocated
    communications area between the client and server. */

typedef struct xps
{
    ULONG       xps_server_protocol;    /* server's protocol level */
    ULONG       xps_client_protocol;    /* client's protocol level */
    PID_T       xps_server_proc_id;     /* server's process id */
    PID_T       xps_client_proc_id;     /* client's process id */
    USHORT      xps_flags;              /* flags word */
    XNetChannel	xps_channels[4];        /* comm channels */
    ULONG       xps_data[1];            /* start of data area */
} *XPS;


/* xps_channel numbers */

#define XPS_CHANNEL_C2S_DATA   0        /* 0 - client to server data */
#define XPS_CHANNEL_S2C_DATA   1        /* 1 - server to client data */
#define XPS_CHANNEL_C2S_EVENTS 2        /* 2 - client to server events */
#define XPS_CHANNEL_S2C_EVENTS 3        /* 3 - server to client events */


#define XPI_CLIENT_PROTOCOL_VERSION 3L
#define XPI_SERVER_PROTOCOL_VERSION 3L

/* XNET_RESPONSE - server response on client connect request */

class XNetResponse {
public:
	ULONG proc_id;
	ULONG slots_per_map;
	ULONG pages_per_slot;
	ULONG map_num;
	ULONG slot_num;
	time_t timestamp;
}; // XNET_RESPONSE, *PXNET_RESPONSE;

/* XNET_CONNECT_RESPONZE_SIZE - amount of bytes server writes on connect response */

#define XNET_CONNECT_RESPONZE_SIZE  sizeof(XNET_RESPONSE)

/* Windows names used to identify various named objects */

#define XNET_PREFIX				"FirebirdXNET"
#define XNET_MAPPED_FILE_NAME	"%s_MAP_%"ULONGFORMAT"_%"ULONGFORMAT

#define XNET_MA_CONNECT_MAP		"%s_CONNECT_MAP"
#define XNET_MU_CONNECT_MUTEX	"%s_CONNECT_MUTEX"
#define XNET_E_CONNECT_EVENT	"%s_E_CONNECT_EVENT"
#define XNET_E_RESPONSE_EVENT	"%s_E_RESPONSE_EVENT"

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

#endif /* REMOTE_XNET_H */


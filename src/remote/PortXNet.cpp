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

#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include "fbdev.h"
#include "common.h"
#include "ibase.h"
#include "PortXNet.h"
#include "../remote/remote.h"
#include "../jrd/iberr.h"
#include "../remote/xnet.h"
#include "../remote/proto_proto.h"
#include "../remote/remot_proto.h"
#include "../remote/serve_proto.h"
//#include "../remote/os/win32/window.h"
#include "../jrd/gds_proto.h"
#include "../jrd/isc_proto.h"
//#include "../jrd/thd_proto.h"
#include "Mutex.h"
#include "Sync.h"
#include "XNetConnection.h"
#include "XNetChannel.h"
#include "XNetMappedFile.h"

#ifdef WIN_NT
#include <windows.h>
#define getpid	GetCurrentProcessId
#else
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#endif /* WIN_NT */

#ifdef WIN_NT
#define ERRNO		GetLastError()
#endif

#ifdef UNIX
#define ERRNO		errno
#endif

#ifndef SYS_ERR
#define SYS_ERR		isc_arg_win32
#endif

#define XNET_ERROR(po,fu,op,st) po->error(fu,op,st,__LINE__);
#define XNET_LOG_ERROR(msg) logError(__LINE__,msg)
#define XNET_LOG_ERRORC(msg) logError(__LINE__,msg,ERRNO)

#define MAX_SEQUENCE	256

#ifdef SUPERCLIENT
static HANDLE server_process_handle = 0;
//static void xnet_on_server_shutdown(PORT port);
#else
static TEXT XNET_command_line[MAXPATHLEN + 32], *XNET_p;
//static XPM xnet_make_xpm(ULONG, time_t);
#endif // SUPERCLIENT


static xdr_t::xdr_ops xnet_ops = {
	Port::getLong,			//inet_getlong,
	Port::putLong,			//inet_putlong,
	PortXNet::getBytes,		//xnet_getbytes,
	PortXNet::putBytes,		//xnet_putbytes,
	Port::getPosition,		//inet_getpostn,
	Port::setPosition,		//inet_setpostn,
	Port::inlinePointer,	//inet_inline,
	Port::destroy			// inet_destroy
};

#ifndef MAX_PTYPE
#define MAX_PTYPE	ptype_out_of_band
#endif

static ULONG pages_per_slot = XPS_DEF_PAGES_PER_CLI;
static ULONG slots_per_map = XPS_DEF_NUM_CLI;
static XNetMappedFile *client_maps = NULL;

static HANDLE xnet_connect_mutex = 0;
static HANDLE xnet_connect_map_h = 0;
static void  *xnet_connect_map = 0;

static HANDLE xnet_connect_event = 0;
static HANDLE xnet_response_event = 0;


static int currentProcessId;
static bool_t xnet_initialized = FALSE;
static bool_t xnet_shutdown = FALSE;
static bool_t xnet_mutex_ready = FALSE;

static Mutex xnet_mutex;

PortXNet::PortXNet(int size) : Port (size)
{
	init();
}

PortXNet::PortXNet(PortXNet* parent, UCHAR* send_buffer, int send_length, UCHAR* receive_buffer, int receive_length)
		: Port(0)
{
	init();
	TEXT buffer[64];
	ISC_get_host(buffer, sizeof(buffer));
	port_host = buffer;
	port_connection = buffer;
	port_version.Format("XNET Server (%s)", (const char*) port_host);
	
	if (parent) 
		{
		parent->addClient(this);
		port_next = parent->port_clients;
		parent->port_clients = parent->port_next = this;
		port_handle = parent->port_handle;
		port_server = parent->port_server;
		port_server_flags = parent->port_server_flags;
		port_connection = parent->port_connection;
		}

	port_buff_size = send_length;
	port_status_vector = NULL;

	xdrCreate(&port_send, send_buffer,	send_length, XDR_ENCODE);
	xdrCreate(&port_receive, receive_buffer, 0, XDR_DECODE);
}


PortXNet::~PortXNet(void)
{
	if (port_xcc) 
		{
		Sync sync(&xnet_mutex, "PortXNet::~PortXNet");
		sync.lock(Exclusive);
		cleanupComm(port_xcc);
		}
	
	delete [] waitVector;
	delete [] portVector;
}

void PortXNet::init(void)
{
	waitCount = 0;
	waitVector = NULL;
	portVector = NULL;
	port_type = port_xnet;
	port_state = state_pending;
	port_xcc = NULL;
}

bool_t PortXNet::getBytes(XDR* xdrs, SCHAR* buff, u_int count)
{
	SLONG bytecount = count;
	SLONG to_copy;

	PortXNet *port = (PortXNet*) xdrs->x_public;
	XNetConnection *xcc = port->port_xcc;
	XNetChannel *xch = xcc->xcc_recv_channel;
	XNetMappedFile *xpm = xcc->xcc_xpm;

	while (bytecount && !xnet_shutdown) 
		{
		if (xpm->xpm_flags & XPMF_SERVER_SHUTDOWN) 
			{
			if (!(xcc->xcc_flags & XCCF_SERVER_SHUTDOWN)) 
				{
				xcc->xcc_flags |= XCCF_SERVER_SHUTDOWN;
				XNET_ERROR(port, "connection lost: another side is dead", 
						   isc_lost_db_connection, 0);
				}
				
			return FALSE;
			}

		if (xdrs->x_handy >= bytecount)
			to_copy = bytecount;
		else
			to_copy = xdrs->x_handy;
		
		if (xdrs->x_handy)
			{
			if (to_copy == sizeof(SLONG))
				*((SLONG*)buff)	= *((SLONG*)xdrs->x_private);
			else
				memcpy(buff, xdrs->x_private,to_copy);

			xdrs->x_handy -= to_copy;
			xdrs->x_private += to_copy;
			}
		else if (!read(xdrs)) 
			return FALSE;

		if (to_copy) 
			{
			bytecount -= to_copy;
			buff += to_copy;
			}
		}

	if (xnet_shutdown)
		return FALSE;

	return TRUE;
}

bool_t PortXNet::putBytes(XDR* xdrs, const SCHAR* buff, u_int count)
{
#ifdef WIN_NT
	SLONG bytecount = count;
	SLONG to_copy;
	DWORD wait_result;

	PortXNet *port = (PortXNet*)xdrs->x_public;
	XNetConnection *xcc = port->port_xcc;
	XNetChannel *xch = xcc->xcc_send_channel;
	XNetMappedFile *xpm = xcc->xcc_xpm;

	while (bytecount && !xnet_shutdown) 
		{
		if (xpm->xpm_flags & XPMF_SERVER_SHUTDOWN) 
			{
			if (!(xcc->xcc_flags & XCCF_SERVER_SHUTDOWN)) 
				{
				xcc->xcc_flags |= XCCF_SERVER_SHUTDOWN;
				XNET_ERROR(port, "connection lost: another side is dead", 
						   isc_lost_db_connection, 0);
				}
			return FALSE;
			}
		
		if (xdrs->x_handy >= bytecount)
			to_copy = bytecount;
		else
			to_copy = xdrs->x_handy;

		if (xdrs->x_handy) 
			{
			if ((ULONG) xdrs->x_handy == xch->xch_size) 
				{
				while(!xnet_shutdown) 
					{
					wait_result = WaitForSingleObject(xcc->xcc_event_send_channel_empted,
													  XNET_SEND_WAIT_TIMEOUT);
					if (wait_result == WAIT_OBJECT_0)
						break;

					if (wait_result == WAIT_TIMEOUT) 
						{
						/* Check if another side is alive */
						
						if (WaitForSingleObject(xcc->xcc_proc_h,1) == WAIT_TIMEOUT)
							continue; /* Another side is alive */			

						/* Another side is dead or somthing bad has happaned*/
						
						//xnet_on_server_shutdown(port);
						XNET_ERROR(port, "connection lost: another side is dead", 
							        isc_lost_db_connection, 0);								

						return FALSE;
						}

					XNET_ERROR(port, "WaitForSingleObject()", isc_net_write_err, ERRNO);
					
					return FALSE; /* a non-timeout result is an error */
					}
				}

			if (to_copy == sizeof(SLONG))
				*((SLONG*)xdrs->x_private) = *((SLONG*)buff);
			else
				memcpy(xdrs->x_private, buff, to_copy);

			xdrs->x_handy -= to_copy;
			xdrs->x_private += to_copy;
			}
		else 
			if (!write(xdrs)) 
				{
				XNET_ERROR(port, "SetEvent()", isc_net_write_err, ERRNO);
				return FALSE;
				}

		if (to_copy) 
			{
			bytecount -= to_copy;
			buff += to_copy;
			}
		}

	if (xnet_shutdown)
		return FALSE;
#endif // WIN_NT

	return TRUE;
}

/**************************************
 *
 *      X N E T _ a n a l y z e
 *
 **************************************
 *
 * Functional description
 *  Client performs attempt to establish connection
 *  based on the set of protocols.
 *	If a connection is established, return a port block,
 *	otherwise return NULL.
 *
 **************************************/

PortXNet* PortXNet::analyze(TEXT* file_name, USHORT* file_length, ISC_STATUS *status_vector, const TEXT* node_name, const TEXT* user_string, bool uv_flag)
{
	P_CNCT *cnct;
	TEXT *p, user_id[128]; //, buffer[64];
	*file_length = strlen(file_name);

	/* We need to establish a connection to a remote server.  Allocate the necessary
	   blocks and get ready to go. */

	RDatabase *rdb = new RDatabase (NULL);
	Packet *packet = &rdb->rdb_packet;

	/* Pick up some user identification information */

	user_id[0] = CNCT_user;
	p = user_id + 2;
	ISC_get_user(p, 0, 0, 0, 0, 0, 0);
	user_id[1] = strlen(p);

	for (; *p; p++) 
		if (*p >= 'A' && *p <= 'Z')
			*p = *p - 'A' + 'a';

	*p++ = CNCT_host;
	p++;
	ISC_get_host(p, (USHORT) (user_id + sizeof(user_id) - p));
	p[-1] = strlen(p);

	for (; *p; p++) 
		if (*p >= 'A' && *p <= 'Z')
			*p = *p - 'A' + 'a';

	if (uv_flag) 
		{
		*p++ = CNCT_user_verification;
		*p++ = 0;
		}

	const SSHORT user_length = p - user_id;

	/* Establish connection to server */

	cnct = &packet->p_cnct;
	packet->p_operation = op_connect;
	cnct->p_cnct_operation = op_attach;
	cnct->p_cnct_cversion = CONNECT_VERSION2;
	cnct->p_cnct_client = ARCHITECTURE;
	cnct->p_cnct_file.cstr_length = *file_length;
	cnct->p_cnct_file.cstr_address = (UCHAR *) file_name;

	/* Note: prior to V3.1E a recievers could not in truth handle more
	   then 5 protocol descriptions; however, the interprocess server 
	   was created in 4.0 so this does not apply */

	cnct->p_cnct_user_id.cstr_length = user_length;
	cnct->p_cnct_user_id.cstr_address = (UCHAR*) user_id;

	static const p_cnct::p_cnct_repeat protocols_to_try1[] =
		{
		REMOTE_PROTOCOL(PROTOCOL_VERSION7, ptype_rpc, MAX_PTYPE, 1),
		REMOTE_PROTOCOL(PROTOCOL_VERSION8, ptype_rpc, MAX_PTYPE, 2),
		REMOTE_PROTOCOL(PROTOCOL_VERSION10, ptype_rpc, MAX_PTYPE, 3),
		REMOTE_PROTOCOL(PROTOCOL_VERSION11, ptype_rpc, MAX_PTYPE, 4)
		};
		
	cnct->p_cnct_count = FB_NELEM(protocols_to_try1);

	for (size_t i = 0; i < cnct->p_cnct_count; i++)
		cnct->p_cnct_versions[i] = protocols_to_try1[i];

	/* If we can't talk to a server, punt.  Let somebody else generate
	   an error. */

	//PortXNet *port = XNET_connect(node_name, packet, status_vector, FALSE);
	PortXNet *port = connect(node_name, packet, status_vector, FALSE);
	
	if (!port)
		{
		delete rdb;
		return NULL;
		}

	/* Get response packet from server. */

	rdb->rdb_port = port;
	port->port_context = rdb;
	port->receive(packet);

	if (packet->p_operation == op_reject && !uv_flag) 
		{
		//disconnect(port);
		port->disconnect();
		packet->p_operation = op_connect;
		cnct->p_cnct_operation = op_attach;
		cnct->p_cnct_cversion = CONNECT_VERSION2;
		cnct->p_cnct_client = ARCHITECTURE;
		cnct->p_cnct_file.cstr_length = *file_length;
		cnct->p_cnct_file.cstr_address = (UCHAR *) file_name;

		/* try again with next set of known protocols */

		cnct->p_cnct_user_id.cstr_length = user_length;
		cnct->p_cnct_user_id.cstr_address = (UCHAR*) user_id;

		static const p_cnct::p_cnct_repeat protocols_to_try2[] =
			{
			REMOTE_PROTOCOL(PROTOCOL_VERSION4, ptype_rpc, ptype_batch_send, 1),
			REMOTE_PROTOCOL(PROTOCOL_VERSION6, ptype_rpc, ptype_batch_send, 2),
			};
			
		cnct->p_cnct_count = FB_NELEM(protocols_to_try2);

		for (size_t i = 0; i < cnct->p_cnct_count; i++) 
			cnct->p_cnct_versions[i] = protocols_to_try2[i];

		//if (!(port = XNET_connect(node_name, packet, status_vector, FALSE))) 
		if (!(port = connect(node_name, packet, status_vector, FALSE))) 
			{
			delete rdb;
			return NULL;
			}

		/* Get response packet from server. */

		rdb->rdb_port = port;
		port->port_context = rdb;
		port->receive(packet);
		}

	if (packet->p_operation == op_reject && !uv_flag) 
		{
		//disconnect(port);
		port->disconnect();
		packet->p_operation = op_connect;
		cnct->p_cnct_operation = op_attach;
		cnct->p_cnct_cversion = CONNECT_VERSION2;
		cnct->p_cnct_client = ARCHITECTURE;
		cnct->p_cnct_file.cstr_length = *file_length;
		cnct->p_cnct_file.cstr_address = (UCHAR *) file_name;

		/* try again with next set of known protocols */

		cnct->p_cnct_user_id.cstr_length = user_length;
		cnct->p_cnct_user_id.cstr_address = (UCHAR*) user_id;

		static const p_cnct::p_cnct_repeat protocols_to_try3[] =
			{
			REMOTE_PROTOCOL(PROTOCOL_VERSION3, ptype_rpc, ptype_batch_send, 1)
			};
			
		cnct->p_cnct_count = FB_NELEM(protocols_to_try3);

		for (size_t i = 0; i < cnct->p_cnct_count; i++) 
			cnct->p_cnct_versions[i] = protocols_to_try3[i];

		//if (!(port = XNET_connect(node_name, packet, status_vector, FALSE))) 
		if (!(port =connect(node_name, packet, status_vector, FALSE))) 
			{
			delete rdb;
			return NULL;
			}

		/* Get response packet from server. */

		rdb->rdb_port = port;
		port->port_context = rdb;
		port->receive(packet);
		}

	if (packet->p_operation != op_accept) 
		{
		*status_vector++ = isc_arg_gds;
		*status_vector++ = isc_connect_reject;
		*status_vector++ = isc_arg_end;
		//disconnect(port);
		port->disconnect();
		
		return NULL;
		}

	port->port_protocol = packet->p_acpt.p_acpt_version;

	/* once we've decided on a protocol, concatenate the version 
	   string to reflect it...  */

	port->port_version.Format("%s/P%d", (const char*) port->port_version, port->port_protocol);

	if (packet->p_acpt.p_acpt_architecture == ARCHITECTURE)
		port->port_flags |= PORT_symmetric;

	if (packet->p_acpt.p_acpt_type == ptype_rpc)
		port->port_flags |= PORT_rpc;

	if (packet->p_acpt.p_acpt_type != ptype_out_of_band)
		port->port_flags |= PORT_no_oob;

	return port;
}

/**************************************
 *
 *      X N E T _ c o n n e c t
 *
 **************************************
 *
 * Functional description
 *	Establish half of a communication link.
 *
 **************************************/

PortXNet* PortXNet::connect(const TEXT* name, Packet* packet, ISC_STATUS *status_vector, int flag)
{

	if (xnet_shutdown)
		return NULL;

	// set up for unavailable server
	status_vector[0] = isc_arg_gds;
	status_vector[1] = isc_unavailable;
	status_vector[2] = isc_arg_end;

	/*
	LONG test_xnet_initialized;

	InterlockedExchange((LPLONG) &test_xnet_initialized, xnet_initialized);
	if (!test_xnet_initialized) {
		InterlockedExchange((LPLONG) &xnet_initialized, TRUE);
		THD_mutex_init(&xnet_mutex);
		currentProcessId = GetCurrentProcessId();
		gds__register_cleanup((FPTR_VOID_PTR) exit_handler, NULL);
		InterlockedExchange((LPLONG) &xnet_mutex_ready, (LONG) TRUE);
	}
	else {
		while (!xnet_mutex_ready)
			Sleep(10);
	}
	*/
	
	PortXNet *port = NULL;
	XNetConnection *xcc = NULL;
	XNetMappedFile *xpm = NULL;
	XPS xps = NULL;

	XNetResponse response;
	ULONG map_num, slot_num;
	time_t timestamp;
	UCHAR *start_ptr;
	ULONG avail;

	if (!xnet_initialized) 
		{
		xnet_initialized = TRUE;
		//THD_mutex_init(&xnet_mutex);
		currentProcessId = getpid();
		gds__register_cleanup(exitHandler, NULL);
		}

	Sync sync(&xnet_mutex, "PortXNet::connect");
	sync.lock(Exclusive);
	
	if (!connectInit()) 
		return NULL;

	// waiting for xnet connect lock to release
	
#ifdef WIN_NT
	if (WaitForSingleObject(xnet_connect_mutex, XNET_CONNECT_TIMEOUT) != WAIT_OBJECT_0) 
		{
		//xnet_connect_fini();
		connectFini();
		return NULL;
		}

	/* writing connect request */

	// mark connect area with XNET_INVALID_MAP_NUM to
	// detect server faults on response
	
	((XNetResponse*) xnet_connect_map)->map_num = XNET_INVALID_MAP_NUM;
	((XNetResponse*) xnet_connect_map)->proc_id = currentProcessId; 
	SetEvent(xnet_connect_event);

	// waiting for server response
	
	if (WaitForSingleObject(xnet_response_event, XNET_CONNECT_TIMEOUT) != WAIT_OBJECT_0) 
		{
		ReleaseMutex(xnet_connect_mutex);
		//xnet_connect_fini();
		connectFini();
				
		return NULL;
		}

	memcpy(&response, xnet_connect_map, sizeof(XNetResponse));
	ReleaseMutex(xnet_connect_mutex);
	//xnet_connect_fini();
	connectFini();

	//XNET_UNLOCK;

	if (response.map_num == XNET_INVALID_MAP_NUM) 
		{
		XNET_LOG_ERROR("server failed to response on connect request");
		return NULL;
		}

	pages_per_slot = response.pages_per_slot;
	slots_per_map = response.slots_per_map;
	map_num = response.map_num;
	slot_num = response.slot_num;
	timestamp = response.timestamp;

	try 
		{
		//XNET_LOCK;
		Sync sync(&xnet_mutex, "PortXNet::connect(2)");
		sync.lock(Exclusive);

		// see if area is already mapped for this client
		
		for (xpm = client_maps; xpm; xpm = xpm->xpm_next) 
			if (xpm->xpm_number == map_num &&
					xpm->xpm_timestamp == timestamp &&
					!(xpm->xpm_flags & XPMF_SERVER_SHUTDOWN))
				break;

		if (!xpm) 
			{
			// Area hasn't been mapped. Open new file mapping.
			
			/***
			sprintf(name_buffer, XNET_MAPPED_FILE_NAME, XNET_PREFIX, map_num, (ULONG) timestamp);
			file_handle = OpenFileMapping(FILE_MAP_WRITE, FALSE, name_buffer);
			
			if (!file_handle) 
				{
				//XNET_UNLOCK;
				error("OpenFileMapping");
				}

			mapped_address = MapViewOfFile(file_handle, FILE_MAP_WRITE, 0L, 0L,
										   XPS_MAPPED_SIZE(slots_per_map, pages_per_slot));
			if (!mapped_address) 
				{
				//XNET_UNLOCK;
				error("MapViewOfFile");
				}
			***/
			
			xpm = new XNetMappedFile(map_num, timestamp, slots_per_map, pages_per_slot);
			xpm->mapFile(false);
			xpm->xpm_next = client_maps;
			client_maps = xpm;
			/***
			xpm->xpm_count = 0;
			xpm->xpm_number = map_num;
			xpm->xpm_handle = file_handle;
			xpm->xpm_address = mapped_address;
			xpm->xpm_timestamp = timestamp;
			xpm->xpm_flags = 0;
			***/
			}

		//XNET_UNLOCK;

		xcc = new XNetConnection;
		xcc->xcc_map_handle = xpm->xpm_handle;
		xcc->xcc_mapped_addr =(UCHAR *) xpm->xpm_address + XPS_SLOT_OFFSET(pages_per_slot, slot_num);
		xcc->xcc_map_num = map_num;
		xcc->xcc_slot = slot_num;
		xcc->xcc_xpm = xpm;
		xcc->xcc_flags = 0;
		xcc->xcc_proc_h = 0;
		xcc->open(false, timestamp);
		xps = (XPS) xcc->xcc_mapped_addr;

		// only speak if server has correct protocol
		
		if (xps->xps_server_protocol != XPI_SERVER_PROTOCOL_VERSION)
			error("wrong protocol version");

		xps->xps_client_protocol = XPI_CLIENT_PROTOCOL_VERSION;

		// open server process handle to watch server health during communication session
		
		xcc->xcc_proc_h = OpenProcess(SYNCHRONIZE, 0, xps->xps_server_proc_id);
		
		if (!xcc->xcc_proc_h) 
			error("OpenProcess");

		xpm->xpm_count++;
		xcc->open(false, timestamp);

		/***
		sprintf(name_buffer, XNET_E_C2S_DATA_CHAN_FILLED,
				XNET_PREFIX, map_num, slot_num, (ULONG) timestamp);
		xcc->xcc_event_send_channel_filled = OpenEvent(EVENT_ALL_ACCESS, FALSE, name_buffer);
			
		if (!xcc->xcc_event_send_channel_filled) 
			error("xxx");

		sprintf(name_buffer, XNET_E_C2S_DATA_CHAN_EMPTED,
				XNET_PREFIX, map_num, slot_num, (ULONG) timestamp);
		xcc->xcc_event_send_channel_empted = OpenEvent(EVENT_ALL_ACCESS, FALSE, name_buffer);
			
		if (!xcc->xcc_event_send_channel_empted)
			error("xxx");

		sprintf(name_buffer, XNET_E_S2C_DATA_CHAN_FILLED,
				XNET_PREFIX, map_num, slot_num, (ULONG) timestamp);
		xcc->xcc_event_recv_channel_filled = OpenEvent(EVENT_ALL_ACCESS, FALSE, name_buffer);
				
		if (!xcc->xcc_event_recv_channel_filled)
			error("xxx");

		sprintf(name_buffer, XNET_E_S2C_DATA_CHAN_EMPTED,
				XNET_PREFIX, map_num, slot_num, (ULONG) timestamp);
		xcc->xcc_event_recv_channel_empted = OpenEvent(EVENT_ALL_ACCESS, FALSE, name_buffer);
				
		if (!xcc->xcc_event_recv_channel_empted) 
			error("xxx");
		***/

		/* added this here from the server side as this part is called by the client 
		   and the server address need not be valid for the client -smistry 10/29/98 */
		   
		xcc->xcc_recv_channel = &xps->xps_channels[XPS_CHANNEL_S2C_DATA];
		xcc->xcc_send_channel = &xps->xps_channels[XPS_CHANNEL_C2S_DATA];

		/* we also need to add client side flags or channel pointer as they 
		   differ from the server side */

		avail =	(ULONG) (XPS_USEFUL_SPACE(pages_per_slot) - (XNET_EVENT_SPACE * 2)) / 2;
		start_ptr = (UCHAR*) xps + (sizeof(struct xps) + (XNET_EVENT_SPACE * 2));

		/* send channel */
		
		xps->xps_channels[XPS_CHANNEL_C2S_DATA].xch_client_ptr = start_ptr;
		
		/* receive channel */
		
		xps->xps_channels[XPS_CHANNEL_S2C_DATA].xch_client_ptr = (start_ptr + avail);
		}
	catch (...) 
		{
		delete xpm;
		delete xcc;

		return NULL;
		}

	port = new PortXNet(0, xcc->xcc_send_channel->xch_client_ptr,
						xcc->xcc_send_channel->xch_size,
						xcc->xcc_recv_channel->xch_client_ptr,
						xcc->xcc_recv_channel->xch_size);
						   
	status_vector[1] = FB_SUCCESS;
	port->port_status_vector = status_vector;
	port->port_xcc = xcc;
	gds__register_cleanup(exitHandler, port);
	port->sendPacket(packet);

#endif // WIN_NT
		
	return port;
}

void PortXNet::disconnect(void)
{
	Port *parent, **ptr;

	/* If this is a sub-port, unlink it from it's parent */

	if ((parent = port_parent) != NULL) 
		{
		if (port_async) 
			{
			//disconnect(port_async);
			port_async->disconnect();
			port_async = NULL;
			}

		for (ptr = &parent->port_clients; *ptr; ptr = &(*ptr)->port_next)
			if (*ptr == this) 
				{
				*ptr = port_next;
				
				if (ptr == &parent->port_clients)
					parent->port_next = *ptr;
					
				break;
				}
		}
	else if (port_async) 
		{
		/* If we're MULTI_THREAD then we cannot free the port because another
		 * thread might be using it.  If we're SUPERSERVER we must free the
		 * port to avoid a memory leak.  What we really need to know is if we
		 * have multi-threaded events, but this is transport specific.
		 * -smistry 10/29/98 */
		 
#if (defined (MULTI_THREAD) && !defined (SUPERSERVER))
		port_async->port_flags |= PORT_disconnect;
#else
		//disconnect(port->port_async);
		port_async->disconnect();
		port_async = NULL;
#endif
		}

	gds__unregister_cleanup(exitHandler, this);
	delete this;	
	//xnet_cleanup_port(this);
}

int PortXNet::xdrCreate(XDR* xdrs, UCHAR* buffer, int length, enum xdr_op x_op)
{
	xdrs->x_public = (caddr_t) this;
	xdrs->x_private = (SCHAR *) buffer;
	xdrs->x_base = xdrs->x_private;
	xdrs->x_handy = length;
	xdrs->x_ops = &xnet_ops;
	xdrs->x_op = x_op;

	return TRUE;
}

void PortXNet::exitHandler(void* arg)
{
	xnet_shutdown = TRUE;
	PortXNet *mainPort = (PortXNet*) arg;
	
	if (mainPort) 
		//disconnect(main_port);
		mainPort->disconnect();
	else 
		//xnet_release_all();
		releaseAll();
}

void PortXNet::cleanupComm(XNetConnection* xcc)
{
	//XNetMappedFile * pxpm;
	XNetMappedFile *xpm = xcc->xcc_xpm;
	delete xcc;
	xcc = NULL;

	// if this was the last area for this map, unmap it
	
	if (xpm) 
		{
		xpm->xpm_count--;

		if (!xpm->xpm_count && client_maps)
			{
			//UnmapViewOfFile(xpm->xpm_address);
			//CloseHandle(xpm->xpm_handle);

			// find xpm in chain and release
			
			for (XNetMappedFile **pxpm = &client_maps; *pxpm; pxpm = &(*pxpm)->xpm_next) 
				if (*pxpm == xpm)
					{
					*pxpm = xpm->xpm_next;
					break;
					}

			delete xpm;
			}
		}
}

void PortXNet::releaseAll(void)
{
	if (!xnet_initialized)
		return;

#ifndef SUPERCLIENT
	//xnet_connect_fini();
	connectFini();
#endif

	Sync sync(&xnet_mutex, "PortXNet::releaseAll");
	sync.lock(Exclusive);

	/* release all map stuf left not released by broken ports */
	
	for (XNetMappedFile *xpm; xpm = client_maps;)
		{
		client_maps = xpm->xpm_next;
		//UnmapViewOfFile(xpm->xpm_address);
		//CloseHandle(xpm->xpm_handle);
		delete xpm;
		}

	xnet_initialized = FALSE;
}

/**************************************
 *
 *  x n e t _ c o n n e c t _ f i n i
 *
 **************************************
 *
 * Functional description
 *  Release resources allocated in
 *  xnet_connect_init() / xnet_srv_init()
 *
 **************************************/

void PortXNet::connectFini(void)
{
	XNetConnection::closeMutex(&xnet_connect_mutex);
	XNetConnection::closeEvent(&xnet_connect_event);
	XNetConnection::closeEvent(&xnet_response_event);
	XNetMappedFile::unmapFile(&xnet_connect_map);
	XNetMappedFile::closeFile(&xnet_connect_map_h);
}

int PortXNet::accept(p_cnct* cnct)
{
	return TRUE;
}

Port* PortXNet::receive(Packet* packet)
{
	if (!(port_server_flags & SRVR_multi_client)) 
		{
		if (!xdr_protocol(&port_receive, packet))
			//packet->p_operation = op_exit;
			return NULL;
			
		return this;
		}

	currentProcessId = getpid();
	XNetResponse *presponse = (XNetResponse*)xnet_connect_map;
	
#ifdef WIN_NT
	while (!xnet_shutdown)
		{
		Port *child;
		int count = 1;
		
		for (child = port_clients; child; child = child->port_next)
			++count;
		
		if (count > waitCount)
			{
			delete [] waitVector;
			delete [] portVector;
			waitCount += 32;
			waitVector = new HANDLE[waitCount];
			portVector = new PortXNet*[waitCount];
			}
		
		count = 0;
		portVector[count] = this;
		waitVector[count++] = xnet_connect_event;
		
		for (child = port_clients; child; child = child->port_next)
			{
			PortXNet *port = (PortXNet*) child;
			portVector[count] = port;
			waitVector[count++] = port->port_xcc->xcc_event_recv_channel_filled;
			}
			
		//DWORD wait_res = WaitForSingleObject(xnet_connect_event,INFINITE);
		DWORD wait_res = WaitForMultipleObjects(count, waitVector, false, INFINITE);
		int index = wait_res - WAIT_OBJECT_0;
		PortXNet *port = portVector[index];
		
		if (xnet_shutdown)
			break;

		ULONG client_pid = presponse->proc_id;
		
		if (client_pid == currentProcessId)
			continue; // dummy xnet_connect_event fire -  no connect request
		
		if (port == this)
			{	
			presponse->slots_per_map = slots_per_map;
			presponse->pages_per_slot = pages_per_slot;
			presponse->timestamp = time_t(0);

			time_t timestamp = time(NULL);
			Sync sync(&xnet_mutex, "PortXNet::server");
			sync.lock(Exclusive);
			ULONG map_num, slot_num;
			XNetMappedFile *xpm = getFreeSlot(&map_num, &slot_num, &timestamp);
			sync.unlock();
			PortXNet *port = NULL;
			
			// pack combined mapped area and number
			
			if (xpm) 
				{
				presponse->proc_id = currentProcessId;
				presponse->map_num = map_num;
				presponse->slot_num = slot_num;
				presponse->timestamp = timestamp;
				ISC_STATUS *status_vector = new ISC_STATUS[ISC_STATUS_LENGTH];
				port = getServerPort(this, client_pid, xpm, map_num, slot_num, timestamp, status_vector);
				}
			else 
				XNET_LOG_ERROR("xnet_get_free_slot() failed");
						
			if (!xpm || !port) 
				XNET_LOG_ERROR("failed to allocate server port for communication");

			SetEvent(xnet_response_event);
			}
			
		/* We've got data -- lap it up and use it */

		if (!xdr_protocol(&port->port_receive, packet))
			packet->p_operation = op_exit;
		
		return port;
		}
#endif

	return NULL;
}

XDR_INT PortXNet::sendPacket(Packet* packet)
{
	if (!xdr_protocol(&port_send, packet))
		return FALSE;

	//if (xnet_write(&port_send))
	if (write(&port_send))
		return TRUE;
		
	XNET_ERROR(this, "SetEvent()", isc_net_write_err, ERRNO);
	
	return FALSE;
}

XDR_INT PortXNet::sendPartial(Packet* packet)
{
	return xdr_protocol(&port_send, packet);
}

/**************************************
 *
 *	a u x _ c o n n e c t
 *
 **************************************
 *
 * Functional description
 *	Try to establish an alternative connection for handling events.
 *  Somebody has already done a successfull connect request.
 *  This uses the existing xcc for the parent port to more
 *  or less duplicate a new xcc for the new aux port pointing
 *  to the event stuff in the map.
 *
 **************************************/

Port* PortXNet::connect(Packet* packet, void(* secondaryConnection)(Port*))
{
	if (port_server_flags)
		{
		port_flags |= PORT_async;
		return this;
		}

 	PortXNet *new_port = NULL;
	XNetConnection *parent_xcc = NULL;
	XNetConnection *xcc = NULL;
	XPS xps = NULL;
	XNetMappedFile *xpm = NULL;

	try 
		{
		// make a new xcc
		
		parent_xcc = port_xcc;
		xps = (XPS) parent_xcc->xcc_mapped_addr;

		xcc = new XNetConnection(parent_xcc);
		/***
		xpm = xcc->xcc_xpm = parent_xcc->xcc_xpm;
		xcc->xcc_map_num = parent_xcc->xcc_map_num;
		xcc->xcc_slot = parent_xcc->xcc_slot;
		xcc->xcc_proc_h = parent_xcc->xcc_proc_h;
		xcc->xcc_flags = 0;
		xcc->xcc_map_handle = parent_xcc->xcc_map_handle;
		xcc->xcc_mapped_addr = parent_xcc->xcc_mapped_addr;
		xcc->xcc_xpm->xpm_count++;
		***/
		xcc->open(true, xpm->xpm_timestamp);
		/***
		sprintf(name_buffer, XNET_E_C2S_EVNT_CHAN_FILLED,
				XNET_PREFIX, xcc->xcc_map_num, xcc->xcc_slot,
				(ULONG) xpm->xpm_timestamp);
		xcc->xcc_event_send_channel_filled = OpenEvent(EVENT_ALL_ACCESS, FALSE, name_buffer);
			
		if (!xcc->xcc_event_send_channel_filled) 
			error("xxx");

		sprintf(name_buffer, XNET_E_C2S_EVNT_CHAN_EMPTED,
				XNET_PREFIX, xcc->xcc_map_num, xcc->xcc_slot,
				(ULONG) xpm->xpm_timestamp);
		xcc->xcc_event_send_channel_empted = OpenEvent(EVENT_ALL_ACCESS, FALSE, name_buffer);
			
		if (!xcc->xcc_event_send_channel_empted) 
			error("xxx");

		sprintf(name_buffer, XNET_E_S2C_EVNT_CHAN_FILLED,
				XNET_PREFIX, xcc->xcc_map_num, xcc->xcc_slot,
				(ULONG) xpm->xpm_timestamp);
		xcc->xcc_event_recv_channel_filled = OpenEvent(EVENT_ALL_ACCESS, FALSE, name_buffer);
			
		if (!xcc->xcc_event_recv_channel_filled)
			error("xxx");

		sprintf(name_buffer, XNET_E_S2C_EVNT_CHAN_EMPTED,
				XNET_PREFIX, xcc->xcc_map_num, xcc->xcc_slot,
				(ULONG) xpm->xpm_timestamp);
		xcc->xcc_event_recv_channel_empted = OpenEvent(EVENT_ALL_ACCESS, FALSE, name_buffer);
			
		if (!xcc->xcc_event_recv_channel_empted) 
			error("xxx");
		***/
		
		// send events channel
		
		xps->xps_channels[XPS_CHANNEL_C2S_EVENTS].xch_client_ptr =
			((UCHAR *) xpm->xpm_address + sizeof(struct xps));

		// receive events channel
		
		xps->xps_channels[XPS_CHANNEL_S2C_EVENTS].xch_client_ptr =
			((UCHAR *) xpm->xpm_address + sizeof(struct xps) + (XNET_EVENT_SPACE));

		xcc->xcc_send_channel = &xps->xps_channels[XPS_CHANNEL_C2S_EVENTS];		
		xcc->xcc_recv_channel = &xps->xps_channels[XPS_CHANNEL_S2C_EVENTS];

		// alloc new port and link xcc to it
		
		new_port = new PortXNet(NULL,
								xcc->xcc_send_channel->xch_client_ptr,
								xcc->xcc_send_channel->xch_size,
								xcc->xcc_recv_channel->xch_client_ptr,
								xcc->xcc_recv_channel->xch_size);
								
		port_async = new_port;
		new_port->port_flags = port_flags & PORT_no_oob;
		new_port->port_flags |= PORT_async;
		new_port->port_xcc = xcc;
		gds__register_cleanup(exitHandler, new_port);

		return new_port;
		}
	catch (...) 
		{
		XNET_LOG_ERROR("aux_connect() failed");
		delete xcc;

		return NULL;
		}
}
/**************************************
 *
 *	a u x _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *  A remote interface has requested the server to
 *  prepare an auxiliary connection.   This is done
 *  by allocating a new port and comm (xcc) structure,
 *  using the event stuff in the map rather than the
 *  normal database channels.
 *
 **************************************/

Port* PortXNet::auxRequest(Packet* packet)
{
 	PortXNet* new_port = NULL;
	XNetConnection *parent_xcc = NULL;
	XNetConnection *xcc = NULL;
	//TEXT name_buffer[128];
	XPS xps = NULL;
	XNetMappedFile *xpm = NULL;

	try 
		{
		// make a new xcc
		parent_xcc = port_xcc;
		xps = (XPS) parent_xcc->xcc_mapped_addr;

		//xcc = (XCC) ALLR_alloc(sizeof(struct xcc));
		xcc = new XNetConnection(parent_xcc);
		xpm = parent_xcc->xcc_xpm;
		/***
		xpm = xcc->xcc_xpm = parent_xcc->xcc_xpm;
		xcc->xcc_map_num = parent_xcc->xcc_map_num;
		xcc->xcc_slot = parent_xcc->xcc_slot;
		xcc->xcc_proc_h = parent_xcc->xcc_proc_h;
		xcc->xcc_flags = 0;
		xcc->xcc_map_handle = parent_xcc->xcc_map_handle;
		xcc->xcc_mapped_addr = parent_xcc->xcc_mapped_addr;
		xcc->xcc_xpm->xpm_count++;
		***/
#ifdef WIN_NT

		xcc->create(true, xpm->xpm_timestamp);
		/***
		sprintf(name_buffer, XNET_E_C2S_EVNT_CHAN_FILLED,
				XNET_PREFIX, xcc->xcc_map_num, xcc->xcc_slot,
				(ULONG) xpm->xpm_timestamp);
		xcc->xcc_event_recv_channel_filled =
			CreateEvent(ISC_get_security_desc(), FALSE, FALSE, name_buffer);
			
		if (!xcc->xcc_event_recv_channel_filled ||
			(xcc->xcc_event_recv_channel_filled && ERRNO == ERROR_ALREADY_EXISTS))
			error("xxx");

		sprintf(name_buffer, XNET_E_C2S_EVNT_CHAN_EMPTED,
				XNET_PREFIX, xcc->xcc_map_num, xcc->xcc_slot,
				(ULONG) xpm->xpm_timestamp);
		xcc->xcc_event_recv_channel_empted =
			CreateEvent(ISC_get_security_desc(), TRUE, TRUE, name_buffer);
			
		if (!xcc->xcc_event_recv_channel_empted ||
			(xcc->xcc_event_recv_channel_empted && ERRNO == ERROR_ALREADY_EXISTS))
			error("xxx");

		sprintf(name_buffer, XNET_E_S2C_EVNT_CHAN_FILLED,
				XNET_PREFIX, xcc->xcc_map_num, xcc->xcc_slot,
				(ULONG) xpm->xpm_timestamp);
		xcc->xcc_event_send_channel_filled =
			CreateEvent(ISC_get_security_desc(), FALSE, FALSE, name_buffer);
			
		if (!xcc->xcc_event_send_channel_filled ||
			(xcc->xcc_event_send_channel_filled && ERRNO == ERROR_ALREADY_EXISTS)) 
			error("xxx");

		sprintf(name_buffer, XNET_E_S2C_EVNT_CHAN_EMPTED,
				XNET_PREFIX, xcc->xcc_map_num, xcc->xcc_slot,
				(ULONG) xpm->xpm_timestamp);
		xcc->xcc_event_send_channel_empted =
			CreateEvent(ISC_get_security_desc(), TRUE, TRUE, name_buffer);
			
		if (!xcc->xcc_event_send_channel_empted ||
			(xcc->xcc_event_send_channel_empted && ERRNO == ERROR_ALREADY_EXISTS)) 
			error("xxx");
		***/

#else
	// STUB : This should be event_init() calls ?
#endif

		// send events channel
		
		xps->xps_channels[XPS_CHANNEL_S2C_EVENTS].xch_client_ptr =
			((UCHAR *) xpm->xpm_address + sizeof(struct xps) + (XNET_EVENT_SPACE));

		// receive events channel
		
		xps->xps_channels[XPS_CHANNEL_C2S_EVENTS].xch_client_ptr =
			((UCHAR *) xpm->xpm_address + sizeof(struct xps));

		xcc->xcc_send_channel = &xps->xps_channels[XPS_CHANNEL_S2C_EVENTS];		
		xcc->xcc_recv_channel = &xps->xps_channels[XPS_CHANNEL_C2S_EVENTS];

		// alloc new port and link xcc to it
		
		new_port = new PortXNet(NULL,
								xcc->xcc_send_channel->xch_client_ptr,
								xcc->xcc_send_channel->xch_size,
								xcc->xcc_recv_channel->xch_client_ptr,
								xcc->xcc_recv_channel->xch_size);
								
		port_async = new_port;
		new_port->port_xcc = xcc;
		new_port->port_flags = port_flags & PORT_no_oob;
		new_port->port_server_flags = port_server_flags;

		return new_port;
		}
	catch (...) 
		{
		XNET_LOG_ERROR("aux_request() failed");
		delete xcc;

		return NULL;
		}
}

int PortXNet::error(TEXT* function, ISC_STATUS operation, int status, int source_line_num)
{
	logError(source_line_num, function, status);

	if (status)
		genError(operation, SYS_ERR, status, 0);
	else
		genError(operation, 0);
		
	return 0;
}

bool_t PortXNet::write(XDR* xdrs)
{
	PortXNet *port = (PortXNet*) xdrs->x_public;
	XNetConnection *xcc = port->port_xcc;
	XNetChannel *xch = xcc->xcc_send_channel;
	xch->xch_length = xdrs->x_private - xdrs->x_base;
	
#ifdef WIN_NT
	if (SetEvent(xcc->xcc_event_send_channel_filled)) 
		{
		port->port_misc1 = (port->port_misc1 + 1) % MAX_SEQUENCE;
		xdrs->x_private = xdrs->x_base;
		xdrs->x_handy = xch->xch_size;

		return TRUE;
		}
#endif //WIN_NT

	return FALSE;
}

void PortXNet::logError(int source_line_num, const TEXT* err_msg, int error_code)
{
	char err_msg_buff[256];

	if (error_code)
		sprintf(err_msg_buff, "XNET error (xnet:%d)  %s  Win32 error = %"ULONGFORMAT"\n",
			source_line_num,err_msg,error_code);
	else
		sprintf(err_msg_buff, "XNET error (xnet:%d)  %s\n",
			source_line_num,err_msg);

	gds__log(err_msg_buff);
}

void PortXNet::genError(ISC_STATUS status, ...)
{
	ISC_STATUS *status_vector = NULL;
	port_flags |= PORT_broken;
	port_state = state_broken;

	if (port_context != NULL)
		status_vector = port_context->rdb_status_vector;
		
	if (status_vector == NULL)
		status_vector = port_status_vector;
		
	if (status_vector != NULL) 
		{
		STUFF_STATUS(status_vector, status)
		REMOTE_save_status_strings(status_vector);
		}
}

PortXNet* PortXNet::reconnect(int client_pid, ISC_STATUS *status_vector)
{
	PortXNet *port = NULL;
	XNetMappedFile *xpm = NULL;
	slots_per_map = 1;
	pages_per_slot = XPS_MAX_PAGES_PER_CLI;
	xnet_response_event = 0;

	// currentProcessId used as map number
	
	currentProcessId = getpid();

#ifdef WIN_NT

	try 
		{
		xnet_response_event = XNetConnection::openEvent(XNET_E_RESPONSE_EVENT);

		//xpm = xnet_make_xpm(currentProcessId, time_t(0));
		xpm = new XNetMappedFile(currentProcessId, time_t(0), slots_per_map, pages_per_slot);

		//port = xnet_get_srv_port(client_pid, xpm, currentProcessId, 0, 0, status_vector);
		port = getServerPort(NULL, client_pid, xpm, currentProcessId, 0, 0, status_vector);
		
		if (!port) 
			error("getServerPort");

		//make signal for client
		
		SetEvent(xnet_response_event);
		
		if (xnet_response_event) 
			CloseHandle(xnet_response_event);
		}
	catch (...) 
		{
		XNET_LOG_ERROR("Unable to initialize child process");
		status_vector[1] = isc_unavailable;

		if (port) 
			{
			//xnet_cleanup_port(port);
			delete port;
			port = NULL;
			}

		if (xnet_response_event) 
			{
			SetEvent(xnet_response_event); // To prevent client blocking
			CloseHandle(xnet_response_event);
			}
		}

#endif

	return port;
}

PortXNet* PortXNet::getServerPort(PortXNet *parent, int client_pid, XNetMappedFile *xpm, ULONG map_num, ULONG slot_num, time_t timestamp, ISC_STATUS *status_vector)
{
	PortXNet *port = NULL;
	XPS xps;
	ULONG avail;

	// allocate a communications control structure and fill it in
	
	XNetConnection *xcc = new XNetConnection;
	xcc->create(false, timestamp);
	
#ifdef WIN_NT

	try 
		{
		UCHAR *p = (UCHAR *) xpm->xpm_address + XPS_SLOT_OFFSET(pages_per_slot, slot_num);
		memset(p, 0, XPS_MAPPED_PER_CLI(pages_per_slot));
		xcc->xcc_mapped_addr = p;
		xcc->xcc_xpm = xpm;
		xcc->xcc_slot = slot_num;

		// Open client process handle to watch clients health during communication session
		
		xcc->xcc_proc_h = OpenProcess(SYNCHRONIZE, 0, client_pid);
		
		if (!xcc->xcc_proc_h) 
			error("OpenProcess");

		xcc->xcc_map_num = map_num;
		xps = (XPS) xcc->xcc_mapped_addr;
		xps->xps_client_proc_id = client_pid;
		xps->xps_server_proc_id = getpid();

		// make sure client knows what this server speaks

		xps->xps_server_protocol = XPI_SERVER_PROTOCOL_VERSION;
		xps->xps_client_protocol = 0L;

		xcc->create(false, timestamp);
		/***
		sprintf(name_buffer, XNET_E_C2S_DATA_CHAN_FILLED,
				XNET_PREFIX, map_num, slot_num, (ULONG) timestamp);
		xcc->xcc_event_recv_channel_filled =
			CreateEvent(ISC_get_security_desc(), FALSE, FALSE, name_buffer);
			
		if (!xcc->xcc_event_recv_channel_filled) 
			error("xxx");

		sprintf(name_buffer, XNET_E_C2S_DATA_CHAN_EMPTED,
				XNET_PREFIX, map_num, slot_num, (ULONG) timestamp);
		xcc->xcc_event_recv_channel_empted =
			CreateEvent(ISC_get_security_desc(), FALSE, FALSE, name_buffer);
			
		if (!xcc->xcc_event_recv_channel_empted) 
			error("xxx");

		sprintf(name_buffer, XNET_E_S2C_DATA_CHAN_FILLED,
				XNET_PREFIX, map_num, slot_num, (ULONG) timestamp);
		xcc->xcc_event_send_channel_filled =
			CreateEvent(ISC_get_security_desc(), FALSE, FALSE, name_buffer);
			
		if (!xcc->xcc_event_send_channel_filled)
			error("xxx");

		sprintf(name_buffer, XNET_E_S2C_DATA_CHAN_EMPTED,
				XNET_PREFIX, map_num, slot_num, (ULONG) timestamp);
		xcc->xcc_event_send_channel_empted =
			CreateEvent(ISC_get_security_desc(), FALSE, FALSE, name_buffer);
			
		if (!xcc->xcc_event_send_channel_empted)
			error("xxx");
		***/

		p += sizeof(struct xps);

		avail =	(USHORT) (XPS_USEFUL_SPACE(pages_per_slot) - (XNET_EVENT_SPACE * 2)) / 2;

		xps->xps_channels[XPS_CHANNEL_C2S_EVENTS].xch_buffer = p;	/* client to server events */
		xps->xps_channels[XPS_CHANNEL_C2S_EVENTS].xch_size = XNET_EVENT_SPACE;

		p += XNET_EVENT_SPACE;
		xps->xps_channels[XPS_CHANNEL_S2C_EVENTS].xch_buffer = p;	/* server to client events */
		xps->xps_channels[XPS_CHANNEL_S2C_EVENTS].xch_size = XNET_EVENT_SPACE;

		p += XNET_EVENT_SPACE;
		xps->xps_channels[XPS_CHANNEL_C2S_DATA].xch_buffer = p;	/* client to server data */
		xps->xps_channels[XPS_CHANNEL_C2S_DATA].xch_size = avail;

		p += avail;
		xps->xps_channels[XPS_CHANNEL_S2C_DATA].xch_buffer = p;	/* server to client data */
		xps->xps_channels[XPS_CHANNEL_S2C_DATA].xch_size = avail;

		xcc->xcc_recv_channel = &xps->xps_channels[XPS_CHANNEL_C2S_DATA];
		xcc->xcc_send_channel = &xps->xps_channels[XPS_CHANNEL_S2C_DATA];

		/* finally, allocate and set the port structure for this client */
		
		//port = xnet_alloc_port(0,
		port = new PortXNet(parent,
			                xcc->xcc_send_channel->xch_buffer,
			                xcc->xcc_send_channel->xch_size,
			                xcc->xcc_recv_channel->xch_buffer,
			                xcc->xcc_recv_channel->xch_size);
			                
		port->port_xcc = xcc;
		port->port_server_flags |= SRVR_server;

		status_vector[0] = isc_arg_gds;
		status_vector[1] = FB_SUCCESS;
		status_vector[2] = isc_arg_end;
		port->port_status_vector = status_vector;
		gds__register_cleanup(exitHandler, port);
		}
	catch (...) 
		{
		delete xcc;
			
		return NULL;
		}

#endif // WIN_NT

	return port;
}

bool PortXNet::serverInit(void)
{
	TEXT name_buffer[128];

	// init the limits

	slots_per_map = XPS_MAX_NUM_CLI;
	pages_per_slot = XPS_MAX_PAGES_PER_CLI;

	xnet_connect_mutex = 0;
	xnet_connect_map_h = 0;
	xnet_connect_map = 0;

	xnet_connect_event = 0;
	xnet_response_event = 0;

#ifdef WIN_NT
	try 
		{
		xnet_connect_mutex = XNetConnection::createMutex(XNET_MU_CONNECT_MUTEX);
		xnet_connect_event = XNetConnection::createEvent(XNET_E_CONNECT_EVENT);
		xnet_response_event = XNetConnection::createEvent(XNET_E_RESPONSE_EVENT);

		sprintf(name_buffer, XNET_MA_CONNECT_MAP, XNET_PREFIX);
		xnet_connect_map_h = CreateFileMapping(INVALID_HANDLE_VALUE,
												ISC_get_security_desc(),
												PAGE_READWRITE,
												0,
												sizeof(XNetResponse),
												name_buffer);
												
		if (!xnet_connect_map_h || (xnet_connect_map_h && ERRNO == ERROR_ALREADY_EXISTS)) 
			error("CreateFileMapping");

		xnet_connect_map = MapViewOfFile(xnet_connect_map_h, FILE_MAP_WRITE, 0L, 0L,
										 sizeof(XNetResponse));
		if (!xnet_connect_map)
			error("MapViewOfFile");
	
		return TRUE;
		}
	catch (...) 
		{
		//xnet_connect_fini();
		connectFini();
		}
#endif // WIN_NT

	return false;
}

XNetMappedFile* PortXNet::getFreeSlot(ULONG* map_num, ULONG* slot_num, time_t* timestamp)
{
	XNetMappedFile *xpm = NULL;
	ULONG slot = 0;
	ULONG map = 0;

	/* go through list of maps */
	
	for (xpm = client_maps; xpm; xpm = xpm->xpm_next, ++map) 
		{
		/* find an available unused comm area */

		for (slot = 0; slot < slots_per_map; slot++)
			if (xpm->xpm_ids[slot] == XPM_FREE)
				break;

		if (slot < slots_per_map) 
			{
			xpm->xpm_count++;
			xpm->xpm_ids[slot] = XPM_BUSY;
			map = xpm->xpm_number;
			break;
			}
		}

	/* if the mapped file structure has not yet been initialized,
	   make one now */

	if (!xpm) 
		{
		/* allocate new map file and first slot */

		//xpm = xnet_make_xpm(map, *timestamp);
		xpm = new XNetMappedFile (map, *timestamp, slots_per_map, pages_per_slot);
		xpm->mapFile(true);

		/* check for errors in creation of mapped file */

		slot = 0;
		xpm->xpm_ids[0] = XPM_BUSY;
		xpm->xpm_count++;
		}
	else
		*timestamp = xpm->xpm_timestamp;

	*map_num = map;
	*slot_num = slot;

	return xpm;
}

bool PortXNet::connectInit(void)
{
	TEXT name_buffer[128];

	xnet_connect_mutex = 0;
	xnet_connect_map_h = 0;
	xnet_connect_map = 0;

	xnet_connect_event = 0;
	xnet_response_event = 0;

#ifdef WIN_NT

	try 
		{
		//sprintf(name_buffer, XNET_MU_CONNECT_MUTEX, XNET_PREFIX);
		//xnet_connect_mutex = OpenMutex(MUTEX_ALL_ACCESS, TRUE, name_buffer);
		
		xnet_connect_mutex = XNetConnection::openMutex(XNET_MU_CONNECT_MUTEX);
		xnet_connect_event = XNetConnection::openEvent(XNET_E_CONNECT_EVENT);
		xnet_response_event = XNetConnection::openEvent(XNET_E_RESPONSE_EVENT);

		sprintf(name_buffer, XNET_MA_CONNECT_MAP, XNET_PREFIX);
		xnet_connect_map_h = OpenFileMapping(FILE_MAP_WRITE, TRUE, name_buffer);
		
		if (!xnet_connect_map_h) 
			error("OpenFileMapping");

		xnet_connect_map =
			MapViewOfFile(xnet_connect_map_h, FILE_MAP_WRITE, 0, 0, sizeof(XNetResponse));
						  
		if (!xnet_connect_map)
			error("MapViewOfFile");

		return TRUE;
		}
	catch (...) 
		{
		//xnet_connect_fini();
		connectFini();
		}
#endif // WIN_NT

	return FALSE;
}

PortXNet* PortXNet::connect(int server_flags, ISC_STATUS *statusVector)
{
	/***
	PortXNet *port;
	ULONG map_num, slot_num;
	XNetMappedFile *xpm;
	ISC_STATUS* status_vector;
	DWORD wait_res;
	ULONG client_pid;
	currentProcessId = getpid();
	//XNET_command_line[0] = 0;
	***/
	
	if (!serverInit()) 
		{
		XNET_LOG_ERROR("XNET server initialization failed");
		return NULL;
		}

	xnet_initialized = TRUE;
	gds__register_cleanup(exitHandler, NULL);
	
	PortXNet *port = new PortXNet(0);
	port->port_server_flags = server_flags;
	
	return port;
}

void PortXNet::error(const char* operation)
{
	throw -1;
}

bool_t PortXNet::read(XDR* xdrs)
{
#ifdef WIN_NT
	PortXNet *port = (PortXNet*)xdrs->x_public;
	XNetConnection *xcc = port->port_xcc;
	XNetChannel *xch = xcc->xcc_recv_channel;
	XNetMappedFile *xpm = xcc->xcc_xpm;

	if (xnet_shutdown)
		return FALSE;

	if (!SetEvent(xcc->xcc_event_recv_channel_empted)) 
		{
		XNET_ERROR(port, "SetEvent()", isc_net_read_err, ERRNO);
		return FALSE;
		}

	while (!xnet_shutdown) 
		{
		if (xpm->xpm_flags & XPMF_SERVER_SHUTDOWN) 
			{
			if (!(xcc->xcc_flags & XCCF_SERVER_SHUTDOWN)) 
				{
				xcc->xcc_flags |= XCCF_SERVER_SHUTDOWN;
				XNET_ERROR(port, "connection lost: another side is dead", 
						   isc_lost_db_connection, 0);
				}
			}

		DWORD wait_result = WaitForSingleObject(xcc->xcc_event_recv_channel_filled,
		                                  XNET_RECV_WAIT_TIMEOUT);
		                                  
		if (wait_result == WAIT_OBJECT_0) 
			{
			/* Client wrote some data for us(for server) to read*/
			xdrs->x_handy = xch->xch_length;
			xdrs->x_private = xdrs->x_base;
			return TRUE;
			}

		if (wait_result == WAIT_TIMEOUT) 
			{
			/* Check if another side is alive */
			
			if (WaitForSingleObject(xcc->xcc_proc_h,1) == WAIT_TIMEOUT)
				continue; /* Another side is alive */			

			/* Another side is dead or somthing bad has happaned*/

			//xnet_on_server_shutdown(port);
			XNET_ERROR(port, "connection lost: another side is dead", 
						isc_lost_db_connection, 0);								

			return FALSE;
			}
			
		XNET_ERROR(port, "WaitForSingleObject()", isc_net_read_err, ERRNO);
		
		return FALSE; /* a non-timeout result is an error */
		}
#endif // WIN_NT

	return FALSE;
}

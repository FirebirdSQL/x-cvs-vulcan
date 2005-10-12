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
#include "../jrd/gds_proto.h"
#include "../jrd/isc_proto.h"
#include "../jrd/isc_f_proto.h"
#include "Mutex.h"
#include "Sync.h"
#include "XNetConnection.h"
#include "XNetChannel.h"
#include "XNetMappedFile.h"

#ifdef WIN_NT
//#include <windows.h>
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

#define REMOTE_PREFIX	"ipc#"

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
//static HANDLE xnet_connect_map_h = 0;
static void  *xnet_connect_map = 0;
static XNetMappedFile	connectFile;

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

PortXNet::PortXNet(PortXNet* parent, XNetConnection *connection) : Port(0)
{
	init();
	port_xcc = connection;
	TEXT buffer[64];
	ISC_get_host(buffer, sizeof(buffer));
	port_host = buffer;
	port_connection = buffer;
	port_version.Format("XNET Server (%s)", (const char*) port_host);
	
	if (parent) 
		{
		parent->addClient(this);
		port_handle = parent->port_handle;
		port_server = parent->port_server;
		port_server_flags = parent->port_server_flags;
		port_connection = parent->port_connection;
		configuration = (ConfObject*) parent->configuration;
		}

	port_buff_size = connection->sendChannel.getMsgSize();
	port_status_vector = NULL;

	xdrCreate(&port_send, connection->sendChannel.data,  port_buff_size, XDR_ENCODE);
	xdrCreate(&port_receive, connection->recvChannel.data, 0, XDR_DECODE);
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
	port_flags |= PORT_cwd_reqd;
}

bool_t PortXNet::getBytes(XDR* xdrs, SCHAR* buff, u_int count)
{
	SLONG bytecount = count;
	SLONG to_copy;

	PortXNet *port = (PortXNet*) xdrs->x_public;
	XNetConnection *xcc = port->port_xcc;
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
	SLONG bytecount = count;
	SLONG to_copy;
	PortXNet *port = (PortXNet*)xdrs->x_public;
	XNetConnection *xcc = port->port_xcc;
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
			//if ((ULONG) xdrs->x_handy == xch->xch_size) 
			if ((ULONG) xdrs->x_handy == xcc->sendChannel.channelControl->xch_size) 
				{
				while(!xnet_shutdown) 
					{
					if (xcc->preSend(XNET_SEND_WAIT_TIMEOUT))
						break;

					if (!xcc->stillAlive())
						{
						//xnet_on_server_shutdown(port);
						XNET_ERROR(port, "connection lost: another side is dead", 
							        isc_lost_db_connection, 0);								

						return FALSE;
						}

					XNET_ERROR(port, "WaitForSingleObject()", isc_net_write_err, ERRNO);
					
					return FALSE;
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

PortXNet* PortXNet::analyze(ConfObject *configuration,
							TEXT* fileName, 
							USHORT* file_length, 
							ISC_STATUS *status_vector, 
							const TEXT* node_name, 
							const TEXT* user_string, 
							bool uv_flag)
{
	if (strncmp(fileName, REMOTE_PREFIX, sizeof(REMOTE_PREFIX) - 1) != 0)
		return NULL;

	const TEXT *truncatedName = fileName + sizeof(REMOTE_PREFIX) - 1;	
	TEXT expandedName [MAXPATHLEN];
	//ISC_expand_filename(truncatedName, strlen (truncatedName), expandedName);
	strcpy(expandedName, truncatedName);
	strcpy(fileName, expandedName);
	*file_length = strlen(expandedName);
	
	P_CNCT *cnct;
	TEXT *p, user_id[128]; //, buffer[64];

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
	cnct->p_cnct_file.cstr_address = (UCHAR *) expandedName;

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

	PortXNet *port = connect(configuration, node_name, packet, status_vector, FALSE);
	
	if (!port)
		{
		rdb->release();
		return NULL;
		}

	/* Get response packet from server. */

	rdb->rdb_port = port;
	port->port_context = rdb;
	port->receive(packet);

	if (packet->p_operation == op_reject && !uv_flag) 
		{
		port->disconnect();
		packet->p_operation = op_connect;
		cnct->p_cnct_operation = op_attach;
		cnct->p_cnct_cversion = CONNECT_VERSION2;
		cnct->p_cnct_client = ARCHITECTURE;
		cnct->p_cnct_file.cstr_length = *file_length;
		cnct->p_cnct_file.cstr_address = (UCHAR *) expandedName;

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

		if (!(port = connect(configuration, node_name, packet, status_vector, FALSE))) 
			{
			rdb->release();
			return NULL;
			}

		/* Get response packet from server. */

		rdb->rdb_port = port;
		port->port_context = rdb;
		port->receive(packet);
		}

	if (packet->p_operation == op_reject && !uv_flag) 
		{
		port->disconnect();
		packet->p_operation = op_connect;
		cnct->p_cnct_operation = op_attach;
		cnct->p_cnct_cversion = CONNECT_VERSION2;
		cnct->p_cnct_client = ARCHITECTURE;
		cnct->p_cnct_file.cstr_length = *file_length;
		cnct->p_cnct_file.cstr_address = (UCHAR *) expandedName;

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

		if (!(port = connect(configuration, node_name, packet, status_vector, FALSE))) 
			{
			rdb->release();
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

PortXNet* PortXNet::connect(ConfObject *configuration, 
							const TEXT* name, 
							Packet* packet, 
							ISC_STATUS *status_vector, 
							int flag)
{

	if (xnet_shutdown)
		return NULL;

	// set up for unavailable server
	status_vector[0] = isc_arg_gds;
	status_vector[1] = isc_unavailable;
	status_vector[2] = isc_arg_end;

	PortXNet *port = NULL;
	XNetConnection *xcc = NULL;
	XNetMappedFile *xpm = NULL;
	XPS xps = NULL;

	XNetResponse response;
	UCHAR *start_ptr;
	ULONG avail;

	if (!xnet_initialized) 
		{
		xnet_initialized = TRUE;
		currentProcessId = getpid();
		gds__register_cleanup(exitHandler, NULL);
		}

	Sync sync(&xnet_mutex, "PortXNet::connect");
	sync.lock(Exclusive);
	
	try 
		{
		TEXT name_buffer[128];
		sprintf(name_buffer, XNET_MA_CONNECT_MAP, XNET_PREFIX);
		xnet_connect_map = connectFile.mapFile(name_buffer, sizeof(XNetResponse), false);
		xnet_connect_mutex = XNetChannel::openMutex(XNET_MU_CONNECT_MUTEX);
		xnet_connect_event = XNetChannel::openEvent(XNET_E_CONNECT_EVENT);
		xnet_response_event = XNetChannel::openEvent(XNET_E_RESPONSE_EVENT);
		}
	catch (...) 
		{
		connectFini();
		return NULL;
		}

	// waiting for xnet connect lock to release
	
	if (!XNetChannel::wait(xnet_connect_mutex, XNET_CONNECT_TIMEOUT))
		{
		connectFini();
		return NULL;
		}

	/* writing connect request */

	// mark connect area with XNET_INVALID_MAP_NUM to
	// detect server faults on response
	
	((XNetResponse*) xnet_connect_map)->map_num = XNET_INVALID_MAP_NUM;
	((XNetResponse*) xnet_connect_map)->proc_id = currentProcessId; 
	XNetChannel::postEvent(xnet_connect_event);

	// waiting for server response
	
	if (!XNetChannel::wait(xnet_response_event, XNET_CONNECT_TIMEOUT))
		{
		connectFini();
				
		return NULL;
		}

	memcpy(&response, xnet_connect_map, sizeof(XNetResponse));
	XNetChannel::releaseMutex(xnet_connect_mutex);
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
	ULONG map_num = response.map_num;
	ULONG slot_num = response.slot_num;
	time_t timestamp = response.timestamp;

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
			
			xpm = new XNetMappedFile(map_num, timestamp, slots_per_map, pages_per_slot);
			xpm->mapFile(false);
			xpm->xpm_next = client_maps;
			client_maps = xpm;
			}

		xcc = new XNetConnection(map_num, slot_num);
		xcc->xcc_map_handle = xpm->xpm_handle;
		xcc->xcc_mapped_addr =(UCHAR *) xpm->xpm_address + XPS_SLOT_OFFSET(pages_per_slot, slot_num);
		xcc->xcc_xpm = xpm;
		xps = (XPS) xcc->xcc_mapped_addr;

		// only speak if server has correct protocol
		
		if (xps->xps_server_protocol != XPI_SERVER_PROTOCOL_VERSION)
			error("wrong protocol version");

		xps->xps_client_protocol = XPI_CLIENT_PROTOCOL_VERSION;

		// open server process handle to watch server health during communication session
		
#ifdef WIN_NT
		xcc->xcc_proc_h = OpenProcess(SYNCHRONIZE, 0, xps->xps_server_proc_id);
#endif
		
		if (!xcc->xcc_proc_h) 
			error("OpenProcess");

		xpm->addRef();
		xcc->open(false, timestamp);

		/* added this here from the server side as this part is called by the client 
		   and the server address need not be valid for the client -smistry 10/29/98 */
		   
		xcc->recvChannel.channelControl = &xps->xps_channels[XPS_CHANNEL_S2C_DATA];
		xcc->sendChannel.channelControl = &xps->xps_channels[XPS_CHANNEL_C2S_DATA];

		/* we also need to add client side flags or channel pointer as they 
		   differ from the server side */

		avail =	(ULONG) (XPS_USEFUL_SPACE(pages_per_slot) - (XNET_EVENT_SPACE * 2)) / 2;
		start_ptr = (UCHAR*) xps + (sizeof(struct xps) + (XNET_EVENT_SPACE * 2));

		/* send channel */
		
		xcc->sendChannel.data = start_ptr;
		
		/* receive channel */
		
		xcc->recvChannel.data = start_ptr + avail;
		}
	catch (...) 
		{
		delete xpm;
		delete xcc;

		return NULL;
		}

	port = new PortXNet(0, xcc);
	status_vector[1] = FB_SUCCESS;
	port->port_status_vector = status_vector;
	port->port_xcc = xcc;
	gds__register_cleanup(exitHandler, port);
	port->sendPacket(packet);

	return port;
}

void PortXNet::disconnect(void)
{
	/* If we have a asynchronous port for events, check out with it */
	
	if (port_async) 
		{
		//port_async->port_flags |= PORT_disconnect;	???
		port_async->disconnect();
		port_async = NULL;
		}

	
	/* If this is a sub-port, unlink it from it's parent */

	if (port_parent) 
		{
		port_parent->removeClient(this);
		port_parent = NULL;
		}

	gds__unregister_cleanup(exitHandler, this);
	release();	
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
		mainPort->disconnect();
	else 
		releaseAll();
}

void PortXNet::cleanupComm(XNetConnection* xcc)
{
	XNetMappedFile *xpm = xcc->xcc_xpm;
	delete xcc;

	// if this was the last area for this map, unmap it
	
	if (xpm) 
		{
		xpm->xpm_count--;

		if (!xpm->xpm_count && client_maps)
			{
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
	connectFini();
#endif

	Sync sync(&xnet_mutex, "PortXNet::releaseAll");
	sync.lock(Exclusive);

	/* release all map stuf left not released by broken ports */
	
	for (XNetMappedFile *xpm; xpm = client_maps;)
		{
		client_maps = xpm->xpm_next;
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
	XNetChannel::closeMutex(&xnet_connect_mutex);
	XNetChannel::closeEvent(&xnet_connect_event);
	XNetChannel::closeEvent(&xnet_response_event);
	connectFile.close();
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
		
		port_xcc->postReceive();
		
		return this;
		}

	currentProcessId = getpid();
	XNetResponse *presponse = (XNetResponse*)xnet_connect_map;
	
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
			//waitVector[count++] = port->port_xcc->xcc_event_recv_channel_filled;
			waitVector[count++] = port->port_xcc->recvChannel.channelFilled;
			}
			
		int index;

#ifdef WIN_NT
		//DWORD wait_res = WaitForSingleObject(xnet_connect_event,INFINITE);
		DWORD wait_res = WaitForMultipleObjects(count, waitVector, false, INFINITE);
		index = wait_res - WAIT_OBJECT_0;
#endif

		PortXNet *port = portVector[index];
		
		if (xnet_shutdown)
			break;

		ULONG client_pid = presponse->proc_id;
		
		/***
		if (client_pid == currentProcessId)
			continue; // dummy xnet_connect_event fire -  no connect request
		***/
		
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
			PortXNet *newPort = NULL;
			
			// pack combined mapped area and number
			
			if (xpm) 
				{
				presponse->proc_id = currentProcessId;
				presponse->map_num = map_num;
				presponse->slot_num = slot_num;
				presponse->timestamp = timestamp;
				ISC_STATUS *status_vector = new ISC_STATUS[ISC_STATUS_LENGTH];
				newPort = getServerPort(this, client_pid, xpm, map_num, slot_num, timestamp, status_vector);
				}
			else 
				XNET_LOG_ERROR("xnet_get_free_slot() failed");
						
			if (!xpm || !newPort) 
				XNET_LOG_ERROR("failed to allocate server port for communication");

			//SetEvent(xnet_response_event);
			XNetChannel::postEvent(xnet_response_event);
			port = newPort;
			}
		else
			{
			XDR *xdrs = &port->port_receive;
			xdrs->x_handy = port->port_xcc->recvChannel.getMsgLength(); //xch->xch_length;
			xdrs->x_private = xdrs->x_base;
			}
			
		/* We've got data -- lap it up and use it */

		if (!xdr_protocol(&port->port_receive, packet))
			packet->p_operation = op_exit;

		port->port_xcc->postReceive();
		port->addRef();
		
		return port;
		}

	return NULL;
}

XDR_INT PortXNet::sendPacket(Packet* packet)
{
	if (!xdr_protocol(&port_send, packet))
		return FALSE;

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

	XNetConnection *xcc = NULL;

	try 
		{
		// make a new xcc
		
		XNetConnection *parent_xcc = port_xcc;
		XNetMappedFile *xpm = parent_xcc->xcc_xpm;
		XPS xps = (XPS) parent_xcc->xcc_mapped_addr;

		xcc = new XNetConnection(parent_xcc);
		xcc->open(true, xpm->xpm_timestamp);
		
		// send events channel
		
		xcc->sendChannel.data = ((UCHAR *) xpm->xpm_address + sizeof(struct xps));

		// receive events channel
		
		xcc->recvChannel.data = ((UCHAR *) xpm->xpm_address + sizeof(struct xps) + (XNET_EVENT_SPACE));
		xcc->sendChannel.channelControl = &xps->xps_channels[XPS_CHANNEL_C2S_EVENTS];		
		xcc->recvChannel.channelControl = &xps->xps_channels[XPS_CHANNEL_S2C_EVENTS];

		// alloc new port and link xcc to it
		
		port_async = new PortXNet(NULL, xcc);
		port_async->port_flags = port_flags & PORT_no_oob;
		port_async->port_flags |= PORT_async;
		gds__register_cleanup(exitHandler, port_async);

		return port_async;
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
	XNetConnection *xcc = NULL;

	try 
		{
		// make a new xcc
		XNetConnection *parent_xcc = port_xcc;
		XPS xps = (XPS) parent_xcc->xcc_mapped_addr;
		xcc = new XNetConnection(parent_xcc);
		XNetMappedFile *xpm = parent_xcc->xcc_xpm;
		xcc->create(true, xpm->xpm_timestamp);

		// send events channel
		
		xcc->sendChannel.data = ((UCHAR *) xpm->xpm_address + sizeof(struct xps) + (XNET_EVENT_SPACE));

		// receive events channel
		
		xcc->recvChannel.data =  ((UCHAR *) xpm->xpm_address + sizeof(struct xps));
		xcc->sendChannel.channelControl = &xps->xps_channels[XPS_CHANNEL_S2C_EVENTS];		
		xcc->recvChannel.channelControl = &xps->xps_channels[XPS_CHANNEL_C2S_EVENTS];

		// alloc new port and link xcc to it
		
		port_async = new PortXNet(NULL, xcc);
		port_async->port_flags = port_flags & PORT_no_oob;
		port_async->port_server_flags = port_server_flags;

		return port_async;
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
	//XNetChannel *xch = xcc->xcc_send_channel;
	//xch->xch_length = xdrs->x_private - xdrs->x_base;

	xcc->send(xdrs->x_private - xdrs->x_base);
	port->port_misc1 = (port->port_misc1 + 1) % MAX_SEQUENCE;
	xdrs->x_private = xdrs->x_base;
	//xdrs->x_handy = xch->xch_size;
	xdrs->x_handy = xcc->sendChannel.getMsgSize();

	return TRUE;
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

	try 
		{
		xnet_response_event = XNetChannel::openEvent(XNET_E_RESPONSE_EVENT);
		xpm = new XNetMappedFile(currentProcessId, time_t(0), slots_per_map, pages_per_slot);
		port = getServerPort(NULL, client_pid, xpm, currentProcessId, 0, 0, status_vector);
		
		if (!port) 
			error("getServerPort");

		//make signal for client
		
		//SetEvent(xnet_response_event);
		XNetChannel::postEvent(xnet_response_event);
		
		if (xnet_response_event) 
			//CloseHandle(xnet_response_event);
			XNetChannel::closeEvent(&xnet_response_event);
		}
	catch (...) 
		{
		XNET_LOG_ERROR("Unable to initialize child process");
		status_vector[1] = isc_unavailable;

		if (port) 
			{
			delete port;
			port = NULL;
			}

		if (xnet_response_event) 
			{
			//SetEvent(xnet_response_event); // To prevent client blocking
			XNetChannel::postEvent(xnet_response_event);
			//CloseHandle(xnet_response_event);
			XNetChannel::closeEvent(&xnet_response_event);
			}
		}

	return port;
}

PortXNet* PortXNet::getServerPort(PortXNet *parent, int client_pid, XNetMappedFile *xpm, ULONG map_num, ULONG slot_num, time_t timestamp, ISC_STATUS *status_vector)
{
	PortXNet *port = NULL;
	XPS xps;
	ULONG avail;

	// allocate a communications control structure and fill it in
	
	XNetConnection *xcc = new XNetConnection(slot_num, map_num);
	xcc->create(false, timestamp);
	
	try 
		{
		UCHAR *p = (UCHAR *) xpm->xpm_address + XPS_SLOT_OFFSET(pages_per_slot, slot_num);
		memset(p, 0, XPS_MAPPED_PER_CLI(pages_per_slot));
		xcc->xcc_mapped_addr = p;
		xcc->xcc_xpm = xpm;

		// Open client process handle to watch clients health during communication session
		
#ifdef WIN_NT
		xcc->xcc_proc_h = OpenProcess(SYNCHRONIZE, 0, client_pid);
		
		if (!xcc->xcc_proc_h) 
			error("OpenProcess");
#endif

		xps = (XPS) xcc->xcc_mapped_addr;
		xps->xps_client_proc_id = client_pid;
		xps->xps_server_proc_id = getpid();

		// make sure client knows what this server speaks

		xps->xps_server_protocol = XPI_SERVER_PROTOCOL_VERSION;
		xps->xps_client_protocol = 0L;

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
		xcc->recvChannel.data = p;
		xps->xps_channels[XPS_CHANNEL_C2S_DATA].xch_buffer = p;	/* client to server data */
		xps->xps_channels[XPS_CHANNEL_C2S_DATA].xch_size = avail;

		p += avail;
		xcc->sendChannel.data = p;
		xps->xps_channels[XPS_CHANNEL_S2C_DATA].xch_buffer = p;	/* server to client data */
		xps->xps_channels[XPS_CHANNEL_S2C_DATA].xch_size = avail;

		//xcc->xcc_recv_channel = &xps->xps_channels[XPS_CHANNEL_C2S_DATA];
		xcc->recvChannel.channelControl = &xps->xps_channels[XPS_CHANNEL_C2S_DATA];
		//xcc->xcc_send_channel = &xps->xps_channels[XPS_CHANNEL_S2C_DATA];
		xcc->sendChannel.channelControl = &xps->xps_channels[XPS_CHANNEL_S2C_DATA];

		/* finally, allocate and set the port structure for this client */
		
		port = new PortXNet(parent, xcc);
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

	return port;
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


PortXNet* PortXNet::connect(int server_flags, ISC_STATUS *statusVector)
{
	slots_per_map = XPS_MAX_NUM_CLI;
	pages_per_slot = XPS_MAX_PAGES_PER_CLI;

	try 
		{
		xnet_connect_mutex = XNetChannel::createMutex(XNET_MU_CONNECT_MUTEX);
		xnet_connect_event = XNetChannel::createEvent(XNET_E_CONNECT_EVENT);
		xnet_response_event = XNetChannel::createEvent(XNET_E_RESPONSE_EVENT);

		TEXT name_buffer[128];
		sprintf(name_buffer, XNET_MA_CONNECT_MAP, XNET_PREFIX);
		xnet_connect_map = connectFile.mapFile(name_buffer, sizeof(XNetResponse), true);
		}
	catch (...) 
		{
		connectFini();
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
	PortXNet *port = (PortXNet*)xdrs->x_public;
	XNetConnection *xcc = port->port_xcc;
	XNetMappedFile *xpm = xcc->xcc_xpm;

	if (xnet_shutdown)
		return FALSE;

	xcc->recvChannel.postReceive();

	while (!xnet_shutdown) 
		{
		if (xpm->xpm_flags & XPMF_SERVER_SHUTDOWN) 
			if (!(xcc->xcc_flags & XCCF_SERVER_SHUTDOWN)) 
				{
				xcc->xcc_flags |= XCCF_SERVER_SHUTDOWN;
				XNET_ERROR(port, "connection lost: another side is dead", 
						   isc_lost_db_connection, 0);
				}

		if (xcc->recvChannel.receive(XNET_RECV_WAIT_TIMEOUT))
			{
			/* Client wrote some data for us(for server) to read*/
			xdrs->x_handy = xcc->recvChannel.getMsgLength(); //xch->xch_length;
			xdrs->x_private = xdrs->x_base;
			return TRUE;
			}

		/* Check if another side is alive */
		
		if (xcc->stillAlive())
			continue; /* Another side is alive */			

		/* Another side is dead or somthing bad has happaned*/

		//xnet_on_server_shutdown(port);
		XNET_ERROR(port, "connection lost: another side is dead", 
					isc_lost_db_connection, 0);								

		return FALSE;
		}

	return FALSE;
}

void PortXNet::addRef(void)
{
	Port::addRef();
}

void PortXNet::release(void)
{
	Port::release();
}

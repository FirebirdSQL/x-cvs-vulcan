/*
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
 * 2004.05.28 Jim Starkey -- Created Class
 */

#ifdef	WIN_NT
#define FD_SETSIZE 1024
#endif

#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include "fbdev.h"
#include "common.h"
#include "ibase.h"
#include "remote.h"
#include "PortInet.h"
#include "../remote/remot_proto.h"
#include "../remote/proto_proto.h"
#include "../remote/inet_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/isc_proto.h"
#include "../jrd/jrd_time.h"
#include "../jrd/iberr.h"
#include "Sync.h"
#include "Mutex.h"
#include "../jrd/ib_stdio.h"

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h> /* for socket() */
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


#ifdef SET_TCP_NO_DELAY
#include <netinet/tcp.h>
#endif

#ifdef HAVE_SYS_TIMEB_H
# include <sys/timeb.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#ifdef HAVE_GRP_H
#include <grp.h>
#endif

#if !(defined VMS || defined WIN_NT)
#include <netdb.h>
#include <arpa/inet.h>
/* EKU: SINIX-Z does not define INADDR_NONE */
#ifndef INADDR_NONE
#define INADDR_NONE (in_addr_t)-1
#endif
#endif

#ifndef REQUESTER
#include "../jrd/os/isc_i_proto.h"
#include "../jrd/sch_proto.h"
#endif /* REQUESTER */

#include "ConfObject.h"
#include "Parameters.h"

#ifndef PROXY_FILE
#define PROXY_FILE	"/etc/gds_proxy"
#endif

#ifndef HOSTS_FILE
#define HOSTS_FILE	"/etc/hosts.equiv"
#endif

#ifndef GDS_HOSTS_FILE
#ifdef SMALL_FILE_NAMES
#define GDS_HOSTS_FILE	"/etc/gdshosts.eqv"
#else
#define GDS_HOSTS_FILE	"/etc/gds_hosts.equiv"
#endif
#endif

#if (defined hpux || defined SCO_UNIX)
extern int h_errno;
#define H_ERRNO	h_errno
#endif

#ifdef VMS
#include <ib_perror.h>
#include <socket.h>
#define NO_FORK
#define MAX_PTYPE	ptype_batch_send
#define PROXY_FILE	"[sysmgr]gds_proxy.dat"
#error "vms implementation must be completed"
#endif

#ifdef WIN_NT
#include <fcntl.h>
#include <process.h>
#include <signal.h>
//#include "../utilities/install/install_nt.h"
#include "../utilities/SystemServices/SystemServices.h"

#define ERRNO		WSAGetLastError()
#define H_ERRNO		WSAGetLastError()
//#define SOCLOSE		closesocket
#define SYS_ERR		isc_arg_win32
#define INET_RETRY_ERRNO	WSAEINPROGRESS
#define INET_ADDR_IN_USE	WSAEADDRINUSE
#define sleep(seconds)  Sleep ((seconds) * 1000)

/*
** Winsock has a typedef for socket, so #define SOCKET to the typedef here
** so that it won't be redefined below.
*/

/* TMN: 28 Jul 2000 - Fixed compiler warning */

# ifndef SOCKET
#  define SOCKET		SOCKET
# endif	/* SOCKET */
#define ERRNO		WSAGetLastError()
#define H_ERRNO		WSAGetLastError()

#endif /* WIN_NT */

#ifdef SYSV_SIGNALS
#define NO_ITIMER
#endif

#ifndef SYS_ERR
#define SYS_ERR		isc_arg_unix
#endif

#ifndef SV_INTERRUPT
#define SV_INTERRUPT	0
#endif

#ifndef EINTR
#define EINTR		0
#endif

#ifndef hpux
#define sigvector	sigvec
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET  -1
#endif

#ifndef SOCKET
#define SOCKET  int
#endif

#ifndef SOCLOSE
//#define SOCLOSE	close
#endif

#ifndef ERRNO
#define ERRNO	errno
#endif

#ifndef INET_ADDR_IN_USE
#define INET_ADDR_IN_USE EADDRINUSE
#endif

#ifndef INET_RETRY_ERRNO
#define INET_RETRY_ERRNO TRY_AGAIN
#endif

#ifndef INET_RETRY_CALL
#define INET_RETRY_CALL 5
#endif

#ifndef H_ERRNO
//#define H_ERRNO	h_errno
#define H_ERRNO		errno
#endif

#ifndef SIGURG
#define SIGURG	SIGINT
#endif

#ifndef ENOBUFS
#define ENOBUFS	0
#endif

#define MAX_DATA_LW	1448		/* Low  Water mark */
#define MAX_DATA_HW	32768		/* High Water mark */
#define DEF_MAX_DATA	8192
#define MAX_SEQUENCE	256

#ifndef MAXHOSTLEN
#define MAXHOSTLEN	64
#endif

#ifdef	WIN_NT
#define	INTERRUPT_ERROR(x)	(SYSCALL_INTERRUPTED(x) || (x) == WSAEINTR)
#else
#define	INTERRUPT_ERROR(x)	(SYSCALL_INTERRUPTED(x))
#endif

#ifndef MAX_PTYPE
#define MAX_PTYPE       ptype_batch_send
#endif

#ifdef WIN_NT
#define BAD_SOCKET		WSAENOTSOCK
#else
#define BAD_SOCKET		EBADF
#endif

#define	SELECT_TIMEOUT	60		/* Dispatch thread select timeout (sec) */
#define inet_copy(from,to,size)	memcpy(to, from, size)
#define inet_zero(to,size)		memset(to, 0, size)


static void copy_p_cnct_repeat_array(	p_cnct::p_cnct_repeat*			pDest,
										const p_cnct::p_cnct_repeat*	pSource,
										size_t							nEntries);

static PortInet*	inet_try_connect(ConfObject *configuration,
								 Packet*,
									RDatabase*,
									USHORT,
									TEXT*,
									const TEXT*,
									ISC_STATUS*,
									const UCHAR* dpb,
									SSHORT);

static caddr_t	inet_inline(XDR *, u_int);
static bool_t	inet_putlong(XDR*, SLONG*);
static bool_t	inet_putbytes(XDR*, const SCHAR*, u_int);
static bool_t	inet_write(XDR *, int);

static in_addr get_bind_address(ConfObject *configuration);
static in_addr get_host_address(const TEXT* name);

static XDR::xdr_ops inet_ops =
{
	Port::getLong,			// inet_getlong,
	Port::putLong,			// inet_putlong,
	PortInet::getBytes,		// inet_getbytes,
	PortInet::putBytes,		// inet_putbytes,
	Port::getPosition,		// inet_getpostn,
	Port::setPosition,		// inet_setpostn,
	Port::inlinePointer,	// inet_inline,
	Port::destroy			// inet_destroy
};

static slct INET_select = { 0, 0, 0 };
static int initialized;;
static Mutex getServiceMutex;

size_t *debugPtr = (size_t*) 0x89751c;

Port* INET_analyze(	ConfObject *configuration,
					TEXT*	file_name,
					USHORT*	file_length,
					ISC_STATUS*	status_vector,
					const TEXT*	node_name,
					const TEXT*	user_string,
					bool	uv_flag,
					const UCHAR*	dpb,
					SSHORT	dpb_length)
{
/**************************************
 *
 *	I N E T _ a n a l y z e
 *
 **************************************
 *
 * Functional description
 *	File_name is on node_name.
 *	Establish an external connection to node_name.
 *
 *	If a connection is established, return a port block, otherwise
 *	return NULL.
 *
 *	If the "uv_flag" is non-zero, user verification also takes place.
 *
 **************************************/
	*file_length = strlen(file_name);

	/* We need to establish a connection to a remote server.  Allocate the necessary
	   blocks and get ready to go. */

	RDatabase *rdb = new RDatabase (NULL);
	rdb->rdb_status_vector = status_vector;
	Packet*	packet	= &rdb->rdb_packet;
	Sync sync(&rdb->syncObject, "xyzzy");
	sync.lock(Exclusive);
	P_CNCT*	cnct = &packet->p_cnct;
	
	/* Pick up some user identification information */

	char	user_id[200];
	user_id[0] = CNCT_user;
	char* p = user_id + 2;
	int		eff_gid;
	int		eff_uid;

	ISC_get_user(p, &eff_uid, &eff_gid, 0, 0, 0, user_string);
	user_id[1] = (char) strlen(p);
	p = p + user_id[1];

	*p++ = CNCT_host;
	p++;
	ISC_get_host(p, 64);
	p[-1] = (UCHAR) strlen((SCHAR *) p);

	for (; *p; p++) 
		if (*p >= 'A' && *p <= 'Z') 
			*p = *p - 'A' + 'a';

	if ((eff_uid == -1) || uv_flag) 
		{
		*p++ = CNCT_user_verification;
		*p++ = 0;
		}
#if !(defined VMS)
	else
		{
		/* Communicate group id info to server, as user maybe running under group
		   id other than default specified in /etc/passwd. */

		*p++ = CNCT_group;
		*p++ = sizeof(SLONG);
		eff_gid = htonl(eff_gid);
		memcpy(p, AOF32L(eff_gid), sizeof(SLONG));
		p += sizeof(SLONG);
		}
#endif

	const SSHORT user_length = (SSHORT) (p - user_id);

	/* Establish connection to server */

	/* Note: prior to V3.1E a recievers could not in truth handle more
	   then 5 protocol descriptions, so we try them in chunks of 5 or less */

	/* If we want user verification, we can't speak anything less than version 7 */


	cnct->p_cnct_user_id.cstr_length = user_length;
	cnct->p_cnct_user_id.cstr_address = (UCHAR*) user_id;


	static const p_cnct::p_cnct_repeat protocols_to_try1[] =
		{
		REMOTE_PROTOCOL(PROTOCOL_VERSION8, ptype_rpc, MAX_PTYPE, 1),
		REMOTE_PROTOCOL(PROTOCOL_VERSION10, ptype_rpc, MAX_PTYPE, 2),
		REMOTE_PROTOCOL(PROTOCOL_VERSION11, ptype_rpc, MAX_PTYPE, 4)
		};

	cnct->p_cnct_count = FB_NELEM(protocols_to_try1);
	copy_p_cnct_repeat_array(cnct->p_cnct_versions, protocols_to_try1, cnct->p_cnct_count);
	
	/* Try connection using first set of protocols.  punt if error */

	PortInet* port = inet_try_connect(configuration, packet, rdb, *file_length, file_name,
								 node_name, status_vector, dpb, dpb_length);
	if (!port) 
		return NULL;

	if (packet->p_operation == op_reject && !uv_flag)
		{
		port->disconnect();

		/* try again with next set of known protocols */

		cnct->p_cnct_user_id.cstr_length = user_length;
		cnct->p_cnct_user_id.cstr_address = (UCHAR*) user_id;

		static const p_cnct::p_cnct_repeat protocols_to_try2[] =
		{
			REMOTE_PROTOCOL(PROTOCOL_VERSION6, ptype_rpc, ptype_batch_send, 1),
			REMOTE_PROTOCOL(PROTOCOL_VERSION7, ptype_rpc, MAX_PTYPE, 2)
		};

		cnct->p_cnct_count = FB_NELEM(protocols_to_try2);

		copy_p_cnct_repeat_array(cnct->p_cnct_versions,
								 protocols_to_try2,
								 cnct->p_cnct_count);

		port = inet_try_connect(configuration, packet, rdb, *file_length, file_name,
								node_name, status_vector, dpb, dpb_length);
		if (!port)
			return NULL;
	}

	if (packet->p_operation == op_reject && !uv_flag)
		{
		port->disconnect();

		/* try again with next set of known protocols */

		cnct->p_cnct_user_id.cstr_length = user_length;
		cnct->p_cnct_user_id.cstr_address = (UCHAR*) user_id;

		static const p_cnct::p_cnct_repeat protocols_to_try3[] =
		{
			REMOTE_PROTOCOL(PROTOCOL_VERSION3, ptype_rpc, ptype_batch_send, 1),
			REMOTE_PROTOCOL(PROTOCOL_VERSION4, ptype_rpc, ptype_batch_send, 2)
		};

		cnct->p_cnct_count = FB_NELEM(protocols_to_try3);

		copy_p_cnct_repeat_array(cnct->p_cnct_versions,
								 protocols_to_try3,
								 cnct->p_cnct_count);

		port = inet_try_connect(configuration, packet, rdb, *file_length, file_name,
								node_name, status_vector, dpb, dpb_length);
		if (!port)
			return NULL;
	}

	if (packet->p_operation != op_accept)
		{
		*status_vector++ = isc_arg_gds;
		*status_vector++ = isc_connect_reject;
		*status_vector++ = 0;
		port->disconnect();
		return NULL;
		}

	port->port_protocol = packet->p_acpt.p_acpt_version;

	/* once we've decided on a protocol, concatenate the version
	   string to reflect it...  */

	//TEXT	buffer[64];
	port->port_version.Format ("%s/P%d", (const char*) port->port_version, port->port_protocol);
	//ALLR_free(port->port_version);
	//port->port_version = REMOTE_make_string(buffer);

	if (packet->p_acpt.p_acpt_architecture == ARCHITECTURE)
		port->port_flags |= PORT_symmetric;

	if (packet->p_acpt.p_acpt_type == ptype_rpc) 
		port->port_flags |= PORT_rpc;

	if (packet->p_acpt.p_acpt_type != ptype_out_of_band) 
		port->port_flags |= PORT_no_oob;

	return port;
}


PortInet::PortInet(int size) : Port (size)
{
}

PortInet::~PortInet(void)
{
}

int PortInet::accept(p_cnct* cnct)
{
	TEXT name[64], password[64];

	/* Default account to "guest" (in theory all packets contain a name) */

	strcpy(name, "guest");
	password[0] = 0;

	/* Pick up account and password, if given */

	const TEXT* id = (TEXT *) cnct->p_cnct_user_id.cstr_address;
	const TEXT* const end = id + cnct->p_cnct_user_id.cstr_length;

	SLONG eff_gid, eff_uid;
	eff_uid = eff_gid = -1;
	bool user_verification = false;
	
	while (id < end)
		{
		switch (*id++)
			{
			case CNCT_user:
				{
				int length = *id++;
				//str* string = (str*) ALLOCV(type_str, length);
				port_user_name.setString(id, length);
				id += length;
				/***
				string->str_length = length;
				if (length) 
					{
					TEXT* p = (TEXT *) string->str_data;
					int l = length;
					do {
						*p++ = *id++;
					} while (--l);
					}
				strncpy(name, string->str_data, length);
				name[length] = (TEXT) 0;
				***/
				strcpy (name, port_user_name);
				break;
				}

			case CNCT_passwd:
				{
				TEXT* p = password;
				int length = *id++;
				if (length != 0) 
					{
					do {
						*p++ = *id++;
					} while (--length);
					}
				*p = 0;
				break;
				}

			case CNCT_group:
				{
				TEXT* p = (TEXT *) &eff_gid;
				int length  = *id++;
				if (length != 0) {
					do {
						*p++ = *id++;
					} while (--length);
				}
				eff_gid = ntohl(eff_gid);
				break;
				}

			/* this case indicates that the client has requested that
			   we force the user name/password to be verified against
			   the security database */

			case CNCT_user_verification:
				user_verification = true;
				id++;
				break;

			default:
				id += *id + 1;
			}
		}

	/* See if user exists.  If not, reject connection */

	if (user_verification) 
		{
		eff_gid = eff_uid = -1;
		port_flags |= PORT_not_trusted;
		}

#if !defined(WIN_NT)

#ifdef VMS
	else
		{
		TEXT host[MAXHOSTLEN];
		int trusted = checkHost(host, name);
		
		if (!trusted) 
			return FALSE;

		if (trusted == -1) 
			{
			eff_gid = eff_uid = -1;
			port_flags |= PORT_not_trusted;
			}
		else
			{
			checkProxy(host, name);

			if (!port_parent) 
				{
				if (!chuser(name))
					return FALSE;
				ISC_get_user(name, &eff_uid, &eff_gid, 0, 0, 0, 0);
				}
			/* ??? How to get uic for multi-client server case ??? */
		}
	}
#else
	else
		{
		/* Security check should be against remote name passed - so do
		   check_host first */

		TEXT host[MAXHOSTLEN];
		struct passwd* passwd = getpwnam(name);
		int trusted = checkHost(host, name, passwd);
		
		if (!trusted) 
			return FALSE;

		if (trusted == -1) 
			{
			eff_gid = eff_uid = -1;
			port_flags |= PORT_not_trusted;
			}
		else
			{
			if (checkProxy(host, name))
				passwd = getpwnam(name);
			if (!passwd)
				return FALSE;
				
#ifndef HAVE_INITGROUPS
			eff_gid = passwd->pw_gid;
#else

			SLONG gids[128];
			initgroups(passwd->pw_name, passwd->pw_gid);
			
			if (eff_gid != -1) 
				{
				const int gid_count = getgroups(FB_NELEM(gids), (gid_t*)gids);
				int i;
				for (i = 0; i < gid_count; ++i) 
					if (gids[i] == eff_gid) 
						break;
				if ((i == gid_count) && passwd) 
					eff_gid = passwd->pw_gid;
				}
			else
				eff_gid = passwd->pw_gid;
				
#endif /* HAVE_INITGROUPS */

			eff_uid = passwd->pw_uid;

			/* if not multi-client: set uid, gid and home directory */

			if (!port_parent) 
				{
				if (!eff_gid || setregid(passwd->pw_gid, eff_gid) == -1) 
					setregid(passwd->pw_gid, passwd->pw_gid);
				if (!setreuid(passwd->pw_uid, passwd->pw_uid)) 
					chdir(passwd->pw_dir);
				}
			}
		}

	{
	
	/* If the environment varible ISC_INET_SERVER_HOME is set,
	 * change the home directory to the specified directory.
	 * Note that this will overrule the normal setting of
	 * the current directory to the effective user's home directory.
	 * This feature was added primarily for testing via remote
	 * loopback - but does seem to be of good general use, so
	 * is activiated for the production product.
	 * 1995-February-27 David Schnepper
	 */
	 
	const char* home = getenv("ISC_INET_SERVER_HOME");
	
	if (home) 
		if (chdir(home)) 
			gds__log("inet_server: unable to cd to %s errno %d\n", home,
						ERRNO);
	}
	
#endif /* VMS */

#endif /* !WIN_NT */

	/* store FULL user identity in port_user_name for security purposes */

	{
	strcpy (name, port_user_name);
	//strncpy(name, port_user_name->str_data, port_user_name->str_length);
	//TEXT* p = &name[port_user_name->str_length];
	port_user_name.Format(".%ld.%ld", eff_gid, eff_uid);
	//ALLR_free((UCHAR *) port_user_name);
	//port_user_name = REMOTE_make_string(name);
	}

	return TRUE;
}

void PortInet::disconnect()
{
#ifndef VMS
	/* SO_LINGER was turned off on the initial bind when the server was started.
	 * This will force a reset to be sent to the client when the socket is closed.
	 * We only want this behavior in the case of the server terminiating
	 * abnormally and not on an orderly shut down.  Because of this, turn the
	 * SO_LINGER option back on for the socket.  The result of setsockopt isn't
	 * too important at this stage since we are closing the socket anyway.  This
	 * is an attempt to return the socket to a state where a graceful shutdown can
	 * occur.
	 */

	if (port_linger.l_onoff) 
		setsockopt((SOCKET) port_handle, SOL_SOCKET, SO_LINGER,
					   (SCHAR *) & port_linger,
					   sizeof(port_linger));

#if defined WIN_NT

	if (port_handle && (SOCKET) port_handle != INVALID_SOCKET) 
		shutdown((int) port_handle, 2);

#else /* WIN_NT */

	if (port_handle) 
		shutdown((int) port_handle, 2);

#endif /* WIN_NT */

#endif /* !VMS */

#if !(defined VMS || defined WIN_NT)
	if (port_ast) 
		ISC_signal_cancel(SIGURG, signalHandler, this);
#endif

/* If this is a sub-port, unlink it from it's parent */

#ifdef  DEFER_PORT_CLEANUP
	bool defer_cleanup = false;
	port_state = state_disconnected;
#endif

	Port* parent = port_parent;
	
	if (port_async) 
		{
		//port_async->port_flags |= PORT_disconnect;
		disconnect();
		port_async = NULL;
		}
			
	if (parent != NULL) 
		{
#ifdef	DEFER_PORT_CLEANUP
		defer_cleanup = true;
#else
		//unhookPort(parent);
		parent->removeClient(this);
#endif
		}

	if (port_handle) 
		closeSocket((SOCKET) port_handle);

	gds__unregister_cleanup(exitHandler, this);

#ifdef DEFER_PORT_CLEANUP
	if (!defer_cleanup)
#endif
		//delete this;
		release();

#ifdef DEBUG
	if (INET_trace & TRACE_summary) 
		{
		ib_fprintf(ib_stdout, "INET_count_send = %lu packets\n",
				   INET_count_send);
		ib_fprintf(ib_stdout, "INET_bytes_send = %lu bytes\n",
				   INET_bytes_send);
		ib_fprintf(ib_stdout, "INET_count_recv = %lu packets\n",
				   INET_count_recv);
		ib_fprintf(ib_stdout, "INET_bytes_recv = %lu bytes\n",
				   INET_bytes_recv);
		ib_fflush(ib_stdout);
		}
#endif
}

Port* PortInet::receive(Packet* packet)
{
	/* If this isn't a multi-client server, just do the operation and get it
	   over with */

	if (!(port_server_flags & SRVR_multi_client)) 
		{
		/* loop as long as we are receiving dummy packets, just
		   throwing them away--note that if we are a server we won't
		   be receiving them, but it is better to check for them at
		   this level rather than try to catch them in all places where
		   this routine is called */

		do 
			{
			if (!xdr_protocol(&port_receive, packet))
				return NULL;
#ifdef DEBUG
			static ULONG op_rec_count = 0;
			op_rec_count++;
			
			if (INET_trace & TRACE_operations) 
				{
				ib_fprintf(ib_stdout, "%04lu: OP Recd %5lu opcode %d\n",
							inet_debug_timer(),
							op_rec_count, packet->p_operation);
				ib_fflush(ib_stdout);
				}
#endif
			}
			
		while (packet->p_operation == op_dummy)
			;

		return this;
		}

	/* Multi-client server multiplexes all known ports for incoming packets. */

	for (;;) 
		{
		PortInet *port = selectPort(&INET_select);
		
		if (port == this) 
			{
			port->release();
			
			if (port = selectAccept())
				return port;
				
			continue;
			}
			
		if (port) 
			{
			if (port->port_dummy_timeout < 0) 
				{
				port->port_dummy_timeout = port->port_dummy_packet_interval;
				if (port->port_flags & PORT_async || port->port_protocol < PROTOCOL_VERSION8) 
					{
					port->release();
					continue;
					}
				packet->p_operation = op_dummy;
				return port;
				}
				
			/* We've got data -- lap it up and use it */

			if (!xdr_protocol(&port->port_receive, packet))
				packet->p_operation = op_exit;
				
#ifdef DEBUG
			static ULONG op_rec_count = 0;
			op_rec_count++;
			
			if (INET_trace & TRACE_operations) 
				{
				ib_fprintf(ib_stdout, "%05lu: OP Recd %5lu opcode %d\n",
							inet_debug_timer(),
							op_rec_count, packet->p_operation);
				ib_fflush(ib_stdout);
				}
#endif

			/*  Make sure that there are no more messages in this port before blocking
				ourselves in select_wait. If there are more messages then set the flag
				corresponding to this port which was cleared in select_port routine.
			*/
			
			if (port->port_receive.x_handy) 
				{
				FD_SET((SLONG) port->port_handle, &INET_select.slct_fdset);
				++INET_select.slct_count;
				}
				
			if (packet->p_operation == op_dummy)
				{
				port->release();
				continue;
				}

			return port;
			}
			
		if (!selectWait(&INET_select))
			return NULL;
		}
}

// was send_full

XDR_INT PortInet::sendPacket(Packet* packet)
{
	if (!xdr_protocol(&port_send, packet))
		return FALSE;

#ifdef DEBUG
	{
		static ULONG op_sent_count = 0;
		op_sent_count++;
		if (INET_trace & TRACE_operations) {
			ib_fprintf(ib_stdout, "%05lu: OP Sent %5lu opcode %d\n",
					   inet_debug_timer(),
					   op_sent_count, packet->p_operation);
			ib_fflush(ib_stdout);
		};
	};
#endif

	return xdrEndOfRecord(&port_send, TRUE);
}

XDR_INT PortInet::sendPartial(Packet* packet)
{
#ifdef DEBUG
	{
		static ULONG op_sentp_count = 0;
		op_sentp_count++;
		if (INET_trace & TRACE_operations) {
			ib_fprintf(ib_stdout, "%05lu: OP Sent %5lu opcode %d (partial)\n",
					   inet_debug_timer(),
					   op_sentp_count, packet->p_operation);
			ib_fflush(ib_stdout);
		};
	};
#endif

	return xdr_protocol(&port_send, packet);
}

Port* PortInet::connect(Packet* packet, void(*ast)(Port*))
{
	SOCKET n;
	struct sockaddr_in address;

	/* If this is a server, we're got an auxiliary connection.  Accept it */

	if (port_server_flags) 
		{
		socklen_t l = sizeof(address);
		n = ::accept(port_channel, (struct sockaddr *) &address, &l);
		if (n == INVALID_SOCKET) 
			{
			error("accept", isc_net_event_connect_err, ERRNO);
			closeSocket(port_channel);
			return NULL;
			}
		closeSocket(port_channel);
		port_handle = n;
		port_flags |= PORT_async;
		return this;
		}

	PortInet *new_port = allocPort(configuration, (PortInet*) port_parent);
	port_async = new_port;
	new_port->port_dummy_packet_interval = port_dummy_packet_interval;
	new_port->port_dummy_timeout = new_port->port_dummy_packet_interval;

	new_port->port_flags |= PORT_async;
	P_RESP* response = &packet->p_resp;

	/* Set up new socket */

	if ((n = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) 
		{
		error("socket", isc_net_event_connect_err, ERRNO);
		return NULL;
		}

	memset(&address, 0, sizeof(address));
	memcpy (&address, response->p_resp_data.cstr_address, response->p_resp_data.cstr_length);
	/***
	inet_copy(reinterpret_cast<const char*>(response->p_resp_data.cstr_address),
			  (SCHAR *) & address, response->p_resp_data.cstr_length);
	***/
	address.sin_family = AF_INET;

	int status = ::connect(n, (struct sockaddr *) &address, sizeof(address));

	if (status < 0) 
		{
		error("connect", isc_net_event_connect_err, ERRNO);
		closeSocket(n);
		return NULL;
		}

#ifdef SIOCSPGRP
	if (ast)
		{
		int arg;
#ifdef HAVE_GETPGRP
		arg = getpgrp();
#else
		arg = getpid();
#endif
		if (ioctl(n, SIOCSPGRP, &arg) < 0) 
			{
			error("ioctl/SIOCSPGRP", isc_net_event_connect_err, ERRNO);
			closeSocket(port_channel);
			return NULL;
			}

		new_port->port_ast = ast;
		ISC_signal(SIGURG, signalHandler, new_port);
		}
#endif /* SIOCSPGRP */

	new_port->port_handle = n;
	new_port->port_flags |= port_flags & PORT_no_oob;

	return new_port;
}

Port* PortInet::auxRequest(Packet* packet)
{
	SOCKET n;
	struct sockaddr_in address;

	/* Set up new socket */

	address.sin_family = AF_INET;
	in_addr bind_addr = get_bind_address(configuration);
	memcpy (&address.sin_addr, &bind_addr, sizeof(address.sin_addr));
	/***
	inet_copy(reinterpret_cast<const SCHAR*>(&bind_addr),
				(SCHAR*) &address.sin_addr,
				sizeof(address.sin_addr));
	***/
				
	//address.sin_port = htons(Config::getRemoteAuxPort());
	address.sin_port = configuration->getValue (RemoteAuxPort,RemoteAuxPortValue);

	if ((n = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) 
		{
		error("socket", isc_net_event_listen_err, ERRNO);
		return NULL;
		}

	int optval;
	setsockopt(n, SOL_SOCKET, SO_REUSEADDR,
			   (SCHAR *) &optval, sizeof(optval));

	if (bind(n, (struct sockaddr *) &address, sizeof(address)) < 0) 
		{
		error("bind", isc_net_event_listen_err, ERRNO);
		return NULL;
		}

	socklen_t length = sizeof(address);

	if (getsockname(n, (struct sockaddr *) &address, &length) < 0) 
		{
		error("getsockname", isc_net_event_listen_err, ERRNO);
		return NULL;
		}

	if (listen(n, 1) < 0) 
		{
		error("listen", isc_net_event_listen_err, ERRNO);
		return NULL;
		}

    PortInet *new_port = allocPort(configuration, (PortInet*) port_parent);
	port_async = new_port;
	new_port->port_dummy_packet_interval = port_dummy_packet_interval;
	new_port->port_dummy_timeout = new_port->port_dummy_packet_interval;

	new_port->port_server_flags = port_server_flags;
	new_port->port_channel = (int) n;
	new_port->port_flags = port_flags & PORT_no_oob;

	P_RESP* response = &packet->p_resp;
	struct sockaddr_in port_address;
	
	if (getsockname((SOCKET) port_handle, (struct sockaddr *) &port_address, &length) < 0) 
		{
		error("getsockname", isc_net_event_listen_err, ERRNO);
		return NULL;
		}
		
	inet_copy(reinterpret_cast<SCHAR*>(&port_address.sin_addr),
				(SCHAR*) &address.sin_addr,
				sizeof(address.sin_addr));

	response->p_resp_data.cstr_address = (UCHAR *) & response->p_resp_blob_id;
	response->p_resp_data.cstr_length = sizeof(response->p_resp_blob_id);
	inet_copy(reinterpret_cast<const SCHAR*>(&address),
			  reinterpret_cast<char*>(response->p_resp_data.cstr_address),
			  response->p_resp_data.cstr_length);

	return new_port;
}

/***
void PortInet::unhookPort(Port* parent)
{
	for (Port** ptr = &parent->port_clients; *ptr; ptr = &(*ptr)->port_next) 
		if (*ptr == this) 
			{
			*ptr = port_next;
			if (ptr == &parent->port_clients)
				parent->port_next = *ptr;
			break;
			}
}
***/

void PortInet::exitHandler(void* arg)
{
	PortInet *main_port = (PortInet*) arg;

#ifdef WIN_NT
	if (!main_port) {
		WSACleanup();
		return;
	}
#endif

#ifndef VMS
	for (PortInet *port = main_port; port; port = (PortInet*) port->port_next) 
		{
		shutdown((int) port->port_handle, 2);
		closeSocket((SOCKET) port->port_handle);
		}
#endif
}

PortInet* PortInet::selectPort(slct* selct)
{
	PortInet *port;

#ifdef WIN_NT

	/* NT's socket handles are addresses */
	/* TMN: No, they are "black-box" handles. */

	//START_PORT_CRITICAL;
	
#ifdef  DEFER_PORT_CLEANUP
	unhook_disconnected_ports(this);
#endif

	Sync sync(&syncObject, "PortInet::selectPort");
	sync.lock(Shared);
	
	for (port = this; port; port = (PortInet*) port->port_next) 
		{
		if (FD_ISSET(port->port_handle, &selct->slct_fdset)) 
			{
			port->port_dummy_timeout = port->port_dummy_packet_interval;
			FD_CLR((SOCKET) port->port_handle, &selct->slct_fdset);
			--selct->slct_count;
			port->addRef();
			//STOP_PORT_CRITICAL;
			return port;
			}
		else if (port->port_dummy_timeout < 0) 
			{
			port->addRef();
			//STOP_PORT_CRITICAL;
			return port;
			}
		}

	//STOP_PORT_CRITICAL;
#else // !defined(WIN_NT)
	//START_PORT_CRITICAL;
	
#ifdef  DEFER_PORT_CLEANUP
	unhook_disconnected_ports(main_port);
#endif

	for (port = this; port; port = (PortInet*) port->port_next) 
		{
		const int n = (int) port->port_handle;
		
		if (n < selct->slct_width && FD_ISSET(n, &selct->slct_fdset)) 
			{
			port->port_dummy_timeout = port->port_dummy_packet_interval;
			FD_CLR(n, &selct->slct_fdset);
			--selct->slct_count;
			port->addRef();
			//STOP_PORT_CRITICAL;
			return port;
			}
		else if (port->port_dummy_timeout < 0) 
			{
			port->addRef();
			//STOP_PORT_CRITICAL;
			return port;
			}
		}
		
	//STOP_PORT_CRITICAL;
#endif

	return NULL;
}

PortInet* PortInet::selectAccept(void)
{
	struct sockaddr_in address;
	PortInet *port = allocPort(configuration, this);
	socklen_t l = sizeof(address);

	port->port_handle = ::accept(port_handle, (struct sockaddr *) &address, &l);
										
	if ((SOCKET) port->port_handle == INVALID_SOCKET) 
		{
		error("accept", isc_net_connect_err, ERRNO);
		port->disconnect();
		return NULL;
		}

	int optval = 1;
	setsockopt((SOCKET) port->port_handle, SOL_SOCKET, SO_KEEPALIVE,
			   (SCHAR*) &optval, sizeof(optval));

/***
#if !(defined SUPERSERVER || defined VMS || defined WIN_NT)
	int n;
	
	for (n = 0, port = (PortInet*) port_clients; port; n++, port = (PortInet*) port->port_next)
		;
		
	if (n >= INET_max_clients) 
		{
		port_state = state_closed;
		closeSocket((int) port_handle);
		TEXT msg[64];
		sprintf(msg,
				"INET/select_accept: exec new server at client limit: %d", n);
		gds__log(msg, 0);

		setreuid(0, 0);
		kill(getppid(), SIGUSR1);
		}
#endif
***/

	if (port_server_flags & SRVR_thread_per_port) 
		{
		port->port_server_flags =
			(SRVR_server | SRVR_inet | SRVR_thread_per_port);
		return port;
		}

	return NULL;
}

int PortInet::selectWait(slct* selct)
{
	TEXT msg[64];
	struct timeval timeout;

	for (;;)
		{
		selct->slct_count = selct->slct_width = 0;
		FD_ZERO(&selct->slct_fdset);
		bool found = false;

		/* Use the time interval between select() calls to expire
		   keepalive timers on all ports. */

		SLONG delta_time;
		
		if (selct->slct_time)
			{
			delta_time = (SLONG) time(NULL) - selct->slct_time;
			selct->slct_time += delta_time;
			}
		else
			{
			delta_time = 0;
			selct->slct_time = (SLONG) time(NULL);
			}

		//START_PORT_CRITICAL;
		
#ifdef  DEFER_PORT_CLEANUP
		unhook_disconnected_ports(main_port);
#endif

		for (PortInet *port = this; port; port = (PortInet*) port->port_next)
			if ((port->port_state == state_active) || (port->port_state == state_pending))
				{
				/* Adjust down the port's keepalive timer. */

				if (port->port_dummy_packet_interval)
					port->port_dummy_timeout -= delta_time;

				FD_SET((SLONG) port->port_handle, &selct->slct_fdset);
#ifdef WIN_NT
				++selct->slct_width;
#else
				selct->slct_width = MAX(selct->slct_width, (int) port->port_handle);
#endif
				found = true;
				}
			
		//STOP_PORT_CRITICAL;

		if (!found)
			{
			gds__log("INET/select_wait: client rundown complete, server exiting", 0);
			return FALSE;
			}

		++selct->slct_width;

		for (;;)
			{
			// Some platforms change the timeout in the select call.
			// Reset timeout for each iteration to avoid problems.
			timeout.tv_sec = SELECT_TIMEOUT;
			timeout.tv_usec = 0;

#ifdef WIN_NT
			selct->slct_count = select(FD_SETSIZE, &selct->slct_fdset, NULL, NULL, &timeout);
#else
			selct->slct_count = select(selct->slct_width, &selct->slct_fdset, NULL, NULL, &timeout);
#endif

			if (selct->slct_count != -1)
				{
				/* if selct->slct_count is zero it means that we timed out of
				   select with nothing to read or accept, so clear the fd_set
				   bit as this value is undefined on some platforms (eg. HP-UX),
				   when the select call times out. Once these bits are cleared
				   they can be used in select_port() */
				   
				if (selct->slct_count == 0)
					for (PortInet* port = this; port; port = (PortInet*) port->port_next)
						FD_CLR(port->port_handle, &selct->slct_fdset);

				return TRUE;
				}
			else if (INTERRUPT_ERROR(ERRNO))
				continue;
			else if (ERRNO == BAD_SOCKET)
				break;
			else 
				{
				//THREAD_ENTER;
				sprintf(msg, "INET/select_wait: select failed, errno = %d", ERRNO);
				gds__log(msg, 0);
				
				return FALSE;
				}
			}
		}
}

PortInet* PortInet::allocPort(ConfObject* configuration, PortInet* parent)
{
	TEXT buffer[64];

#ifdef WIN_NT
	if (!initialized) 
		{
	    const WORD version = MAKEWORD(2, 0);
		WSADATA INET_wsadata;
		if (WSAStartup(version, &INET_wsadata)) 
			{
			if (parent)
				parent->error("WSAStartup", isc_net_init_error, ERRNO);
			else 
				{
				sprintf(buffer,
						"INET/alloc_port: WSAStartup failed, error code = %d",
						ERRNO);
				gds__log(buffer, 0);
				}
			return NULL;
			}
			
		gds__register_cleanup(exitHandler, 0);
		initialized = true;
		}
#endif

	int INET_remote_buffer = configuration->getValue(TcpRemoteBufferSize,TcpRemoteBufferSizeValue);

	if (INET_remote_buffer < MAX_DATA_LW || INET_remote_buffer > MAX_DATA_HW)
		INET_remote_buffer = DEF_MAX_DATA;

	int INET_max_data = INET_remote_buffer;
	
#ifdef DEBUG
		{
		char messg[128];
		sprintf(messg, " Info: Remote Buffer Size set to %ld",
				INET_remote_buffer);
		gds__log(messg, 0);
		}
#endif
		

	PortInet *port = new PortInet (INET_remote_buffer * 2);
	port->port_type = port_inet;
	port->port_state = state_pending;
	port->configuration = configuration;
	REMOTE_get_timeout_params(port, 0, 0);

	if (parent)
		port->configuration = parent->configuration;
		
	gethostname(buffer, sizeof(buffer));
	port->port_host = buffer; 
	port->port_connection =  buffer;
	port->port_version.Format("tcp (%s)", (const char*) port->port_host);

	if (parent && !(parent->port_server_flags & SRVR_thread_per_port)) 
		{
		parent->addClient (port);
		port->port_handle = parent->port_handle;
		port->port_server = parent->port_server;
		port->port_server_flags = parent->port_server_flags;
		}
		
	port->port_buff_size = (USHORT) INET_remote_buffer;

	port->xdrCreate(&port->port_send, &port->port_buffer[INET_remote_buffer],
					INET_remote_buffer, XDR_ENCODE);

	port->xdrCreate(&port->port_receive, port->port_buffer, 0, XDR_DECODE);

	return port;
}

PortInet* PortInet::auxConnect(Packet* packet, void (*ast)(Port*))
{
/**************************************
 *
 *	a u x _ c o n n e c t
 *
 **************************************
 *
 * Functional description
 *	Try to establish an alternative connection.  Somebody has already
 *	done a successfull connect request ("packet" contains the response).
 *
 **************************************/
	SOCKET n;
	struct sockaddr_in address;

	/* If this is a server, we're got an auxiliary connection.  Accept it */

	if (port_server_flags) 
		{
		socklen_t l = sizeof(address);
		n = ::accept(port_channel, (struct sockaddr *) &address, &l);
		
		if (n == INVALID_SOCKET) 
			{
			error("accept", isc_net_event_connect_err, ERRNO);
			closeSocket(port_channel);
			return NULL;
			}
			
		closeSocket(port_channel);
		port_handle = n;
		port_flags |= PORT_async;
		return this;
		}

	PortInet *new_port = allocPort(port_parent->configuration, (PortInet*) port_parent);
	port_async = new_port;
	new_port->port_dummy_packet_interval = port_dummy_packet_interval;
	new_port->port_dummy_timeout = port_dummy_timeout;

	new_port->port_flags |= PORT_async;
	P_RESP* response = &packet->p_resp;

/* Set up new socket */

	if ((n = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) 
		{
		error("socket", isc_net_event_connect_err, ERRNO);
		return NULL;
		}

	/***
	inet_zero((SCHAR *) & address, sizeof(address));
	inet_copy(reinterpret_cast<const char*>(response->p_resp_data.cstr_address),
			  (SCHAR *) & address, response->p_resp_data.cstr_length);
	***/
	memset (&address, 0, sizeof (address));
	memcpy (&address, response->p_resp_data.cstr_address, response->p_resp_data.cstr_length);
	address.sin_family = AF_INET;

	int status = ::connect(n, (struct sockaddr *) &address, sizeof(address));

	if (status < 0) 
		{
		error("connect", isc_net_event_connect_err, ERRNO);
		closeSocket(n);
		return NULL;
		}

#ifdef SIOCSPGRP
	if (ast)
	{
		int arg;
#ifdef HAVE_GETPGRP
		arg = getpgrp();
#else
		arg = getpid();
#endif
		if (ioctl(n, SIOCSPGRP, &arg) < 0) {
			error("ioctl/SIOCSPGRP", isc_net_event_connect_err,
					   ERRNO);
			closeSocket(port_channel);
			return NULL;
		}

		new_port->port_ast = ast;
		ISC_signal(SIGURG, signalHandler, new_port);
	}
#endif /* SIOCSPGRP */

	new_port->port_handle = n;
	new_port->port_flags |= port_flags & PORT_no_oob;

	return new_port;
}

int PortInet::error(const char* function, ISC_STATUS operation, int status)
{
	TEXT msg[64];

#ifdef WIN_NT
	int code = WSAGetLastError();
#endif

	if (status) 
		{
#ifdef VMS
		if ((status >= sys_nerr || !sys_errlist[status]) &&
			status < WIN_NERR && win_errlist[status])
			genError(isc_network_error,
						   isc_arg_string,
						   (ISC_STATUS) port_connection->str_data,
						   isc_arg_gds, operation, isc_arg_string,
						   (ISC_STATUS) win_errlist[status], 0);
		else
#endif
			genError(isc_network_error,
						   isc_arg_string,
						   (const char*) port_connection,
						   isc_arg_gds, operation, SYS_ERR, status, 0);

		sprintf(msg, "INET/inet_error: %s errno = %d", function, status);
		gds__log(msg, 0);
		}
	else 
		{
		/* No status value, just format the basic arguments. */

		genError(isc_network_error, isc_arg_string, (const char*) port_connection, 
				 isc_arg_gds, operation, 0);
		}

	return 0;
}

void PortInet::genError(ISC_STATUS status, ...)
{
	Sync sync (&syncRequest, "PortInet::genError");
	sync.lock(Exclusive);
	port_flags |= PORT_broken;
	port_state = state_broken;

	ISC_STATUS* status_vector = NULL;
	
	if (port_context != NULL)
		status_vector = port_context->rdb_status_vector;
	
	if (status_vector == NULL) 
		status_vector = port_status_vector;
	
	if (status_vector != NULL) 
		{
		STUFF_STATUS(status_vector, status);
		REMOTE_save_status_strings(status_vector);
		}
}

int PortInet::xdrCreate(xdr_t* xdrs, UCHAR* buffer, int length, xdr_op x_op)
{
	xdrs->x_public = (caddr_t) this;
	xdrs->x_base = xdrs->x_private = (SCHAR *) buffer;
	xdrs->x_handy = length;
	xdrs->x_ops = (xdr_t::xdr_ops *) & inet_ops;
	xdrs->x_op = x_op;

	return TRUE;
}


static bool_t inet_read( XDR * xdrs)
{
/**************************************
 *
 *	i n e t _ r e a d
 *
 **************************************
 *
 * Functional description
 *	Read a buffer full of data.  If we receive a bad packet,
 *	send the moral equivalent of a NAK and retry.  ACK all
 *	partial packets.  Don't ACK the last packet -- the next
 *	message sent will handle this.
 *
 **************************************/
	PortInet *port = (PortInet*) xdrs->x_public;
	char* p = xdrs->x_base;
	const char* const end = p + port->port_buff_size;

	/* If buffer is not completely empty, slide down what's left */

	if (xdrs->x_handy > 0) 
		{
		memmove(p, xdrs->x_private, xdrs->x_handy);
		p += xdrs->x_handy;
		}

	/* If an ACK is pending, do an ACK.  The alternative is deadlock. */

	/*
	if (port->port_flags & PORT_pend_ack)
		if (!packet_send (port, 0, 0))
		return FALSE;
	*/

	while (true) 
		{
		short length = end - p;
		if (!port->receive ((UCHAR*) p, length, &length))
			{
			return FALSE;
			/***
			if (!packet_send (port, 0, 0))
				return FALSE;
			continue;
			***/
			}
		if (length >= 0) 
			{
			p += length;
			break;
			}
		p -= length;
		if (!port->send (NULL, 0))
			return FALSE;
		}

	port->port_flags |= PORT_pend_ack;
	xdrs->x_handy = (int) ((SCHAR *) p - xdrs->x_base);
	xdrs->x_private = xdrs->x_base;

	return TRUE;
}


static bool_t inet_write( XDR * xdrs, bool_t end_flag)
{
/**************************************
 *
 *	i n e t _ w r i t e
 *
 **************************************
 *
 * Functional description
 *	Write a buffer full of data.  If the end_flag isn't set, indicate
 *	that the buffer is a fragment, and reset the XDR for another buffer
 *	load.
 *
 **************************************/
	/* Encode the data portion of the packet */

	PortInet *port = (PortInet*) xdrs->x_public;
	const char* p = xdrs->x_base;
	SSHORT length = xdrs->x_private - p;

	/* Send data in manageable hunks.  If a packet is partial, indicate
	   that with a negative length.  A positive length marks the end. */

	//p = xdrs->x_base; redundant

	while (length) 
		{
		port->port_misc1 = (port->port_misc1 + 1) % MAX_SEQUENCE;
		int l = MIN(length, port->port_buff_size);
		length -= l;
		
		//if (!packet_send(port, p, (SSHORT) ((length) ? -l : l)))
		if (!port->send(p, (length) ? -l : l))
			return FALSE;
			
		p += l;
		}

	xdrs->x_private = xdrs->x_base;
	xdrs->x_handy = port->port_buff_size;

	return TRUE;

#ifdef PIGGYBACK
// CVC: Screwed logic here: if I initialize l2 to zero, nothing useful executes.
	SCHAR aux_buffer[BUFFER_SIZE];
	SSHORT l2 = 0;
#error Assign l2 some meaningful value before running this.
/* If the other end has not piggy-backed the next packet, we're done. */

	if (!l2)
		return TRUE;

/* We've got a piggy-backed response.  If the packet is partial,
   send an ACK for part we did receive. */

	char* p2 = aux_buffer;

	while (l2 < 0) {
		if (!packet_send(port, 0, 0))
			return FALSE;
		p2 -= l2;
		length = aux_buffer + sizeof(aux_buffer) - p2;
		if (!packet_receive(port, p2, length, &l2)) {
			p2 += l2;
			continue;
		}
	}

	length = p2 - aux_buffer + l2;

/* Now we're got a encode glump ready to stuff into the read buffer.
   Unfortunately, if we just add it to the read buffer, we will shortly
   overflow the buffer.  To avoid this, "scrumpf down" the active bits
   in the read buffer, then add out stuff at the end. */

	xdrs = &port->port_receive;
	p2 = xdrs->x_base;

	if (xdrs->x_handy && p2 != xdrs->x_private) {
		memmove(p2, xdrs->x_private, xdrs->x_handy);
	}

	p2 += xdrs->x_handy;

	xdrs->x_private = xdrs->x_base;
/*
xdrs->x_handy += JAP_decode (aux_buffer, length, p2);
*/
	port->port_flags |= PORT_pend_ack;

	return TRUE;
#endif
}

int PortInet::receive(UCHAR* buffer, int buffer_length, short* length)
{
#ifndef REQUESTER

	/* set the time interval for sending dummy packets to the client */

	timeval timeout;
	timeout.tv_sec = port_dummy_packet_interval;
	timeout.tv_usec = 0;

	timeval* time_ptr = NULL;
	
	if (port_protocol >= PROTOCOL_VERSION8 && port_dummy_packet_interval > 0)
		time_ptr = &timeout;

	/* If the protocol is 0 we are still in the process of establishing
	   a connection. Add a time out to the wait */
	   
	if (port_protocol == 0)
		{
		timeout.tv_sec = port_connect_timeout;
		time_ptr = &timeout;
		}

	// on Linux systems (and possibly others too) select will eventually
	// change timout values so save it here for later reuse.
	// Thanks to Brad Pepers who reported this bug  FSG 3 MAY 2001
	
	timeval savetime = timeout;

	int ph = (int) port_handle;
	// Unsed to send a dummy packet, but too big to be defined in the loop.
	Packet packet;
#endif

#ifdef VMS
	if (ISC_tcp_gettype() == TCP_TYPE_MULTINET)
		ph = ph / 16;
#endif

	int n = 0;

	for (;;)
		{

#ifndef REQUESTER

		/* Implement an error-detection protocol to ensure that the client
		   is still there.  Use the select() call with a timeout to wait on
		   the connection for an incoming packet.  If none comes within a
		   suitable time interval, write a dummy packet on the connection.
		   If the client is not there, an error will be returned on the write.
		   If the client is there, the dummy packet will be ignored by all
		   InterBase clients V4 or greater.  This protocol will detect when
		   clients are lost abnormally through reboot or network disconnect. */

		/* Don't send op_dummy packets on aux port; the server won't
		   read them because it only writes to aux ports. */

		if (port_flags & PORT_async)
			;
		else
#ifdef VMS
			if (ISC_tcp_gettype() == TCP_TYPE_MULTINET)
#endif
			{
			fd_set slct_fdset;
			FD_ZERO(&slct_fdset);
			FD_SET(ph, &slct_fdset);

			int slct_count;
			
			for (;;) 
				{
#if (defined WIN_NT)
				slct_count = select(FD_SETSIZE, &slct_fdset, NULL, NULL, time_ptr);
#else
				slct_count = select((SOCKET) port_handle + 1, &slct_fdset, NULL, NULL, time_ptr);
#endif

				// restore original timeout value FSG 3 MAY 2001
				savetime = timeout;

				if (slct_count != -1 || !INTERRUPT_ERROR(ERRNO))
					break;
				}

			if (slct_count == -1)
				return error("select in packet_receive", isc_net_read_err, ERRNO);

			if (!slct_count && port_protocol >= PROTOCOL_VERSION8)
				{
#ifdef DEBUG
				if (INET_trace & TRACE_operations)
					{
					ib_fprintf(ib_stdout, "%05lu: OP Sent: op_dummy\n",
							   inet_debug_timer());
					ib_fflush(ib_stdout);
					}
#endif
				packet.p_operation = op_dummy;
				//if (!send_full(port, &packet))
				if (!sendFull(&packet))
					return FALSE;
				continue;
				}

			if (!slct_count && port_protocol == 0)
				return FALSE;
			}
#endif /* REQUESTER */

		n = recv((SOCKET) port_handle,
				 reinterpret_cast<char*>(buffer), buffer_length, 0);

		if (n != -1 || !INTERRUPT_ERROR(ERRNO))
			break;
		}

	if (n <= 0 && (port_flags & PORT_async))
		return FALSE;

	if (n == -1)
		return error("read", isc_net_read_err, ERRNO);

	if (!n)
		return error("read end_of_file", isc_net_read_err, 0);

#ifndef REQUESTER
#ifdef DEBUG
	{
		INET_count_recv++;
		INET_bytes_recv += n;
		if (INET_trace & TRACE_packets)
			packet_print("receive", buffer, n, INET_count_recv);
		INET_force_error--;
		
		if (INET_force_error == 0) 
			{
			INET_force_error = 1;
			return inet_error(port, "simulated error - read",
							  isc_net_read_err, 0);
			}
		}
#endif
#endif

	*length = n;

	return TRUE;
}

bool PortInet::send(const SCHAR* buffer, int buffer_length)
{
#ifdef HAVE_SETITIMER
	struct itimerval internal_timer, client_timer;
	struct sigaction internal_handler, client_handler;
#endif /* HAVE_SETITIMER */

#ifdef SINIXZ
// Please systems with ill-defined send() function, like SINIX-Z.
	char* data = const_cast<char*>(buffer);
#else
	const char* data = buffer;
#endif
	SSHORT length = buffer_length;

	while (length) 
		{
#ifdef DEBUG
		if (INET_trace & TRACE_operations) 
			{
			ib_fprintf(ib_stdout, "Before Send\n");
			ib_fflush(ib_stdout);
			}
#endif
		int n = ::send((SOCKET) port_handle, data, length, 0);
		
#ifdef DEBUG
		if (INET_trace & TRACE_operations) 
			{
			ib_fprintf(ib_stdout, "After Send n is %d\n", n);
			ib_fflush(ib_stdout);
			}
#endif
		if (n == length) 
			break;

		if (n == -1)
			{
			if (INTERRUPT_ERROR(ERRNO))
				continue;

			return error("send", isc_net_write_err, ERRNO);
			}

		data += n;
		length -= n;
		}

	if ((port_flags & PORT_async) && !(port_flags & PORT_no_oob))
		{
		int count = 0;
		SSHORT n;
#ifdef SINIXZ
		char* b = const_cast<char*>(buffer);
#else
		const char* b = buffer;
#endif

		while ((n = ::send((SOCKET) port_handle, b, 1, MSG_OOB)) == -1 &&
				(ERRNO == ENOBUFS || INTERRUPT_ERROR(ERRNO)))
			{
			if (count++ > 20)
				break;

#ifndef HAVE_SETITIMER
#ifdef WIN_NT
			SleepEx(50, TRUE);
#else
			sleep(1);
#endif
			} // end of while() loop for systems without setitimer.
#else /* HAVE_SETITIMER */

			if (count == 1)
				{
				/* Wait in a loop until the lock becomes available */

				internal_timer.it_interval.tv_sec = 0;
				internal_timer.it_interval.tv_usec = 0;
				internal_timer.it_value.tv_sec = 0;
				internal_timer.it_value.tv_usec = 0;
				setitimer(ITIMER_REAL, &internal_timer, &client_timer);
				internal_handler.sa_handler = alarmHandler;
				sigemptyset(&internal_handler.sa_mask);
				internal_handler.sa_flags = SA_RESTART;
				sigaction(SIGALRM, &internal_handler, &client_handler);
				}

			internal_timer.it_value.tv_sec = 0;
			internal_timer.it_value.tv_usec = 50000;
			setitimer(ITIMER_REAL, &internal_timer, NULL);
			pause();
			} // end of while() loop for systems with setitimer

		if (count)
			{
			/* Restore user's outstanding alarm request and handler */

			internal_timer.it_value.tv_sec = 0;
			internal_timer.it_value.tv_usec = 0;
			setitimer(ITIMER_REAL, &internal_timer, NULL);
			sigaction(SIGALRM, &client_handler, NULL);
			setitimer(ITIMER_REAL, &client_timer, NULL);
			}
#endif /* HAVE_SETITIMER */

		if (n == -1)
			return error("send/oob", isc_net_write_err, ERRNO);
		}

#ifndef REQUESTER
#ifdef DEBUG
	{
	INET_count_send++;
	INET_bytes_send += buffer_length;
	if (INET_trace & TRACE_packets)
		packet_print("send", (const UCHAR*) buffer, buffer_length, INET_count_send);
	INET_force_error--;
	if (INET_force_error == 0) 
		{
		INET_force_error = 1;
		return error("simulated error - send",isc_net_write_err, 0);
		}
	}
#endif
#endif

	port_flags &= ~PORT_pend_ack;

	return TRUE;
}

int PortInet::sendFull(Packet* packet)
{
	if (!xdr_protocol(&port_send, packet))
		return FALSE;

#ifdef DEBUG
	{
		static ULONG op_sent_count = 0;
		op_sent_count++;
		
		if (INET_trace & TRACE_operations) 
			{
			ib_fprintf(ib_stdout, "%05lu: OP Sent %5lu opcode %d\n",
					   inet_debug_timer(),
					   op_sent_count, packet->p_operation);
			ib_fflush(ib_stdout);
			};
	};
#endif

	return xdrEndOfRecord(&port_send, TRUE);
}

bool PortInet::xdrEndOfRecord(xdr_t* xdrs, bool flushnow)
{
	return inet_write(xdrs, flushnow);
}

static in_addr get_bind_address(ConfObject *configuration)
{
/**************************************
 *
 *	g e t _ b i n d _ a d d r e s s
 *
 **************************************
 *
 * Functional description
 *	Return local address to bind sockets to.
 *
 **************************************/
	in_addr config_address;

	//const char* config_option = Config::getRemoteBindAddress();
	const char* config_option = configuration->getValue(RemoteBindAddress,RemoteBindAddressValue);
	config_address.s_addr = (config_option) ? inet_addr(config_option) : INADDR_NONE;

	if (config_address.s_addr == INADDR_NONE) 
		config_address.s_addr = INADDR_ANY;
		
	return config_address;
}
static bool_t inet_getbytes( XDR * xdrs, SCHAR * buff, u_int count)
{
/**************************************
 *
 *	i n e t _ g e t b y t e s
 *
 **************************************
 *
 * Functional description
 *	Get a bunch of bytes from a memory stream if it fits.
 *
 **************************************/
	SLONG bytecount = count;

	/* Use memcpy to optimize bulk transfers. */

	while (bytecount > (SLONG) sizeof(ISC_QUAD))
		{
		if (xdrs->x_handy >= bytecount) 
			{
			memcpy(buff, xdrs->x_private, bytecount);
			xdrs->x_private += bytecount;
			xdrs->x_handy -= bytecount;
			return TRUE;
			}
		else 
			{
			if (xdrs->x_handy > 0) 
				{
				memcpy(buff, xdrs->x_private, xdrs->x_handy);
				xdrs->x_private += xdrs->x_handy;
				buff += xdrs->x_handy;
				bytecount -= xdrs->x_handy;
				xdrs->x_handy = 0;
				}
			if (!inet_read(xdrs))
				return FALSE;
			}
		}

	/* Scalar values and bulk transfer remainder fall thru
	   to be moved byte-by-byte to avoid memcpy setup costs. */

	if (!bytecount)
		return TRUE;

	if (xdrs->x_handy >= bytecount) 
		{
		xdrs->x_handy -= bytecount;
		do
			*buff++ = *xdrs->x_private++;
		while (--bytecount);
		return TRUE;
		}

	while (--bytecount >= 0) 
		{
		if (!xdrs->x_handy && !inet_read(xdrs))
			return FALSE;
		*buff++ = *xdrs->x_private++;
		--xdrs->x_handy;
		}

	return TRUE;
}

static void copy_p_cnct_repeat_array(	p_cnct::p_cnct_repeat*			pDest,
										const p_cnct::p_cnct_repeat*	pSource,
										size_t							nEntries)
{
	for (size_t i=0; i < nEntries; ++i) 
		pDest[i] = pSource[i];
}

static PortInet* inet_try_connect(ConfObject *configuration,
								Packet		*packet,
								RDatabase	*rdb,
								USHORT		file_length,
								TEXT*		file_name,
								const TEXT* node_name, 
								ISC_STATUS* status_vector,
								const UCHAR* dpb, 
								SSHORT		dpb_length)
{
/**************************************
 *
 *	i n e t _ t r y _ c o n n e c t
 *
 **************************************
 *
 * Functional description
 *	Given a packet with formatted protocol infomation,
 *	set header information and try the connection.
 *
 *	If a connection is established, return a port block, otherwise
 *	return NULL.
 *
 **************************************/
	int n = sizeof (*rdb);
	P_CNCT* cnct = &packet->p_cnct;
	packet->p_operation = op_connect;
	cnct->p_cnct_operation = op_attach;
	cnct->p_cnct_cversion = CONNECT_VERSION2;
	cnct->p_cnct_client = ARCHITECTURE;
	cnct->p_cnct_file.cstr_length = file_length;
	cnct->p_cnct_file.cstr_address = (UCHAR *) file_name;

	/* If we can't talk to a server, punt.  Let somebody else generate
	   an error.  status_vector will have the network error info. */

	PortInet* port = (PortInet*) INET_connect(node_name, configuration, packet, status_vector, FALSE, dpb, dpb_length);

	if (!port) 
		{
		rdb->release();
		return NULL;
		}

	/* Get response packet from server. */

	rdb->rdb_port = port;
	port->port_context = rdb;
	
	if (!port->receive(packet)) 
		{
		port->error("receive in try_connect", isc_net_connect_err, ERRNO);
		port->disconnect();
		rdb->release();
		return NULL;
		}

	return port;
}

Port* INET_connect(const TEXT* name,
				  ConfObject *configuration, 
				  Packet* packet,
				  ISC_STATUS* status_vector,
				  USHORT flag, 
				  const UCHAR* dpb, 
				  SSHORT dpb_length)
{
/**************************************
 *
 *	I N E T _ c o n n e c t
 *
 **************************************
 *
 * Functional description
 *	Establish half of a communication link.  If a connect packet is given,
 *	the connection is on behalf of a remote interface.  Otherwise the connect
 *	is for a server process.
 *
 **************************************/
	socklen_t l;
    int n;
	SOCKET s;
	TEXT temp[128];
	struct sockaddr_in address;
#ifndef VMS
	struct servent *service;
	TEXT msg[64];
#endif

#ifdef DEBUG
	{
	if (INET_trace & TRACE_operations) 
		{
		ib_fprintf(ib_stdout, "INET_connect\n");
		ib_fflush(ib_stdout);
		};
	INET_start_time = inet_debug_timer();
	const char* p = getenv("INET_force_error");
	if (p != NULL)
		INET_force_error = atoi(p);
	}
#endif

	PortInet *port = PortInet::allocPort(configuration, NULL);
	port->port_status_vector = status_vector;
	REMOTE_get_timeout_params(port, reinterpret_cast<const UCHAR*>(dpb),
							  dpb_length);
	status_vector[0] = isc_arg_gds;
	status_vector[1] = 0;
	status_vector[2] = isc_arg_end;
	
#ifdef VMS
	ISC_tcp_setup(ISC_wait, gds__completion_ast);
#endif

	const TEXT* protocol = NULL;

	if (name) 
		{
		strcpy(temp, name);
		for (TEXT* p = temp; *p;) 
			{
			if (*p++ == '/') 
				{
				p[-1] = 0;
				name = temp;
				protocol = p;
				break;
				}
			}
		}

	if (name && *name) 
		{
		/***
		if (port->port_connection) 
			ALLR_free(port->port_connection);
		***/
		port->port_connection = name; //REMOTE_make_string(name);
		}
	else 
		name = port->port_host;

	JString svc;
	
	if (!protocol) 
		{
		int port = configuration->getValue (RemoteServicePort,RemoteServicePortValue);
		if (port) 
			{
			snprintf(temp, sizeof(temp), "%d", port);
			protocol = temp;
			}
		else
			{
			svc = configuration->getValue (RemoteServiceName,RemoteServiceNameValue);
			protocol = svc;
			}
		}

/* Set up Inter-Net socket address */

	inet_zero((SCHAR *) &address, sizeof(address));

#ifdef VMS
	/* V M S */
	
	if (getservport(protocol, "tcp", &address.sin_port) == -1) 
		{
		inet_error(port, "getservbyname", isc_net_connect_err, 0);
		disconnect(port);
		return NULL;
		}
		
	if (packet) 
		{
		if (getaddr(name, &address) == -1) 
			{
			inet_error(port, "gethostbyname", isc_net_connect_err, 0);
			disconnect(port);
			return NULL;
			}
		}
	else 
		address.sin_addr.s_addr = INADDR_ANY;

#else

/* U N I X style sockets */

	address.sin_family = AF_INET;

	in_addr host_addr;

	if (packet) 
		{
		// client connection
		host_addr = get_host_address(name);

		if (host_addr.s_addr == INADDR_NONE) 
			{
			sprintf(msg,
					"INET/INET_connect: gethostbyname failed, error code = %d",
					H_ERRNO);
			gds__log(msg, 0);
			port->genError(isc_network_error, isc_arg_string, (const char*) port->port_connection,
						   isc_arg_gds, isc_net_lookup_err, isc_arg_gds, isc_host_unknown, 0);

			port->disconnect();
			return NULL;
			}
		}
	else 
		host_addr = get_bind_address(configuration);

	inet_copy(reinterpret_cast<const SCHAR*>(&host_addr),
				(SCHAR*) &address.sin_addr,
				sizeof(address.sin_addr));

	{
#ifdef WIN_NT
	Sync sync (&getServiceMutex, "INET_connect");
	sync.lock (Exclusive);
#endif
	service = getservbyname(protocol, "tcp");
	}
	

	/* Modification by luz (slightly modified by FSG)
		instead of failing here, try applying hard-wired
		translation of "gds_db" into "3050"
		This way, a connection to a remote FB server
		works even from clients with missing "gds_db"
		entry in "services" file, which is important
		for zero-installation clients.
		*/
		
	if (!service) 
		{
		if (strcmp(protocol, FB_SERVICE_NAME) == 0) 
			address.sin_port = htons(FB_SERVICE_PORT);	/* apply hardwired translation */
		else 
			{
			/* modification by FSG 23.MAR.2001 */
			/* The user has supplied something as protocol
			 * let's see whether this is a port number
			 * instead of a service name
			 */
			address.sin_port = htons(atoi(protocol));
			}

		if (address.sin_port == 0) 
			{
			/* end of modification by FSG */
			/* this is the original code */
			sprintf(msg, "INET/INET_connect: getservbyname failed, error code = %d", H_ERRNO);
			gds__log(msg, 0);
			port->genError(isc_network_error,isc_arg_string, (const char*) port->port_connection,
						   isc_arg_gds,isc_net_lookup_err,
						   isc_arg_gds,isc_service_unknown,isc_arg_string,protocol, isc_arg_string, "tcp", 
						   0);
			return NULL;
			} 
		}
	else 
		{
		/* if we have got a service-struct, get port number from there
		   * (in case of hardwired gds_db to 3050 translation, address.sin_port was
		   * already set above */
		address.sin_port = service->s_port;
		}							/* else (service found) */

	/* end of modifications by luz */
#endif /* VMS */

/* Allocate a port block and initialize a socket for communications */

	port->port_handle = socket(AF_INET, SOCK_STREAM, 0);

	if ((SOCKET) port->port_handle == INVALID_SOCKET)
		{
		port->error("socket", isc_net_connect_err, ERRNO);
		port->disconnect();
		return NULL;
		}

	/* If we're a host, just make the connection */

	if (packet) 
		{
		n = ::connect((SOCKET) port->port_handle,
					(struct sockaddr *) &address, sizeof(address));
		if (n != -1 && port->sendFull(packet))
			return port;
		port->error("connect", isc_net_connect_err, ERRNO);
		port->disconnect();
		return NULL;
		}

	/* We're a server, so wait for a host to show up */

	if (flag & SRVR_multi_client) 
		{
		socklen_t optlen;
		struct linger lingerInfo;

		lingerInfo.l_onoff = 0;
		lingerInfo.l_linger = 0;

		int optval = TRUE;
		n = setsockopt((SOCKET) port->port_handle, SOL_SOCKET, SO_REUSEADDR,
					   (SCHAR*) &optval, sizeof(optval));
		if (n == -1) 
			{
			port->error("setsockopt REUSE", isc_net_connect_listen_err,
					   ERRNO);
			port->disconnect();
			return NULL;
			}

		/* Get any values for SO_LINGER so that they can be reset during
		 * disconnect.  SO_LINGER should be set by default on the socket
		 */
		optlen = sizeof(port->port_linger);
		n = getsockopt((SOCKET) port->port_handle, SOL_SOCKET, SO_LINGER,
					   (SCHAR*) &port->port_linger, &optlen);

		if (n != 0)				/* getsockopt failed */
			port->port_linger.l_onoff = 0;

		n = setsockopt((SOCKET) port->port_handle, SOL_SOCKET, SO_LINGER,
					   (SCHAR *) & lingerInfo, sizeof(lingerInfo));
		if (n == -1) 
			{
			port->error("setsockopt LINGER", isc_net_connect_listen_err,ERRNO);
			port->disconnect();
			return NULL;
			}

#ifdef SET_TCP_NO_DELAY

		bool noNagle = configuration->getValue (TcpNoNagle,TcpNoNagleValue);

		if (noNagle)
			{
			int optval = TRUE;
			n = setsockopt((SOCKET) port->port_handle, SOL_SOCKET,
						   TCP_NODELAY, (SCHAR*) &optval, sizeof(optval));
			gds__log("inet log: disabled Nagle algorithm \n");

			if (n == -1) 
				{
				port->error("setsockopt TCP_NODELAY",
						   isc_net_connect_listen_err, ERRNO);
				port->disconnect();
				return NULL;
				}
			}
#endif

	}

	n = bind((SOCKET) port->port_handle,
			 (struct sockaddr *) &address, sizeof(address));

	if (n == -1) 
		{
		/* On Linux platform, when the server dies the system holds a port
		   for some time. */

		if (ERRNO == INET_ADDR_IN_USE) 
			{
			for (int retry = 0; retry < INET_RETRY_CALL; retry++) 
				{
				sleep(10);
				n = bind((SOCKET) port->port_handle,
						 (struct sockaddr *) &address, sizeof(address));
				if (n == 0)
					break;
				}
			}
		else 
			{
			port->error("bind", isc_net_connect_listen_err, ERRNO);
			port->disconnect();
			return NULL;
			}
		}

	n = listen((SOCKET) port->port_handle, 25);

	if (n == -1) 
		{
		port->error("listen", isc_net_connect_listen_err, ERRNO);
		port->disconnect();
		return NULL;
		}

	if (flag & SRVR_multi_client) 
		{
		/* Prevent the generation of dummy keepalive packets on the
		   connect port. */

		port->port_dummy_packet_interval = 0;
		port->port_dummy_timeout = 0;
		port->port_server_flags |= (SRVR_server | SRVR_multi_client);
		gds__register_cleanup(PortInet::exitHandler, (void *) port);
		return port;
		}

	while (true) 
		{
		l = sizeof(address);
		s = accept((SOCKET) port->port_handle, (struct sockaddr *) &address, &l);
		if (s == INVALID_SOCKET) 
			{
			port->error("accept", isc_net_connect_err, ERRNO);
			port->disconnect();
			return NULL;
			}
/*** I don't understand this at all -- jas, 5/30/04
#ifdef WIN_NT
		if ((flag & SRVR_debug) || !fork(s, flag))
#else
		if ((flag & SRVR_debug) || !fork())
#endif
			{
			THREAD_ENTER;
			closeSocket((SOCKET) port->port_handle);
			port->port_handle = (HANDLE) s;
			port->port_server_flags |= SRVR_server;
			return port;
			}
***/
		PortInet::closeSocket(s);
		}
}

static in_addr get_host_address(const TEXT* name)
{
/**************************************
 *
 *	g e t _ h o s t _ a d d r e s s
 *
 **************************************
 *
 * Functional description
 *	Return host address.
 *
 **************************************/
	in_addr address;
	address.s_addr = inet_addr(name);

	if (address.s_addr == INADDR_NONE) 
		{
		const hostent* host = gethostbyname(name);

		/* On Windows NT/9x, gethostbyname can only accomodate
		 * 1 call at a time.  In this case it returns the error
		 * WSAEINPROGRESS. On UNIX systems, this call may not succeed
		 * because of a temporary error.  In this case, it returns
		 * h_error set to TRY_AGAIN.  When these errors occur,
		 * retry the operation a few times.
		 * NOTE: This still does not guarantee success, but helps.
		 */
		 
		if (!host) 
			if (H_ERRNO == INET_RETRY_ERRNO) 
				for (int retry = 0; retry < INET_RETRY_CALL; retry++) 
					if (host = gethostbyname(name))
						break;

		if (host)
			inet_copy(host->h_addr, (SCHAR*) &address, sizeof(address));
		}

	//THREAD_ENTER;

	return address;
}

Port* INET_server(ConfObject *configuration, int sock)
{
/**************************************
 *
 *	I N E T _ s e r v e r
 *
 **************************************
 *
 * Functional description
 *	We have been spawned by a master server with a connection
 *	established.  Set up port block with the appropriate socket.
 *
 **************************************/
#ifdef VMS
	ISC_tcp_setup(ISC_wait, gds__completion_ast);
#endif

	PortInet *port = PortInet::allocPort(configuration, 0);
	port->port_server_flags |= SRVR_server;
	port->port_handle = sock;
	port->configuration = configuration;

	int optval = 1;
	setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE,
			   (SCHAR *) & optval, sizeof(optval));
	return port;
}

void INET_set_clients( int count)
{
/**************************************
 *
 *	I N E T _ s e t _ c l i e n t s
 *
 **************************************
 *
 * Functional description
 *	Set maxinum number of clients served before
 *	starting new server
 *
 **************************************/
	//INET_max_clients = (count && count < MAXCLIENTS) ? count : MAXCLIENTS;
}

Port* INET_reconnect(HANDLE handle, ConfObject *configuration, ISC_STATUS* status_vector)
{
/**************************************
 *
 *	I N E T _ r e c o n n e c t
 *
 **************************************
 *
 * Functional description
 *	A communications link has been established by another
 *	process.  We have inheritted the handle.  Set up
 *	a port block.
 *
 **************************************/
#ifdef DEBUG_SERVICES
	DebugBreak();
#endif
	PortInet *port = PortInet::allocPort(configuration, 0);
	port->port_status_vector = status_vector;
	port->configuration = configuration;
	status_vector[0] = isc_arg_gds;
	status_vector[1] = 0;
	status_vector[2] = isc_arg_end;

	port->port_handle = (SOCKET) handle;
	port->port_server_flags |= SRVR_server;

	return port;
}

int PortInet::checkHost(char* host_name, char* user_name, struct passwd* passwd)
{
#ifdef VMS
	int length, result;
	struct sockaddr_in address;
	TEXT hosts_file[MAXPATHLEN];

	length = sizeof(address);

	if (getpeername((int) port_handle, &address, &length) == -1)
		return FALSE;

	if (getnamebyaddr(&address, sizeof(address), host_name) == -1)
		return FALSE;

	gethosts(hosts_file);
	result = parseHosts(hosts_file, host_name, user_name);
	if (result == -1)
		result = FALSE;

	return result;
	
#else
#ifdef WIN_NT
	return false;
#else

	TEXT user[64], rhosts[MAXPATHLEN], *hosts_file;
	IB_FILE *fp;
	socklen_t length;
    int result;
	struct sockaddr_in address;
	struct hostent *host;

	length = sizeof(address);

	if (getpeername((int) port_handle, (struct sockaddr*)&address, &length) == -1)
		return 0;


	if (!(host = gethostbyaddr((SCHAR *) &address.sin_addr, 
							   sizeof(address.sin_addr), address.sin_family))) 
		return 0;

	result = -1;
	strcpy(host_name, host->h_name);

	if (passwd) 
		{
		strcpy(user, passwd->pw_name);
		strcpy(rhosts, passwd->pw_dir);
		strcat(rhosts, "/.rhosts");
		result = parseHosts(rhosts, host_name, user);
		}
	else
		strcpy(user, user_name);

	if (result == -1) 
		{
		fp = ib_fopen(GDS_HOSTS_FILE, "r");
		hosts_file = fp ? (TEXT*)GDS_HOSTS_FILE : (TEXT*)HOSTS_FILE;
		
		if (fp)
			ib_fclose(fp);

		result = parseHosts(hosts_file, host_name, user);
		if (result == -1)
			result = FALSE;
		}
		
	return result;
#endif
#endif
}

bool PortInet::checkProxy(char* host_name, char* user_name)
{
#ifdef WIN_NT
	return false;
#else
	IB_FILE *proxy;
	TEXT *p;
	TEXT proxy_file[MAXPATHLEN];
	TEXT source_user[64];
	TEXT source_host[MAXHOSTLEN];
	TEXT target_user[64];
	TEXT line[128];
	int c;
	SLONG length;

#ifndef VMS
	strcpy(proxy_file, PROXY_FILE);
#else
	gds__prefix(proxy_file, PROXY_FILE);
#endif

	if (!(proxy = ib_fopen(proxy_file, "r")))
		return false;

	/* Read lines, scan, and compare */

	bool result = false;

	for (;;) 
		{
		for (p = line; ((c = ib_getc(proxy)) != 0) && c != EOF && c != '\n';)
			*p++ = c;
		*p = 0;
		
		if (sscanf(line, " %[^:]:%s%s", source_host, source_user, target_user) >= 3)
			if ((!strcmp(source_host, host_name) || !strcmp(source_host, "*"))
				&& (!strcmp(source_user, user_name) || !strcmp(source_user, "*"))) 
				{
				/***
				ALLR_free(port_user_name);
				length = strlen(target_user);
				port_user_name = string =
					(STR) ALLOCV(type_str, (int) length);
				string->str_length = length;
				strncpy(string->str_data, target_user, length);
				***/
				port_user_name = target_user;
				strcpy(user_name, target_user);
				result = true;
				break;
				}
		if (c == EOF)
			break;
		}

	ib_fclose(proxy);

	return result;
#endif
}

void PortInet::signalHandler(void *object)
{
	PortInet *port = reinterpret_cast<PortInet*>(object);

	/* If there isn't any out of band data, this signal isn't for us */
	char junk;
	const int n = recv((SOCKET) port->port_handle, &junk, 1, MSG_OOB);
	
	if (n < 0) 
		return;
	
	(*((void(*)(Port*))port->port_ast)) (port);
}

int PortInet::parseHosts(char* file_name, char* host_name, char* user_name)
{
	IB_FILE *fp;
	int c, result;
	TEXT *p, line[256], entry1[256], entry2[256];

	result = -1;
	fp = ib_fopen(file_name, "r");

	if (fp) 
		{
		for (;;) 
			{
			entry1[0] = entry2[0] = 0;
			entry1[1] = entry2[1] = 0;
			for (p = line; (c = ib_getc(fp)) != EOF && c != '\n';)
				*p++ = c;
			*p = 0;
			sscanf(line, "%s", entry1);
			sscanf(&line[strlen(entry1)], "%s", entry2);
			result = parseLine(entry1, entry2, host_name, user_name);
			if (c == EOF || result > -1)
				break;
			}
			
		ib_fclose(fp);
		}

	return result;
}

/*****************************************************************
 *
 *	p a r s e _ l i n e
 *
 *****************************************************************
 *
 * Functional description:
 *	Parse hosts file (.rhosts or hosts.equiv) to determine
 *	if user_name on host_name should be allowed access.
 *
 *  Returns 
 *  1 if user_name is allowed
 *  0 if not allowed and 
 *  -1 if there is not a host_name or a user_name
 *
 * only supporting:
 *    + - anybody on any machine
 *    -@machinegroup [2nd entry optional] - nobody on that machine
 *    +@machinegroup - anybody on that machine
 *    +@machinegroup username - this person on these machines
 *    +@machinegroup +@usergroup - these people  on these machines
 *    +@machinegroup -@usergroup - anybody on these machines EXCEPT these people
 *    machinename - anyone on this machine
 *    machinename username - this person on this machine
 *    machinename +@usergroup - these people on this machine
 *    machinename -@usergroup - anybody but these people on this machine
 *    machinename + - anybody on this machine
 *
 ******************************************************************/

int PortInet::parseLine(char* entry1, char* entry2, char* host_name, char* user_name)
{
	/* if entry1 is simply a "+" - everyone's in */

	if (!strcmp(entry1, "+"))
		return TRUE;

	/* if we don't have a host_name match, don't bother */

	if (strcmp(entry1, host_name))
#ifdef UNIX
#if !(defined SINIXZ)
		if (entry1[1] == '@') 
			{
			if (!innetgr(&entry1[2], host_name, 0, 0))
				return -1;
			}
		else
#endif
#endif
			return -1;

	/* if we have a host_name match and an exclude symbol - they're out */

	if (entry1[0] == '-')
		return FALSE;

	/* if we matched the machine and seen a + after the machine
	   name they are in (eg: <machinename> + ) */

	if ((entry2[0] == '+') && (strlen(entry2) == 1))
		return TRUE;

	/* if we don't have a second entry OR it matches the user_name - they're in */

	if (!entry2[0] || !strcmp(entry2, user_name))
		return TRUE;

	/* if they're in the user group: + they're in, - they're out */

#ifdef UNIX
#if !(defined SINIXZ)
	if (entry2[1] == '@') 
		{
		if (innetgr(&entry2[2], 0, user_name, 0)) 
			{
			if (entry2[0] == '+')
				return TRUE;
			else
				return FALSE;
			}
		else 
			{
			/* if they're NOT in the user group AND we're excluding it
			   - they're in */

			if (entry2[0] == '-')
				return TRUE;
			}
		}
#endif
#endif

	/* if we get here - we matched machine but not user - maybe next line ... */

	return -1;
}

void PortInet::alarmHandler(int x)
{
}

bool_t PortInet::getBytes(XDR* xdrs, SCHAR* buff, u_int count)
{
	SLONG bytecount = count;

	/* Use memcpy to optimize bulk transfers. */

	while (bytecount > (SLONG) sizeof(ISC_QUAD))
		{
		if (xdrs->x_handy >= bytecount) 
			{
			memcpy(buff, xdrs->x_private, bytecount);
			xdrs->x_private += bytecount;
			xdrs->x_handy -= bytecount;
			
			return TRUE;
			}
		else 
			{
			if (xdrs->x_handy > 0) 
				{
				memcpy(buff, xdrs->x_private, xdrs->x_handy);
				xdrs->x_private += xdrs->x_handy;
				buff += xdrs->x_handy;
				bytecount -= xdrs->x_handy;
				xdrs->x_handy = 0;
				}
				
			if (!inet_read(xdrs))
				return FALSE;
			}
		}

	/* Scalar values and bulk transfer remainder fall thru
	   to be moved byte-by-byte to avoid memcpy setup costs. */

	if (!bytecount)
		return TRUE;

	if (xdrs->x_handy >= bytecount) 
		{
		xdrs->x_handy -= bytecount;
		do
			*buff++ = *xdrs->x_private++;
		while (--bytecount);
		return TRUE;
		}

	while (--bytecount >= 0) 
		{
		if (!xdrs->x_handy && !inet_read(xdrs))
			return FALSE;
		*buff++ = *xdrs->x_private++;
		--xdrs->x_handy;
		}

	return TRUE;
}

bool_t PortInet::putBytes(XDR* xdrs, const SCHAR* buff, u_int count)
{
	SLONG bytecount = count;

	/* Use memcpy to optimize bulk transfers. */

	while (bytecount > (SLONG) sizeof(ISC_QUAD))
		 {
		if (xdrs->x_handy >= bytecount) 
			{
			memcpy(xdrs->x_private, buff, bytecount);
			xdrs->x_private += bytecount;
			xdrs->x_handy -= bytecount;
			
			return TRUE;
			}
		else 
			{
			if (xdrs->x_handy > 0) 
				{
				memcpy(xdrs->x_private, buff, xdrs->x_handy);
				xdrs->x_private += xdrs->x_handy;
				buff += xdrs->x_handy;
				bytecount -= xdrs->x_handy;
				xdrs->x_handy = 0;
				}
				
			if (!inet_write(xdrs, 0))
				return FALSE;
			}
		}

	/* Scalar values and bulk transfer remainder fall thru
	   to be moved byte-by-byte to avoid memcpy setup costs. */

	if (!bytecount)
		return TRUE;

	if (xdrs->x_handy >= bytecount) 
		{
		xdrs->x_handy -= bytecount;
		
		do {
			*xdrs->x_private++ = *buff++;
		} while (--bytecount);
		
		return TRUE;
		}

	while (--bytecount >= 0) 
		{
		if (xdrs->x_handy <= 0 && !inet_write(xdrs, 0))
			return FALSE;
			
		--xdrs->x_handy;
		*xdrs->x_private++ = *buff++;
		}

	return TRUE;
}

int PortInet::closeSocket(SOCKET socket)
{
#ifdef WIN_NT
	return closesocket(socket);
#else
	return close(socket);
#endif
}

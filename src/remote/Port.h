/*
 *	PROGRAM:	JRD Remote Interface/Server
 *	MODULE:		Port.h
 *	DESCRIPTION:	Remote Port Definition
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
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#ifndef PORT_H
#define PORT_H

#include "HandleManager.h"
#include "protocol.h"
#include "ConfObj.h"
#include "xdr.h"
#include "SyncObject.h"
#include "RefObject.h"
#include "JString.h"

#define PORT_symmetric		1	/* Server/client archiectures are symmetic */
#define PORT_rpc			2	/* Protocol is remote procedure call */
#define PORT_pend_ack		4	/* An ACK is pending on the port */
#define PORT_broken			8	/* Connect is broken */
#define PORT_async			16	/* Port is asynchronous channel for events */
#define PORT_no_oob			32	/* Don't send out of band data */
#define PORT_disconnect		64	/* Disconnect is in progress */
#define PORT_pend_rec		128	/* A record is pending on the port */
#define PORT_not_trusted	256	/* Connection is from an untrusted node */
#define PORT_impersonate	512	/* A remote user is being impersonated */
#define PORT_dummy_pckt_set	1024	/* A dummy packet interval is set  */

enum rem_port_t
{
	port_mailbox,		/* Apollo mailbox */
	port_pcic,			/* IBM PC interconnect */
	port_inet,			/* Internet (TCP/IP) */
	port_asyn_homebrew,	/* homebrew asynchronous connection */
	port_decnet,		/* DECnet connection */
	port_ipc,			/* NetIPC connection */
	port_pipe,			/* Windows NT named pipe connection */
	port_mslan,			/* Microsoft LanManager connection */
	port_spx,			/* Novell SPX connection */
	port_ipserver,		/* InterBase interprocess server */
	port_xnet			/* Windows NT named xnet connection */
};

enum state_t
{
	state_closed,		/* no connection */
	state_pending,		/* connection is pending */
	state_eof,			/* other side has shut down */
	state_broken,		/* connection is broken */
	state_active,		/* connection is complete */
	state_disconnected          /* port is disconnected */
};


class RDatabase;
class RProcedure;
class RStatement;

struct rmtque;
struct vec;
struct str;
struct Server;

class Port : public RefObject
{
public:
	//struct blk		port_header;
	enum rem_port_t	port_type;			/* type of port */
	enum state_t	port_state;			/* state of port */
	P_ARCH			port_client_arch;	/* so we can tell arch of client */
	Port*			port_clients;		/* client ports */
	Port*			port_next;			/* next client port */
	Port*			port_parent;		/* parent port (for client ports) */
	Port*			port_async;			/* asynchronous sibling port */
	Server*			port_server;		/* server of port */
	USHORT			port_server_flags;	/* TRUE if server */
	USHORT			port_protocol;		/* protocol version number */
	USHORT			port_buff_size;		/* port buffer size (approx) */
	USHORT			port_flags;			/* Misc flags */
	SLONG			port_connect_timeout;   /* Connection timeout value */
	SLONG			port_dummy_packet_interval; /* keep alive dummy packet interval */
	SLONG			port_dummy_timeout;	/* time remaining until keepalive packet */
	ISC_STATUS*		port_status_vector;
	//HANDLE			port_handle;		/* handle for connection (from by OS) */
	int				port_channel;		/* handle for connection (from by OS) */
	int				port_misc1;
	SLONG			port_semaphore;
	//struct linger	port_linger;		/* linger value as defined by SO_LINGER */
	ConfObj			configuration;
	
	/* port function pointers (C "emulation" of virtual functions) */

	virtual int			accept(struct p_cnct*) = 0;
	virtual void		disconnect() = 0;
	virtual Port*		receive(PACKET*) = 0;
	virtual XDR_INT		sendPacket(PACKET*) = 0;
	virtual XDR_INT		sendPartial(PACKET*) = 0;
	virtual Port*		connect(PACKET*, void(*)(Port*)) = 0;	// Establish secondary connection 
	virtual Port*		request(PACKET*) = 0;			// Request to establish secondary connection
	
	/***
	int				(*port_accept)(Port*, struct p_cnct*);
	void			(*port_disconnect)(Port*);
	Port*			(*port_receive_packet)(Port*, PACKET*);
	XDR_INT			(*port_send_packet)(Port*, PACKET*);
	XDR_INT			(*port_send_partial)(Port*, PACKET*);
	Port*			(*port_connect)(Port*, PACKET*, void(*)());	// Establish secondary connection 
	Port*			(*port_request)(Port*, PACKET*);			// Request to establish secondary connection
	***/
	
	RDatabase*		port_context;
	void			(*port_ast)(Port*);		/* AST for events */
	XDR				port_receive;
	XDR				port_send;
	
	//vec				*port_packet_vector;	/* Vector of send/receive packets */
	//VEC				port_object_vector;
	//BLK*			port_objects;
	JString			port_version;
	JString			port_host;			/* Our name */
	JString			port_connection;	/* Name of connection */
	JString			port_user_name;
	JString			port_passwd;
	RProcedure*		port_rpr;			/* port stored procedure reference */
	RStatement*		port_statement;		/* Statement for execute immediate */
	rmtque*			port_receive_rmtque;	/* for client, responses waiting */
	USHORT			port_requests_queued;	/* requests currently queued */
#ifdef VMS
	USHORT			port_iosb[4];
#endif
	void*			port_xcc;              /* interprocess structure */
	//UCHAR			port_buffer[1];
	UCHAR			*port_buffer;
	SyncObject		syncRequest;
	SyncObject		syncObject;
	
	/***	
	int		accept(p_cnct* cnct);
	void	disconnect();
	Port*	receive(PACKET* pckt);
	XDR_INT	sendPacket(PACKET* pckt);
	XDR_INT	send_partial(PACKET* pckt);
	Port*	connect(PACKET* pckt, void(*ast)());
	Port*	request(PACKET* pckt);
	***/
	
	/* TMN: The following member functions are conceptually private
	 *      to server.cpp and should be _made_ private in due time!
	 *      That is, if we don't factor these method out.
	 */
	ISC_STATUS	compile(P_CMPL*, PACKET*);
	ISC_STATUS	ddl(P_DDL*, PACKET*);
	void	disconnect(PACKET*, PACKET*);
	void	drop_database(P_RLSE*, PACKET*);

	ISC_STATUS	end_blob(P_OP, P_RLSE*, PACKET*);
	ISC_STATUS	end_database(P_RLSE*, PACKET*);
	ISC_STATUS	end_request(P_RLSE*, PACKET*);
	ISC_STATUS	end_statement(P_SQLFREE*, PACKET*);
	ISC_STATUS	end_transaction(P_OP, P_RLSE*, PACKET*);
	ISC_STATUS	execute_immediate(P_OP, P_SQLST*, PACKET*);
	ISC_STATUS	execute_statement(P_OP, P_SQLDATA*, PACKET*);
	ISC_STATUS	fetch(P_SQLDATA*, PACKET*);
	ISC_STATUS	fetch_blob(P_SQLDATA*, PACKET*);
	ISC_STATUS	get_segment(P_SGMT*, PACKET*);
	ISC_STATUS	get_slice(P_SLC*, PACKET*);
	ISC_STATUS	info(P_OP, P_INFO*, PACKET*);
	ISC_STATUS	insert(P_SQLDATA*, PACKET*);
	ISC_STATUS	open_blob(P_OP, P_BLOB*, PACKET*);
	ISC_STATUS	prepare(P_PREP*, PACKET*);
	ISC_STATUS	prepare_statement(P_SQLST*, PACKET*);
	ISC_STATUS	put_segment(P_OP, P_SGMT*, PACKET*);
	ISC_STATUS	put_slice(P_SLC*, PACKET*);
	ISC_STATUS	que_events(P_EVENT*, PACKET*);
	ISC_STATUS	receive_after_start(P_DATA*, PACKET*, ISC_STATUS*);
	ISC_STATUS	receive_msg(P_DATA*, PACKET*);
	ISC_STATUS	seek_blob(P_SEEK*, PACKET*);
	ISC_STATUS	send_msg(P_DATA*, PACKET*);
	ISC_STATUS	send_response(PACKET*, OBJCT, USHORT, ISC_STATUS*);
	ISC_STATUS	service_attach(P_ATCH*, PACKET*);
	ISC_STATUS	service_end(P_RLSE*, PACKET*);
	ISC_STATUS	service_start(P_INFO*, PACKET*);
	ISC_STATUS	set_cursor(P_SQLCUR*, PACKET*);
	ISC_STATUS	start(P_OP, P_DATA*, PACKET*);
	ISC_STATUS	start_and_send(P_OP, P_DATA*, PACKET*);
	ISC_STATUS	start_transaction(P_OP, P_STTR*, PACKET*);
	ISC_STATUS	transact_request(P_TRRQ *, PACKET*);
	
	HandleManager	portObjects;
	
	void* getObject(int objectId);
	void setObject(void* object, int objectId);
	OBJCT	getObjectId(void*);
	void releaseObjectId(int objectId);
	void clearStatement(void);
	RStatement* getStatement(void);
	Port(int size);
	void clearContext(void);

protected:
	virtual ~Port(void);

public:
	void addClient(Port* port);
	void removeClient(Port* client);
};

#endif


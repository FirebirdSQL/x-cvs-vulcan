/*
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
 *
 */

#ifndef RSERVER_H
#define RSERVER_H

#include "SyncObject.h"
#include "Port.h"
#include "protocol.h"

class RServer;
CLASS (Threads);

struct Server
	{
	Server*		srvr_next;
	Port*		srvr_parent_port;
	//rem_port_t	srvr_port_type;
	USHORT		srvr_flags;
	};

struct ServerRequest
{
	ServerRequest	*req_next;
	ServerRequest	*req_chain;
	RServer			*req_server;
	Port			*req_port;
	Packet			req_send;
	Packet			req_receive;
};


class RServer
{
public:
	SyncObject		syncObject;
	SyncObject		syncRequestQue;
	Server			*servers;
	Port			*mainPort;
	ServerRequest	*freeRequests;
	ServerRequest	*activeRequests;
	ServerRequest	*requestQue;
	int				threadsWaiting;
	int				extraThreads;
	int				serverFlags;
	Threads			*threads;
	
	RServer(void);
	virtual ~RServer(void);
	void setServer(Port* port, int flags);
	void runMultiThreaded(Port* main_port, int flags);
	void processPackets(Port* port, int flags);
	bool processPacket(Port* port, Packet* send, Packet* receive, Port** result);
	bool acceptConnection(Port* port, P_CNCT* connect, Packet* send);
	ISC_STATUS attachDatabase(Port* port, P_OP operation, P_ATCH* attach, Packet* send);
	ISC_STATUS cancelEvents(Port* port, P_EVENT* stuff, Packet* send);
	void auxRequest(Port* port, P_REQ* request, Packet* send);
	void auxConnect(Port* port, P_REQ* request, Packet* send);
	ISC_STATUS allocateStatement(Port* port, P_RLSE* allocate, Packet* send);
	void success(ISC_STATUS* status_vector);
	int appendRequestChain(ServerRequest* request, ServerRequest** que);
	int appendRequestNext(ServerRequest* request, ServerRequest** que);
	ServerRequest* getRequest(void);
	void processRequest(ServerRequest* request, int flags);
	void freeRequest(ServerRequest* request);
	int thread(int flags);
	static int serverThread(void* server);
	void cancelOperation(Port* port);
	void agent(ServerRequest* serverRequest);
	static void agentThread(void* arg);
};

#endif


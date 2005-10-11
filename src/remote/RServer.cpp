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

#include <stdlib.h>
#include <stdio.h>
#include "fbdev.h"
#include "ibase.h"
#include "common.h"
#include "RServer.h"
#include "remote.h"
#include "Sync.h"
#include "OSRIException.h"
#include "../remote/remot_proto.h"
#include "why_proto.h"
#include "thd.h"
#include "thd_proto.h"
#include "Threads.h"
#include "Thread.h"

RServer::RServer(void)
{


	servers = NULL;
	freeRequests = NULL;
	activeRequests = NULL;
	requestQue = NULL;
	threadsWaiting = 0;
	extraThreads = 0;
	threads = new Threads;
}

RServer::~RServer(void)
{
	for (Server *server; server = servers;)
		{
		servers = server->srvr_next;
		delete server;
		}
	
	threads->release();
}

void RServer::setServer(Port* port, int flags)
{
	Sync sync (&syncObject, "RServer::setServer");
	sync.lock(Exclusive);
	Server *server;
	
	for (server = servers; server; server = server->srvr_next)
		if (server->srvr_parent_port->port_type == port->port_type)
			break;
	
	if (!server)
		{
		server = new Server;
		server->srvr_next = servers;
		servers = server;
		server->srvr_flags = flags;
		server->srvr_parent_port = port;
		}
	
	port->port_server = server;		
}

void RServer::runMultiThreaded(Port* main_port, int flags)
{
	serverFlags = flags;
	ServerRequest *request = NULL;
	Port* port = NULL;
	ISC_STATUS_ARRAY status_vector;
	Sync sync (&syncRequestQue, "SRVR_multi_thread");
	setServer(main_port, flags);

	/* We need to have this error handler as there is so much underlaying code
	 * that is depending on it being there.  The expected failure
	 * that can occur in the call paths from here is out-of-memory
	 * and it is unknown if other failures can get us here
	 * Failures can occur from set_server as well as RECEIVE
	 *
	 * Note that if a failure DOES occur, we reenter the loop and try to continue
	 * operations.  This is important as this is the main receive loop for
	 * new incoming requests, if we exit here no new requests can come to the
	 * server.
	 */

	try 
		{
		while (true)
			{
			port = NULL;
			request = getRequest();
			request->req_send.zap(true);
			request->req_receive.zap(true);
			request->req_next = NULL;
			request->req_chain = NULL;

			if (!(port = main_port->receive(&request->req_receive)))
				{
				gds__log("SRVR_multi_thread/RECEIVE: error on main_port, shutting down");
				return;
				}

			request->req_port = port;
			//processRequest(request, flags);
			threads->start("RServer::runMultiThreaded", agentThread, request);
			}
		}
	catch (OSRIException&)
		{
		/* If we got as far as having a port allocated before the error, disconnect it
		* gracefully.
		*/
		
		if (port != NULL)
			{
			gds__log("SRVR_multi_thread: forcefully disconnecting a port");

			/* To handle recursion within the error handler */
			try 
				{
				/* If we have a port, request really should be non-null, but just in case ... */
				if (request != NULL) 
					{
					/* Send client a real status indication of why we disconnected them */
					/* Note that send_response() can post errors that wind up in this same handler */
					port->send_response(&request->req_send, 0, 0, status_vector);
					port->disconnect(&request->req_send, &request->req_receive);
					}
				else 
					{
					/* Can't tell the client much, just make 'em go away.  Their side should detect
					* a network error
					*/
					port->disconnect(NULL, NULL);
					}
				port = NULL;
				}
			catch (...) 
				{
				port->disconnect(NULL, NULL);
				port = NULL;
				}
			}

		/* There was an error in the processing of the request, if we have allocated
		* a request, free it up and continue.
		*/
			
		if (request != NULL) 
			{
			freeRequest (request);
			request = NULL;
			}
		}
}

void RServer::processPackets(Port* mainPort, int flags)
{
	Packet send, receive;
	receive.zap (true);
	send.zap (true);
	setServer(mainPort, flags);

	for (Port *port; port = mainPort->receive (&receive);)
		if (!processPacket(port, &send, &receive, NULL))
			break;
}

bool RServer::processPacket(Port* port, Packet* send, Packet* receive, Port** result)
{
	P_OP op;
	TEXT msg[128];
	Server *server;
	Sync sync (&port->syncRequest, "RServer::processPacket");
	sync.lock(Exclusive);
	
	try 
		{
		switch (op = receive->p_operation)
			{
			case op_connect:
				if (!acceptConnection(port, &receive->p_cnct, send)) 
					{
					//if (string = port->port_user_name) 
						{
						sprintf(msg, "SERVER/processPacket: connection rejected for %/", 
								(const char*) port->port_user_name);
								//string->str_length, string->str_length,
								//string->str_data);
						gds__log(msg, 0);
						}
					if (port->port_server->srvr_flags & SRVR_multi_client) 
						port->port_state = state_broken;
					else 
						{
						gds__log ("SERVER/processPacket: connect reject, server exiting", 0);

						return false;
						}
					}
				break;

			case op_compile:
				port->compile(&receive->p_cmpl, send);
				break;

			case op_attach:
			case op_create:
				attachDatabase(port, op, &receive->p_atch, send);
				break;

			case op_service_attach:
				port->service_attach(&receive->p_atch, send);
				break;

			case op_service_start:
				port->service_start(&receive->p_info, send);
				break;

			case op_disconnect:
			case op_exit:
				if (!(server = port->port_server))
					break;
				if ((server->srvr_flags & SRVR_multi_client) && port != server->srvr_parent_port) 
					{
					sync.unlock();
					port->disconnect(send, receive);
					port = NULL;
					break;
					}
				if ((server->srvr_flags & SRVR_multi_client) && port == server->srvr_parent_port)
					gds__log("SERVER/processPacket: Multi-client server shutdown", 0);
				sync.unlock();
				port->disconnect(send, receive);

				return false;

			case op_receive:
				port->receive_msg(&receive->p_data, send);
				break;

			case op_send:
				port->send_msg(&receive->p_data, send);
				break;

			case op_start:
			case op_start_and_receive:
				port->start(op, &receive->p_data, send);
				break;

			case op_start_and_send:
			case op_start_send_and_receive:
				port->start_and_send(op, &receive->p_data, send);
				break;

			case op_transact:
				port->transact_request(&receive->p_trrq, send);
				break;

			case op_reconnect:
			case op_transaction:
				port->start_transaction(op, &receive->p_sttr, send);
				break;

			case op_prepare:
			case op_rollback:
			case op_rollback_retaining:
			case op_commit:
			case op_commit_retaining:
				port->end_transaction(op, &receive->p_rlse, send);
				break;

			case op_detach:
				port->addRef();
				port->end_database(&receive->p_rlse, send);
				sync.unlock();
				port->release();
				break;

			case op_service_detach:
				port->service_end(&receive->p_rlse, send);
				break;

			case op_drop_database:
				port->drop_database(&receive->p_rlse, send);
				break;

			case op_create_blob:
			case op_open_blob:
			case op_create_blob2:
			case op_open_blob2:
				port->open_blob(op, &receive->p_blob, send);
				break;

			case op_batch_segments:
			case op_put_segment:
				port->put_segment(op, &receive->p_sgmt, send);
				break;

			case op_get_segment:
				port->get_segment(&receive->p_sgmt, send);
				break;

			case op_seek_blob:
				port->seek_blob(&receive->p_seek, send);
				break;

			case op_cancel_blob:
			case op_close_blob:
				port->end_blob(op, &receive->p_rlse, send);
				break;

			case op_prepare2:
				port->prepare(&receive->p_prep, send);
				break;

			case op_release:
				port->end_request(&receive->p_rlse, send);
				break;

			case op_info_blob:
			case op_info_database:
			case op_info_request:
			case op_info_transaction:
			case op_service_info:
			case op_info_sql:
				port->info(op, &receive->p_info, send);
				break;

			case op_que_events:
				port->que_events(&receive->p_event, send);
				break;

			case op_cancel_events:
				cancelEvents(port, &receive->p_event, send);
				break;

			case op_connect_request:
				auxRequest(port, &receive->p_req, send);
				break;

			case op_aux_connect:
				auxConnect(port, &receive->p_req, send);
				break;

			case op_ddl:
				port->ddl(&receive->p_ddl, send);
				break;

			case op_get_slice:
				port->get_slice(&receive->p_slc, send);
				break;

			case op_put_slice:
				port->put_slice(&receive->p_slc, send);
				break;

			case op_allocate_statement:
				allocateStatement(port, &receive->p_rlse, send);
				break;

			case op_execute:
			case op_execute2:
				port->execute_statement(op, &receive->p_sqldata, send);
				break;

			case op_exec_immediate:
			case op_exec_immediate2:
				port->execute_immediate(op, &receive->p_sqlst, send);
				break;

			case op_fetch:
				port->fetch(&receive->p_sqldata, send);
				break;

			case op_free_statement:
				port->end_statement(&receive->p_sqlfree, send);
				break;

			case op_insert:
				port->insert(&receive->p_sqldata, send);
				break;

			case op_prepare_statement:
				port->prepare_statement(&receive->p_sqlst, send);
				break;

			case op_set_cursor:
				port->set_cursor(&receive->p_sqlcur, send);
				break;

			case op_update_account_info:
				port->updateAccountInfo(&receive->p_account_update, send);
				break;

			case op_authenticate_user:
				port->authenticateUser(&receive->p_authenticate_user, send);
				break;

			case op_dummy:
				send->p_operation = op_dummy;
				port->sendPacket(send);
				break;

			default:
				sprintf(msg, "SERVER/processPacket: don't understand packet type %d",
						receive->p_operation);
				gds__log(msg, 0);
				port->port_state = state_broken;
				break;
			}

		if (port && port->port_state == state_broken) 
			{
			if (!port->port_parent) 
				{
				gds__log("SERVER/processPacket: broken port, server exiting", 0);
				
				if (port->port_type == port_inet)
					port->disconnect();
				else
					port->disconnect(send, receive);
					
				return false;
				}
				
			sync.unlock();
			port->disconnect(send, receive);
			port = NULL;
			}

		if (result)
			*result = port;

		}
	catch (OSRIException &exception) 
		{
		/* There must be something better to do here.  BUT WHAT? */

		gds__log("SERVER/processPacket: out of memory", 0);

		/*  It would be nice to log an error to the user, instead of just terminating them!  */
		
		port->send_response(send, 0, 0, exception.statusVector);
		sync.unlock();
		port->disconnect(send, receive);	/*  Well, how about this...  */

		return false;
		}

	return true;
}

bool RServer::acceptConnection(Port* port, P_CNCT* connect, Packet* send)
{
	P_ACPT *accept;
	P_ARCH architecture;
	USHORT weight, version, type;
	p_cnct::p_cnct_repeat * protocol, *end;
	//TEXT buffer[64];

	/* Accept the physical connection */

	send->p_operation = op_reject;
	accept = &send->p_acpt;
	weight = 0;

	if (!port->accept(connect)) 
		{
		port->sendPacket(send);
		return false;
		}

	/* Select the most appropriate protocol (this will get smarter) */

	protocol = connect->p_cnct_versions;

	for (end = protocol + connect->p_cnct_count; protocol < end; protocol++)
		switch (protocol->p_cnct_version)
			{
			case PROTOCOL_VERSION3:
			case PROTOCOL_VERSION4:
			case PROTOCOL_VERSION5:
			case PROTOCOL_VERSION6:
			case PROTOCOL_VERSION7:
			case PROTOCOL_VERSION8:
			case PROTOCOL_VERSION9:
			case PROTOCOL_VERSION10:
			case PROTOCOL_VERSION11:
				if ((protocol->p_cnct_architecture == arch_generic ||
				      protocol->p_cnct_architecture == ARCHITECTURE) &&
				     protocol->p_cnct_weight >= weight)
					{
					weight = protocol->p_cnct_weight;
					version = protocol->p_cnct_version;
					architecture = protocol->p_cnct_architecture;
					type = MIN(protocol->p_cnct_max_type, ptype_out_of_band);
					send->p_operation = op_accept;
					}
				break;
			}

	/* Send off out gracious acceptance or flag rejection */

	accept->p_acpt_version = port->port_protocol = version;
	accept->p_acpt_architecture = architecture;
	accept->p_acpt_type = type;

	/* and modify the version string to reflect the chosen protocol */

	port->port_version.Format ("%s/P%d", (const char*) port->port_version, port->port_protocol);

	if (architecture == ARCHITECTURE)
		port->port_flags |= PORT_symmetric;

	if (type == ptype_rpc)
		port->port_flags |= PORT_rpc;

	if (type != ptype_out_of_band)
		port->port_flags |= PORT_no_oob;

	port->sendPacket(send);

	return true;
}


ISC_STATUS RServer::attachDatabase(Port* port, P_OP operation, P_ATCH* attach, Packet* send)
{
	UCHAR new_dpb_buffer[512];
	isc_handle handle;
	ISC_STATUS_ARRAY status_vector;
	RDB rdb;
	//STR string;

	send->p_operation = op_accept;
	handle = NULL_HANDLE;
	const char* file = reinterpret_cast<const char*>(attach->p_atch_file.cstr_address);
	const USHORT l = attach->p_atch_file.cstr_length;
	const UCHAR* dpb = attach->p_atch_dpb.cstr_address;
	USHORT dl = attach->p_atch_dpb.cstr_length;

	/* If we have user identification, append it to database parameter block */

	UCHAR* new_dpb = new_dpb_buffer;
	
	if (!port->port_user_name.IsEmpty()) 
		{
		int length = port->port_user_name.length();
		const char *string = port->port_user_name;
		
		if (dl + 3 + length > sizeof(new_dpb_buffer))
			new_dpb = new UCHAR [dl + 3 + length];
			
		UCHAR* p = new_dpb;
		
		if (dl) 
			for (const UCHAR* const end = dpb + dl; dpb < end;)
				*p++ = *dpb++;
		else
			*p++ = isc_dpb_version1;
			
		*p++ = isc_dpb_sys_user_name;
		*p++ = (UCHAR) length;
		dpb = (UCHAR *) string;
		
		for (const UCHAR* const end = dpb + length; dpb < end;)
			*p++ = *dpb++;
			
		dpb = new_dpb;
		dl = p - new_dpb;
		}

	/* See if user has specified parameters relevant to the connection,
	   they will be stuffed in the DPB if so. */
	   
	REMOTE_get_timeout_params(port, dpb, dl);

	if (operation == op_attach)
		isc_attach_database(status_vector, l, file,
							&handle, dl, reinterpret_cast<const char*>(dpb));
	else
		isc_create_database(status_vector, l, file,
							&handle, dl, reinterpret_cast<const char*>(dpb), 0);

	if (new_dpb != new_dpb_buffer) 
		delete new_dpb;

	if (!status_vector[1])
		{
		//port->port_context = rdb = (RDB) ALLOC(type_rdb);
		port->port_context = rdb = new RDatabase (port);
		
		rdb->rdb_port = port;
		rdb->rdb_handle = handle;
		}

	return port->send_response(send, 0, 0, status_vector);
}

ISC_STATUS RServer::cancelEvents(Port* port, P_EVENT* stuff, Packet* send)
{
	ISC_STATUS_ARRAY status_vector;
	RDatabase *rdb = port->port_context;
	REvent *event = rdb->findEvent (stuff->p_event_rid);
	
	/* If no event found, pretend it was cancelled */

	if (!event)
		return port->send_response(send, 0, 0, status_vector);

	/* cancel the event */

	if (event->rvnt_id) 
		isc_cancel_events(status_vector, &rdb->rdb_handle, &event->rvnt_id);

	/* zero event info */

	event->rvnt_id = 0L;
	event->rvnt_rid = 0L;
	event->rvnt_ast = 0;

	return port->send_response(send, 0, 0, status_vector);
}

void RServer::auxRequest(Port* port, P_REQ* request, Packet* send)
{
	ISC_STATUS_ARRAY status_vector;

	/* save the port status vector */

	ISC_STATUS *save_status = port->port_status_vector;
	port->port_status_vector = status_vector;
	success(status_vector);

	/* We do this silliness with buffer because the SPX protocol
	   requires a 12 byte buffer to be sent back.  Other protocols
	   can do what they want to with cstr_address. */

	UCHAR buffer[12];
	CSTRING save_cstring = send->p_resp.p_resp_data;
	send->p_resp.p_resp_data.cstr_address = buffer;
	Port *aux_port = port->auxRequest(send);
	RDatabase *rdb = port->port_context;
	port->send_response(send, rdb->rdb_id,
				  send->p_resp.p_resp_data.cstr_length, status_vector);

	if (status_vector[1]) 
		{
		/* restore the port status vector */

		port->port_status_vector = save_status;
		send->p_resp.p_resp_data = save_cstring;
		return;
		}

	if (aux_port) 
		{
		aux_port->connect(send, 0);
		aux_port->port_context = rdb;
		}

	/* restore the port status vector */

	port->port_status_vector = save_status;
	send->p_resp.p_resp_data = save_cstring;
}

void RServer::auxConnect(Port* port, P_REQ* request, Packet* send)
{
	port->connect(0, 0);
	Port *partner = (Port*)(long) request->p_req_partner;
	partner->port_async = port;
}

ISC_STATUS RServer::allocateStatement(Port* port, P_RLSE* allocate, Packet* send)
{
	RSR statement;
	ISC_STATUS_ARRAY status_vector;
	RDatabase *rdb = port->port_context;
	isc_handle handle = NULL_HANDLE;
	isc_dsql_allocate_statement(status_vector, &rdb->rdb_handle, &handle);
	OBJCT object = 0;

	if (!status_vector[1])
		{
		statement = rdb->createStatement();
		statement->rsr_handle = handle;
		
		if (statement->rsr_id = port->getObjectId(statement))
			object = statement->rsr_id;
		else 
			{
			isc_dsql_free_statement(status_vector, &statement->rsr_handle, DSQL_drop);
			delete statement;
			status_vector[0] = isc_arg_gds;
			status_vector[1] = isc_too_many_handles;
			status_vector[2] = isc_arg_end;
			}
		}

	return port->send_response(send, object, 0, status_vector);
}

void RServer::success(ISC_STATUS* status_vector)
{
	status_vector[0] = isc_arg_gds;
	status_vector[1] = FB_SUCCESS;
	status_vector[2] = isc_arg_end;
}

int RServer::appendRequestChain(ServerRequest* request, ServerRequest** que)
{
	int requests;

	for (requests = 1; *que; que = &(*que)->req_chain)
		++requests;

	*que = request;

	return requests;
}

int RServer::appendRequestNext(ServerRequest* request, ServerRequest** que)
{
	int requests;

	for (requests = 1; *que; que = &(*que)->req_next)
		++requests;

	*que = request;

	return requests;
}

ServerRequest* RServer::getRequest(void)
{
	Sync sync (&syncRequestQue, "RServer::getReques");
	sync.lock(Exclusive);
	ServerRequest *request = freeRequests;
	
	if (request)
		freeRequests = request->req_next;
	else
		{
		request = new ServerRequest;
		request->req_server = this;
		}
	
	return request;
}

void RServer::freeRequest(ServerRequest* request)
{
	request->req_next = freeRequests;
	freeRequests = request;
}

void RServer::processRequest(ServerRequest* request, int flags)
{
	Port *port = request->req_port;
	P_OP operation = request->req_receive.p_operation;
	Sync sync (&syncRequestQue, "RServer::processRequest");
	sync.lock (Exclusive);
	ServerRequest *active;

	/* If port has an active request, link this one in */

	for (active = activeRequests; active; active = active->req_next)
		if (active->req_port == port) 
			{
			/* Don't queue a dummy keepalive packet if there is
				an active request running on this port. */

			if (operation == op_dummy) 
				{
				request->req_next = freeRequests;
				freeRequests = request;
				return;
				}
				
			port->port_requests_queued++;
			appendRequestChain(request, &active->req_chain);
			
#ifdef CANCEL_OPERATION
			if (operation == op_exit || operation == op_disconnect)
				cancelOperation(port);
#endif
			return;
			}

	/* If port has an pending request, link this one in */

	for (active = requestQue; active; active = active->req_next)
		if (active->req_port == port)
			{
			/* Don't queue a dummy keepalive packet if there is
			a pending request against this port. */

			if (operation == op_dummy) 
				{
				freeRequest (request);
				return;
				}
				
			port->port_requests_queued++;
			appendRequestChain(request, &active->req_chain);
			

#ifdef CANCEL_OPERATION
			if (operation == op_exit || operation == op_disconnect)
				cancelOperation(port);
#endif
			return;
			}

	/* No port to assign request to, add it to the waiting queue and wake up a
	 * thread to handle it 
	 */
		
	REMOTE_TRACE(("Enqueue request %p", request));
	int pending_requests = appendRequestNext(request, &requestQue);
	port->port_requests_queued++;
	sync.unlock();
	

	/* NOTE: we really *should* have something that limits how many
	* total threads we allow in the system.  As each thread will
	* eat up memory that other threads could use to complete their work
	*/
		
	/* NOTE: The setting up of extraThreads variable is done below to let waiting
	   threads know if their services may be needed for the current set
	   of requests.  Otherwise, some idle threads may leave the system 
	   freeing up valuable memory.
	*/
	
	extraThreads = threadsWaiting - pending_requests;
	
	if (extraThreads < 0) 
		{
		THD_start_thread(	serverThread,
							this, //(void*)(long) flags,
							THREAD_medium,
							THREAD_ast,
							0);
		}

	REMOTE_TRACE(("Post event"));
}

int RServer::thread(int flags)
{
	ServerRequest *request, *next, **req_ptr;
	Port *port, *parent_port;
	Sync sync (&syncRequestQue, "server thread");
	
#ifdef WIN_NT_XXX
	SCHAR *thread;
	
	if (!(flags & SRVR_non_service))
		thread = reinterpret_cast<SCHAR*>(CNTL_insert_thread());
#endif

	int inactive_count = 0;
	int timedout_count = 0;
	
	for (;;)
		{
		sync.lock (Exclusive);
		
		if (request = requestQue)
			{
			inactive_count = 0;
			timedout_count = 0;
			REMOTE_TRACE(("Dequeue request %p", requestQue));
			requestQue = request->req_next;
			//sync.unlock();
			
			while (request)
				{
				/* Bind a thread to a port. */

				if (request->req_port->port_server_flags & SRVR_thread_per_port)
					{
					port = request->req_port;
					freeRequest (request);
					//SRVR_main(port, port->port_server_flags);
					sync.unlock();
					processPackets(port, port->port_server_flags);
					sync.lock(Exclusive);
					request = NULL;
					continue;
					}
					
				/* Splice request into list of active requests, execute request,
				   and unsplice */

				request->req_next = activeRequests;
				activeRequests = request;

				/* Validate port.  If it looks ok, process request */

				parent_port = request->req_port->port_server->srvr_parent_port;
				sync.unlock();
				
				if (parent_port == request->req_port)
					processPacket(parent_port, &request->req_send, &request->req_receive, &port);
				else
					for (port = parent_port->port_clients; port; port = port->port_next)
						if (port == request->req_port && port->port_state != state_disconnected) 
							{
							processPacket(port, &request->req_send, &request->req_receive, &port);
							break;
							}

				/* Take request out of list of active requests */
				
				sync.lock(Exclusive);
				
				for (req_ptr = &activeRequests; *req_ptr; req_ptr = &(*req_ptr)->req_next)
					if (*req_ptr == request) 
						{
						*req_ptr = request->req_next;
						break;
						}

				/* If this is a explicit or implicit disconnect, get rid of
				   any chained requests */

				if (!port)
					{
					while (next = request->req_chain) 
						{
						request->req_chain = next->req_chain;
						freeRequest(next);
						}
					if (request->req_send.p_operation == op_void &&
						request->req_receive.p_operation == op_void) 
						{
						delete request;
						request = NULL;
						}
					}
				else 
					{
					port->port_requests_queued--;
					}

				/* Pick up any remaining chained request, and free current request */

				if (request) 
					{
					next = request->req_chain;
					freeRequest(request);
					request = next;
					}
				}	// while (request)
			}
		else 
			{
			inactive_count++;
			
			/* If the main server thread is not relying on me to take on a new request
			   and I have been idling for a while, it is time to quit and thereby
			   release some valuable system resources like memory */
			   
			if (extraThreads > 1 && (inactive_count > 20 || timedout_count > 2)) 
				{
				extraThreads--;	/* Count me out */
				break;
				}

			threadsWaiting++;
			/* Wait for 1 minute (60 seconds) on a new request */
			REMOTE_TRACE(("Wait for event"));

			/*** I really don't understand this		
			if (!requests_semaphore.tryEnter(60)) 
				{
				REMOTE_TRACE(("timeout!"));
				timedout_count++;
				} 
			else 
				REMOTE_TRACE(("got it"));
			***/
				
			--threadsWaiting;
			}
		}

#ifdef WIN_NT_XXX
	if (!(flags & SRVR_non_service))
		CNTL_remove_thread(thread);
#endif

	return 0;
}

int RServer::serverThread(void* arg)
{
	RServer *server = (RServer*) arg;
	
	return server->thread(server->serverFlags);
}

void RServer::cancelOperation(Port* port)
{
#ifdef CANCEL_OPERATION
	RDatabase *rdb;
	ISC_STATUS_ARRAY status_vector;

	if ((port->port_flags & (PORT_async | PORT_disconnect)) ||
		!(rdb = port->port_context))
		return;

	if (rdb->rdb_handle)
		if (!(rdb->rdb_flags & RDB_service))
			isc_cancel_operation(status_vector, &rdb->rdb_handle, CANCEL_raise);
#endif
}

void RServer::agent(ServerRequest* request)
{
	Port *parent_port = request->req_port->port_server->srvr_parent_port;
	Port *port = NULL;
	
	if (parent_port == request->req_port)
		processPacket(parent_port, &request->req_send, &request->req_receive, &port);
	else
		for (port = parent_port->port_clients; port; port = port->port_next)
			if (port == request->req_port && port->port_state != state_disconnected) 
				{
				processPacket(port, &request->req_send, &request->req_receive, &port);
				break;
				}
	
	freeRequest(request);
	
	if (port)
		port->release();
}

void RServer::agentThread(void* arg)
{
	ServerRequest *serverRequest = (ServerRequest*) arg;
	serverRequest->req_server->agent(serverRequest);
}

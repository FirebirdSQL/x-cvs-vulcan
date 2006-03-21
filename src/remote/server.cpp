/*
 *	PROGRAM:	JRD Remote Server
 *	MODULE:		server.cpp
 *	DESCRIPTION:	Remote server
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
 * 2002.02.27 Claudio Valderrama: Fix SF Bug #509995.
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#include "fbdev.h"
#include "../jrd/ib_stdio.h"
#include <string.h>
#include "../jrd/ibase.h"
#include "../jrd/gds_proto.h"
#include "../jrd/gdsassert.h"
#include "../remote/remote.h"
//#include "../jrd/thd.h"
#include "../jrd/isc.h"
#include "../jrd/license.h"
#include "../jrd/jrd_time.h"
#include "../remote/merge_proto.h"
#include "../remote/parse_proto.h"
#include "../remote/remot_proto.h"
#include "../remote/serve_proto.h"

#ifdef WIN_NT
#include "../remote/os/win32/cntl_proto.h"
#include <stdlib.h>
#endif

#include "../jrd/isc_proto.h"
#include "../jrd/why_proto.h"
#include "OSRIException.h"
#include "SyncObject.h"
#include "Sync.h"
#include "gen/iberror.h"

#include "../remote/proto_proto.h"	// xdr_protocol_overhead()


//#pragma FB_COMPILER_MESSAGE("What kind of crap is this?! FIX!")

#define CHECK_HANDLE(blk,cast,type,id,err)							\
	{																\
		if (id >= port->port_object_vector->vec_count ||			\
			!(blk = (cast) port->port_objects [id]))				\
		{															\
			status_vector [0] = isc_arg_gds;						\
			status_vector [1] = err;								\
			status_vector [2] = isc_arg_end;						\
			return port->send_response(send, 0, 0, status_vector);	\
		}															\
	}

#define CHECK_HANDLE_MEMBER(blk,cast,type,id,err)					\
	{																\
		if (id >= this->port_object_vector->vec_count ||			\
			!(blk = (cast) this->port_objects [id]))				\
		{															\
			status_vector [0] = isc_arg_gds;						\
			status_vector [1] = err;								\
			status_vector [2] = isc_arg_end;						\
			return this->send_response(send, 0, 0, status_vector);	\
		}															\
	}

#define STMT_BLOB		1
#define STMT_NO_BATCH	2
#define STMT_OTHER		0

typedef struct server_req_t
{
	server_req_t*	req_next;
	server_req_t*	req_chain;
	PORT			req_port;
	PACKET			req_send;
	PACKET			req_receive;
} *SERVER_REQ;

typedef struct srvr
{
	struct srvr*	srvr_next;
	Port*			srvr_parent_port;
	enum rem_port_t	srvr_port_type;
	USHORT			srvr_flags;
} *SRVR;

static bool	accept_connection(PORT, P_CNCT*, PACKET*);
static ISC_STATUS	allocate_statement(PORT, P_RLSE*, PACKET*);
static SLONG	append_request_chain(SERVER_REQ, SERVER_REQ*);
static SLONG	append_request_next(SERVER_REQ, SERVER_REQ*);
static ISC_STATUS	attach_database(PORT, P_OP, P_ATCH*, PACKET*);
static void		aux_connect(PORT, P_REQ*, PACKET*);
static void		aux_request(PORT, P_REQ*, PACKET*);
static ISC_STATUS	cancel_events(PORT, P_EVENT*, PACKET*);

#ifdef CANCEL_OPERATION
static void	cancel_operation(PORT);
#endif

static bool	check_request(RRQ, USHORT, USHORT);
static USHORT	check_statement_type(RSR);

#ifdef SCROLLABLE_CURSORS
static REM_MSG		dump_cache(rrq_repeat*);
#endif

static bool		get_next_msg_no(RRQ, USHORT, USHORT*);
//static RTR		make_transaction(RDB, isc_handle );

//static void	release_blob(RBL);
//static void	release_event(RVNT);
//static void	release_request(RRQ);
//static void	release_statement(RSR*);
//static void	release_sql_request(RSR);
//static void	release_transaction(RTR);

#ifdef SCROLLABLE_CURSORS
static REM_MSG	scroll_cache(rrq_repeat*, USHORT *, ULONG *);
#endif

static void	server_ast(void*, USHORT, const UCHAR*);
static void		success(ISC_STATUS *);
static int THREAD_ROUTINE thread(void *);


// static data - NOT THREAD SAFE!

static SLONG		threads_waiting		= 0;
static SLONG		extra_threads		= 0;
static SERVER_REQ	request_que			= NULL;
static SERVER_REQ	free_requests		= NULL;
static SERVER_REQ	active_requests		= NULL;
static SRVR			servers;

/***
#ifdef MULTI_THREAD
static Firebird::Semaphore requests_semaphore;
#endif
***/

static SyncObject	syncRequestQue;

static const UCHAR request_info[] =
{
	isc_info_state,
	isc_info_message_number,
	isc_info_end
};

static const UCHAR sql_info[] =
{
	isc_info_sql_stmt_type,
	isc_info_sql_batch_fetch
};


#define GDS_DSQL_ALLOCATE	isc_dsql_allocate_statement
#define GDS_DSQL_EXECUTE	isc_dsql_execute2_m
#define GDS_DSQL_EXECUTE_IMMED	isc_dsql_exec_immed3_m
#define GDS_DSQL_FETCH		isc_dsql_fetch_m
#define GDS_DSQL_FREE		isc_dsql_free_statement
#define GDS_DSQL_INSERT		isc_dsql_insert_m
#define GDS_DSQL_PREPARE	isc_dsql_prepare_m
#define GDS_DSQL_SET_CURSOR	isc_dsql_set_cursor_name
#define GDS_DSQL_SQL_INFO	isc_dsql_sql_info


void SRVR_main(PORT main_port, USHORT flags)
{
/**************************************
 *
 *	S R V R _ m a i n
 *
 **************************************
 *
 * Functional description
 *	Main entrypoint of server.
 *
 **************************************/
	PACKET send, receive;
	PORT port;

	receive.zap (true);
	send.zap (true);
	THREAD_ENTER;
	set_server(main_port, flags);

	while (true)
	{
		//
		// Note: The following is cloned in server other SRVR_main instances.
		//
		port = main_port->receive(&receive);
		if (!port) {
			break;
		}
		if (!process_packet(port, &send, &receive, 0)) {
			break;
		}
	}

	THREAD_EXIT;
}


void SRVR_multi_thread( PORT main_port, USHORT flags)
{
#ifdef MULTI_THREAD
/**************************************
 *
 *	S R V R _ m u l t i _ t h r e a d
 *
 **************************************
 *
 * Functional description
 *	Multi-threaded flavor of server.
 *
 **************************************/
	SERVER_REQ request = NULL, active;
	volatile PORT port = NULL;
	SLONG pending_requests;
	P_OP operation;
	ISC_STATUS_ARRAY status_vector;
	Sync sync (&syncRequestQue, "SRVR_multi_thread");

	try 
		{
		set_server(main_port, flags);

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
				sync.lock (Exclusive);
				
				/* Allocate a memory block to store the request in */

				if (request = free_requests)
					free_requests = request->req_next;
				else
					{
					if (!(request = new server_req_t))
						{
						/* System is out of memory, let's delay processing this
						request and hope another thread will free memory or
						request blocks that we can then use. */

						THREAD_SLEEP(1 * 1000);
						continue;
						}
						
					request->req_send.zap(true);
					request->req_receive.zap(true);
#ifdef DEBUG_REMOTE_MEMORY
					ib_printf("SRVR_multi_thread         allocate request %x\n", request);
#endif
					}

				sync.unlock();
				request->req_next = NULL;
				request->req_chain = NULL;

				if (!(port = main_port->receive(&request->req_receive)))
					{
					gds__log("SRVR_multi_thread/RECEIVE: error on main_port, shutting down");
					return;
					}

				sync.lock (Exclusive);
				request->req_port = port;
				operation = request->req_receive.p_operation;

				/* If port has an active request, link this one in */

				for (active = active_requests; active; active = active->req_next)
					if (active->req_port == port) 
						{
						/* Don't queue a dummy keepalive packet if there is
						an active request running on this port. */

						if (operation == op_dummy) 
							{
							request->req_next = free_requests;
							free_requests = request;
							sync.unlock();
							goto finished;
							}
						port->port_requests_queued++;
						append_request_chain(request, &active->req_chain);
#ifdef DEBUG_REMOTE_MEMORY
						ib_printf
							("SRVR_multi_thread    ACTIVE     request_queued %d\n",
							port->port_requests_queued);
						ib_fflush(ib_stdout);
#endif
#ifdef CANCEL_OPERATION
						if (operation == op_exit || operation == op_disconnect)
							cancel_operation(port);
#endif
						sync.unlock();
						goto finished;
						}

				/* If port has an pending request, link this one in */

				for (active = request_que; active; active = active->req_next)
					if (active->req_port == port)
						{
						/* Don't queue a dummy keepalive packet if there is
						a pending request against this port. */

						if (operation == op_dummy) 
							{
							request->req_next = free_requests;
							free_requests = request;
							sync.unlock();
							goto finished;
							}
							
						port->port_requests_queued++;
						append_request_chain(request, &active->req_chain);
						
#ifdef DEBUG_REMOTE_MEMORY
						ib_printf ("SRVR_multi_thread    PENDING     request_queued %d\n",
							port->port_requests_queued);
						ib_fflush(ib_stdout);
#endif

#ifdef CANCEL_OPERATION
						if (operation == op_exit || operation == op_disconnect)
							cancel_operation(port);
#endif
						sync.unlock();
						goto finished;
						}

				/* No port to assign request to, add it to the waiting queue and wake up a
				 * thread to handle it 
				 */
					
				REMOTE_TRACE(("Enqueue request %p", request));
				pending_requests = append_request_next(request, &request_que);
				port->port_requests_queued++;
				sync.unlock();
				
#ifdef DEBUG_REMOTE_MEMORY
				ib_printf
					("SRVR_multi_thread    APPEND_PENDING     request_queued %d\n",
					port->port_requests_queued);
				ib_fflush(ib_stdout);
#endif

				/* NOTE: we really *should* have something that limits how many
				* total threads we allow in the system.  As each thread will
				* eat up memory that other threads could use to complete their work
				*/
				 
				/* NOTE: The setting up of extra_threads variable is done below to let waiting
				threads know if their services may be needed for the current set
				of requests.  Otherwise, some idle threads may leave the system 
				freeing up valuable memory.
				*/
				
				extra_threads = threads_waiting - pending_requests;
				
				if (extra_threads < 0) 
					{
					THD_start_thread(	reinterpret_cast<FPTR_INT_VOID_PTR>(thread),
										(void*)(long) flags,
										THREAD_medium,
										THREAD_ast,
										0);
					}

				REMOTE_TRACE(("Post event"));
				finished:
					;
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
				request->req_next = free_requests;
				free_requests = request;
				request = NULL;
				}
			}

		}
	catch (...) 
		{
		/* Some kind of unhandled error occured during server setup.  In lieu
		 * of anything we CAN do, log something (and we might be so hosed
		 * we can't log anything) and give up.
		 * The likely error here is out-of-memory.
		 */
		gds__log("SRVR_multi_thread: error during startup, shutting down");

		//RESTORE_THREAD_DATA;
		THREAD_EXIT;
		return;
		}
#endif
}


static bool accept_connection(PORT port,
							  P_CNCT * connect,
							  PACKET* send)
{
/**************************************
 *
 *	a c c e p t _ c o n n e c t i o n
 *
 **************************************
 *
 * Functional description
 *	Process a connect packet.
 *
 **************************************/
	P_ACPT *accept;
	P_ARCH architecture;
	USHORT weight, version, type;
	p_cnct::p_cnct_repeat * protocol, *end;
	TEXT buffer[64];

/* Accept the physical connection */

	send->p_operation = op_reject;
	accept = &send->p_acpt;
	weight = 0;

	if (!port->accept(connect)) {
		port->sendPacket(send);
		return false;
	}

/* Select the most appropriate protocol (this will get smarter) */

	protocol = connect->p_cnct_versions;

	for (end = protocol + connect->p_cnct_count; protocol < end; protocol++)
		if ((protocol->p_cnct_version == PROTOCOL_VERSION3 ||
			 protocol->p_cnct_version == PROTOCOL_VERSION4 ||
			 protocol->p_cnct_version == PROTOCOL_VERSION5 ||
			 protocol->p_cnct_version == PROTOCOL_VERSION6 ||
			 protocol->p_cnct_version == PROTOCOL_VERSION7 ||
			 protocol->p_cnct_version == PROTOCOL_VERSION8 ||
			 protocol->p_cnct_version == PROTOCOL_VERSION9 ||
			 protocol->p_cnct_version == PROTOCOL_VERSION10
#ifdef SCROLLABLE_CURSORS
			 || protocol->p_cnct_version == PROTOCOL_SCROLLABLE_CURSORS
#endif
			) &&
			 (protocol->p_cnct_architecture == arch_generic ||
			 protocol->p_cnct_architecture == ARCHITECTURE) &&
			protocol->p_cnct_weight >= weight)
		{
			weight = protocol->p_cnct_weight;
			version = protocol->p_cnct_version;
			architecture = protocol->p_cnct_architecture;
			type = MIN(protocol->p_cnct_max_type, ptype_out_of_band);
			send->p_operation = op_accept;
		}

/* Send off out gracious acceptance or flag rejection */

	accept->p_acpt_version = port->port_protocol = version;
	accept->p_acpt_architecture = architecture;
	accept->p_acpt_type = type;

/* and modify the version string to reflect the chosen protocol */

	sprintf(buffer, "%s/P%d", port->port_version->str_data,
			port->port_protocol);
	ALLR_free(port->port_version);
	port->port_version = REMOTE_make_string(buffer);

	if (architecture == ARCHITECTURE)
		port->port_flags |= PORT_symmetric;

	if (type == ptype_rpc)
		port->port_flags |= PORT_rpc;

	if (type != ptype_out_of_band)
		port->port_flags |= PORT_no_oob;

	port->sendPacket(send);

	return true;
}


static ISC_STATUS allocate_statement( PORT port, P_RLSE * allocate, PACKET* send)
{
/**************************************
 *
 *	a l l o c a t e _ s t a t e m e n t
 *
 **************************************
 *
 * Functional description
 *	Allocate a statement handle.
 *
 **************************************/
	RDB rdb;
	RSR statement;
	isc_handle handle;
	OBJCT object;
	ISC_STATUS_ARRAY status_vector;

	rdb = port->port_context;
	handle = NULL_HANDLE;

	THREAD_EXIT;
	GDS_DSQL_ALLOCATE(status_vector, &rdb->rdb_handle, &handle);
	THREAD_ENTER;

	if (status_vector[1])
		object = 0;
	else 
		{
		/* Allocate SQL request block */

		//statement = (RSR) ALLOC(type_rsr);
		statement = rdb->createStatement();
		//statement->rsr_rdb = rdb;
		statement->rsr_handle = handle;
		
		if (statement->rsr_id = port->getObjectId(statement))
			object = statement->rsr_id;
		else 
			{
			object = 0;
			THREAD_EXIT;
			GDS_DSQL_FREE(status_vector, &statement->rsr_handle, DSQL_drop);
			THREAD_ENTER;
			delete statement;
			//ALLR_release(statement);
			status_vector[0] = isc_arg_gds;
			status_vector[1] = isc_too_many_handles;
			status_vector[2] = isc_arg_end;
			}
		}

	return port->send_response(send, object, 0, status_vector);
}


static SLONG append_request_chain( SERVER_REQ request, SERVER_REQ * que)
{
/**************************************
 *
 *	a p p e n d _ r e q u e s t _ c h a i n
 *
 **************************************
 *
 * Functional description
 *	Traverse using req_chain ptr and append 
 *	a request at the end of a que.
 *	Return count of pending requests.
 *
 **************************************/
	SLONG requests;

	for (requests = 1; *que; que = &(*que)->req_chain)
		++requests;

	*que = request;

	return requests;
}

static SLONG append_request_next( SERVER_REQ request, SERVER_REQ * que)
{
/**************************************
 *
 *	a p p e n d _ r e q u e s t _ n e x t
 *
 **************************************
 *
 * Functional description
 *	Traverse using req_next ptr and append 
 *	a request at the end of a que.
 *	Return count of pending requests.
 *
 **************************************/
	SLONG requests;

	for (requests = 1; *que; que = &(*que)->req_next)
		++requests;

	*que = request;

	return requests;
}


static ISC_STATUS attach_database(
							  PORT port,
							  P_OP operation, P_ATCH * attach, PACKET* send)
{
/**************************************
 *
 *	a t t a c h _ d a t a b a s e
 *
 **************************************
 *
 * Functional description
 *	Process an attach or create packet.
 *
 **************************************/
	UCHAR new_dpb_buffer[512];
	isc_handle handle;
	ISC_STATUS_ARRAY status_vector;
	RDB rdb;
	STR string;

	send->p_operation = op_accept;
	handle = NULL_HANDLE;
	const char* file = reinterpret_cast<const char*>(attach->p_atch_file.cstr_address);
	const USHORT l = attach->p_atch_file.cstr_length;
	const UCHAR* dpb = attach->p_atch_dpb.cstr_address;
	USHORT dl = attach->p_atch_dpb.cstr_length;

	/* If we have user identification, append it to database parameter block */

	UCHAR* new_dpb = new_dpb_buffer;
	
	if (string = port->port_user_name) 
		{
		if ((size_t)(dl + 3 + string->str_length) > sizeof(new_dpb_buffer))
			new_dpb = ALLR_alloc((SLONG) (dl + 3 + string->str_length));
		UCHAR* p = new_dpb;
		
		if (dl) 
			{
			for (const UCHAR* const end = dpb + dl; dpb < end;)
				*p++ = *dpb++;
			}
		else
			*p++ = isc_dpb_version1;
		*p++ = isc_dpb_sys_user_name;
		*p++ = (UCHAR) string->str_length;
		dpb = (UCHAR *) string->str_data;
		for (const UCHAR* const end = dpb + string->str_length; dpb < end;)
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
		ALLR_free(new_dpb);

	if (!status_vector[1])
		{
		//port->port_context = rdb = (RDB) ALLOC(type_rdb);
		port->port_context = rdb = new RDatabase (port);
		
#ifdef DEBUG_REMOTE_MEMORY
		ib_printf("attach_databases(server)  allocate rdb     %x\n", rdb);
#endif
		rdb->rdb_port = port;
		rdb->rdb_handle = handle;
		}

	return port->send_response(send, 0, 0, status_vector);
}


static void aux_connect( PORT port, P_REQ * request, PACKET* send)
{
/**************************************
 *
 *	a u x _ c o n n e c t
 *
 **************************************
 *
 * Functional description
 *	We're receive a auxiliary connection on the main communications
 *	channel.  Accept connection and reply politely.
 *
 **************************************/

	port->connect(0, 0);
	PORT partner = (PORT) request->p_req_partner;
	partner->port_async = port;
}


static void aux_request( PORT port, P_REQ * request, PACKET* send)
{
/**************************************
 *
 *	a u x _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Other guy wants to establish a secondary connection.
 *	Humor him.
 *
 **************************************/
	ISC_STATUS *save_status;
	ISC_STATUS_ARRAY status_vector;
	PORT aux_port;
	RDB rdb;
	UCHAR buffer[12];
	CSTRING save_cstring;

/* save the port status vector */

	save_status = port->port_status_vector;

	port->port_status_vector = status_vector;
	success(status_vector);

/* We do this silliness with buffer because the SPX protocol
   requires a 12 byte buffer to be sent back.  Other protocols
   can do what they want to with cstr_address. */

	save_cstring = send->p_resp.p_resp_data;
	send->p_resp.p_resp_data.cstr_address = buffer;
	aux_port = port->request(send);
	rdb = port->port_context;
	port->send_response(send, rdb->rdb_id,
				  send->p_resp.p_resp_data.cstr_length, status_vector);

	if (status_vector[1]) {
		/* restore the port status vector */

		port->port_status_vector = save_status;
		send->p_resp.p_resp_data = save_cstring;
		return;
	}

	if (aux_port) {
		aux_port->connect(send, 0);
		aux_port->port_context = rdb;
	}

/* restore the port status vector */

	port->port_status_vector = save_status;
	send->p_resp.p_resp_data = save_cstring;
}


static ISC_STATUS cancel_events( PORT port, P_EVENT * stuff, PACKET* send)
{
/**************************************
 *
 *	c a n c e l _ e v e n t s
 *
 **************************************
 *
 * Functional description
 *	Cancel events.
 *
 **************************************/
	RDB rdb;
	RVNT event;
	ISC_STATUS_ARRAY status_vector;

/* Which database ? */

	rdb = port->port_context;

/* Find the event */

	for (event = rdb->rdb_events; event; event = event->rvnt_next)
		if (event->rvnt_rid == stuff->p_event_rid)
			break;

/* If no event found, pretend it was cancelled */

	if (!event)
		return port->send_response(send, 0, 0, status_vector);

/* cancel the event */

	if (event->rvnt_id) {
		THREAD_EXIT;
		isc_cancel_events(status_vector, &rdb->rdb_handle, &event->rvnt_id);
		THREAD_ENTER;
	}

/* zero event info */

	event->rvnt_id = 0L;
	event->rvnt_rid = 0L;
	event->rvnt_ast = 0;

/* return response */

	return port->send_response(send, 0, 0, status_vector);
}


#ifdef CANCEL_OPERATION
static void cancel_operation( PORT port)
{
/**************************************
 *
 *	c a n c e l _ o p e r a t i o n
 *
 **************************************
 *
 * Functional description
 *	Flag a running operation for cancel.
 *	Service operations are not currently
 *	able to be canceled.
 *
 **************************************/
	RDB rdb;
	ISC_STATUS_ARRAY status_vector;

	if ((port->port_flags & (PORT_async | PORT_disconnect)) ||
		!(rdb = port->port_context))
	{
		return;
	}

	if (rdb->rdb_handle)
	{
		if (!(rdb->rdb_flags & RDB_service))
		{
			THREAD_EXIT;
			isc_cancel_operation(status_vector, (isc_handle *) &rdb->rdb_handle,
								  CANCEL_raise);
			THREAD_ENTER;
		}
	}
}
#endif


static bool check_request(RRQ request,
						  USHORT incarnation,
						  USHORT msg_number)
{
/**************************************
 *
 *	c h e c k _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Check to see if a request is ready to send us a particular
 *	message.  If so, return TRUE, otherwise FALSE.
 *
 **************************************/
	USHORT n;

	if (!get_next_msg_no(request, incarnation, &n))
		return false;

	return (msg_number == n);
}


static USHORT check_statement_type( RSR statement)
{
/**************************************
 *
 *	c h e c k _ s t a t e m e n t _ t y p e
 *
 **************************************
 *
 * Functional description
 *	Return the type of SQL statement.
 *
 **************************************/
	UCHAR buffer[16], *info;
	USHORT l, type;
	ISC_STATUS_ARRAY local_status;
	USHORT ret = STMT_OTHER;
	bool done = false;

	THREAD_EXIT;
	if (!GDS_DSQL_SQL_INFO(local_status, &statement->rsr_handle,
						   sizeof(sql_info), (const SCHAR*) sql_info,
						   sizeof(buffer), reinterpret_cast<char*>(buffer)))
	{
		for (info = buffer; (*info != isc_info_end) && !done;)
		{
			l = (USHORT) gds__vax_integer(info + 1, 2);
			type = (USHORT) gds__vax_integer(info + 3, l);
			switch (*info)
			{
			case isc_info_sql_stmt_type:
				if (type == isc_info_sql_stmt_get_segment ||
					type == isc_info_sql_stmt_put_segment)
				{
					ret = STMT_BLOB;
					done = true;
				}
				break;
			case isc_info_sql_batch_fetch:
				if (type == 0)
					ret = STMT_NO_BATCH;
				break;
			case isc_info_error:
			case isc_info_truncated:
				done = true;
				break;

			}
			info += 3 + l;
		}
	}
	THREAD_ENTER;

	return ret;
}


#ifdef BEEN_MOVED
ISC_STATUS Port::compile(P_CMPL* compile, PACKET* send)
{
/**************************************
 *
 *	c o m p i l e
 *
 **************************************
 *
 * Functional description
 *	Compile and request.
 *
 **************************************/
	ISC_STATUS_ARRAY status_vector;
	rrq_repeat* tail;
	OBJCT object;

	RDB rdb = this->port_context;
	isc_handle  handle = NULL_HANDLE;
	const UCHAR* blr = compile->p_cmpl_blr.cstr_address;
	USHORT blr_length = compile->p_cmpl_blr.cstr_length;

	THREAD_EXIT;
	isc_compile_request(status_vector, &rdb->rdb_handle, &handle, blr_length,
						reinterpret_cast<const char*>(blr));
	THREAD_ENTER;

	if (status_vector[1])
		return this->send_response(send, 0, 0, status_vector);

/* Parse the request to find the messages */

	REM_MSG message = PARSE_messages(blr, blr_length);
	USHORT max_msg = 0;

	REM_MSG next;
	for (next = message; next; next = next->msg_next)
		max_msg = MAX(max_msg, next->msg_number);

/* Allocate block and merge into data structures */

	RRQ request = (RRQ) ALLOCV(type_rrq, max_msg + 1);
#ifdef DEBUG_REMOTE_MEMORY
	ib_printf("compile(server)           allocate request %x\n", request);
#endif
	request->rrq_handle = handle;
	request->rrq_rdb = rdb;
	request->rrq_max_msg = max_msg;
	if (request->rrq_id = this->get_id(&request->rrq_header))
	{
		object = request->rrq_id;
		request->rrq_next = rdb->rdb_requests;
		rdb->rdb_requests = request;
	}
	else {
		THREAD_EXIT;
		isc_release_request(status_vector, &request->rrq_handle);
		THREAD_ENTER;
		//ALLR_release(request);
		delete request;
		status_vector[0] = isc_arg_gds;
		status_vector[1] = isc_too_many_handles;
		status_vector[2] = isc_arg_end;
		return this->send_response(send, 0, 0, status_vector);
	}

	while (message) {
		next = message->msg_next;
		message->msg_next = message;
#ifdef SCROLLABLE_CURSORS
		message->msg_prior = message;
#endif

		tail = request->rrq_rpt + message->msg_number;
		tail->rrq_message = message;
		tail->rrq_xdr = message;
		tail->rrq_format = message->msg_format;

		message->msg_address = NULL;
		message = next;
	}

	return this->send_response(send, object, 0, status_vector);
}


ISC_STATUS Port::ddl(P_DDL* ddl, PACKET* send)
{
/**************************************
 *
 *	d d l
 *
 **************************************
 *
 * Functional description
 *	Execute isc_ddl call.
 *
 **************************************/
	ISC_STATUS_ARRAY status_vector;
	USHORT blr_length;
	RTR transaction;

	CHECK_HANDLE_MEMBER(transaction,
						RTR,
						type_rtr,
						ddl->p_ddl_transaction,
						isc_bad_trans_handle);

	RDB rdb = this->port_context;
	const UCHAR* blr = ddl->p_ddl_blr.cstr_address;
	blr_length = ddl->p_ddl_blr.cstr_length;

	THREAD_EXIT;
	isc_ddl(status_vector, &rdb->rdb_handle, &transaction->rtr_handle,
			blr_length, reinterpret_cast<const char*>(blr));
	THREAD_ENTER;

	return this->send_response(send, 0, 0, status_vector);
}


void Port::disconnect(PACKET* send, PACKET* receive)
{
/**************************************
 *
 *	d i s c o n n e c t
 *
 **************************************
 *
 * Functional description
 *	We've lost the connection to the parent.  Stop everything.
 *
 **************************************/

	RTR transaction;
	ISC_STATUS_ARRAY status_vector;

	if (this->port_flags & PORT_async)
		return;

	this->port_flags |= PORT_disconnect;

	RDB rdb = this->port_context;

	if (!rdb) {
		REMOTE_free_packet(this, send);
		REMOTE_free_packet(this, receive);
		this->disconnect();
		return;
	}

	/* For WNET and XNET we should send dummy op_disconnect packet
	   to wakeup async port handling events on client side.
	   For INET it's not necessary because INET client's async port
	   wakes up while server performs shutdown(socket) call on its async port.
	   See interface.cpp - event_thread(). */

	PACKET *packet = &rdb->rdb_packet;
	
	if ((this->port_async) &&
		((this->port_type == port_xnet) || (this->port_type == port_pipe)))
		{
		packet->p_operation = op_disconnect;
		this->port_async->sendPacket(packet);
		}

	if (rdb->rdb_handle)
		if (!(rdb->rdb_flags & RDB_service)) 
			{
#ifdef CANCEL_OPERATION
			/* Prevent a pending or spurious cancel from aborting
			   a good, clean detach from the database. */

			isc_cancel_operation(status_vector, (isc_handle *) &rdb->rdb_handle, CANCEL_disable);
#endif
			while (transaction = rdb->rdb_transactions) 
				{
				if (!(transaction->rtr_flags & RTR_limbo))
					isc_rollback_transaction(status_vector, &transaction->rtr_handle);
#ifdef SUPERSERVER_XXX // beat me!   JAS 5/6/04
				/* The underlying JRD subsystem will release all
				   memory resources related to a limbo transaction
				   as a side-effect of the database detach call
				   below. However, the y-valve handle must be released
				   as well since an isc_disconnect_transaction()
				   call doesn't exist. */

				else
					gds__handle_cleanup(status_vector, &transaction->rtr_handle);
#endif
				delete rdb->rdb_transactions;
				}
				
			isc_detach_database(status_vector, &rdb->rdb_handle);
			rdb->clearObjects();		
			}
		else
			isc_service_detach(status_vector, &rdb->rdb_handle);

	REMOTE_free_packet(this, send);
	REMOTE_free_packet(this, receive);

#ifdef DEBUG_REMOTE_MEMORY
	ib_printf("disconnect(server)        free rdb         %x\n", rdb);
#endif
	this->port_context = NULL;
	//ALLR_release(rdb);
	delete rdb;
	
	if (this->port_object_vector)
	{
#ifdef DEBUG_REMOTE_MEMORY
		ib_printf("disconnect(server)        free vector      %x\n",
				  this->port_object_vector);
#endif
		ALLR_release(this->port_object_vector);
		this->port_object_vector = NULL;
	}
	
	if (this->port_connection)
	{
#ifdef DEBUG_REMOTE_MEMORY
		ib_printf("disconnect(server)        free string      %x\n",
				  this->port_connection);
#endif
		ALLR_release(this->port_connection);
		this->port_connection = NULL;
	}
	if (this->port_version)
	{
#ifdef DEBUG_REMOTE_MEMORY
		ib_printf("disconnect(server)        free string      %x\n",
				  this->port_version);
#endif
		ALLR_release(this->port_version);
		this->port_version = NULL;
	}
	if (this->port_passwd)
	{
#ifdef DEBUG_REMOTE_MEMORY
		ib_printf("disconnect(server)        free string      %x\n",
				  this->port_passwd);
#endif
		ALLR_release(this->port_passwd);
		this->port_passwd = NULL;
	}
	if (this->port_user_name)
	{
#ifdef DEBUG_REMOTE_MEMORY
		ib_printf("disconnect(server)        free string      %x\n",
				  this->port_user_name);
#endif
		ALLR_release(this->port_user_name);
		this->port_user_name = NULL;
	}
	if (this->port_host)
	{
#ifdef DEBUG_REMOTE_MEMORY
		ib_printf("disconnect(server)        free string      %x\n",
				  this->port_host);
#endif
		ALLR_release(this->port_host);
		this->port_host = NULL;
	}
	this->disconnect();
}


void Port::drop_database(P_RLSE* release, PACKET* send)
{
/**************************************
 *
 *	d r o p _ d a t a b a s e
 *
 **************************************
 *
 * Functional description
 *	End a request.
 *
 **************************************/
	RDB rdb;
	ISC_STATUS_ARRAY status_vector;

	rdb = this->port_context;

	THREAD_EXIT;
	isc_drop_database(status_vector, &rdb->rdb_handle);
	THREAD_ENTER;

	if (status_vector[1]
		&& (status_vector[1] != isc_drdb_completed_with_errs)) {
		this->send_response(send, 0, 0, status_vector);
		return;
	};

	rdb->clearObjects();

	this->send_response(send, 0, 0, status_vector);
}
#endif


#ifdef SCROLLABLE_CURSORS
static REM_MSG dump_cache( rrq_repeat* tail)
{
/**************************************
 *
 *	d u m p _ c a c h e
 *
 **************************************
 *
 * Functional description
 *	We have encountered a situation where what's in 
 *	cache is not useful, so empty the cache in 
 *	preparation for refilling it. 
 *
 **************************************/
	REM_MSG message;

	message = tail->rrq_xdr;
	while (true) {
		message->msg_address = NULL;
		message = message->msg_next;
		if (message == tail->rrq_xdr)
			break;
	}

	tail->rrq_message = message;

	return message;
}
#endif



static bool get_next_msg_no(RRQ request,
							USHORT incarnation,
							USHORT * msg_number)
{
/**************************************
 *
 *	g e t _ n e x t _ m s g _ n o
 *
 **************************************
 *
 * Functional description
 *	Return the number of the next message
 *	in the request.
 *
 **************************************/
	USHORT l;
	USHORT n;
	ISC_STATUS_ARRAY status_vector;
	UCHAR info_buffer[128], *info;

	THREAD_EXIT;
	isc_request_info(status_vector, &request->rrq_handle, incarnation,
					 sizeof(request_info), reinterpret_cast<const SCHAR*>(request_info),
					 sizeof(info_buffer), reinterpret_cast<char*>(info_buffer));
	THREAD_ENTER;

	if (status_vector[1])
		return false;

	bool result = false;
	for (info = info_buffer; *info != isc_info_end;) {
		l = (USHORT) gds__vax_integer(info + 1, 2);
		n = (USHORT) gds__vax_integer(info + 3, l);
		switch (*info) {
		case isc_info_state:
			if (n != isc_info_req_send)
				return false;
			break;

		case isc_info_message_number:
			*msg_number = n;
			result = true;
			break;

		default:
			return false;
		}
		info += 3 + l;
	}

	return result;
}




bool process_packet(PORT port,
					PACKET* send,
					PACKET* receive,
					PORT * result)
{
/**************************************
 *
 *	p r o c e s s _ p a c k e t
 *
 **************************************
 *
 * Functional description
 *	Given an packet received from client, process it a packet ready to be
 *	sent.
 *
 **************************************/
	P_OP op;
	STR string;
	TEXT msg[128];
	SRVR server;
	
	/***
	struct trdb thd_context, *trdb;
	trdb = &thd_context;
	trdb->trdb_status_vector = port->port_status_vector;
	THD_put_specific((THDD) trdb, THDD_TYPE_SRVR);
	trdb->trdb_thd_data.thdd_type = THDD_TYPE_TRDB;
	***/
	
	try 
		{
		switch (op = receive->p_operation)
			{
			case op_connect:
				if (!accept_connection(port, &receive->p_cnct, send)) 
					{
					if (string = port->port_user_name) 
						{
						sprintf(msg,
								"SERVER/process_packet: connection rejected for %*.*s",
								string->str_length, string->str_length,
								string->str_data);
						gds__log(msg, 0);
						}
					if (port->port_server->srvr_flags & SRVR_multi_client) 
						port->port_state = state_broken;
					else 
						{
						gds__log ("SERVER/process_packet: connect reject, server exiting", 0);
						THD_restore_specific(THDD_TYPE_SRVR);
						return false;
						}
					}
				break;

			case op_compile:
				port->compile(&receive->p_cmpl, send);
				break;

			case op_attach:
			case op_create:
				attach_database(port, op, &receive->p_atch, send);
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
					port->disconnect(send, receive);
					port = NULL;
					break;
					}
				if ((server->srvr_flags & SRVR_multi_client) && port == server->srvr_parent_port)
					gds__log("SERVER/process_packet: Multi-client server shutdown", 0);
				port->disconnect(send, receive);
				THD_restore_specific(THDD_TYPE_SRVR);
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
				port->end_database(&receive->p_rlse, send);
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
				cancel_events(port, &receive->p_event, send);
				break;

			case op_connect_request:
				aux_request(port, &receive->p_req, send);
				break;

			case op_aux_connect:
				aux_connect(port, &receive->p_req, send);
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
				allocate_statement(port, &receive->p_rlse, send);
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

			case op_dummy:
				send->p_operation = op_dummy;
				port->sendPacket(send);
				break;

			default:
				sprintf(msg, "SERVER/process_packet: don't understand packet type %d",
						receive->p_operation);
				gds__log(msg, 0);
				port->port_state = state_broken;
				break;
			}

		if (port && port->port_state == state_broken) 
			{
			if (!port->port_parent) 
				{
				gds__log("SERVER/process_packet: broken port, server exiting", 0);
				if (port->port_type == port_inet)
					port->disconnect();
				else
					port->disconnect(send, receive);
				THD_restore_specific(THDD_TYPE_SRVR);
				return false;
				}
			port->disconnect(send, receive);
			port = NULL;
			}

		if (result)
			*result = port;

		THD_restore_specific(THDD_TYPE_SRVR);
		}
	catch (OSRIException &exception) 
		{
		/* There must be something better to do here.  BUT WHAT? */

		gds__log("SERVER/process_packet: out of memory", 0);

		/*  It would be nice to log an error to the user, instead of just terminating them!  */
		port->send_response(send, 0, 0, exception.statusVector);
		port->disconnect(send, receive);	/*  Well, how about this...  */
		THD_restore_specific(THDD_TYPE_SRVR);
		return false;
		}

	return true;
}

#ifdef BEEN_MOVED
ISC_STATUS Port::put_segment(P_OP op, P_SGMT * segment, PACKET* send)
{
/**************************************
 *
 *	p u t _ s e g m e n t
 *
 **************************************
 *
 * Functional description
 *	Write a single blob segment.
 *
 **************************************/
	RBL blob;
	USHORT length;
	ISC_STATUS_ARRAY status_vector;

	CHECK_HANDLE_MEMBER(blob,
						RBL,
						type_rbl,
						segment->p_sgmt_blob,
						isc_bad_segstr_handle);

	const UCHAR* p = segment->p_sgmt_segment.cstr_address;
	length = segment->p_sgmt_segment.cstr_length;

/* Do the signal segment version.  If it failed, just pass on the
   bad news. */

	if (op == op_put_segment) {
		THREAD_EXIT;
		isc_put_segment(status_vector, &blob->rbl_handle, length,
						reinterpret_cast<const char*>(p));
		THREAD_ENTER;
		return this->send_response(send, 0, 0, status_vector);
	}

/* We've got a batch of segments.  This is only slightly more complicated */

	const UCHAR* const end = p + length;

	while (p < end) {
		length = *p++;
		length += *p++ << 8;
		THREAD_EXIT;
		isc_put_segment(status_vector, &blob->rbl_handle, length,
						reinterpret_cast<const char*>(p));
		THREAD_ENTER;
		if (status_vector[1])
			return this->send_response(send, 0, 0, status_vector);
		p += length;
	}

	return this->send_response(send, 0, 0, status_vector);
}


ISC_STATUS Port::put_slice(P_SLC * stuff, PACKET* send)
{
/**************************************
 *
 *	p u t _ s l i c e
 *
 **************************************
 *
 * Functional description
 *	Put an array slice.
 *
 **************************************/
	RDB rdb;
	RTR transaction;
	ISC_STATUS_ARRAY status_vector;

	CHECK_HANDLE_MEMBER(transaction,
						RTR,
						type_rtr,
						stuff->p_slc_transaction,
						isc_bad_trans_handle);

	rdb = this->port_context;

	THREAD_EXIT;
	send->p_resp.p_resp_blob_id = stuff->p_slc_id;
	isc_put_slice(status_vector, &rdb->rdb_handle, &transaction->rtr_handle,
				  (ISC_QUAD*) &send->p_resp.p_resp_blob_id,
				  stuff->p_slc_sdl.cstr_length,
				  reinterpret_cast<char*>(stuff->p_slc_sdl.cstr_address),
				  stuff->p_slc_parameters.cstr_length,
				  (ISC_LONG *) stuff->p_slc_parameters.cstr_address,
				  stuff->p_slc_slice.lstr_length,
				  stuff->p_slc_slice.lstr_address);
	THREAD_ENTER;

	return this->send_response(send, 0, 0, status_vector);
}


ISC_STATUS Port::que_events(P_EVENT * stuff, PACKET* send)
{
/**************************************
 *
 *	q u e _ e v e n t s
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	ISC_STATUS_ARRAY status_vector;
	RDB rdb;
	RVNT event;
	SLONG id;

	rdb = this->port_context;

/* Find unused event block or, if necessary, a new one */

	for (event = rdb->rdb_events; event; event = event->rvnt_next) {
		if (!event->rvnt_id) {
			break;
		}
	}

	if (!event)
	{
		event = (RVNT) ALLOC(type_rvnt);
#ifdef DEBUG_REMOTE_MEMORY
		ib_printf("que_events(server)        allocate event   %x\n", event);
#endif
		event->rvnt_next = rdb->rdb_events;
		rdb->rdb_events = event;
	}

	event->rvnt_ast = stuff->p_event_ast;
	// CVC: Going from SLONG to void*, problems when sizeof(void*) > 4
	event->rvnt_arg = (void *) stuff->p_event_arg;
	event->rvnt_rid = stuff->p_event_rid;
	event->rvnt_rdb = rdb;

	THREAD_EXIT;
	isc_que_events(status_vector, &rdb->rdb_handle, &event->rvnt_id,
				   stuff->p_event_items.cstr_length,
				   reinterpret_cast<const char*>(stuff->p_event_items.cstr_address),
				   server_ast,
				   event);
	THREAD_ENTER;

	id = event->rvnt_id;
	if (status_vector[1]) {
		event->rvnt_id = 0;
	}

	return this->send_response(send, (OBJCT) id, 0, status_vector);
}


ISC_STATUS Port::receive_after_start(	P_DATA*	data,
									PACKET*	send,
									ISC_STATUS*	status_vector)
{
/**************************************
 *
 *	r e c e i v e _ a f t e r _ s t a r t
 *
 **************************************
 *
 * Functional description
 *	Receive a message.
 *
 **************************************/
	RRQ request;
	USHORT level, msg_number;
	FMT format;
	P_RESP *response;
	rrq_repeat* tail;

	CHECK_HANDLE_MEMBER(request,
						RRQ,
						type_rrq,
						data->p_data_request,
						isc_bad_req_handle);

	level = data->p_data_incarnation;
	request = REMOTE_find_request(request, level);

/* Figure out the number of the message that we're stalled on. */

	if (!get_next_msg_no(request, level, &msg_number)) {
		return this->send_response(send, 0, 0, status_vector);
	}

	send->p_operation = op_response_piggyback;
	response = &send->p_resp;
	response->p_resp_object = msg_number;
	response->p_resp_status_vector = status_vector;
	response->p_resp_data.cstr_length = 0;

	this->send_partial(send);

/* Fill in packet to fool receive into thinking that it has been
   called directly by the client. */

	tail = request->rrq_rpt + msg_number;
	format = tail->rrq_format;

	data->p_data_message_number = msg_number;
	if (this->port_flags & PORT_rpc)
	{
		data->p_data_messages = 1;
	}
	else
	{
		data->p_data_messages =
			(USHORT) REMOTE_compute_batch_size(this,
											   (USHORT)
											   xdr_protocol_overhead
											   (op_response_piggyback),
											   op_send,
											   format);
	}

	return this->receive_msg(data, send);
}


ISC_STATUS Port::receive_msg(P_DATA * data, PACKET* send)
{
/**************************************
 *
 *	r e c e i v e _ m s g
 *
 **************************************
 *
 * Functional description
 *	Receive a message.
 *
 **************************************/

	ISC_STATUS_ARRAY status_vector;
	REM_MSG message, next, prior;
	RRQ request;
	FMT format;
	USHORT msg_number, count, count2, level;

#ifdef SCROLLABLE_CURSORS
	USHORT direction;
	ULONG offset;
#endif

	rrq_repeat* tail;

/* Find the database, request, message number, and the number of 
   messages the client is willing to cope with.  Then locate the
   message control block for the request and message type. */

	CHECK_HANDLE_MEMBER(request,
						RRQ,
						type_rrq,
						data->p_data_request,
						isc_bad_req_handle);

	level = data->p_data_incarnation;
	request = REMOTE_find_request(request, level);
	msg_number = data->p_data_message_number;
	count2 = count =
		(this->port_flags & PORT_rpc) ? 1 : data->p_data_messages;
	tail = request->rrq_rpt + msg_number;
	format = tail->rrq_format;

/* Get ready to ship the data out */

	send->p_operation = op_send;
	send->p_data.p_data_request = data->p_data_request;
	send->p_data.p_data_message_number = msg_number;
	send->p_data.p_data_incarnation = level;
	send->p_data.p_data_messages = 1;

#ifdef SCROLLABLE_CURSORS
/* check the direction and offset for possible redirection; if the user 
   scrolls in a direction other than we were going or an offset other 
   than 1, then we need to resynchronize: 
   if the direction is the same as the lookahead cache, scroll forward 
   through the cache to see if we find the record; otherwise scroll the 
   next layer down backward by an amount equal to the number of records 
   in the cache, plus the amount asked for by the next level up */

	if (this->port_protocol < PROTOCOL_SCROLLABLE_CURSORS)
	{
		direction = blr_forward;
		offset = 1;
	}
	else
	{
		direction = data->p_data_direction;
		offset = data->p_data_offset;
		tail->rrq_xdr = scroll_cache(tail, &direction, &offset);
	}
#endif

/* Check to see if any messages are already sitting around */

	while (true) {
		message = tail->rrq_xdr;

		/* If we don't have a message cached, get one from the next layer down. */

		if (!message->msg_address) {
			/* If we have an error queued for delivery, send it now */

			if (request->rrq_status_vector[1]) {
				ISC_STATUS res;
				res =
					this->send_response(send, 0, 0,
								  request->rrq_status_vector);
				memset(request->rrq_status_vector, 0,
					   sizeof(request->rrq_status_vector));
				return res;
			}

			THREAD_EXIT;
#ifdef SCROLLABLE_CURSORS
			isc_receive2(status_vector, &request->rrq_handle, msg_number, 
						 format->fmt_length, message->msg_buffer, level, 
						 direction, offset);
#else
			isc_receive(status_vector, &request->rrq_handle, msg_number,
						format->fmt_length, message->msg_buffer, level);
#endif
			THREAD_ENTER;
			if (status_vector[1])
				return this->send_response(send, 0, 0, status_vector);

#ifdef SCROLLABLE_CURSORS
			/* set the appropriate flags according to the way we just scrolled 
			   the next layer down, and calculate the offset from the beginning 
			   of the result set */

			switch (direction) {
			case blr_forward:
				tail->rrq_flags &= ~RRQ_backward;
				tail->rrq_absolute +=
					(tail->
					 rrq_flags & RRQ_absolute_backward) ? -offset : offset;
				break;

			case blr_backward:
				tail->rrq_flags |= RRQ_backward;
				tail->rrq_absolute +=
					(tail->
					 rrq_flags & RRQ_absolute_backward) ? offset : -offset;
				break;

			case blr_bof_forward:
				tail->rrq_flags &= ~RRQ_backward;
				tail->rrq_flags &= ~RRQ_absolute_backward;
				tail->rrq_absolute = offset;
				direction = blr_forward;
				break;

			case blr_eof_backward:
				tail->rrq_flags |= RRQ_backward;
				tail->rrq_flags |= RRQ_absolute_backward;
				tail->rrq_absolute = offset;
				direction = blr_backward;
				break;
			}

			message->msg_absolute = tail->rrq_absolute;

			/* if we have already scrolled to the location indicated, 
			   then we just want to continue one by one in that direction */

			offset = 1;
#endif

			message->msg_address = message->msg_buffer;
		}

		/* If there aren't any buffers remaining, break out of loop */

		if (--count <= 0)
			break;

		/* There's a buffer waiting -- see if the request is ready to send */

		next = message->msg_next;

		if ((next == message || !next->msg_address) &&
			!check_request(request, data->p_data_incarnation, msg_number)) {
			/* We've reached the end of the RSE - don't prefetch and flush
			   everything we've buffered so far */

			count2 = 0;
			break;
		}

		if (!this->send_partial(send))
			return FALSE;
		message->msg_address = NULL;
	}

	send->p_data.p_data_messages = 0;
	this->sendPacket(send);
	message->msg_address = NULL;

/* Bump up the message pointer to resync with rrq_xdr (rrq_xdr 
   was incremented by xdr_request in the SEND call).  */

	tail->rrq_message = message->msg_next;

/* Since we have a little time on our hands while this packet is sent
   and processed, get the next batch of records.  Start by finding the
   next free buffer. */

	message = tail->rrq_xdr;
	prior = NULL;

	while (message->msg_address && message->msg_next != tail->rrq_xdr)
		message = message->msg_next;

	for (;
		 count2
		 && check_request(request, data->p_data_incarnation, msg_number);
		 --count2) {
		if (message->msg_address) {
			if (!prior)
#ifdef SCROLLABLE_CURSORS
				prior = message->msg_prior;
#else
				for (prior = tail->rrq_xdr; prior->msg_next != message;
					 prior = prior->msg_next);
#endif

			/* allocate a new message block and put it in the cache */

			message = (REM_MSG) ALLOCV(type_msg, format->fmt_length);
#ifdef DEBUG_REMOTE_MEMORY
			ib_printf("receive_msg(server)       allocate message %x\n",
					  message);
#endif
			message->msg_number = prior->msg_number;
			message->msg_next = prior->msg_next;
#ifdef SCROLLABLE_CURSORS
			message->msg_prior = prior;
#endif

			prior->msg_next = message;
			prior = message;
		}

		/* fetch the next record into cache; even for scrollable cursors, we are 
		   just doing a simple lookahead continuing on in the last direction specified, 
		   so there is no reason to do an isc_receive2() */

		THREAD_EXIT;
		isc_receive(status_vector, &request->rrq_handle, msg_number,
					format->fmt_length,
					message->msg_buffer, data->p_data_incarnation);
		THREAD_ENTER;

		/* Did we have an error?  If so, save it for later delivery */

		if (status_vector[1]) {
			/* If already have an error queued, don't overwrite it */

			if (!request->rrq_status_vector[1]) {
				memcpy(request->rrq_status_vector, status_vector,
					   sizeof(request->rrq_status_vector));
			}
			break;
		}

#ifdef SCROLLABLE_CURSORS
		/* if we have already scrolled to the location indicated, 
		   then we just want to continue on in that direction */

		switch (direction)
		{
		case blr_forward:
			tail->rrq_absolute +=
				(tail->rrq_flags & RRQ_absolute_backward) ? -offset : offset;
			break;

		case blr_backward:
			tail->rrq_absolute +=
				(tail->rrq_flags & RRQ_absolute_backward) ? offset : -offset;
			break;
		}

		message->msg_absolute = tail->rrq_absolute;
#endif

		message->msg_address = message->msg_buffer;
		message = message->msg_next;
	}

	return TRUE;
}
#endif


static void release_event( RVNT event)
{
/**************************************
 *
 *	r e l e a s e _ e v e n t
 *
 **************************************
 *
 * Functional description
 *	Release an event block.
 *
 **************************************/
	RDB rdb;
	RVNT *p;

	rdb = event->rvnt_rdb;

	for (p = &rdb->rdb_events; *p; p = &(*p)->rvnt_next)
		if (*p == event) {
			*p = event->rvnt_next;
			break;
		}

	ALLR_release(event);
}



#ifdef SCROLLABLE_CURSORS
static REM_MSG scroll_cache(
						rrq_repeat* tail,
						USHORT * direction, ULONG * offset)
{
/**************************************
 *
 *	s c r o l l _ c a c h e
 *
 **************************************
 *
 * Functional description
 *	
 * Try to fetch the requested record from cache, if possible.  This algorithm depends 
 * on all scrollable cursors being INSENSITIVE to database changes, so that absolute 
 * record numbers within the result set will remain constant. 
 *
 *  1.  BOF Forward or EOF Backward:  Retain the record number of the offset from the 
 *      beginning or end of the result set.  If we can figure out the relative offset 
 *      from the absolute, then scroll to it.  If it's in cache, great, otherwise dump 
 *      the cache and have the server scroll the correct number of records. 
 *
 *  2.  Forward or Backward:  Try to scroll the desired offset in cache.  If we  
 *      scroll off the end of the cache, dump the cache and ask the server for a 
 *      packetful of records.  
 *
 *  This routine differs from the corresponding routine on the client in that 
 *  we are using only a lookahead cache.  There is no point in caching records backward, 
 *  in that the client already has them and would not request them from us. 
 *
 **************************************/
	REM_MSG message;

/* if we are to continue in the current direction, set direction to 
   the last direction scrolled; then depending on the direction asked 
   for, save the last direction asked for by the next layer above */

	if (*direction == blr_continue) {
		if (tail->rrq_flags & RRQ_last_backward)
			*direction = blr_backward;
		else
			*direction = blr_forward;
	}

	if (*direction == blr_forward || *direction == blr_bof_forward)
		tail->rrq_flags &= ~RRQ_last_backward;
	else
		tail->rrq_flags |= RRQ_last_backward;

/* set to the first message; if it has no record, this means the cache is 
   empty and there is no point in trying to find the record here */

	message = tail->rrq_xdr;
	if (!message->msg_address)
		return message;

/* if we are scrolling from BOF and the cache was started from EOF (or vice 
   versa), the cache is unusable. */

	if (
		(*direction == blr_bof_forward
		 && (tail->rrq_flags & RRQ_absolute_backward))
		|| (*direction == blr_eof_backward
			&& !(tail->
				 rrq_flags & RRQ_absolute_backward))) return dump_cache(tail);

/* if we are going to an absolute position, see if we can find that position 
   in cache, otherwise change to a relative seek from our former position */

	if (*direction == blr_bof_forward || *direction == blr_eof_backward) {
		/* If offset is before our current position, just dump the cache because 
		   the server cache is purely a lookahead cache--we don't bother to cache 
		   back records because the client will cache those records, making it 
		   unlikely the client would be asking us for a record which is in our cache. */

		if (*offset < message->msg_absolute)
			return dump_cache(tail);

		/* convert the absolute to relative, and prepare to scroll forward to look for the record */

		*offset -= message->msg_absolute;
		if (*direction == blr_bof_forward)
			*direction = blr_forward;
		else
			*direction = blr_backward;
	}

	if ((*direction == blr_forward && (tail->rrq_flags & RRQ_backward)) ||
		(*direction == blr_backward && !(tail->rrq_flags & RRQ_backward))) {
		/* lookahead cache was in opposite direction from the scroll direction, 
		   so increase the scroll amount by the amount we looked ahead, then 
		   dump the cache */

		for (message = tail->rrq_xdr; message->msg_address;) {
			(*offset)++;
			message = message->msg_next;
			if (message == tail->rrq_message)
				break;
		}

		return dump_cache(tail);
	}
	else {
		/* lookahead cache is in same direction we want to scroll, so scroll 
		   forward through the cache, decrementing the offset */

		for (message = tail->rrq_xdr; message->msg_address;) {
			if (*offset == 1)
				break;
			(*offset)--;
			message = message->msg_next;
			if (message == tail->rrq_message)
				break;
		}
	}

	return message;
}
#endif

#ifdef BEEN_MOVED
ISC_STATUS Port::seek_blob(P_SEEK * seek, PACKET* send)
{
/**************************************
 *
 *	s e e k _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Execute a blob seek operation.
 *
 **************************************/
	RBL blob;
	SSHORT mode;
	SLONG offset, result;
	ISC_STATUS_ARRAY status_vector;

	CHECK_HANDLE_MEMBER(blob,
						RBL,
						type_rbl,
						seek->p_seek_blob,
						isc_bad_segstr_handle);

	mode = seek->p_seek_mode;
	offset = seek->p_seek_offset;

	THREAD_EXIT;
	isc_seek_blob(status_vector, &blob->rbl_handle, mode, offset, &result);
	THREAD_ENTER;

	send->p_resp.p_resp_blob_id.bid_number = result;

	return this->send_response(send, 0, 0, status_vector);
}


ISC_STATUS Port::send_msg(P_DATA * data, PACKET* send)
{
/**************************************
 *
 *	s e n d _ m s g
 *
 **************************************
 *
 * Functional description
 *	Handle a isc_send operation.
 *
 **************************************/
	ISC_STATUS_ARRAY status_vector;
	RRQ request;
	FMT format;
	USHORT number;

	CHECK_HANDLE_MEMBER(request,
						RRQ,
						type_rrq,
						data->p_data_request,
						isc_bad_req_handle);

	number = data->p_data_message_number;
	request = REMOTE_find_request(request, data->p_data_incarnation);
	REM_MSG message = request->rrq_rpt[number].rrq_message;
	format = request->rrq_rpt[number].rrq_format;

	THREAD_EXIT;
	isc_send(status_vector, &request->rrq_handle, number, format->fmt_length,
			 message->msg_address, data->p_data_incarnation);
	THREAD_ENTER;

	message->msg_address = NULL;

	return this->send_response(send, 0, 0, status_vector);
}


ISC_STATUS Port::send_response(	PACKET*	send,
							OBJCT	object,
							USHORT	length,
							ISC_STATUS* status_vector)
{
/**************************************
 *
 *	s e n d _ r e s p o n s e
 *
 **************************************
 *
 * Functional description
 *	Send a response packet.
 *
 **************************************/
	ISC_STATUS *v, code, exit_code;
	ISC_STATUS_ARRAY new_vector;
	USHORT l, sw;
	TEXT *p, *q, **sp, buffer[1024];
	P_RESP *response;

/* Start by translating the status vector into "generic" form */

	v = new_vector;
	p = buffer;
	exit_code = status_vector[1];

	for (sw = TRUE; *status_vector && sw;)
		{
		switch ((USHORT) * status_vector)
			{
			case isc_arg_warning:
			case isc_arg_gds:
				{
				USHORT fac = 0, class_ = 0;

				/* When talking with older (pre 6.0) clients, do not send
				 * warnings.
				 */

				if (*status_vector == isc_arg_warning &&
					this->port_protocol < PROTOCOL_VERSION10)
					{
					sw = FALSE;
					continue;
					}

				*v++ = *status_vector++;

				/* The status codes are converted to their offsets so that they
				* were compatible with the RDB implementation.  This was fine
				* when status codes were restricted to a single facility.  Now
				* that the facility is part of the status code we need to know
				* this on the client side, thus when talking with 6.0 and newer
				* clients, do not decode the status code, just send it to the
				* client.  The same check is made in interface.c::check_response
				*/

				if (this->port_protocol < PROTOCOL_VERSION10)
					*v++ = code = gds__decode(*status_vector++, &fac, &class_);
				else
					*v++ = code = *status_vector++;
					
				for (;;) 
					{
					switch (*status_vector) 
						{
						case isc_arg_string:
						case isc_arg_number:
							*v++ = *status_vector++;
							*v++ = *status_vector++;
							continue;

						case isc_arg_cstring:
							*v++ = isc_arg_string;
							sp = (TEXT **) v;
							*sp++ = p;
							v = (ISC_STATUS *) sp;
							status_vector++;
							l = (USHORT) (*status_vector++);
							q = (TEXT *) * status_vector++;
							if (l)
								do
									*p++ = *q++;
								while (--l);
							*p++ = 0;
							continue;
							}
					break;
					}
				}
				continue;

			case isc_arg_interpreted:
				*v++ = *status_vector++;
				*v++ = *status_vector++;
				continue;
			}
			
		l = (USHORT) isc_interprete (p, &status_vector);
		
		if (l == 0)
			break;
			
		*v++ = isc_arg_interpreted;
		sp = (TEXT **) v;
		*sp++ = p;
		v = (ISC_STATUS *) sp;
		p += l;
		sw = FALSE;
		}

	*v = isc_arg_end;

	/* Format and send response.  Note: the blob_id and data address fields
	   of the response packet may contain valid data.  Don't trash them. */

	send->p_operation = op_response;
	response = &send->p_resp;
	response->p_resp_object = object;
	response->p_resp_status_vector = new_vector;
	response->p_resp_data.cstr_length = length;

	this->sendPacket(send);

	return exit_code;
}
#endif


static void server_ast(void* event_void, USHORT length, const UCHAR* items)
{
/**************************************
 *
 *	s e r v e r _ a s t
 *
 **************************************
 *
 * Functional description
 *	Send an asynchrous event packet back to client.
 *
 **************************************/
	RVNT event = reinterpret_cast<RVNT>(event_void);
	RDB rdb;
	PORT port;
	PACKET packet;
	P_EVENT *p_event;

	THREAD_ENTER;
	event->rvnt_id = 0;
	rdb = event->rvnt_rdb;

	if (!(port = rdb->rdb_port->port_async)) {
		THREAD_EXIT;
		return;
	}

	packet.p_operation = op_event;
	p_event = &packet.p_event;
	p_event->p_event_database = rdb->rdb_id;
	p_event->p_event_items.cstr_length = length;
	// Probalby should define this item with CSTRING_CONST instead.
	p_event->p_event_items.cstr_address = const_cast<UCHAR*>(items);
	//p_event->p_event_ast = event->rvnt_ast;
	//p_event->p_event_arg = (SLONG) event->rvnt_arg;
	p_event->p_event_rid = event->rvnt_rid;

	port->sendPacket(&packet);
	THREAD_EXIT;
}


#ifdef BEEN_MOVED
ISC_STATUS Port::service_attach(P_ATCH* attach, PACKET* send)
{
/**************************************
 *
 *	s e r v i c e _ a t t a c h 
 *
 **************************************
 *
 * Functional description
 *	Connect to an Interbase service.
 *
 **************************************/
	USHORT service_length, spb_length;
	UCHAR *service_name, *spb, new_spb_buffer[512], *new_spb, *p, *end;
	isc_handle handle;
	ISC_STATUS_ARRAY status_vector;
	RDB rdb;
	STR string;

	send->p_operation = op_accept;
	handle = NULL_HANDLE;
	service_name = attach->p_atch_file.cstr_address;
	service_length = attach->p_atch_file.cstr_length;
	spb = attach->p_atch_dpb.cstr_address;
	spb_length = attach->p_atch_dpb.cstr_length;

/* If we have user identification, append it to database parameter block */

	new_spb = new_spb_buffer;
	if (string = this->port_user_name)
	{
		if ((spb_length + 3 + string->str_length) > (int)sizeof(new_spb_buffer))
			new_spb =
				ALLR_alloc((SLONG) (spb_length + 3 + string->str_length));
		p = new_spb;
		if (spb_length)
			for (end = spb + spb_length; spb < end;)
				*p++ = *spb++;
		else
			*p++ = isc_spb_current_version;

		*p++ = isc_spb_sys_user_name;
		*p++ = (UCHAR) string->str_length;
		for (spb = (UCHAR *) string->str_data, end = spb + string->str_length;
			 spb < end;)
			*p++ = *spb++;
		spb = new_spb;
		spb_length = p - new_spb;
	}

/* See if user has specified parameters relevent to the connection,
   they will be stuffed in the SPB if so. */
	REMOTE_get_timeout_params(this, spb, spb_length);

	THREAD_EXIT;
	isc_service_attach(status_vector,
					   service_length,
					   reinterpret_cast<char*>(service_name),
					   &handle,
					   spb_length,
					   reinterpret_cast<char*>(spb));
	THREAD_ENTER;

	if (new_spb != new_spb_buffer)
		ALLR_free(new_spb);

	if (!status_vector[1]) {
		this->port_context = rdb = (RDB) ALLOC(type_rdb);
#ifdef DEBUG_REMOTE_MEMORY
		ib_printf("attach_service(server)  allocate rdb     %x\n", rdb);
#endif
		rdb->rdb_port = this;
		rdb->rdb_handle = handle;
		rdb->rdb_flags |= RDB_service;
	}

	return this->send_response(send, 0, 0, status_vector);
}


ISC_STATUS Port::service_end(P_RLSE * release, PACKET* send)
{
/**************************************
 *
 *	s e r v i c e _ e n d
 *
 **************************************
 *
 * Functional description
 *	Close down a service.
 *
 **************************************/

	ISC_STATUS_ARRAY status_vector;

	RDB rdb = this->port_context;

	THREAD_EXIT;
	isc_service_detach(status_vector, &rdb->rdb_handle);
	THREAD_ENTER;

	return this->send_response(send, 0, 0, status_vector);
}


ISC_STATUS Port::service_start(P_INFO * stuff, PACKET* send)
{
/**************************************
 *
 *	s e r v i c e _ s t a r t
 *
 **************************************
 *
 * Functional description
 *	Start a service on the server
 *
 **************************************/
	RDB rdb;
	ISC_STATUS_ARRAY status_vector;
	ULONG *reserved = 0;		/* reserved for future use */

	rdb = this->port_context;

	THREAD_EXIT;
	isc_service_start(status_vector,
					  &rdb->rdb_handle,
					  reinterpret_cast<long*>(reserved),
					  stuff->p_info_items.cstr_length,
					  reinterpret_cast<char*>(stuff->p_info_items.cstr_address));
	THREAD_ENTER;

	return this->send_response(send, 0, 0, status_vector);
}


ISC_STATUS Port::set_cursor(P_SQLCUR * sqlcur, PACKET* send)
{
/*****************************************
 *
 *	s e t _ c u r s o r
 *
 *****************************************
 *
 * Functional description
 *	Set a cursor name for a dynamic request.
 *
 *****************************************/
	RSR statement;
	ISC_STATUS_ARRAY status_vector;

	CHECK_HANDLE_MEMBER(statement,
						RSR,
						type_rsr,
						sqlcur->p_sqlcur_statement,
						isc_bad_req_handle);

	THREAD_EXIT;
	GDS_DSQL_SET_CURSOR(status_vector,
						&statement->rsr_handle,
						reinterpret_cast<const char*>(sqlcur->p_sqlcur_cursor_name.cstr_address),
						sqlcur->p_sqlcur_type);
	THREAD_ENTER;

	return this->send_response(send, 0, 0, status_vector);
}
#endif


void set_server( PORT port, USHORT flags)
{
/**************************************
 *
 *	s e t _ s e r v e r
 *
 **************************************
 *
 * Functional description
 *	Look up the server for this type
 *	of port.  If one doesn't exist,
 *	create it.
 *
 **************************************/
	SRVR server;

	for (server = servers; server; server = server->srvr_next) {
		if (port->port_type == server->srvr_port_type) {
			break;
		}
	}

	if (!server) {
		server = (SRVR) ALLR_alloc((SLONG) sizeof(struct srvr));
		server->srvr_next = servers;
		servers = server;
		server->srvr_port_type = port->port_type;
		server->srvr_parent_port = port;
		server->srvr_flags = flags;
	}

	port->port_server = server;
}


#ifdef BEEN_MOVED
ISC_STATUS Port::start(P_OP operation, P_DATA * data, PACKET* send)
{
/**************************************
 *
 *	s t a r t
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	ISC_STATUS_ARRAY status_vector;
	RRQ request;
	RTR transaction;

	CHECK_HANDLE_MEMBER(transaction,
						RTR,
						type_rtr,
						data->p_data_transaction,
						isc_bad_trans_handle);

	CHECK_HANDLE_MEMBER(request,
						RRQ,
						type_rrq,
						data->p_data_request,
						isc_bad_req_handle);

	request = REMOTE_find_request(request, data->p_data_incarnation);
	REMOTE_reset_request(request, 0);

	THREAD_EXIT;
	isc_start_request(status_vector, &request->rrq_handle,
					  &transaction->rtr_handle, data->p_data_incarnation);
	THREAD_ENTER;

	if (!status_vector[1]) {
		request->rrq_rtr = transaction;
		if (operation == op_start_and_receive)
			return this->receive_after_start(data, send, status_vector);
	}

	return this->send_response(send, 0, 0, status_vector);
}


ISC_STATUS Port::start_and_send(P_OP	operation,
							P_DATA*	data,
							PACKET*	send)
{
/**************************************
 *
 *	s t a r t _ a n d _ s e n d
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	ISC_STATUS_ARRAY status_vector;
	RRQ request;
	FMT format;
	RTR transaction;
	USHORT number;

	CHECK_HANDLE_MEMBER(transaction,
						RTR,
						type_rtr,
						data->p_data_transaction,
						isc_bad_trans_handle);

	CHECK_HANDLE_MEMBER(request,
						RRQ,
						type_rrq,
						data->p_data_request,
						isc_bad_req_handle);

	request = REMOTE_find_request(request, data->p_data_incarnation);
	number = data->p_data_message_number;
	REM_MSG message = request->rrq_rpt[number].rrq_message;
	format = request->rrq_rpt[number].rrq_format;
	REMOTE_reset_request(request, message);

	THREAD_EXIT;
	isc_start_and_send(status_vector, &request->rrq_handle,
					   &transaction->rtr_handle, number,
					   format->fmt_length, message->msg_address,
					   data->p_data_incarnation);
	THREAD_ENTER;

	if (!status_vector[1]) {
		request->rrq_rtr = transaction;
		if (operation == op_start_send_and_receive) {
			return this->receive_after_start(data, send, status_vector);
		}
	}

	return this->send_response(send, 0, 0, status_vector);
}


ISC_STATUS Port::start_transaction(P_OP operation, P_STTR * stuff, PACKET* send)
{
/**************************************
 *
 *	s t a r t _ t r a n s a c t i o n
 *
 **************************************
 *
 * Functional description
 *	Start a transaction.
 *
 **************************************/

	RTR transaction;
	OBJCT object;
	ISC_STATUS_ARRAY status_vector;

	RDB rdb = this->port_context;
	isc_handle handle = NULL_HANDLE;
	THREAD_EXIT;
	
	if (operation == op_reconnect)
		isc_reconnect_transaction(status_vector, &rdb->rdb_handle, &handle,
								  stuff->p_sttr_tpb.cstr_length,
								  reinterpret_cast<const char*>(stuff->p_sttr_tpb.cstr_address));
	else
		isc_start_transaction(status_vector, &handle, (SSHORT) 1, &rdb->rdb_handle,
							  stuff->p_sttr_tpb.cstr_length,
							  stuff->p_sttr_tpb.cstr_address);
	THREAD_ENTER;

	if (status_vector[1])
		object = 0;
	else
		{
		if (transaction = make_transaction(rdb, handle))
			{
			object = transaction->rtr_id;
			if (operation == op_reconnect)
				transaction->rtr_flags |= RTR_limbo;
#ifdef DEBUG_REMOTE_MEMORY
			ib_printf("start_transaction(server) allocate trans   %x\n", transaction);
#endif
			}
		else 
			{
			object = 0;
			THREAD_EXIT;
			if (operation != op_reconnect)
				isc_rollback_transaction(status_vector, &handle);
#ifdef SUPERSERVER_XXX // beat me!   JAS 5/6/04
			/* Note that there is an underlying transaction pool
			   that won't be released until this connection is
			   detached. There is no isc_disconnect_transaction()
			   call that gets rid of a transaction reference --
			   there is only commit and rollback. It's not worth
			   introducing such a call to all subsystems. At least
			   release the y-valve handle. */

			else 
				gds__handle_cleanup(status_vector, &handle);
#endif
			THREAD_ENTER;
			status_vector[0] = isc_arg_gds;
			status_vector[1] = isc_too_many_handles;
			status_vector[2] = isc_arg_end;
		}
	}

	return this->send_response(send, object, 0, status_vector);
}
#endif


static void success( ISC_STATUS * status_vector)
{
/**************************************
 *
 *	s u c c e s s
 *
 **************************************
 *
 * Functional description
 *	Set status vector to indicate success.
 *
 **************************************/

	status_vector[0] = isc_arg_gds;
	status_vector[1] = FB_SUCCESS;
	status_vector[2] = isc_arg_end;
}

#ifdef MULTI_THREAD
static int THREAD_ROUTINE thread(void* flags)
{
/**************************************
 *
 *	t h r e a d
 *
 **************************************
 *
 * Functional description
 *	Execute requests in a happy loop.
 *
 **************************************/
	SERVER_REQ request, next, *req_ptr;
	PORT port, parent_port;
	SCHAR *thread;
	Sync sync (&syncRequestQue, "server thread");
	
#ifdef WIN_NT
	if (!((SLONG) flags & SRVR_non_service))
		thread = reinterpret_cast<SCHAR*>(CNTL_insert_thread());
#endif

	int inactive_count = 0;
	int timedout_count = 0;
	THREAD_ENTER;
	
	for (;;)
		{
		sync.lock (Exclusive);
		if (request = request_que)
			{
			inactive_count = 0;
			timedout_count = 0;
			REMOTE_TRACE(("Dequeue request %p", request_que));
			request_que = request->req_next;
			sync.unlock();
			while (request)
				{
				/* Bind a thread to a port. */

				if (request->req_port->port_server_flags & SRVR_thread_per_port)
					{
					port = request->req_port;
					request->req_next = free_requests;
					free_requests = request;
					THREAD_EXIT;
					SRVR_main(port, port->port_server_flags);
					THREAD_ENTER;
					request = 0;
					continue;
					}
					
				/* Splice request into list of active requests, execute request,
				   and unsplice */

				request->req_next = active_requests;
				active_requests = request;

				/* Validate port.  If it looks ok, process request */

				parent_port = request->req_port->port_server->srvr_parent_port;
				
				if (parent_port == request->req_port)
					process_packet(parent_port, &request->req_send, &request->req_receive, &port);
				else
					for (port = parent_port->port_clients; port; port = port->port_next)
						if (port == request->req_port && port->port_state != state_disconnected) 
							{
							process_packet(port, &request->req_send, &request->req_receive, &port);
							break;
							}

				/* Take request out of list of active requests */

				for (req_ptr = &active_requests; *req_ptr; req_ptr = &(*req_ptr)->req_next)
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
						next->req_next = free_requests;
						free_requests = next;
						}
					if (request->req_send.p_operation == op_void &&
						request->req_receive.p_operation == op_void) 
						{
						//gds__free(request);
						delete request;
						request = 0;
						}
					}
				else 
					{
					port->port_requests_queued--;
#ifdef DEBUG_REMOTE_MEMORY
					ib_printf("thread    ACTIVE     request_queued %d\n",
							  port->port_requests_queued);
					ib_fflush(ib_stdout);
#endif
					}

				/* Pick up any remaining chained request, and free current request */

				if (request) 
					{
					next = request->req_chain;
					request->req_next = free_requests;
					free_requests = request;
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
			   
			if (extra_threads > 1 && (inactive_count > 20 || timedout_count > 2)) 
				{
				extra_threads--;	/* Count me out */
				break;
				}

			threads_waiting++;
			THREAD_EXIT;
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
				
			THREAD_ENTER;
			--threads_waiting;
			}
		}

	THREAD_EXIT;

#ifdef WIN_NT
	if (!((SLONG) flags & SRVR_non_service))
		CNTL_remove_thread(thread);
#endif

	return 0;
}
#endif


#ifdef BEEN_MOVED
ISC_STATUS Port::transact_request(P_TRRQ * trrq, PACKET* send)
{
/**************************************
 *
 *	t r a n s a c t _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	ISC_STATUS_ARRAY status_vector;
	RDB rdb;
	RPR procedure;
	RTR transaction;
	P_DATA *data;
	UCHAR *blr, *in_msg, *out_msg;
	USHORT blr_length, in_msg_length, out_msg_length;

	CHECK_HANDLE_MEMBER(transaction,
						RTR,
						type_rtr,
						trrq->p_trrq_transaction,
						isc_bad_trans_handle);

	rdb = this->port_context;
	blr = trrq->p_trrq_blr.cstr_address;
	blr_length = trrq->p_trrq_blr.cstr_length;
	procedure = this->port_rpr;
	in_msg =
		(procedure->rpr_in_msg) ? procedure->rpr_in_msg->msg_address : NULL;
	in_msg_length =
		(procedure->rpr_in_format) ? procedure->rpr_in_format->fmt_length : 0;
	out_msg =
		(procedure->rpr_out_msg) ? procedure->rpr_out_msg->msg_address : NULL;
	out_msg_length =
		(procedure->rpr_out_format) ? procedure->rpr_out_format->
		fmt_length : 0;

	THREAD_EXIT;
	isc_transact_request(status_vector,
						 &rdb->rdb_handle,
						 &transaction->rtr_handle,
						 blr_length,
						 reinterpret_cast<const char*>(blr),
						 in_msg_length,
						 reinterpret_cast<char*>(in_msg),
						 out_msg_length,
						 reinterpret_cast<char*>(out_msg));
	THREAD_ENTER;

	if (status_vector[1])
		return this->send_response(send, 0, 0, status_vector);

	data = &send->p_data;
	send->p_operation = op_transact_response;
	data->p_data_messages = 1;
	this->sendPacket(send);

	return FB_SUCCESS;
}
#endif



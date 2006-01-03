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
#include "common.h"
#include "../jrd/ib_stdio.h"
#include <string.h>
#include "../jrd/ibase.h"
#include "../jrd/gds_proto.h"
#include "../jrd/gdsassert.h"
#include "../remote/remote.h"
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
#include "TempSpace.h"
#include "gen/iberror.h"

#ifdef SUPERSERVER
//#include "../jrd/os/isc_i_proto.h"
#endif

#include "../remote/proto_proto.h"	// xdr_protocol_overhead()

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
		if (!(blk = (cast) portObjects.getObject(id)))				\
		{															\
			status_vector [0] = isc_arg_gds;						\
			status_vector [1] = err;								\
			status_vector [2] = isc_arg_end;						\
			return send_response(send, 0, 0, status_vector);	\
		}															\
	}


/***
typedef struct server_req_t
{
	server_req_t*	req_next;
	server_req_t*	req_chain;
	Port*			req_port;
	Packet			req_send;
	Packet			req_receive;
} *SERVER_REQ;
***/

/***
typedef struct srvr
{
	struct srvr*	srvr_next;
	Port*			srvr_parent_port;
	enum rem_port_t	srvr_port_type;
	USHORT			srvr_flags;
} *SRVR;
***/

#ifdef MISC_STUFF

static ISC_STATUS	allocate_statement(Port*, P_RLSE*, Packet*);
static SLONG		append_request_chain(SERVER_REQ, SERVER_REQ*);
static SLONG		append_request_next(SERVER_REQ, SERVER_REQ*);
static ISC_STATUS	attach_database(Port*, P_OP, P_ATCH*, Packet*);
static void			aux_connect(Port*, P_REQ*, Packet*);
static void			aux_request(Port*, P_REQ*, Packet*);
static ISC_STATUS	cancel_events(Port*, P_EVENT*, Packet*);

#ifdef CANCEL_OPERATION
static void	cancel_operation(Port*);
#endif

static bool	check_request(RRQ, USHORT, USHORT);
static USHORT	check_statement_type(RSR);

#ifdef SCROLLABLE_CURSORS
static REM_MSG		dump_cache(rrq_repeat*);
#endif

static bool		get_next_msg_no(RRQ, USHORT, USHORT*);


#ifdef SCROLLABLE_CURSORS
static REM_MSG	scroll_cache(rrq_repeat*, USHORT *, ULONG *);
#endif

static void		success(ISC_STATUS *);
static int THREAD_ROUTINE thread(void *);
static void		zap_packet(Packet*, bool);


#endif

static void			server_ast(void*, USHORT, const UCHAR*);
static ISC_STATUS	attach_database(Port*, P_OP, P_ATCH*, Packet*);
static void			success(ISC_STATUS *);
static USHORT		check_statement_type(RSR);
static bool			check_request(RRQ, USHORT, USHORT);


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


Port::Port(int size)
{
	port_buffer = new UCHAR [size];
	port_rpr = NULL;
	port_receive_rmtque = NULL;
	//port_clients = NULL;
	port_next = NULL;
	port_parent = NULL;
	port_async = NULL;
	port_server = NULL;
	port_context = NULL;
	port_statement = NULL;
	port_version = NULL;
	port_host = NULL;
	port_connection = NULL;
	port_user_name = NULL;
	port_passwd = NULL;
	port_server_flags = 0;
	port_status_vector = NULL;
	port_flags = 0;
}

Port::~Port(void)
{
	if (port_async)
		{
		port_async->disconnect();
		port_async = NULL;
		}
		
	delete [] port_buffer;

	if (port_context)
		{
		port_context->clearPort();
		port_context->release();
		}

	if (port_parent)
		port_parent->removeClient(this);
}


ISC_STATUS Port::compile(P_CMPL* compile, Packet* send)
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

	RDB rdb = port_context;
	isc_handle  handle = NULL_HANDLE;
	UCHAR* blr = compile->p_cmpl_blr.cstr_address;
	USHORT blr_length = compile->p_cmpl_blr.cstr_length;

	isc_compile_request(status_vector, &rdb->rdb_handle, &handle, blr_length, (char*) blr);

	if (status_vector[1])
		return send_response(send, 0, 0, status_vector);

/* Parse the request to find the messages */

	REM_MSG message = PARSE_messages(blr, blr_length);
	USHORT max_msg = 0;

	REM_MSG next;
	for (next = message; next; next = next->msg_next)
		max_msg = MAX(max_msg, next->msg_number);

/* Allocate block and merge into data structures */

	//RRQ request = (RRQ) ALLOCV(type_rrq, max_msg + 1);
	RRequest *request = rdb->createRequest (max_msg + 1);
	
	request->rrq_handle = handle;
	request->rrq_rdb = rdb;
	request->rrq_max_msg = max_msg;
	
	if (request->rrq_id = getObjectId(request))
		{
		object = request->rrq_id;
		//request->rrq_next = rdb->rdb_requests;
		//rdb->rdb_requests = request;
		}
	else 
		{
		isc_release_request(status_vector, &request->rrq_handle);
		//ALLR_release(request);
		delete request;
		status_vector[0] = isc_arg_gds;
		status_vector[1] = isc_too_many_handles;
		status_vector[2] = isc_arg_end;
		return send_response(send, 0, 0, status_vector);
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

	return send_response(send, object, 0, status_vector);
}


ISC_STATUS Port::ddl(P_DDL* ddl, Packet* send)
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

	RDB rdb = port_context;
	const UCHAR* blr = ddl->p_ddl_blr.cstr_address;
	blr_length = ddl->p_ddl_blr.cstr_length;

	isc_ddl(status_vector, &rdb->rdb_handle, &transaction->rtr_handle,
			blr_length, reinterpret_cast<const char*>(blr));

	return send_response(send, 0, 0, status_vector);
}


void Port::disconnect(Packet* send, Packet* receive)
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

	if (port_flags & PORT_async)
		return;

	if (port_parent)
		port_parent->removeClient(this);
		
	port_flags |= PORT_disconnect;
	RDB rdb = port_context;

	if (!rdb) 
		{
		REMOTE_free_packet(this, send);
		REMOTE_free_packet(this, receive);
		disconnect();
		return;
		}

	/* For WNET and XNET we should send dummy op_disconnect packet
	   to wakeup async port handling events on client side.
	   For INET it's not necessary because INET client's async port
	   wakes up while server performs shutdown(socket) call on its async port.
	   See interface.cpp - event_thread(). */

	Packet *packet = &rdb->rdb_packet;
	
	if ((port_async) &&
		((port_type == port_xnet) || (port_type == port_pipe)))
		{
		packet->p_operation = op_disconnect;
		port_async->sendPacket(packet);
		}

	if (rdb->rdb_handle)
		if (!(rdb->rdb_flags & RDB_service)) 
			{
#ifdef CANCEL_OPERATION
			/* Prevent a pending or spurious cancel from aborting
			   a good, clean detach from the database. */

			isc_cancel_operation(status_vector, (isc_handle *) &rdb->rdb_handle,
								  CANCEL_disable);
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
				delete transaction;
				//release_transaction(rdb->rdb_transactions);
				}
				
			isc_detach_database(status_vector, &rdb->rdb_handle);
			
			clearStatement();
			rdb->clearObjects();
			}
		else
			{
			isc_service_detach(status_vector, &rdb->rdb_handle);
			}

	REMOTE_free_packet(this, send);
	REMOTE_free_packet(this, receive);

	disconnect();
}


void Port::drop_database(P_RLSE* release, Packet* send)
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

	rdb = port_context;

	isc_drop_database(status_vector, &rdb->rdb_handle);

	if (status_vector[1] && (status_vector[1] != isc_drdb_completed_with_errs)) 
		{
		send_response(send, 0, 0, status_vector);
		return;
		}

	clearStatement();
	rdb->clearObjects();

	send_response(send, 0, 0, status_vector);
}


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


ISC_STATUS Port::end_blob(P_OP operation, P_RLSE * release, Packet* send)
{
/**************************************
 *
 *	e n d _ b l o b
 *
 **************************************
 *
 * Functional description
 *	End a blob.
 *
 **************************************/
	RBL blob;
	ISC_STATUS_ARRAY status_vector;

	CHECK_HANDLE_MEMBER(blob,
						RBL,
						type_rbl,
						release->p_rlse_object,
						isc_bad_segstr_handle);

	if (operation == op_close_blob)
		isc_close_blob(status_vector, &blob->rbl_handle);
	else
		isc_cancel_blob(status_vector, &blob->rbl_handle);

	if (!status_vector[1]) 
		delete blob;

	return send_response(send, 0, 0, status_vector);
}


ISC_STATUS Port::end_database(P_RLSE * release, Packet* send)
{
/**************************************
 *
 *	e n d _ d a t a b a s e
 *
 **************************************
 *
 * Functional description
 *	End a request.
 *
 **************************************/
	RDB rdb;
	ISC_STATUS_ARRAY status_vector;

	rdb = port_context;

	isc_detach_database(status_vector,&rdb->rdb_handle);

	if (status_vector[1])
		return send_response(send, 0, 0, status_vector);

	clearStatement();
	rdb->clearObjects();

	return send_response(send, 0, 0, status_vector);
}


ISC_STATUS Port::end_request(P_RLSE * release, Packet* send)
{
/**************************************
 *
 *	e n d _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	End a request.
 *
 **************************************/
	RRQ request;
	ISC_STATUS_ARRAY status_vector;

	CHECK_HANDLE_MEMBER(request,
						RRQ,
						type_rrq,
						release->p_rlse_object,
						isc_bad_req_handle);

	isc_release_request(status_vector, &request->rrq_handle);

	if (!status_vector[1])
		delete request;

	return send_response(send, 0, 0, status_vector);
}


ISC_STATUS Port::end_statement(P_SQLFREE* free_stmt, Packet* send)
{
/*****************************************
 *
 *	e n d _ s t a t e m e n t
 *
 *****************************************
 *
 * Functional description
 *	Free a statement.
 *
 *****************************************/
	RSR statement;
	USHORT object;
	ISC_STATUS_ARRAY status_vector;

	CHECK_HANDLE_MEMBER(statement,
						RSR,
						type_rsr,
						free_stmt->p_sqlfree_statement,
						isc_bad_req_handle);

	GDS_DSQL_FREE(status_vector,
				  &statement->rsr_handle,
				  free_stmt->p_sqlfree_option);

	if (status_vector[1])
		return send_response(send, 0, 0, status_vector);

	if (!statement->rsr_handle) 
		{
		delete statement;
		statement = NULL;
		}
	else 
		{
		statement->rsr_flags &= ~RSR_fetched;
		statement->rsr_rtr = NULL;
		REMOTE_reset_statement(statement);
		statement->rsr_message = statement->rsr_buffer;
		}

	object = (statement) ? statement->rsr_id : (USHORT) - 1;

	return send_response(send, object, 0, status_vector);
}


ISC_STATUS Port::end_transaction(P_OP operation, P_RLSE * release, Packet* send)
{
/**************************************
 *
 *	e n d _ t r a n s a c t i o n
 *
 **************************************
 *
 * Functional description
 *	End a transaction.
 *
 **************************************/
	RTR transaction;
	ISC_STATUS_ARRAY status_vector;

	CHECK_HANDLE_MEMBER(transaction,
						RTR,
						type_rtr,
						release->p_rlse_object,
						isc_bad_trans_handle);

	switch (operation)
	{
	case op_commit:
		isc_commit_transaction(status_vector, &transaction->rtr_handle);
		break;

	case op_rollback:
		isc_rollback_transaction(status_vector, &transaction->rtr_handle);
		break;

	case op_rollback_retaining:
		isc_rollback_retaining(status_vector, &transaction->rtr_handle);
		break;

	case op_commit_retaining:
		isc_commit_retaining(status_vector, &transaction->rtr_handle);
		break;

	case op_prepare:
		if (!isc_prepare_transaction(status_vector, &transaction->rtr_handle))
				transaction->rtr_flags |= RTR_limbo;
		break;
	}

	if (!status_vector[1])
		if (operation == op_commit || operation == op_rollback) 
			delete transaction;

	return send_response(send, 0, 0, status_vector);
}


ISC_STATUS Port::execute_immediate(P_OP op, P_SQLST * exnow, Packet* send)
{
/*****************************************
 *
 *	e x e c u t e _ i m m e d i a t e
 *
 *****************************************
 *
 * Functional description
 *	process an execute immediate DSQL packet
 *
 *****************************************/
	RDB rdb;
	RTR transaction = NULL;
	USHORT in_blr_length, in_msg_type, in_msg_length, parser_version,
		out_blr_length, out_msg_type, out_msg_length;
	UCHAR *in_blr, *in_msg, *out_blr, *out_msg;
	isc_handle handle;
	ISC_STATUS_ARRAY status_vector;

	rdb = port_context;

	/** Do not call CHECK_HANDLE if this is the start of a transaction **/

	if (exnow->p_sqlst_transaction) 
		{
		CHECK_HANDLE_MEMBER(transaction,
							RTR,
							type_rtr,
							exnow->p_sqlst_transaction,
							isc_bad_trans_handle);
		}

	in_msg_length = out_msg_length = 0;
	in_msg = out_msg = NULL;
	if (op == op_exec_immediate2)
	{
		in_blr_length = exnow->p_sqlst_blr.cstr_length;
		in_blr = exnow->p_sqlst_blr.cstr_address;
		in_msg_type = exnow->p_sqlst_message_number;
		if (port_statement->rsr_bind_format)
		{
			in_msg_length = port_statement->rsr_bind_format->fmt_length;
			in_msg = port_statement->rsr_message->msg_address;
		}
		out_blr_length = exnow->p_sqlst_out_blr.cstr_length;
		out_blr = exnow->p_sqlst_out_blr.cstr_address;
		out_msg_type = exnow->p_sqlst_out_message_number;
		if (port_statement->rsr_select_format)
		{
			out_msg_length =
				port_statement->rsr_select_format->fmt_length;
			if (!port_statement->rsr_message->msg_address)
			{
				/* TMN: Obvious bugfix. Please look at your compilers warnings. */
				/* They are not enemies, they're friends! */
				/* port->port_statement->rsr_message->msg_address = &port->port_statement->rsr_message->msg_buffer; */
				port_statement->rsr_message->msg_address =
					port_statement->rsr_message->msg_buffer;
			}
			out_msg = port_statement->rsr_message->msg_address;
		}
	}
	else
	{
		in_blr_length = out_blr_length = 0;
		in_blr = out_blr = NULL;
		in_msg_type = out_msg_type = 0;
	}

	handle = (transaction) ? transaction->rtr_handle : NULL_HANDLE;

/* Since the API to GDS_DSQL_EXECUTE_IMMED is public and can not be changed, there needs to
 * be a way to send the parser version to DSQL so that the parser can compare the keyword
 * version to the parser version.  To accomplish this, the parser version is combined with
 * the client dialect and sent across that way.  In dsql8_execute_immediate, the parser version
 * and client dialect are separated and passed on to their final desintations.  The information
 * is combined as follows:
 *     Dialect * 10 + parser_version
 *
 * and is extracted in dsql8_execute_immediate as follows:
 *      parser_version = ((dialect *10)+parser_version)%10
 *      client_dialect = ((dialect *10)+parser_version)/10
 *
 * For example, parser_version = 1 and client dialect = 1
 *
 *  combined = (1 * 10) + 1 == 11
 *
 *  parser = (combined) %10 == 1
 *  dialect = (combined) / 19 == 1
 */

	parser_version = (port_protocol < PROTOCOL_VERSION10) ? 1 : 2;

	GDS_DSQL_EXECUTE_IMMED(status_vector,
						   &rdb->rdb_handle,
						   &handle,
						   exnow->p_sqlst_SQL_str.cstr_length,
						   reinterpret_cast<char*>(exnow->p_sqlst_SQL_str.cstr_address),
						   (USHORT) ((exnow->p_sqlst_SQL_dialect * 10) +
									 parser_version),
						   in_blr_length,
						   reinterpret_cast<char*>(in_blr),
						   in_msg_type,
						   in_msg_length,
						   reinterpret_cast<char*>(in_msg),
						   out_blr_length,
						   reinterpret_cast<char*>(out_blr),
						   out_msg_type,
						   out_msg_length,
						   reinterpret_cast<char*>(out_msg));

	if (op == op_exec_immediate2)
	{
		port_statement->rsr_format =
			port_statement->rsr_select_format;

		send->p_operation = op_sql_response;
		send->p_sqldata.p_sqldata_messages = (status_vector[1]
											  || !out_msg) ? 0 : 1;
		sendPartial(send);
	}

	if (!status_vector[1]) 
		{
		if (transaction && !handle) 
			{
			delete transaction;
			transaction = NULL;
			}
		else if (!transaction && handle) 
			{
			transaction = rdb->createTransactionHandle (handle);
			if (!transaction) 
				{
				status_vector[0] = isc_arg_gds;
				status_vector[1] = isc_too_many_handles;
				status_vector[2] = isc_arg_end;
				}
		}
	}

	return send_response(	send,
								(OBJCT) (transaction ? transaction->rtr_id : 0),
								0,
								status_vector);
}


ISC_STATUS Port::execute_statement(P_OP op, P_SQLDATA* sqldata, Packet* send)
{
/*****************************************
 *
 *	e x e c u t e _ s t a t e m e n t
 *
 *****************************************
 *
 * Functional description
 *	Execute a non-SELECT dynamic SQL statement.
 *
 *****************************************/
	RTR transaction = NULL;
	RSR statement;
	USHORT in_msg_length, out_msg_type, out_blr_length, out_msg_length;
	UCHAR *in_msg, *out_blr, *out_msg;
	isc_handle handle;
	ISC_STATUS_ARRAY status_vector;

/** Do not call CHECK_HANDLE if this is the start of a transaction **/
	if (sqldata->p_sqldata_transaction)
	{
		CHECK_HANDLE_MEMBER(transaction,
							RTR,
							type_rtr,
							sqldata->p_sqldata_transaction,
							isc_bad_trans_handle);
	}
	CHECK_HANDLE_MEMBER(statement,
						RSR,
						type_rsr,
						sqldata->p_sqldata_statement,
						isc_bad_req_handle);

	in_msg_length = out_msg_length = 0;
	in_msg = out_msg = NULL;
	if (statement->rsr_format)
	{
		in_msg_length = statement->rsr_format->fmt_length;
		in_msg = statement->rsr_message->msg_address;
	}
	if (op == op_execute2)
	{
		out_blr_length = sqldata->p_sqldata_out_blr.cstr_length;
		out_blr = sqldata->p_sqldata_out_blr.cstr_address;
		out_msg_type = sqldata->p_sqldata_out_message_number;
		if (port_statement->rsr_select_format)
		{
			out_msg_length =
				port_statement->rsr_select_format->fmt_length;
			out_msg = port_statement->rsr_message->msg_buffer;
		}
	}
	else
	{
		out_blr_length = 0;
		out_msg_type = 0;
		out_blr = NULL;
	}
	statement->rsr_flags &= ~RSR_fetched;

	handle = (transaction) ? transaction->rtr_handle : NULL_HANDLE;

	GDS_DSQL_EXECUTE(status_vector,
					 &handle,
					 &statement->rsr_handle,
					 sqldata->p_sqldata_blr.cstr_length,
					 reinterpret_cast<char*>(sqldata->p_sqldata_blr.cstr_address),
					 sqldata->p_sqldata_message_number,
					 in_msg_length,
					 reinterpret_cast<char*>(in_msg),
					 out_blr_length,
					 reinterpret_cast<char*>(out_blr),
					 out_msg_type,
					 out_msg_length,
					 reinterpret_cast<char*>(out_msg));

	if (op == op_execute2)
		{
		port_statement->rsr_format = port_statement->rsr_select_format;
		send->p_operation = op_sql_response;
		send->p_sqldata.p_sqldata_messages = (status_vector[1]
											  || !out_msg) ? 0 : 1;
		sendPartial(send);
		}

	if (!status_vector[1]) 
		{
		if (transaction && !handle) 
			{
			delete transaction;
			transaction = NULL;
			}
		else if (!transaction && handle) 
			{
			if (!(transaction = statement->rsr_rdb->createTransactionHandle(handle)))
				{
				status_vector[0] = isc_arg_gds;
				status_vector[1] = isc_too_many_handles;
				status_vector[2] = isc_arg_end;
				}
		}

		statement->rsr_rtr = transaction;
	}

	return send_response(	send,
								(OBJCT) (transaction ? transaction->rtr_id : 0),
								0,
								status_vector);
}


ISC_STATUS Port::fetch(P_SQLDATA * sqldata, Packet* send)
{
/*****************************************
 *
 *	f e t c h
 *
 *****************************************
 *
 * Functional description
 *	Fetch next record from a dynamic SQL cursor.
 *
 *****************************************/
	RSR statement;
	REM_MSG message, next;
	USHORT msg_length;
	//P_SQLDATA *response;
	//ISC_STATUS s;
	ISC_STATUS_ARRAY status_vector;

	CHECK_HANDLE_MEMBER(statement, RSR, type_rsr, sqldata->p_sqldata_statement, 
					    isc_bad_req_handle);

	if (statement->rsr_flags & RSR_blob) 
		return fetch_blob(sqldata, send);

	if (statement->rsr_format) 
		msg_length = statement->rsr_format->fmt_length;
	else 
		msg_length = 0;
		
	USHORT count = ((port_flags & PORT_rpc) ||
			(statement->rsr_flags & RSR_no_batch)) ? 1 :
			sqldata->p_sqldata_messages;
	USHORT count2 = (statement->rsr_flags & RSR_no_batch) ? 0 : count;

	/* On first fetch, clear the end-of-stream flag & reset the message buffers */

	if (!(statement->rsr_flags & RSR_fetched)) 
		{
		statement->rsr_flags &= ~(RSR_eof | RSR_stream_err);
		memset(statement->rsr_status_vector, 0, sizeof(statement->rsr_status_vector));
		
		if ((message = statement->rsr_message) != NULL) 
			{
			statement->rsr_buffer = message;
			
			while (true) 
				{
				message->msg_address = NULL;
				message = message->msg_next;
				if (message == statement->rsr_message)
					break;
				}
			}
		}

	/* Get ready to ship the data out */

	P_SQLDATA *response = &send->p_sqldata;
	send->p_operation = op_fetch_response;
	response->p_sqldata_statement = sqldata->p_sqldata_statement;
	response->p_sqldata_status = 0;
	response->p_sqldata_messages = 1;
	ISC_STATUS s = 0;
	message = NULL;

	/* Check to see if any messages are already sitting around */

	while (true) 
		{
		/* Have we exhausted the cache & reached cursor EOF? */
		
		if ((statement->rsr_flags & RSR_eof) && !statement->rsr_msgs_waiting) 
			{
			statement->rsr_flags &= ~RSR_eof;
			s = 100;
			count2 = 0;
			break;
			}

		/* Have we exhausted the cache & have a pending error? */
		
		if ((statement->rsr_flags & RSR_stream_err) && !statement->rsr_msgs_waiting) 
			{
			statement->rsr_flags &= ~RSR_stream_err;
			return send_response(send, 0, 0, statement->rsr_status_vector);
			}

		message = statement->rsr_buffer;

		/* Make sure message can be de referenced, if not then return false */
		
		if (message == NULL)
			return FALSE;

		/* If we don't have a message cached, get one from the
		   access method. */

		if (!message->msg_address) 
			{
			fb_assert(statement->rsr_msgs_waiting == 0);
			s = GDS_DSQL_FETCH(status_vector,
							   &statement->rsr_handle,
							   sqldata->p_sqldata_blr.cstr_length,
							   reinterpret_cast<const char*>(sqldata->p_sqldata_blr.cstr_address),
							   sqldata->p_sqldata_message_number,
							   msg_length,
							   reinterpret_cast<char*>(message->msg_buffer));

			statement->rsr_flags |= RSR_fetched;
			
			if (s) 
				{
				if (s == 100 || s == 101) 
					{
					count2 = 0;
					break;
					} 
				else 
					return send_response(send, 0, 0, status_vector);
				}
				
			message->msg_address = message->msg_buffer;
			}
		else 
			{
			/* Take a message from the outqoing queue */
			fb_assert(statement->rsr_msgs_waiting >= 1);
			statement->rsr_msgs_waiting--;
			}

		/* For compatibility with Protocol 7, we must break out of the
		   loop before sending the last record. */

		count--;
		
		if (port_protocol <= PROTOCOL_VERSION7 && count <= 0) 
			break;

		/* There's a buffer waiting -- send it */

		if (!sendPartial(send)) 
			return FALSE;

		message->msg_address = NULL;

		/* If we've sent the requested amount, break out of loop */

		if (count <= 0)
			break;
		}

	response->p_sqldata_status = s;
	response->p_sqldata_messages = 0;
	sendPacket(send);
	
	if (message) 
		message->msg_address = NULL;

	/* Since we have a little time on our hands while this packet is sent
	   and processed, get the next batch of records.  Start by finding the
	   next free buffer. */

	message = statement->rsr_buffer;
	next = NULL;

	while (message->msg_address && message->msg_next != statement->rsr_buffer)
		message = message->msg_next;

	for (; count2; --count2) 
		{
		if (message->msg_address) 
			{
			if (!next)
				for (next = statement->rsr_buffer; next->msg_next != message;
					 next = next->msg_next);
					 
			message = new RMessage (statement->rsr_fmt_length);
			message->msg_number = next->msg_number;
			message->msg_next = next->msg_next;
			next->msg_next = message;
			next = message;
			}
			
		s = GDS_DSQL_FETCH(status_vector,
						   &statement->rsr_handle,
						   sqldata->p_sqldata_blr.cstr_length,
						   reinterpret_cast<char*>(sqldata->p_sqldata_blr.cstr_address),
						   sqldata->p_sqldata_message_number,
						   msg_length,
						   reinterpret_cast<char*>(message->msg_buffer));
		if (s) 
			{
			if (status_vector[1]) 
				{
				/* If already have an error queued, don't overwrite it */
				
				if (!(statement->rsr_flags & RSR_stream_err)) 
					{
					statement->rsr_flags |= RSR_stream_err;
					memcpy(statement->rsr_status_vector, status_vector, sizeof(statement->rsr_status_vector));
					}
				}
				
			if (s == 100)
				statement->rsr_flags |= RSR_eof;
			break;
			}
			
		message->msg_address = message->msg_buffer;
		message = message->msg_next;
		statement->rsr_msgs_waiting++;
		}

	return TRUE;
}


ISC_STATUS Port::fetch_blob(P_SQLDATA * sqldata, Packet* send)
{
/*****************************************
 *
 *	f e t c h _ b l o b
 *
 *****************************************
 *
 * Functional description
 *	Fetch next record from a dynamic SQL cursor.
 *
 *****************************************/
	RSR statement;
	USHORT msg_length;
	REM_MSG message;
	P_SQLDATA *response;
	ISC_STATUS s;
	ISC_STATUS_ARRAY status_vector;

	CHECK_HANDLE_MEMBER(statement,
						RSR,
						type_rsr,
						sqldata->p_sqldata_statement,
						isc_bad_req_handle);

	if (statement->rsr_format)
		msg_length = statement->rsr_format->fmt_length;
	else
		msg_length = 0;

	if ((message = statement->rsr_message) != NULL)
		statement->rsr_buffer = message;

	/* Get ready to ship the data out */

	response = &send->p_sqldata;
	send->p_operation = op_fetch_response;
	response->p_sqldata_statement = sqldata->p_sqldata_statement;
	response->p_sqldata_status = 0;
	response->p_sqldata_messages = 1;
	s = 0;
	message = statement->rsr_buffer;

	s = GDS_DSQL_FETCH(status_vector,
					   &statement->rsr_handle,
					   sqldata->p_sqldata_blr.cstr_length,
					   reinterpret_cast<char*>(sqldata->p_sqldata_blr.cstr_address),
					   sqldata->p_sqldata_message_number,
					   msg_length,
					   reinterpret_cast<char*>(message->msg_buffer));

	if (!status_vector[1] ||
		status_vector[1] != isc_segment || status_vector[1] != isc_segstr_eof) 
		{
		message->msg_address = message->msg_buffer;
		response->p_sqldata_status = s;
		response->p_sqldata_messages = (status_vector[1] == isc_segstr_eof) ? 0 : 1;
		sendPartial(send);
		message->msg_address = NULL;
		}

	return send_response(send, 0, 0, status_vector);
}


OBJCT Port::getObjectId(void* object)
{
/**************************************
 *
 *	g e t _ i d
 *
 **************************************
 *
 * Functional description
 *	Allocate an object slot for an object.
 *	If the object vector cannot be expanded
 *	to accomodate the object slot then
 *	REMOTE_set_object() will return a NULL
 *	object slot.
 *
 **************************************/

	return portObjects.allocateHandle (object);
}


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

	isc_request_info(status_vector, &request->rrq_handle, incarnation,
					 sizeof(request_info), reinterpret_cast<const SCHAR*>(request_info),
					 sizeof(info_buffer), reinterpret_cast<char*>(info_buffer));

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


ISC_STATUS Port::get_segment(P_SGMT* segment, Packet* send)
{
/**************************************
 *
 *	g e t _ s e g m e n t
 *
 **************************************
 *
 * Functional description
 *	Get a single blob segment.
 *
 **************************************/
	RBL blob;
	UCHAR *buffer;
	USHORT length, buffer_length;
	UCHAR *p;
	ISC_STATUS state, status;
	ISC_STATUS_ARRAY status_vector;
	UCHAR temp_buffer[BLOB_LENGTH];

	CHECK_HANDLE_MEMBER(blob,
						RBL,
						type_rbl,
						segment->p_sgmt_blob,
						isc_bad_segstr_handle);

	buffer_length = segment->p_sgmt_length;
	
	if (buffer_length <= sizeof(temp_buffer))
		buffer = temp_buffer;
	else
		{
		if (buffer_length > blob->rbl_buffer_length) 
			blob->allocBuffer (buffer_length);
		buffer = blob->rbl_buffer;
		}
		
	send->p_resp.p_resp_data.cstr_address = buffer;

	/* Be backwards compatible */

	if (port_flags & PORT_rpc)
		{
		length = 0;
		isc_get_segment(status_vector, &blob->rbl_handle, &length,
						segment->p_sgmt_length,
						reinterpret_cast<char*>(buffer));
		status = send_response(send, blob->rbl_id, length, status_vector);
		
		/***
		if (status_vector[1] == isc_segstr_eof)
			if (blob->rbl_buffer != blob->rbl_data) 
				{
				ALLR_free(blob->rbl_buffer);
				blob->rbl_buffer = blob->rbl_data;
				blob->rbl_buffer_length = 1;
				}
		***/
		
		return status;
		}

	/* Gobble up a buffer's worth of segments */

	p = buffer;
	state = 0;

	while (buffer_length > 2) 
		{
		buffer_length -= 2;
		p += 2;
		isc_get_segment(status_vector, &blob->rbl_handle, &length,
						buffer_length, reinterpret_cast<char*>(p));
		if (status_vector[1] == isc_segstr_eof)
			{
			state = 2;
			success(status_vector);
			p -= 2;
			break;
			}
			
		if (status_vector[1] && (status_vector[1] != isc_segment))
			{
			p -= 2;
			break;
			}
			
		p[-2] = (UCHAR) length;
		p[-1] = (UCHAR) (length >> 8);
		p += length;
		buffer_length -= length;
		
		if (status_vector[1] == isc_segment) 
			{
			state = 1;
			success(status_vector);
			break;
			}
		}

	status = send_response(send, (OBJCT)state, (USHORT) (p - buffer), status_vector);

	/***
	if (status_vector[1] == isc_segstr_eof)
		if (blob->rbl_buffer != blob->rbl_data) 
			{
			ALLR_free(blob->rbl_buffer);
			blob->rbl_buffer = blob->rbl_data;
			blob->rbl_buffer_length = 1;
			}
	***/
	
	return status;
}


ISC_STATUS Port::get_slice(P_SLC * stuff, Packet* send)
{
/**************************************
 *
 *	g e t _ s l i c e
 *
 **************************************
 *
 * Functional description
 *	Get an array slice.
 *
 **************************************/
	RDB rdb;
	RTR transaction;
	UCHAR *slice;
	P_SLR *response;
	ISC_STATUS status;
	ISC_STATUS_ARRAY status_vector;
	UCHAR temp_buffer[4096];

	rdb = port_context;
	CHECK_HANDLE_MEMBER(transaction,
						RTR,
						type_rtr,
						stuff->p_slc_transaction,
						isc_bad_trans_handle);

	if (!stuff->p_slc_length)
		slice = 0;
	else 
		{
		if (stuff->p_slc_length <= sizeof(temp_buffer))
			slice = temp_buffer;
		else
			slice = new UCHAR [stuff->p_slc_length]; //ALLR_alloc((SLONG) stuff->p_slc_length);
		}

	if (slice) 
		memset(slice, 0, stuff->p_slc_length);
		
	response = &send->p_slr;

	isc_get_slice(status_vector, 
				  &rdb->rdb_handle, 
				  &transaction->rtr_handle,
				  (ISC_QUAD*) &stuff->p_slc_id, 
				  stuff->p_slc_sdl.cstr_length,
				  reinterpret_cast<const char*>(stuff->p_slc_sdl.cstr_address),
				  stuff->p_slc_parameters.cstr_length,
				  (const ISC_LONG*) stuff->p_slc_parameters.cstr_address,
				  stuff->p_slc_length, 
				  slice,
				  (ISC_LONG*) &response->p_slr_length);

	if (status_vector[1])
		status = send_response(send, 0, 0, status_vector);
	else 
		{
		send->p_operation = op_slice;
		response->p_slr_slice.lstr_address = slice;
		response->p_slr_slice.lstr_length = response->p_slr_length;
		response->p_slr_sdl = stuff->p_slc_sdl.cstr_address;
		response->p_slr_sdl_length = stuff->p_slc_sdl.cstr_length;
		sendPacket(send);
		response->p_slr_sdl = 0;
		status = FB_SUCCESS;
		}

	if (slice) 
		{
		if (slice != temp_buffer)
			delete [] slice; //ALLR_free(slice);
		}

	return status;
}


ISC_STATUS Port::info(P_OP op, P_INFO * stuff, Packet* send)
{
/**************************************
 *
 *	i n f o
 *
 **************************************
 *
 * Functional description
 *	Get info for a blob, database, request, service,
 *	statement, or transaction.
 *
 **************************************/
	RBL blob;
	RTR transaction;
	RRQ request;
	RSR statement;
	UCHAR temp [1024], *temp_buffer;
	TEXT version [256];
	ISC_STATUS status;
	ISC_STATUS_ARRAY status_vector;

	RDB rdb = port_context;

	/* Make sure there is a suitable temporary blob buffer */

	UCHAR *buffer = new UCHAR [stuff->p_info_buffer_length]; //ALLR_alloc((SLONG) stuff->p_info_buffer_length);
	memset(buffer, 0, stuff->p_info_buffer_length);
	
	if (op == op_info_database && stuff->p_info_buffer_length > sizeof(temp)) 
	    temp_buffer = new UCHAR [stuff->p_info_buffer_length]; //ALLR_alloc((SLONG) stuff->p_info_buffer_length);
	else
    	temp_buffer = temp;

	switch (op) 
		{
		case op_info_blob:
			CHECK_HANDLE_MEMBER(blob,
								RBL,
								type_rbl,
								stuff->p_info_object,
								isc_bad_segstr_handle);
			isc_blob_info(status_vector, &blob->rbl_handle,
						stuff->p_info_items.cstr_length,
						reinterpret_cast<char*>(stuff->p_info_items.cstr_address),
						stuff->p_info_buffer_length,
						reinterpret_cast<char*>(buffer));
			break;

		case op_info_database:
			isc_database_info(status_vector, &rdb->rdb_handle,
							stuff->p_info_items.cstr_length,
							reinterpret_cast<const char*>(stuff->p_info_items.cstr_address),
							stuff->p_info_buffer_length /*sizeof (temp)*/,
							reinterpret_cast<char*>(temp_buffer) /*temp*/);
			if (!status_vector[1]) {
				sprintf(version, "%s/%s", GDS_VERSION, (const char*) port_version);
				MERGE_database_info(temp_buffer /*temp*/, buffer, stuff->p_info_buffer_length,
									IMPLEMENTATION, 4, 1,
									reinterpret_cast<UCHAR*>(version),
									reinterpret_cast<const UCHAR*>((const char*) port_host),
									0);
			}
			break;

		case op_info_request:
			CHECK_HANDLE_MEMBER(request,
								RRQ,
								type_rrq,
								stuff->p_info_object,
								isc_bad_req_handle);
			isc_request_info(status_vector, &request->rrq_handle,
							stuff->p_info_incarnation,
							stuff->p_info_items.cstr_length,
							reinterpret_cast<const char*>(stuff->p_info_items.cstr_address),
							stuff->p_info_buffer_length,
							reinterpret_cast<char*>(buffer));
			break;

		case op_info_transaction:
			CHECK_HANDLE_MEMBER(transaction,
								RTR,
								type_rtr,
								stuff->p_info_object,
								isc_bad_trans_handle);
			isc_transaction_info(status_vector, &transaction->rtr_handle,
								stuff->p_info_items.cstr_length,
								reinterpret_cast<const char*>(stuff->p_info_items.cstr_address),
								stuff->p_info_buffer_length,
								reinterpret_cast < char *>(buffer));
			break;

		case op_service_info:
			isc_service_query(status_vector,
							&rdb->rdb_handle,
							NULL,
							stuff->p_info_items.cstr_length,
							reinterpret_cast<
							const char*>(stuff->p_info_items.cstr_address),
							stuff->p_info_recv_items.cstr_length,
							reinterpret_cast<
							const char*>(stuff->p_info_recv_items.cstr_address),
							stuff->p_info_buffer_length,
							reinterpret_cast<char*>(buffer));
			break;

		case op_info_sql:
			CHECK_HANDLE_MEMBER(statement,
								RSR,
								type_rsr,
								stuff->p_info_object,
								isc_bad_req_handle);
			GDS_DSQL_SQL_INFO(status_vector,
							&statement->rsr_handle,
							stuff->p_info_items.cstr_length,
							reinterpret_cast<
							const char*>(stuff->p_info_items.cstr_address),
							stuff->p_info_buffer_length,
							reinterpret_cast < char *>(buffer));
			break;
		}

	if (temp_buffer != temp) 
    	delete [] temp_buffer; //ALLR_free(temp_buffer);

	/* Send a response that includes the segment. */

	send->p_resp.p_resp_data.cstr_address = buffer;

	status = send_response(send, stuff->p_info_object,
						   stuff->p_info_buffer_length, status_vector);

	delete [] buffer;; //ALLR_free(buffer);

	return status;
}


ISC_STATUS Port::insert(P_SQLDATA * sqldata, Packet* send)
{
/*****************************************
 *
 *	i n s e r t
 *
 *****************************************
 *
 * Functional description
 *	Insert next record into a dynamic SQL cursor.
 *
 *****************************************/
	RSR statement;
	USHORT msg_length;
	UCHAR *msg;
	ISC_STATUS_ARRAY status_vector;

	CHECK_HANDLE_MEMBER(statement,
						RSR,
						type_rsr,
						sqldata->p_sqldata_statement,
						isc_bad_req_handle);

	if (statement->rsr_format)
	{
		msg_length = statement->rsr_format->fmt_length;
		msg = statement->rsr_message->msg_address;
	}
	else
	{
		msg_length = 0;
		msg = NULL;
	}

	GDS_DSQL_INSERT(status_vector,
					&statement->rsr_handle,
					sqldata->p_sqldata_blr.cstr_length,
					reinterpret_cast<const char*>(sqldata->p_sqldata_blr.cstr_address),
					sqldata->p_sqldata_message_number, msg_length,
					reinterpret_cast<const char*>(msg));

	return send_response(send, 0, 0, status_vector);
}


ISC_STATUS Port::open_blob(P_OP op, P_BLOB* stuff, Packet* send)
{
/**************************************
 *
 *	o p e n _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Open or create a new blob.
 *
 **************************************/
	RBL blob;
	RTR transaction;
	USHORT object;
	ISC_STATUS_ARRAY status_vector;

	CHECK_HANDLE_MEMBER(transaction,
						RTR,
						type_rtr,
						stuff->p_blob_transaction,
						isc_bad_trans_handle);

	RDB rdb = port_context;
	isc_handle handle = NULL_HANDLE;
	USHORT bpb_length = 0;
	const UCHAR* bpb = NULL;

	if (op == op_open_blob2 || op == op_create_blob2) 
		{
		bpb_length = stuff->p_blob_bpb.cstr_length;
		bpb = stuff->p_blob_bpb.cstr_address;
		}

	if (op == op_open_blob || op == op_open_blob2)
		isc_open_blob2(status_vector, &rdb->rdb_handle, 
					   &transaction->rtr_handle, &handle,
					   (ISC_QUAD*) &stuff->p_blob_id, bpb_length, bpb);
	else
		isc_create_blob2(status_vector, &rdb->rdb_handle, &transaction->rtr_handle,
						 &handle, (ISC_QUAD*) &send->p_resp.p_resp_blob_id,
						 bpb_length, reinterpret_cast<const char*>(bpb));

	if (status_vector[1])
		object = 0;
	else 
		{
		//blob = (RBL) ALLOCV(type_rbl, 1);
		blob = transaction->createBlob(1);
		
		blob->rbl_buffer_length = 1;
		blob->rbl_buffer = blob->rbl_data;
		blob->rbl_handle = handle;
		//blob->rbl_rdb = rdb;
		
		if (blob->rbl_id = getObjectId(blob))
			object = blob->rbl_id;
		else
			{
			object = 0;
			isc_cancel_blob(status_vector, &blob->rbl_handle);
			//ALLR_release(blob);
			delete blob;
			status_vector[0] = isc_arg_gds;
			status_vector[1] = isc_too_many_handles;
			status_vector[2] = isc_arg_end;
			}
		}

	return send_response(send, object, 0, status_vector);
}


ISC_STATUS Port::prepare(P_PREP * stuff, Packet* send)
{
/**************************************
 *
 *	p r e p a r e
 *
 **************************************
 *
 * Functional description
 *	End a transaction.
 *
 **************************************/
	RTR transaction;
	ISC_STATUS_ARRAY status_vector;

	CHECK_HANDLE_MEMBER(transaction,
						RTR,
						type_rtr,
						stuff->p_prep_transaction,
						isc_bad_trans_handle);

	if (!isc_prepare_transaction2(status_vector, &transaction->rtr_handle,
								  stuff->p_prep_data.cstr_length,
								  stuff->p_prep_data.cstr_address))
		transaction->rtr_flags |= RTR_limbo;

	return send_response(send, 0, 0, status_vector);
}


ISC_STATUS Port::prepare_statement(P_SQLST * prepare, Packet* send)
{
/*****************************************
 *
 *	p r e p a r e _ s t a t m e n t
 *
 *****************************************
 *
 * Functional description
 *	Prepare a dynamic SQL statement for execution.
 *
 *****************************************/
	RTR transaction = NULL;
	RSR statement;
	UCHAR *buffer, local_buffer[1024];
	ISC_STATUS status;
	ISC_STATUS_ARRAY status_vector;
	USHORT state, parser_version;
	isc_handle handle;

/** Do not call CHECK_HANDLE if this is the start of a transaction **/
	if (prepare->p_sqlst_transaction)
		{
		CHECK_HANDLE_MEMBER(transaction,
							RTR,
							type_rtr,
							prepare->p_sqlst_transaction,
							isc_bad_trans_handle);
		}
		
	CHECK_HANDLE_MEMBER(statement,
						RSR,
						type_rsr,
						prepare->p_sqlst_statement,
						isc_bad_req_handle);

	if (prepare->p_sqlst_buffer_length > sizeof(local_buffer))
		buffer = new UCHAR [prepare->p_sqlst_buffer_length]; //ALLR_alloc((SLONG) prepare->p_sqlst_buffer_length);
	else
		buffer = local_buffer;

	handle = (transaction) ? transaction->rtr_handle : NULL_HANDLE;


/* Since the API to GDS_DSQL_PREPARE is public and can not be changed, there needs to
 * be a way to send the parser version to DSQL so that the parser can compare the keyword
 * version to the parser version.  To accomplish this, the parser version is combined with
 * the client dialect and sent across that way.  In dsql8_prepare_statement, the parser version
 * and client dialect are separated and passed on to their final desintations.  The information
 * is combined as follows:
 *     Dialect * 10 + parser_version
 *
 * and is extracted in dsql8_prepare_statement as follows:
 *      parser_version = ((dialect *10)+parser_version)%10
 *      client_dialect = ((dialect *10)+parser_version)/10
 *
 * For example, parser_version = 1 and client dialect = 1
 *
 *  combined = (1 * 10) + 1 == 11
 *
 *  parser = (combined) %10 == 1
 *  dialect = (combined) / 19 == 1
 */
	parser_version = (port_protocol < PROTOCOL_VERSION10) ? 1 : 2;

	GDS_DSQL_PREPARE(status_vector,
					 &handle,
					 &statement->rsr_handle,
					 prepare->p_sqlst_SQL_str.cstr_length,
					 reinterpret_cast<const char*>(prepare->p_sqlst_SQL_str.cstr_address),
					 (prepare->p_sqlst_SQL_dialect * 10) + parser_version,
					 prepare->p_sqlst_items.cstr_length,
					 reinterpret_cast<const char*>(prepare->p_sqlst_items.cstr_address),
					 prepare->p_sqlst_buffer_length,
					 reinterpret_cast<char*>(buffer));

	if (status_vector[1]) 
		{
		if (buffer != local_buffer)
			delete [] buffer;
		return send_response(send, 0, 0, status_vector);
		}

	REMOTE_reset_statement(statement);

	statement->rsr_flags &= ~(RSR_blob | RSR_no_batch);
	state = check_statement_type(statement);
	
	if (state == STMT_BLOB)
		statement->rsr_flags |= RSR_blob;
	else if (state == STMT_NO_BATCH)
		statement->rsr_flags |= RSR_no_batch;
		
	state = (state == STMT_BLOB) ? 1 : 0;

	/* Send a response that includes the info requested. */

	send->p_resp.p_resp_data.cstr_address = buffer;

	status = send_response(send, state, prepare->p_sqlst_buffer_length, status_vector);

	if (buffer != local_buffer) 
		delete [] buffer;

	return status;
}



ISC_STATUS Port::put_segment(P_OP op, P_SGMT * segment, Packet* send)
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
		isc_put_segment(status_vector, &blob->rbl_handle, length,
						reinterpret_cast<const char*>(p));
		return send_response(send, 0, 0, status_vector);
	}

/* We've got a batch of segments.  This is only slightly more complicated */

	const UCHAR* const end = p + length;

	while (p < end) {
		length = *p++;
		length += *p++ << 8;
		isc_put_segment(status_vector, &blob->rbl_handle, length,
						reinterpret_cast<const char*>(p));
		if (status_vector[1])
			return send_response(send, 0, 0, status_vector);
		p += length;
	}

	return send_response(send, 0, 0, status_vector);
}


ISC_STATUS Port::put_slice(P_SLC * stuff, Packet* send)
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

	rdb = port_context;

	send->p_resp.p_resp_blob_id = stuff->p_slc_id;
	isc_put_slice(status_vector, &rdb->rdb_handle, &transaction->rtr_handle,
				  (ISC_QUAD*) &send->p_resp.p_resp_blob_id,
				  stuff->p_slc_sdl.cstr_length,
				  reinterpret_cast<char*>(stuff->p_slc_sdl.cstr_address),
				  stuff->p_slc_parameters.cstr_length,
				  (ISC_LONG *) stuff->p_slc_parameters.cstr_address,
				  stuff->p_slc_slice.lstr_length,
				  stuff->p_slc_slice.lstr_address);

	return send_response(send, 0, 0, status_vector);
}


ISC_STATUS Port::que_events(P_EVENT * stuff, Packet* send)
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

	rdb = port_context;

	/* Find unused event block or, if necessary, a new one */

	for (event = rdb->rdb_events; event; event = event->rvnt_next) 
		if (!event->rvnt_id) 
			break;

	if (!event)
		event = rdb->createEvent();

	event->rvnt_ast = stuff->p_event_ast;
	// CVC: Going from SLONG to void*, problems when sizeof(void*) > 4
	event->rvnt_arg = (void *) stuff->p_event_arg;
	event->rvnt_rid = stuff->p_event_rid;
	event->rvnt_rdb = rdb;

	isc_que_events(status_vector, &rdb->rdb_handle, &event->rvnt_id,
				   stuff->p_event_items.cstr_length,
				   reinterpret_cast<const char*>(stuff->p_event_items.cstr_address),
				   server_ast,
				   event);

	id = event->rvnt_id;
	
	if (status_vector[1]) 
		event->rvnt_id = 0;

	return send_response(send, (OBJCT) id, 0, status_vector);
}


ISC_STATUS Port::receive_after_start(	P_DATA*	data,
									Packet*	send,
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
		return send_response(send, 0, 0, status_vector);
	}

	send->p_operation = op_response_piggyback;
	response = &send->p_resp;
	response->p_resp_object = msg_number;
	response->p_resp_status_vector = status_vector;
	response->p_resp_data.cstr_length = 0;

	sendPartial(send);

/* Fill in packet to fool receive into thinking that it has been
   called directly by the client. */

	tail = request->rrq_rpt + msg_number;
	format = tail->rrq_format;

	data->p_data_message_number = msg_number;
	if (port_flags & PORT_rpc)
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

	return receive_msg(data, send);
}


ISC_STATUS Port::receive_msg(P_DATA * data, Packet* send)
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
		(port_flags & PORT_rpc) ? 1 : data->p_data_messages;
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

	if (port_protocol < PROTOCOL_SCROLLABLE_CURSORS)
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
					send_response(send, 0, 0,
								  request->rrq_status_vector);
				memset(request->rrq_status_vector, 0,
					   sizeof(request->rrq_status_vector));
				return res;
			}

#ifdef SCROLLABLE_CURSORS
			isc_receive2(status_vector, &request->rrq_handle, msg_number, 
						 format->fmt_length, message->msg_buffer, level, 
						 direction, offset);
#else
			isc_receive(status_vector, &request->rrq_handle, msg_number,
						format->fmt_length, message->msg_buffer, level);
#endif

			if (status_vector[1])
				return send_response(send, 0, 0, status_vector);

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

		if (!sendPartial(send))
			return FALSE;
		message->msg_address = NULL;
	}

	send->p_data.p_data_messages = 0;
	sendPacket(send);
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

			message = new RMessage (format);
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

		isc_receive(status_vector, &request->rrq_handle, msg_number,
					format->fmt_length,
					message->msg_buffer, data->p_data_incarnation);

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


ISC_STATUS Port::seek_blob(P_SEEK * seek, Packet* send)
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
	ISC_LONG offset, result;
	ISC_STATUS_ARRAY status_vector;

	CHECK_HANDLE_MEMBER(blob,
						RBL,
						type_rbl,
						seek->p_seek_blob,
						isc_bad_segstr_handle);

	mode = seek->p_seek_mode;
	offset = seek->p_seek_offset;
	isc_seek_blob(status_vector, &blob->rbl_handle, mode, offset, &result);
	send->p_resp.p_resp_blob_id.bid_number = result;

	return send_response(send, 0, 0, status_vector);
}


ISC_STATUS Port::send_msg(P_DATA * data, Packet* send)
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

	isc_send(status_vector, &request->rrq_handle, number, format->fmt_length,
			 message->msg_address, data->p_data_incarnation);

	message->msg_address = NULL;

	return send_response(send, 0, 0, status_vector);
}


ISC_STATUS Port::send_response(	Packet*	send,
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
					port_protocol < PROTOCOL_VERSION10)
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

				if (port_protocol < PROTOCOL_VERSION10)
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

	sendPacket(send);

	return exit_code;
}


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
	REvent *event = reinterpret_cast<RVNT>(event_void);
	event->rvnt_id = 0;
	RDatabase *rdb = event->rvnt_rdb;
	Port *port = rdb->rdb_port->port_async;

	if (!port) 
		return;

	Packet packet;
	packet.p_operation = op_event;
	P_EVENT *p_event = &packet.p_event;
	p_event->p_event_database = rdb->rdb_id;
	p_event->p_event_items.cstr_length = length;
	// Probalby should define this item with CSTRING_CONST instead.
	p_event->p_event_items.cstr_address = const_cast<UCHAR*>(items);
	//p_event->p_event_ast = event->rvnt_ast;
	//p_event->p_event_arg = (SLONG) event->rvnt_arg;
	p_event->p_event_rid = event->rvnt_rid;

	port->sendPacket(&packet);
	//delete event;
	event->rvnt_id = 0;
}


ISC_STATUS Port::service_attach(P_ATCH* attach, Packet* send)
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
	UCHAR *service_name, *spb, new_spb_buffer[512], *p, *end;
	isc_handle handle;
	ISC_STATUS_ARRAY status_vector;
	RDB rdb;
	//STR string;

	send->p_operation = op_accept;
	handle = NULL_HANDLE;
	service_name = attach->p_atch_file.cstr_address;
	service_length = attach->p_atch_file.cstr_length;
	spb = attach->p_atch_dpb.cstr_address;
	spb_length = attach->p_atch_dpb.cstr_length;

	/* If we have user identification, append it to database parameter block */

	UCHAR *new_spb = new_spb_buffer;
	
	if (!port_user_name.IsEmpty())
		{
		const char *string = port_user_name;
		int length = port_user_name.length();
		
		if ((spb_length + 3 + length) > sizeof(new_spb_buffer))
			new_spb = new UCHAR [spb_length + 3 + length];
			
		p = new_spb;
		
		if (spb_length)
			for (end = spb + spb_length; spb < end;)
				*p++ = *spb++;
		else
			*p++ = isc_spb_current_version;

		*p++ = isc_spb_sys_user_name;
		*p++ = (UCHAR) length;
		memcpy (p, string, length);
		p += length;
		spb = new_spb;
		spb_length = p - new_spb;
		}

	/* See if user has specified parameters relevent to the connection,
	   they will be stuffed in the SPB if so. */
	   
	REMOTE_get_timeout_params(this, spb, spb_length);

	isc_service_attach(status_vector,
					   service_length,
					   reinterpret_cast<char*>(service_name),
					   &handle,
					   spb_length,
					   reinterpret_cast<char*>(spb));

	if (new_spb != new_spb_buffer)
		delete [] new_spb;

	if (!status_vector[1]) 
		{
		port_context = rdb = new RDatabase (this);
		rdb->rdb_handle = handle;
		rdb->rdb_flags |= RDB_service;
		}

	return send_response(send, 0, 0, status_vector);
}


ISC_STATUS Port::service_end(P_RLSE * release, Packet* send)
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
	RDB rdb = port_context;
	isc_service_detach(status_vector, &rdb->rdb_handle);

	return send_response(send, 0, 0, status_vector);
}


ISC_STATUS Port::service_start(P_INFO * stuff, Packet* send)
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
	isc_resv_handle *reserved = 0;		/* reserved for future use */

	rdb = port_context;

	isc_service_start(status_vector,
					  &rdb->rdb_handle,
					  reserved,
					  stuff->p_info_items.cstr_length,
					  reinterpret_cast<char*>(stuff->p_info_items.cstr_address));

	return send_response(send, 0, 0, status_vector);
}


ISC_STATUS Port::set_cursor(P_SQLCUR * sqlcur, Packet* send)
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

	GDS_DSQL_SET_CURSOR(status_vector,
						&statement->rsr_handle,
						reinterpret_cast<const char*>(sqlcur->p_sqlcur_cursor_name.cstr_address),
						sqlcur->p_sqlcur_type);

	return send_response(send, 0, 0, status_vector);
}



ISC_STATUS Port::start(P_OP operation, P_DATA * data, Packet* send)
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

	isc_start_request(status_vector, &request->rrq_handle,
					  &transaction->rtr_handle, data->p_data_incarnation);

	if (!status_vector[1]) {
		request->rrq_rtr = transaction;
		if (operation == op_start_and_receive)
			return receive_after_start(data, send, status_vector);
	}

	return send_response(send, 0, 0, status_vector);
}


ISC_STATUS Port::start_and_send(P_OP	operation,
							P_DATA*	data,
							Packet*	send)
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

	isc_start_and_send(status_vector, &request->rrq_handle,
					   &transaction->rtr_handle, number,
					   format->fmt_length, message->msg_address,
					   data->p_data_incarnation);

	if (!status_vector[1]) {
		request->rrq_rtr = transaction;
		if (operation == op_start_send_and_receive) {
			return receive_after_start(data, send, status_vector);
		}
	}

	return send_response(send, 0, 0, status_vector);
}


ISC_STATUS Port::start_transaction(P_OP operation, P_STTR * stuff, Packet* send)
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

	RDB rdb = port_context;
	isc_handle handle = NULL_HANDLE;
	
	if (operation == op_reconnect)
		isc_reconnect_transaction(status_vector, &rdb->rdb_handle, &handle,
								  stuff->p_sttr_tpb.cstr_length,
								  reinterpret_cast<const char*>(stuff->p_sttr_tpb.cstr_address));
	else
		isc_start_transaction(status_vector, &handle, (SSHORT) 1, &rdb->rdb_handle,
							  stuff->p_sttr_tpb.cstr_length,
							  stuff->p_sttr_tpb.cstr_address);

	if (status_vector[1])
		object = 0;
	else
		{
		if (transaction = rdb->createTransactionHandle (handle))
			{
			object = transaction->rtr_id;
			if (operation == op_reconnect)
				transaction->rtr_flags |= RTR_limbo;
			}
		else 
			{
			object = 0;
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
			status_vector[0] = isc_arg_gds;
			status_vector[1] = isc_too_many_handles;
			status_vector[2] = isc_arg_end;
		}
	}

	return send_response(send, object, 0, status_vector);
}


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


ISC_STATUS Port::transact_request(P_TRRQ * trrq, Packet* send)
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

	rdb = port_context;
	blr = trrq->p_trrq_blr.cstr_address;
	blr_length = trrq->p_trrq_blr.cstr_length;
	procedure = port_rpr;
	in_msg =
		(procedure->rpr_in_msg) ? procedure->rpr_in_msg->msg_address : NULL;
	in_msg_length =
		(procedure->rpr_in_format) ? procedure->rpr_in_format->fmt_length : 0;
	out_msg =
		(procedure->rpr_out_msg) ? procedure->rpr_out_msg->msg_address : NULL;
	out_msg_length =
		(procedure->rpr_out_format) ? procedure->rpr_out_format->
		fmt_length : 0;

	isc_transact_request(status_vector,
						 &rdb->rdb_handle,
						 &transaction->rtr_handle,
						 blr_length,
						 reinterpret_cast<const char*>(blr),
						 in_msg_length,
						 reinterpret_cast<char*>(in_msg),
						 out_msg_length,
						 reinterpret_cast<char*>(out_msg));

	if (status_vector[1])
		return send_response(send, 0, 0, status_vector);

	data = &send->p_data;
	send->p_operation = op_transact_response;
	data->p_data_messages = 1;
	sendPacket(send);

	return FB_SUCCESS;
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

	return ret;
}

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


void* Port::getObject(int objectId)
{
	return portObjects.getObject (objectId);
}

/***
int Port::accept(p_cnct* cnct)
{
	//return (*port_accept)(this, cnct);
}

void Port::disconnect()
{
	//(*port_disconnect)(this);
}

Port* Port::receive(Packet* pckt)
{
	//return (*port_receive_packet)(this, pckt);
}

XDR_INT Port::sendPacket(Packet* pckt)
{
	//return (*port_send_packet)(this, pckt);
}

XDR_INT Port::sendPartial(Packet* pckt)
{
	//return (*port_send_partial)(this, pckt);
}

Port* Port::connect(Packet* pckt, void(*ast)())
{
	//return (*port_connect)(this, pckt, ast);
}

Port* Port::request(Packet* pckt)
{
	//return (*port_request)(this, pckt);
}
***/

void Port::setObject(void* object, int objectId)
{
	portObjects.setObject (object, objectId);
}

void Port::releaseObjectId(int objectId)
{
	portObjects.releaseHandle (objectId);
}

void Port::clearStatement(void)
{
	if (port_statement)
		{
		delete port_statement;
		port_statement = NULL;
		}
}

RStatement* Port::getStatement(void)
{
	if (!port_statement)
		port_statement = port_context->createStatement();
		
	return port_statement;
	
}

void Port::clearContext(void)
{
	port_context = NULL;
}

void Port::addClient(Port* port)
{
	Sync sync(&syncObject, "Port::addClient");
	sync.lock(Exclusive);
	port->port_parent = this;
	port->port_next = port_next; //port_clients;
	port_next = port;
	port->addRef();
}

void Port::removeClient(Port* port)
{
	Sync sync(&syncObject, "Port::removeClient");
	sync.lock(Exclusive);
	
	for (Port** ptr = &port_next; *ptr; ptr = &(*ptr)->port_next) 
		if (*ptr == port) 
			{
			*ptr = port->port_next;
			port->release();
			break;
			}
	
	port->port_parent = NULL;
}

ISC_STATUS Port::updateAccountInfo(p_update_account* data, Packet* send)
{
	ISC_STATUS_ARRAY status_vector;
	RDatabase *rdb = port_context;

	fb_update_account_info(status_vector, 
						   &rdb->rdb_handle,
 						   data->p_account_apb.cstr_length,
 						   data->p_account_apb.cstr_address);
 						   
	return send_response(send, 0, 0, status_vector);
}

ISC_STATUS Port::authenticateUser(p_authenticate* data, Packet* send)
{
	ISC_STATUS_ARRAY status_vector;
	RDatabase *rdb = port_context;
	UCHAR temp[1024];
	TempSpace space(sizeof(temp), temp);
	int bufferLength = data->p_auth_buffer_length;
	UCHAR *buffer = space.resize(bufferLength);
	memset(buffer, 0, bufferLength);

	fb_authenticate_user(status_vector, 
						   &rdb->rdb_handle,
 						   data->p_auth_dpb.cstr_length,
 						   data->p_auth_dpb.cstr_address,
 						   data->p_auth_items.cstr_length,
 						   data->p_auth_items.cstr_address,
 						   bufferLength,
 						   buffer);
 						   
	send->p_resp.p_resp_data.cstr_address = buffer;

	return send_response(send, 0, bufferLength, status_vector);
}

caddr_t Port::inlinePointer(XDR* xdrs, u_int bytecount)
{
	if (bytecount > (u_int) xdrs->x_handy)
		return FALSE;

	return xdrs->x_base + bytecount;
}

XDR_INT Port::destroy(XDR* xdrs)
{
	return (XDR_INT)0;
}

bool_t Port::getLong(XDR* xdrs, SLONG* lp)
{
	SLONG l;

	if (!(*xdrs->x_ops->x_getbytes) (xdrs, reinterpret_cast<char*>(&l), 4))
		return FALSE;

	*lp = ntohl(l);

	return TRUE;
}

u_int Port::getPosition(XDR* xdrs)
{
	return (u_int) (xdrs->x_private - xdrs->x_base);
}

bool_t Port::putLong(XDR* xdrs, SLONG* lp)
{
	const SLONG l = htonl(*lp);
	
	return (*xdrs->x_ops->x_putbytes) (xdrs,
									   reinterpret_cast<const char*>(AOF32L(l)),
									   4);
}

bool_t Port::setPosition(XDR* xdrs, u_int bytecount)
{
	if (bytecount > (u_int) xdrs->x_handy)
		return FALSE;

	xdrs->x_private = xdrs->x_base + bytecount;

	return TRUE;
}

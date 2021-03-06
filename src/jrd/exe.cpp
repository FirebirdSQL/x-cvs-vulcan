/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		exe.cpp
 *	DESCRIPTION:	Statement execution
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
 * 2001.6.21 Claudio Valderrama: Allow inserting strings into blob fields.
 * 2001.6.28 Claudio Valderrama: Move code to cleanup_rpb() as directed
 * by Ann Harrison and cleanup of new record in store() routine.
 * 2001.10.11 Claudio Valderrama: Fix SF Bug #436462: From now, we only
 * count real store, modify and delete operations either in an external
 * file or in a table. Counting on a view caused up to three operations
 * being reported instead of one.
 * 2001.12.03 Claudio Valderrama: new visit to the same issue: views need
 * to count virtual operations, not real I/O on the underlying tables.
 * 2002.09.28 Dmitry Yemanov: Reworked internal_info stuff, enhanced
 *                            exception handling in SPs/triggers,
 *                            implemented ROWS_AFFECTED system variable
 *
 * 2002.10.21 Nickolay Samofatov: Added support for explicit pessimistic locks 
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "MPEXL" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "DecOSF" port
 * 2002.10.29 Nickolay Samofatov: Added support for savepoints
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 * 2003.10.05 Dmitry Yemanov: Added support for explicit cursors in PSQL
 */

#include <string.h>
#include <memory.h>
#include "fbdev.h"

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include "../jrd/common.h"
#include "../jrd/ibsetjmp.h"
#include "../jrd/jrd.h"
#include "../jrd/Relation.h"
#include "../jrd/Field.h"
#include "../jrd/tra.h"
#include "../jrd/req.h"
#include "../jrd/val.h"
#include "../jrd/exe.h"
#include "gen/iberror.h"
#include "../jrd/ods.h"
#include "../jrd/btr.h"
#include "../jrd/rse.h"
//#include "../jrd/lck.h"
#include "../jrd/intl.h"
#include "../jrd/rng.h"
#include "../jrd/sbm.h"
#include "../jrd/blb.h"
#include "../jrd/blr.h"
#include "../jrd/all_proto.h"
#include "../jrd/blb_proto.h"
#include "../jrd/btr_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dfw_proto.h"
#include "../jrd/dpm_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/ext_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/idx_proto.h"
#include "../jrd/jrd_proto.h"

#include "../jrd/lck_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/opt_proto.h"
#include "../jrd/par_proto.h"
#include "../jrd/rlck_proto.h"
#include "../jrd/rng_proto.h"
#include "../jrd/thd_proto.h"
#include "../jrd/tra_proto.h"
#include "../jrd/vio_proto.h"
#include "../jrd/isc_s_proto.h"
#include "../jrd/OSRIException.h"
#include "../jrd/Procedure.h"
#include "../jrd/Triggers.h"
#include "Format.h"
#include "Attachment.h"

#include "../jrd/ExecStatement.h"
#include "../jrd/rpb_chain.h"

// StatusXcp class implementation

StatusXcp::StatusXcp()
{
	clear();
}

void StatusXcp::clear()
{
	status[0] = isc_arg_gds;
	status[1] = FB_SUCCESS;
	status[2] = isc_arg_end;
}

void StatusXcp::init(const ISC_STATUS* vector)
{
	memcpy(status, vector, sizeof(ISC_STATUS_ARRAY));
}

void StatusXcp::copyTo(ISC_STATUS* vector) const
{
	memcpy(vector, status, sizeof(ISC_STATUS_ARRAY));
}

bool StatusXcp::success() const
{
	return (status[1] == FB_SUCCESS);
}

SLONG StatusXcp::as_gdscode() const
{
	return status[1];
}

SLONG StatusXcp::as_sqlcode() const
{
	return gds__sqlcode(status);
}

static void assign_xcp_message(thread_db*, STR*, const TEXT*);
static void cleanup_rpb(thread_db*, record_param*);
static JRD_NOD erase(thread_db*, JRD_NOD, SSHORT);
static void execute_looper(thread_db*, Request*, Transaction*, enum req_s);
//static void exec_sql(thread_db*, Request*, DSC *);
static void execute_procedure(thread_db*, JRD_NOD);
static Request *execute_triggers(thread_db*, Relation* relation, Triggers**, Record*, Record*, enum req_ta);
static JRD_NOD looper(thread_db*, Request*, JRD_NOD);
static JRD_NOD modify(thread_db*, JRD_NOD, SSHORT);
static JRD_NOD receive_msg(thread_db*, JRD_NOD);
static void release_blobs(thread_db*, Request*);
static void release_proc_save_points(Request*);
#ifdef SCROLLABLE_CURSORS
static JRD_NOD seek_rse(thread_db*, Request*, JRD_NOD);
static void seek_rsb(thread_db*, Request*, RecordSource*, USHORT, SLONG);
#endif
static JRD_NOD selct(thread_db*, JRD_NOD);
static JRD_NOD send_msg(thread_db*, JRD_NOD);
static void set_error(thread_db*, const xcp_repeat*, JRD_NOD);
static JRD_NOD stall(thread_db*, JRD_NOD);
static JRD_NOD store(thread_db*, JRD_NOD, SSHORT);
static bool test_and_fixup_error(thread_db*, const PsqlException*, Request*);
static void trigger_failure(thread_db*, Request*);
static void validate(thread_db*, JRD_NOD);
inline void PreModifyEraseTriggers(thread_db*, Relation*, Triggers**, SSHORT, record_param*, Record*, req_ta);
static void stuff_stack_trace(Request* request, OSRIException& exception);

#ifdef PC_ENGINE
static JRD_NOD find(thread_db*, JRD_NOD);
static JRD_NOD find_dbkey(thread_db*, JRD_NOD);
static Lock* implicit_record_lock(thread_db* tdbb, Transaction*, record_param*);
static JRD_NOD release_bookmark(thread_db*, JRD_NOD);
static JRD_NOD set_bookmark(thread_db*, JRD_NOD);
static JRD_NOD set_index(thread_db*, JRD_NOD);
static JRD_NOD stream(thread_db*, JRD_NOD);
#endif

#if defined(DEBUG_GDS_ALLOC) && defined(PROD_BUILD)
static SLONG memory_debug = 1;
static SLONG memory_count = 0;
#endif /* DEBUG_GDS_ALLOC */

/* macro definitions */

#define NULL_STRING	"*** null ***"

#if (defined SUPERSERVER) && (defined WIN_NT || defined SOLARIS_MT)
#define MAX_CLONES	750
#endif

#if defined (HP10) && defined (SUPERSERVER)
#define MAX_CLONES	110
#endif

#ifndef MAX_CLONES
#define MAX_CLONES	1000
#endif

#define ALL_TRIGS	0
#define PRE_TRIG	1
#define POST_TRIG	2

const size_t MAX_STACK_TRACE = 2048;

/* this constant defines how many records are locked
   before we check whether record locking has been
   turned off for a given relation; if we set the 
   constant to a low number, we will do too much 
   locking in the case where record locking is always
   turned on; too high and we will do too much record
   locking in the case where someone is only occasionally
   locking a record */

#define RECORD_LOCK_CHECK_INTERVAL	10



void EXE_assignment(thread_db* tdbb, JRD_NOD node)
{
/**************************************
 *
 *	E X E _ a s s i g n m e n t
 *
 **************************************
 *
 * Functional description
 *	Perform an assignment
 *
 **************************************/

	DSC temp;
	//DEV_BLKCHK(node, type_nod);
	//SET_TDBB(tdbb);
	Request *request = tdbb->tdbb_request;
	BLKCHK(node, type_nod);

	/* Get descriptors of receiving and sending fields/parameters, variables, etc. */

	const dsc* missing = NULL;
	
	if (node->nod_arg[e_asgn_missing]) 
		missing = EVL_expr(tdbb, node->nod_arg[e_asgn_missing]);

	JRD_NOD to = node->nod_arg[e_asgn_to];
	DSC* to_desc = EVL_assign_to(tdbb, to);
	request->req_flags &= ~req_null;
	dsc* from_desc = EVL_expr(tdbb, node->nod_arg[e_asgn_from]);
	SSHORT null = (request->req_flags & req_null) ? -1 : 0;

	if (!null && missing && MOV_compare(tdbb, missing, from_desc) == 0) 
		null = -1;

	/* If the value is non-missing, move/convert it.  Otherwise fill the
	   field with appropriate nulls. */

	if (!null)
		{
		/* if necessary and appropriate, use the indicator variable */

		if (to->nod_type == nod_argument && to->nod_arg[e_arg_indicator])
			{
			DSC* indicator    = EVL_assign_to(tdbb, to->nod_arg[e_arg_indicator]);
			temp.dsc_dtype    = dtype_short;
			temp.dsc_length   = sizeof(SSHORT);
			temp.dsc_scale    = 0;
			temp.dsc_sub_type = 0;
			SSHORT len;

			if ((from_desc->dsc_dtype <= dtype_varying) &&
				(to_desc->dsc_dtype <= dtype_varying) &&
				(TEXT_LEN(from_desc) > TEXT_LEN(to_desc)))
				len = TEXT_LEN(from_desc);
			else 
				len = 0;

			temp.dsc_address = (UCHAR *) &len;
			MOV_move(&temp, indicator);

			if (len) 
				{
				temp = *from_desc;
				temp.dsc_length = TEXT_LEN(to_desc);
				if (temp.dsc_dtype == dtype_cstring) 
					temp.dsc_length += 1;
				else if (temp.dsc_dtype == dtype_varying) 
					temp.dsc_length += 2;
				from_desc = &temp;
				}
			}

		if (DTYPE_IS_BLOB(to_desc->dsc_dtype))
			{
			/* CVC: This is a case that has hurt me for years and I'm going to solve it.
					It should be possible to copy a string to a blob, even if the charset is
					lost as a result of this experimental implementation. */
			if (from_desc->dsc_dtype <= dtype_varying)
				BLB_move_from_string(tdbb, from_desc, to_desc, to);
			else 
				BLB_move(tdbb, from_desc, to_desc, to);
			}

		else if (!DSC_EQUIV(from_desc, to_desc))
			MOV_move(from_desc, to_desc);

		else if (from_desc->dsc_dtype == dtype_short)
			*((SSHORT *) to_desc->dsc_address) =
				*((SSHORT *) from_desc->dsc_address);

		else if (from_desc->dsc_dtype == dtype_long)
			*((SLONG *) to_desc->dsc_address) =
				*((SLONG *) from_desc->dsc_address);

		else if (from_desc->dsc_dtype == dtype_int64)
			*((SINT64 *) to_desc->dsc_address) =
				*((SINT64 *) from_desc->dsc_address);

		else if (((U_IPTR) from_desc->dsc_address & (ALIGNMENT - 1)) ||
				 ((U_IPTR) to_desc->dsc_address & (ALIGNMENT - 1)))
			MOVE_FAST(from_desc->dsc_address, to_desc->dsc_address,
					  from_desc->dsc_length);

		else
			MOVE_FASTER(from_desc->dsc_address, to_desc->dsc_address,
						from_desc->dsc_length);
		to_desc->dsc_flags &= ~DSC_null;
		}
	else if (node->nod_arg[e_asgn_missing2] &&
			 (missing = EVL_expr(tdbb, node->nod_arg[e_asgn_missing2])))
		{
		MOV_move(missing, to_desc);
		to_desc->dsc_flags |= DSC_null;
		}
	else
		{
		USHORT l = to_desc->dsc_length;
		UCHAR* p = to_desc->dsc_address;
		switch (to_desc->dsc_dtype) 
			{
			case dtype_text:
				/* YYY - not necessarily the right thing to do */
				/* for text formats that don't have trailing spaces */
				if (l) 
					{
					do {
						*p++ = ' ';
					} while (--l);
					}
				break;

			case dtype_cstring:
				*p = 0;
				break;

			case dtype_varying:
				*(SSHORT *) p = 0;
				break;

			default:
				do
					*p++ = 0;
				while (--l);
				break;
			}
		to_desc->dsc_flags |= DSC_null;
		}

	/* Handle the null flag as appropriate for fields and message arguments. */

	if (to->nod_type == nod_field)
		{
		const SSHORT id = (USHORT)(long) to->nod_arg[e_fld_id];
		Record* record = request->req_rpb[(int) (IPTR) to->nod_arg[e_fld_stream]].rpb_record;
		if (null) 
			{
			SET_NULL(record, id);
			} 
		else 
			{
			CLEAR_NULL(record, id);
			}
		}
	else if (to->nod_type == nod_argument && to->nod_arg[e_arg_flag])
		{
		to_desc = EVL_assign_to(tdbb, to->nod_arg[e_arg_flag]);

		/* If the null flag is a string with an effective length of one,
		   then -1 will not fit.  Therefore, store 1 instead. */

		if (null &&
			to_desc->dsc_dtype <= dtype_varying &&
			to_desc->dsc_length <=
			((to_desc->dsc_dtype == dtype_text) ? 1 :
			((to_desc->dsc_dtype == dtype_cstring) ? 2 : 3)))
			null = 1;

		temp.dsc_dtype = dtype_short;
		temp.dsc_length = sizeof(SSHORT);
		temp.dsc_scale = 0;
		temp.dsc_sub_type = 0;
		temp.dsc_address = (UCHAR *) & null;
		MOV_move(&temp, to_desc);
		
		if (null && to->nod_arg[e_arg_indicator]) 
			{
			to_desc = EVL_assign_to(tdbb, to->nod_arg[e_arg_indicator]);
			MOV_move(&temp, to_desc);
			}
		}

	request->req_operation = req_return;
}




Request *EXE_find_request(thread_db* tdbb, Request *request, bool validate)
{
/**************************************
 *
 *	E X E _ f i n d _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Find an inactive incarnation of a trigger request.  If necessary,
 *	clone it.
 *
 **************************************/

	if (!request)
		BUGCHECK /* REQUEST */ (167);	/* msg 167 invalid SEND request */

	DBB dbb = tdbb->tdbb_database;
#ifdef SHARED_CACHE
	Sync sync(&dbb->syncClone, "EXE_find_request");
	sync.lock(Exclusive);
#endif
	//THD_MUTEX_LOCK(dbb->dbb_mutexes + DBB_MUTX_clone);
	Request *clone = NULL;
	USHORT count = 0;
	
	if (!(request->req_flags & req_in_use))
		clone = request;
	else 
		{
		if (request->req_attachment == tdbb->tdbb_attachment)
			count++;

		/* Request exists and is in use.  Search clones for one in use by
		   this attachment. If not found, return first inactive request. */

		VEC vector = request->req_sub_requests;
		USHORT clones = (vector) ? vector->count() - 1 : 0;
		USHORT n;

		for (n = 1; n <= clones; n++) 
			{
			Request *next = CMP_clone_request(tdbb, request, n, validate);
			
			if (next->req_attachment == tdbb->tdbb_attachment)
				{
				if (next->req_flags & req_in_use) 
					count++;
				else
					{
					clone = next;
					break;
					}
				}
			else if (!(next->req_flags & req_in_use) && !clone)
				clone = next;
			}

		if (count > MAX_CLONES) 
			{
			//THD_MUTEX_UNLOCK(dbb->dbb_mutexes + DBB_MUTX_clone);
			ERR_post(isc_req_max_clones_exceeded, 0);
			}
			
		if (!clone)
			clone = CMP_clone_request(tdbb, request, n, validate);
		}
	
	clone->req_attachment = tdbb->tdbb_attachment;
	clone->req_flags |= req_in_use;
	//THD_MUTEX_UNLOCK(dbb->dbb_mutexes + DBB_MUTX_clone);
	
	return clone;
}



void EXE_receive(thread_db*	tdbb,
				 Request*	request,
				 USHORT		msg,
				 USHORT		length,
				 UCHAR*		buffer)
{
/**************************************
 *
 *	E X E _ r e c e i v e
 *
 **************************************
 *
 * Functional description
 *	Move a message from JRD to the host program.  This corresponds to
 *	a JRD BLR/JRD_NOD send.
 *
 **************************************/
	Savepoint* save_sav_point;
	Transaction *transaction = request->req_transaction;
	request->setThread(tdbb);

	if (!(request->req_flags & req_active)) 
		ERR_post(isc_req_sync, 0);

	if (request->req_flags & req_proc_fetch)
		{
		/* request->req_proc_sav_point stores all the request savepoints.
		   When going to continue execution put request save point list
		   into transaction->tra_save_point so that it is used in looper.
		   When we come back to EXE_receive() restore
		   transaction->tra_save_point and merge all work done under
		   stored procedure savepoints into the current transaction
		   savepoint, which is the savepoint for fetch. */

		save_sav_point = transaction->tra_save_point;
		transaction->tra_save_point = request->req_proc_sav_point;
		request->req_proc_sav_point = save_sav_point;

		if (!transaction->tra_save_point)
			VIO_start_save_point(tdbb, transaction);
		}
	
	try
		{
		if (request->req_message->nod_type == nod_stall
#ifdef SCROLLABLE_CURSORS
			|| request->req_flags & req_fetch_required
#endif
			)
			execute_looper(tdbb, request, transaction, req_sync);

		if (!(request->req_flags & req_active) || request->req_operation != req_send)
			ERR_post(isc_req_sync, 0);

		JRD_NOD message = request->req_message;
		Format* format = (Format*) message->nod_arg[e_msg_format];

		if (msg != (USHORT)(long) message->nod_arg[e_msg_number])
			ERR_post(isc_req_sync, 0);

		if (length != format->fmt_length)
			ERR_post(isc_port_len,
					isc_arg_number, length,
					isc_arg_number, format->fmt_length, 0);

		if ((U_IPTR) buffer & (ALIGNMENT - 1))
			MOVE_FAST (IMPURE (request, message->nod_impure), buffer, length);
		else
			MOVE_FASTER (IMPURE (request, message->nod_impure), buffer, length);

		execute_looper(tdbb, request, transaction, req_proceed);
		}
	catch (OSRIException& exception)
		{
		exception;
		
		if (request->req_flags & req_proc_fetch)
			{
			save_sav_point = transaction->tra_save_point;
			transaction->tra_save_point = request->req_proc_sav_point;
			request->req_proc_sav_point = save_sav_point;
			release_proc_save_points(request);
			}
		throw;
		}
	catch (const std::exception&)
		{
		if (request->req_flags & req_proc_fetch)
			{
			save_sav_point = transaction->tra_save_point;
			transaction->tra_save_point = request->req_proc_sav_point;
			request->req_proc_sav_point = save_sav_point;
			release_proc_save_points(request);
			firebird::status_exception::raise(-1);
			}
		throw;
		}

	if (request->req_flags & req_proc_fetch) 
		{
		save_sav_point = transaction->tra_save_point;
		transaction->tra_save_point = request->req_proc_sav_point;
		request->req_proc_sav_point = save_sav_point;
		VIO_merge_proc_sav_points(tdbb, transaction, &request->req_proc_sav_point);
		}

}


#ifdef SCROLLABLE_CURSORS
void EXE_seek(thread_db* tdbb, Request *request, USHORT direction, ULONG offset)
{
/**************************************
 *
 *      E X E _ s e e k
 *
 **************************************
 *
 * Functional description
 *	Seek a given request in a particular direction 
 *	for offset records. 
 *
 **************************************/
	VEC vector;
	RecordSource* rsb;
	SLONG i;

	SET_TDBB(tdbb);
	DEV_BLKCHK(request, type_req);
	request->setThread(tdbb);

/* loop through all RSEs in the request, 
   and describe the rsb tree for that rsb;
   go backwards because items were popped
   off the stack backwards */

	vector = request->req_fors;
	if (!vector)
		return;

/* find the top-level rsb in the request and seek it */

	for (i = vector->vec_count - 1; i >= 0; i--)
		if (rsb = (RecordSource*) vector->vec_object[i]) {
			seek_rsb(tdbb, request, rsb, direction, offset);
			break;
		}
}
#endif


void EXE_send(thread_db*	tdbb,
			  Request*		request,
			  USHORT	msg,
			  USHORT	length,
			  const UCHAR*	buffer)
{
/**************************************
 *
 *	E X E _ s e n d
 *
 **************************************
 *
 * Functional description
 *	Send a message from the host program to the engine.  
 *	This corresponds to a blr_receive or blr_select statement.
 *
 **************************************/
	JRD_NOD node, message, *ptr, *end;
	request->setThread(tdbb);
	
#ifdef SCROLLABLE_CURSORS
	USHORT save_operation;
	JRD_NOD save_next = NULL, save_message;
#endif

	if (!(request->req_flags & req_active))
		ERR_post(isc_req_sync, 0);

#ifdef SCROLLABLE_CURSORS
	/* look for an asynchronous send message--if such 
	   a message was defined, we allow the user to send 
	   us a message at any time during request execution */

	if ((message = request->req_async_message) &&
		 (node = message->nod_arg[e_send_message]) &&
		 (msg == (USHORT)(ULONG) node->nod_arg[e_msg_number])) 
		{
		/* save the current state of the request so we can go 
		   back to what was interrupted */

		save_operation = request->req_operation;
		save_message = request->req_message;
		save_next = request->req_next;

		request->req_operation = req_receive;
		request->req_message = node;
		request->req_next = message->nod_arg[e_send_statement];

		/* indicate that we are processing an asynchronous message */

		request->req_flags |= req_async_processing;
		}
	else 
		{
#endif
		if (request->req_operation != req_receive)
			ERR_post(isc_req_sync, 0);
			
		node = request->req_message;
#ifdef SCROLLABLE_CURSORS
		}
#endif

	Transaction *transaction = request->req_transaction;

	if (node->nod_type == nod_message)
		message = node;
	else if (node->nod_type == nod_select)
		for (ptr = node->nod_arg, end = ptr + node->nod_count; ptr < end; ptr++) 
			{
			message = (*ptr)->nod_arg[e_send_message];
			if ((USHORT)(long) message->nod_arg[e_msg_number] == msg) 
				{
				request->req_next = *ptr;
				break;
				}
			}
	else
		BUGCHECK(167);			/* msg 167 invalid SEND request */

	Format *format = (Format*) message->nod_arg[e_msg_format];

	if (msg != (USHORT)(long) message->nod_arg[e_msg_number])
		ERR_post(isc_req_sync, 0);

	if (length != format->fmt_length)
		ERR_post(isc_port_len,
				 isc_arg_number, (SLONG) length,
				 isc_arg_number, (SLONG) format->fmt_length, 0);

	if ((U_IPTR) buffer & (ALIGNMENT - 1))
		MOVE_FAST(buffer, IMPURE (request, message->nod_impure), length);
	else
		MOVE_FASTER(buffer, IMPURE (request, message->nod_impure), length);

	execute_looper(tdbb, request, transaction, req_proceed);

#ifdef SCROLLABLE_CURSORS
	if (save_next) {
		/* if the message was sent asynchronously, restore all the 
		   previous values so that whatever we were trying to do when 
		   the message came in is what we do next */

		request->req_operation = save_operation;
		request->req_message = save_message;
		request->req_next = save_next;
	}
#endif
}


void EXE_start(thread_db* tdbb, Request *request, Transaction* transaction)
{
/**************************************
 *
 *	E X E _ s t a r t
 *
 **************************************
 *
 * Functional description
 *	Start an execution running.
 *
 **************************************/

	DBB dbb = tdbb->tdbb_database;

	if (request->req_flags & req_active)
		ERR_post(isc_req_sync, isc_arg_gds, isc_reqinuse, 0);

	if (transaction->tra_flags & TRA_prepared)
		ERR_post(isc_req_no_trans, 0);

	/* Post resources to transaction block.  In particular, the interest locks
	   on relations/indices are copied to the transaction, which is very
	   important for (short-lived) dynamically compiled requests.  This will
	   provide transaction stability by preventing a relation from being
	   dropped after it has been referenced from an active transaction. */

	request->setThread(tdbb);
	TRA_post_resources(tdbb, transaction, request->req_resources);

#ifdef SHARED_CACHE
	Sync sync(&dbb->syncCmpClone, "EXE_start");
	sync.lock(Exclusive);
#endif

	request->req_transaction = transaction;
	request->req_flags &= REQ_FLAGS_INIT_MASK;
	request->req_flags |= req_active;
	request->req_flags &= ~req_reserved;
	request->req_operation = req_evaluate;

	/* set up to count records affected by request */

	request->req_flags |= req_count_records;
	request->req_records_selected = 0;
	request->req_records_updated = 0;
	request->req_records_inserted = 0;
	request->req_records_deleted = 0;

	/* CVC: set up to count virtual operations on SQL views. */

	request->req_view_flags = 0;
	request->req_top_view_store = NULL;
	request->req_top_view_modify = NULL;
	request->req_top_view_erase = NULL;

#ifdef SHARED_CACHE
	sync.unlock();
#endif

	// Store request start time for timestamp work
	
	if (!request->req_timestamp) 
		request->req_timestamp = time(NULL);

	// Set all invariants to not computed.
	
	jrd_nod **ptr, **end;
	
	for (ptr = request->req_invariants.begin(),end = request->req_invariants.end(); ptr < end; ++ptr)
		{
		impure_value* impure = (impure_value*) IMPURE (request, (*ptr)->nod_impure);
		impure->vlu_flags = 0;
		}

	// Start a save point if not in middle of one
	
	if (transaction && (transaction != dbb->dbb_sys_trans)) 
		VIO_start_save_point(tdbb, transaction);

#ifdef WIN_NT
	//START_CHECK_FOR_EXCEPTIONS(NULL);
#endif

	// TODO:
	// 1. Try to fix the problem with MSVC C++ runtime library, making
	// even C++ exceptions that are implemented in terms of Win32 SEH
	// getting catched by the SEH handler below.
	// 2. Check if it really is correct that only Win32 catches CPU
	// exceptions (such as SEH) here. Shouldn't any platform capable
	// of handling signals use this stuff?
	// (see jrd/ibsetjmp.h for implementation of these macros)

	looper(tdbb, request, request->req_top_node);

#ifdef WIN_NT
	//END_CHECK_FOR_EXCEPTIONS(NULL);
#endif

	// If any requested modify/delete/insert ops have completed, forget them

	if (transaction &&
	     (transaction != dbb->dbb_sys_trans) &&
	     transaction->tra_save_point &&
	     !(transaction->tra_save_point->sav_flags & SAV_user) &&
	     !transaction->tra_save_point->sav_verb_count)
		// Forget about any undo for this verb
		VIO_verb_cleanup(tdbb, transaction);
}

#ifdef OBSOLETE
void EXE_unwind(Request *request)
{
/**************************************
 *
 *	E X E _ u n w i n d
 *
 **************************************
 *
 * Functional description
 *	Unwind a request, maybe active, maybe not.  This is particlarly
 *	simple since nothing really needs to be done.
 *
 **************************************/
	//DBB dbb = tdbb->tdbb_database;
	//request->req_tdbb = tdbb;
	DBB dbb = request->req_attachment->att_database;
	thread_db *tdbb = request->req_tdbb;
	
	if (request->req_flags & req_active) 
		{
		if (request->req_fors.getCount())
			{
			JrdMemoryPool *old_pool = tdbb->tdbb_default;
			tdbb->tdbb_default = request->req_pool;
			Request *old_request = tdbb->tdbb_request;
			tdbb->tdbb_request = request;
			Transaction* old_transaction = tdbb->tdbb_transaction;
			tdbb->tdbb_transaction = request->req_transaction;
			RecordSource** ptr = request->req_fors.begin();
			
			for (const RecordSource* const* const end = request->req_fors.end(); ptr < end; ptr++)
				if (*ptr)
					//RSE_close(tdbb, *ptr);
					(*ptr)->close(request);

			tdbb->tdbb_default = old_pool;
			tdbb->tdbb_request = old_request;
			tdbb->tdbb_transaction = old_transaction;
			}
			
		release_blobs(tdbb, request);
		}

	if (request->req_proc_sav_point && (request->req_flags & req_proc_fetch))
		release_proc_save_points(request);

#ifdef SHARED_CACHE
	Sync sync(&dbb->syncClone, "EXE_unwind");
	sync.lock(Exclusive);
#endif

	request->req_flags &= ~(req_active | req_proc_fetch | req_reserved);
	request->req_flags |= req_abort | req_stall;
	request->req_timestamp = 0;
	request->req_tdbb = NULL;
}
#endif

void assign_xcp_message(thread_db* tdbb, STR* xcp_msg, const TEXT* msg)
{
/**************************************
 *
 *	a s s i g n _ x c p _ m e s s a g e
 *
 **************************************
 *
 * Functional description
 *	Copy an exception message into XCP structure.
 *
 **************************************/
	SET_TDBB(tdbb);

	if (msg)
	{
		USHORT len = strlen(msg);
		*xcp_msg = FB_NEW_RPT(*tdbb->tdbb_default, len + 1) str();
		(*xcp_msg)->str_length = len;
		memcpy((*xcp_msg)->str_data, msg, len + 1);
	}
}


/* CVC: Moved to its own routine, originally in store(). */
static void cleanup_rpb(thread_db* tdbb, record_param *rpb)
{
/**************************************
 *
 *	c l e a n u p _ r p b
 *
 **************************************
 *
 * Functional description
 *	Perform cleaning of rpb, zeroing unassigned fields and
 * the impure tail of varying fields that we don't want to carry
 * when the RLE algorithm is applied.
 *
 **************************************/
	DSC *desc = 0;
	SSHORT n;
	USHORT length;
	Record* record = rpb->rpb_record;
	Format* format = record->rec_format;
	UCHAR *p;

	SET_TDBB(tdbb); /* Is it necessary? */

/*
    Starting from the format, walk through its
    array of descriptors.  If the descriptor has
    no address, its a computed field and we shouldn't
    try to fix it.  Get a pointer to the actual data
    and see if that field is null by indexing into
    the null flags between the record header and the
    record data.
*/

	for (n = 0; n < format->fmt_count; n++)
	{
		desc = &format->fmt_desc[n];
		if (!desc->dsc_address)
			continue;
		p = record->rec_data + (long) desc->dsc_address;
		if (TEST_NULL(record, n))
		{
			if (length = desc->dsc_length)
				do *p++ = 0; while (--length);
		}
		else if (desc->dsc_dtype == dtype_varying)
		{
			VARY *vary;
			
			vary = reinterpret_cast<VARY*>(p);
			if ((length = desc->dsc_length - sizeof(USHORT)) > vary->vary_length)
			{
				p = reinterpret_cast<UCHAR*>(vary->vary_string + vary->vary_length);
				length -= vary->vary_length;
				do *p++ = 0; while (--length);
			}
		}
	}
}

inline void PreModifyEraseTriggers(thread_db* tdbb, 
								   Relation* relation,
								   Triggers** trigs, 
								   SSHORT which_trig, 
								   record_param *rpb, 
								   Record* rec,
								   req_ta op)
{
/******************************************************
 *
 *	P r e M o d i f y E r a s e T r i g g e r s
 *
 ******************************************************
 *
 * Functional description
 *	Perform operation's pre-triggers, 
 *  storing active rpb in chain.
 *
 ******************************************************/
 
	if ((*trigs) && (which_trig != POST_TRIG)) 
		{
		if (!tdbb->tdbb_transaction->tra_rpblist) 
			tdbb->tdbb_transaction->tra_rpblist =  FB_NEW(*tdbb->tdbb_transaction->tra_pool) traRpbList(tdbb->tdbb_transaction->tra_pool);

		int rpblevel = tdbb->tdbb_transaction-> tra_rpblist->PushRpb(rpb);
		Request *trigger = execute_triggers(tdbb, relation, trigs, rpb->rpb_record, rec, op);
		tdbb->tdbb_transaction->tra_rpblist->PopRpb(rpb, rpblevel);
		
		if (trigger) 
			trigger_failure(tdbb, trigger);
		}
}

static JRD_NOD erase(thread_db* tdbb, JRD_NOD node, SSHORT which_trig)
{
/**************************************
 *
 *	e r a s e
 *
 **************************************
 *
 * Functional description
 *	Perform erase operation.
 *
 **************************************/
	//SET_TDBB(tdbb);
	//BLKCHK(node, type_nod);

	Database* dbb = tdbb->tdbb_database;
	Request* request = tdbb->tdbb_request;
	Transaction* transaction = request->req_transaction;
	record_param* rpb = &request->req_rpb[(int) (IPTR) node->nod_arg[e_erase_stream]];
	Relation* relation = rpb->rpb_relation;

	switch (request->req_operation) 
		{
		case req_evaluate:
			{
			if (!node->nod_arg[e_erase_statement])
				break;
			Format* format = rpb->rpb_relation->getCurrentFormat(tdbb);
			Record* record = VIO_record(tdbb, rpb, format, tdbb->tdbb_default);
			rpb->rpb_address = record->rec_data;
			rpb->rpb_length = format->fmt_length;
			rpb->rpb_format_number = format->fmt_version;
			return node->nod_arg[e_erase_statement];
			}

		case req_return:
			break;

		default:
			return node->nod_parent;
		}


	request->req_operation = req_return;
	RLCK_reserve_relation(tdbb, transaction, relation, TRUE, TRUE);

	/* If the stream was sorted, the various fields in the rpb are
	   probably junk.  Just to make sure that everything is cool,
	   refetch and release the record. */

	if (rpb->rpb_stream_flags & RPB_s_refetch) 
		{
		SLONG tid_fetch = rpb->rpb_transaction;
		
		if ((!DPM_get(tdbb, rpb, LCK_read)) ||
			(!VIO_chase_record_version(tdbb,
									   rpb,
									   transaction, tdbb->tdbb_default, FALSE)))
				ERR_post(isc_deadlock, isc_arg_gds, isc_update_conflict, 0);
				
		VIO_data(tdbb, rpb, tdbb->tdbb_request->req_pool);

		/* If record is present, and the transaction is read committed,
		 * make sure the record has not been updated.  Also, punt after
		 * VIO_data () call which will release the page.
		 */

		if ((transaction->tra_flags & TRA_read_committed) &&
			(tid_fetch != rpb->rpb_transaction))
				ERR_post(isc_deadlock, isc_arg_gds, isc_update_conflict, 0);

		rpb->rpb_stream_flags &= ~RPB_s_refetch;
		}

	if (transaction != dbb->dbb_sys_trans)
		++transaction->tra_save_point->sav_verb_count;

	/* Handle pre-operation trigger */
	
	PreModifyEraseTriggers(tdbb, relation, &relation->rel_pre_erase,
							which_trig, rpb, 0, req_trigger_delete);

	if (relation->rel_file)
		EXT_erase(tdbb, rpb, reinterpret_cast < int *>(transaction));
	else if (!relation->rel_view_rse)
		VIO_erase(tdbb, rpb, transaction);
		
	/* Handle post operation trigger */

	Request* trigger;
	
	if (relation->rel_post_erase &&which_trig != PRE_TRIG &&
		(trigger = execute_triggers(tdbb, relation, &relation->rel_post_erase,
									rpb->rpb_record, 0,
									req_trigger_delete)))
		{
		VIO_bump_count(tdbb, DBB_delete_count, relation, true);
		trigger_failure(tdbb, trigger);
		}

	/* call IDX_erase (which checks constraints) after all post erase triggers 
	   have fired. This is required for cascading referential integrity, which 
	   can be implemented as post_erase triggers */

	if (!relation->rel_file & !relation->rel_view_rse)
		{
		Relation* bad_relation;
		USHORT bad_index;

		IDX_E error_code =
			IDX_erase(tdbb, rpb, transaction, &bad_relation, &bad_index);

		if (error_code) 
			{
			VIO_bump_count(tdbb, DBB_delete_count, relation, true);
			ERR_duplicate_error(error_code, bad_relation, bad_index);
			}
		}

	/* CVC: Increment the counter only if we called VIO/EXT_erase() and
			we were successful. */
			
	if (!(request->req_view_flags & req_first_erase_return))
		{
		request->req_view_flags |= req_first_erase_return;
		
		if (relation->rel_view_rse) 
			request->req_top_view_erase = relation;
		}
		
	if (relation == request->req_top_view_erase) 
		{
		if (which_trig == ALL_TRIGS || which_trig == POST_TRIG) 
			{
			request->req_records_deleted++;
			request->req_records_affected++;
			}
		}
	else if (relation->rel_file || !relation->rel_view_rse) 
		{
		request->req_records_deleted++;
		request->req_records_affected++;
		}

	if (transaction != dbb->dbb_sys_trans) 
		--transaction->tra_save_point->sav_verb_count;


	return node->nod_parent;
}


static void execute_looper(
						   thread_db* tdbb,
						   Request *request,
						   Transaction* transaction, enum req_s next_state)
{
/**************************************
 *
 *	e x e c u t e _ l o o p e r
 *
 **************************************
 *
 * Functional description
 *	Wrapper around looper. This will execute
 *	looper with the save point mechanism.
 *
 **************************************/

	//DEV_BLKCHK(request, type_req);
	//SET_TDBB(tdbb);
	DBB dbb = tdbb->tdbb_database;

	/* Start a save point */

	if (!(request->req_flags & req_proc_fetch) && request->req_transaction)
		if (transaction && (transaction != dbb->dbb_sys_trans))
			VIO_start_save_point(tdbb, transaction);

	request->req_flags &= ~req_stall;
	request->req_operation = next_state;

	looper(tdbb, request, request->req_next);

	/* If any requested modify/delete/insert ops have completed, forget them */

	if (!(request->req_flags & req_proc_fetch) && request->req_transaction)
		if (transaction && (transaction != dbb->dbb_sys_trans) &&
			 transaction->tra_save_point &&
			 !transaction->tra_save_point->sav_verb_count) 
			VIO_verb_cleanup(tdbb, transaction); //Forget about any undo for this verb 
}

#ifdef TOTALLY_BROKEN	/* this needs to be rewritten with the DSQL */
static void exec_sql(thread_db* tdbb, Request *request, DSC* dsc)
{
/**************************************
 *
 *	e x e c _ s q l
 *
 **************************************
 *
 * Functional description
 *	Execute a string as SQL operator.
 *
 **************************************/

	UCHAR *p;
	vary *v = reinterpret_cast <vary*> (
		FB_NEW(*tdbb->tdbb_transaction->tra_pool) char[BUFFER_LARGE + sizeof(vary)]);
	v->vary_length = BUFFER_LARGE;
	SSHORT l;
	ISC_STATUS *status;
	ISC_STATUS_ARRAY local;

	memset(local, 0, sizeof(local));
	status = local;

	SET_TDBB(tdbb);
	p = 0;
	l = (dsc && !(request->req_flags & req_null)) ?
		MOV_get_string(dsc, &p, v, BUFFER_LARGE) : 0; // !!! How call Msgs ?

	if (p) 
		{
		if (tdbb->tdbb_transaction->tra_callback_count >= MAX_CALLBACKS) 
			ERR_post (isc_exec_sql_max_call_exceeded, isc_arg_end)
		else 
			{
			tdbb->tdbb_transaction->tra_callback_count++;
			callback_execute_immediate(status,
									   tdbb->tdbb_attachment,
									   tdbb->tdbb_transaction,
									   reinterpret_cast<TEXT *>(p), l);
			tdbb->tdbb_transaction->tra_callback_count--;
			}
		}
	else 
		ERR_post (isc_exec_sql_invalid_arg, isc_arg_end);

	delete v;
}
#endif // TOTALLY_BROKEN	this needs to be rewritten with the DSQL


static void execute_procedure(thread_db* tdbb, JRD_NOD node)
{
/**************************************
 *
 *	e x e c u t e _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *	Execute a stored procedure.  Begin by
 *	assigning the input parameters.  End
 *	by assigning the output parameters.
 *
 **************************************/
	JRD_NOD in_message, out_message, temp;
	Format* format;
	USHORT in_msg_length, out_msg_length;
	UCHAR *in_msg, *out_msg;
	STR temp_buffer = NULL;
	Request *request = tdbb->tdbb_request;

	if ( (temp = node->nod_arg[e_esp_inputs]) ) 
		for (JRD_NOD *ptr = temp->nod_arg, *end = ptr + temp->nod_count; ptr < end; ptr++)
			EXE_assignment(tdbb, *ptr);

	if (in_message = node->nod_arg[e_esp_in_msg]) 
		{
		format = (Format*) in_message->nod_arg[e_msg_format];
		in_msg_length = format->fmt_length;
		in_msg = IMPURE (request, in_message->nod_impure);
		}
		
	if (out_message = node->nod_arg[e_esp_out_msg]) 
		{
		format = (Format*) out_message->nod_arg[e_msg_format];
		out_msg_length = format->fmt_length;
		out_msg = IMPURE (request, out_message->nod_impure);
		}

	Procedure *procedure = (Procedure*) node->nod_arg[e_esp_procedure];
	Request *proc_request = EXE_find_request(tdbb, procedure->findRequest(), false);

	if (!out_message) 
		{
		//format = (Format*) procedure->prc_output_msg->nod_arg[e_msg_format];
		format = (Format*) procedure->findOutputMsg()->nod_arg[e_msg_format];
		out_msg_length = format->fmt_length;
		temp_buffer = FB_NEW_RPT(*tdbb->tdbb_default, out_msg_length + DOUBLE_ALIGN - 1) str();
		out_msg = (UCHAR*) FB_ALIGN((U_IPTR) temp_buffer->str_data, DOUBLE_ALIGN);
		}

	/* Save the old pool */

	JrdMemoryPool *old_pool = tdbb->tdbb_default;
	tdbb->tdbb_default = proc_request->req_pool;

	/* Catch errors so we can unwind cleanly */

	try 
		{
		Transaction *transaction = request->req_transaction;
		SLONG save_point_number = transaction->tra_save_point->sav_number;
		proc_request->req_timestamp = request->req_timestamp;
		EXE_start(tdbb, proc_request, transaction);
		
		if (in_message)
			EXE_send(tdbb, proc_request, 0, in_msg_length, in_msg);

		EXE_receive(tdbb, proc_request, 1, out_msg_length, out_msg);

		/* Clean up all savepoints started during execution of the
		   procedure */

		if (transaction != tdbb->tdbb_database->dbb_sys_trans) 
			for (Savepoint* save_point = transaction->tra_save_point;
				  save_point && save_point_number < save_point->sav_number;
				  save_point = transaction->tra_save_point)
			    VIO_verb_cleanup(tdbb, transaction);

		}
	catch (...) 
		{
		tdbb->tdbb_default = old_pool;
		tdbb->tdbb_request = request;
		//EXE_unwind(tdbb, proc_request);
		proc_request->unwind();
		proc_request->req_attachment = NULL;
		proc_request->req_flags &= ~(req_in_use | req_proc_fetch);
		proc_request->req_timestamp = 0;
		delete temp_buffer;
		throw;
		}

	tdbb->tdbb_default = old_pool;
	//EXE_unwind(tdbb, proc_request);
	proc_request->unwind();
	tdbb->tdbb_request = request;
	temp = node->nod_arg[e_esp_outputs];
	
	if (temp) 
		for (JRD_NOD *ptr = temp->nod_arg, *end = ptr + temp->nod_count; ptr < end; ptr++)
			EXE_assignment(tdbb, *ptr);

	delete temp_buffer;
	proc_request->req_attachment = NULL;
	proc_request->req_flags &= ~(req_in_use | req_proc_fetch);
	proc_request->req_timestamp = 0;
}


static Request *execute_triggers(thread_db* tdbb,
								Relation* relation,
								Triggers** triggers,
								Record* old_rec,
								Record* new_rec,
								enum req_ta trigger_action)
{
/**************************************
 *
 *	e x e c u t e _ t r i g g e r s
 *
 **************************************
 *
 * Functional description
 *	Execute group of triggers.  Return pointer to failing trigger
 *	if any blow up.
 *
 **************************************/

#ifdef SHARED_CACHE
	Sync sync (&relation->syncTriggers, "execute_triggers");
	sync.lock(Shared);
#endif
	
	if (!*triggers) 
		return NULL;

	Request* request = NULL;
	Transaction* transaction = tdbb->tdbb_request->req_transaction;
	Triggers* vector = *triggers;
	Request* result = NULL;

	try
		{
		//for (Triggers::iterator ptr = vector->begin(); ptr != vector->end(); ++ptr)
		for (int n = 0; n < vector->size(); ++n)
			{
			Trigger *trigger = (*vector)[n];
			trigger->compile(tdbb);
			request = EXE_find_request(tdbb, trigger->request, false);
			request->req_rpb[0].rpb_record = old_rec;
			request->req_rpb[1].rpb_record = new_rec;
			request->req_timestamp = tdbb->tdbb_request->req_timestamp;
			request->req_trigger_action = trigger_action;
			EXE_start(tdbb, request, transaction);
			request->req_attachment = NULL;
			request->req_flags &= ~req_in_use;
			request->req_timestamp = 0;
			request->req_flags &= ~req_in_use;
			
			if (request->req_operation == req_unwind) 
				{
				result = request;
				break;
				}
			request = NULL;
			}

		if (vector != *triggers) 
			MET_release_triggers(tdbb, &vector);

		return result;

		}
	catch (...)
		{
		if (request)
			{
			request->req_flags &= ~req_in_use;
			request->req_timestamp = 0;
			request->req_flags &= ~req_in_use;
			}
			
		if (vector != *triggers) 
			MET_release_triggers(tdbb, &vector);

		//if (!request) 
		  throw; // trigger probally fails to compile
		  //firebird::status_exception::raise(-1);

		//return request;
		}
}


static void stuff_stack_trace(Request* request, OSRIException& exception)
{
	JString sTrace;
	bool isEmpty = true;

	for (Request* req = request; req; req = req->req_caller)
	{
		JString name;

		if (req->req_trg_name.length()) {
			name = "At trigger '";
			name += req->req_trg_name;
		}
		else if (req->req_procedure) {
			name = "At procedure '";
			name += req->req_procedure->findName();
		}

		if (! name.IsEmpty())
		{
			// name.trim();

			if (sTrace.length() + name.length() + 2 > MAX_STACK_TRACE)
				break;

			if (isEmpty) {
				isEmpty = false;
			}
			else {
				sTrace += "\n";
			}
			sTrace += name + "'";

			//if (req->req_src_line)
			//{
			//	Firebird::string src_info;
			//	src_info.printf(" line: %u, col: %u", req->req_src_line, req->req_src_column);

			//	if (sTrace.length() + src_info.length() > MAX_STACK_TRACE)
			//		break;

			//	sTrace += src_info;
			//}
		}
	}

	if (!isEmpty) 
		exception.appendException(isc_stack_trace, isc_arg_string, sTrace.getString(), 0);
}


static JRD_NOD looper(thread_db* tdbb, Request *request, JRD_NOD in_node)
{
/**************************************
 *
 *	l o o p e r
 *
 **************************************
 *
 * Functional description
 *	Cycle thru request execution tree.  Return next node for
 *	execution on stall or request complete.
 *
 **************************************/

	impure_state* impure;
	SSHORT which_erase_trig = 0;
	SSHORT which_sto_trig   = 0;
	SSHORT which_mod_trig   = 0;
	volatile JRD_NOD top_node = 0;
	volatile JRD_NOD prev_node;
	Transaction* transaction;
	OSRIException pendingException;
	
	/* If an error happens during the backout of a savepoint, then the transaction
	   must be marked 'dead' because that is the only way to clean up after a
	   failed backout.  The easiest way to do this is to kill the application
	   by calling bugcheck.
	   To facilitate catching errors during VIO_verb_cleanup, the following
	   define is used. */
   
#define VERB_CLEANUP									\
	try {												\
	    VIO_verb_cleanup (tdbb, transaction);			\
    }													\
	catch (OSRIException&) \
	{							\
		if (dbb->dbb_flags & DBB_bugcheck) {				\
			throw;	\
		}																\
    	BUGCHECK (290); /* msg 290 error during savepoint backout */	\
	}

	if (!(transaction = request->req_transaction)) 
		ERR_post(isc_req_no_trans, 0);

	//SET_TDBB(tdbb);
	DBB dbb = tdbb->tdbb_database;
	BLKCHK(in_node, type_nod);

	// Save the old pool and request to restore on exit

	JrdMemoryPool* old_pool = tdbb->tdbb_default;
	tdbb->tdbb_default = request->req_pool;

	Request *old_request = tdbb->tdbb_request;
	tdbb->tdbb_request = request;
	tdbb->tdbb_transaction = transaction;
    fb_assert(request->req_caller == NULL);
	request->req_caller = old_request;

	SLONG save_point_number = (transaction->tra_save_point) ? transaction->tra_save_point->sav_number : 0;
	JRD_NOD node = in_node;

	// Catch errors so we can unwind cleanly

	bool error_pending = false;
	bool catch_disabled = false;

	// Execute stuff until we drop

	while (node && !(request->req_flags & req_stall))
		{
		try 
		{
		switch (node->nod_type) 
			{
			case nod_asn_list:
				if (request->req_operation == req_evaluate) 
					{
					for (JRD_NOD *ptr = node->nod_arg, *end = ptr + node->nod_count; ptr < end; ptr++)
						EXE_assignment(tdbb, *ptr);
						
					request->req_operation = req_return;
					}
					
				node = node->nod_parent;
				break;

			case nod_assignment:
				if (request->req_operation == req_evaluate)
					EXE_assignment(tdbb, node);
					
				node = node->nod_parent;
				break;

			case nod_dcl_variable:
				{
				impure_value* variable = (impure_value*) IMPURE (request, node->nod_impure);
				variable->vlu_desc = *(DSC *) (node->nod_arg + e_dcl_desc);
				variable->vlu_desc.dsc_flags = 0;
				variable->vlu_desc.dsc_address = (UCHAR*) &variable->vlu_misc;
				
				if (variable->vlu_desc.dsc_dtype <= dtype_varying && !variable->vlu_string)
					{
					variable->vlu_string =FB_NEW_RPT(*tdbb->tdbb_default, variable->vlu_desc.dsc_length) str();
					variable->vlu_string->str_length = variable->vlu_desc.dsc_length;
					variable->vlu_desc.dsc_address = variable->vlu_string->str_data;
					}
					
				request->req_operation = req_return;
				node = node->nod_parent;
				}
				break;

			case nod_erase:
				if ((request->req_operation == req_return) && (node->nod_arg[e_erase_sub_erase]))
					{
					if (!top_node) 
						{
						top_node = node;
						which_erase_trig = PRE_TRIG;
						}
						
					prev_node = node;
					node = erase(tdbb, node, which_erase_trig);
					
					if (which_erase_trig == PRE_TRIG) 
						{
						node = prev_node->nod_arg[e_erase_sub_erase];
						node->nod_parent = prev_node;
						}
						
					if (top_node == prev_node && which_erase_trig == POST_TRIG) 
						{
						top_node = NULL;
						which_erase_trig = ALL_TRIGS;
						}
					else
						request->req_operation = req_evaluate;
					}
				else 
					{
					prev_node = node;
					node = erase(tdbb, node, ALL_TRIGS);
					
					if (!(prev_node->nod_arg[e_erase_sub_erase]) && which_erase_trig == PRE_TRIG)
						which_erase_trig = POST_TRIG;
					}
				break;
			
			case nod_exec_proc:
				if (request->req_operation == req_unwind) 
					{
					node = node->nod_parent;
					break;
					}
					
				execute_procedure(tdbb, node);
				node = node->nod_parent;
				request->req_operation = req_return;
				break;

			case nod_for:
				switch (request->req_operation) 
					{
					case req_evaluate:
						request->req_records_affected = 0;
						//RSE_open(tdbb, (RecordSource*) node->nod_arg[e_for_rsb]);
						((RecordSource*) node->nod_arg[e_for_rsb])->open(request);
						// fall thru
					case req_return:
						if (node->nod_arg[e_for_stall]) 
							{
							node = node->nod_arg[e_for_stall];
							break;
							}
						// fall thrue
					case req_sync:
						//if (RSE_get_record(tdbb, (RecordSource*) node->nod_arg[e_for_rsb],
						if (((RecordSource*) node->nod_arg[e_for_rsb])->get(request,
#ifdef SCROLLABLE_CURSORS
										RSE_get_next))
#else
										RSE_get_forward))
#endif
							{
							node = node->nod_arg[e_for_statement];
							request->req_operation = req_evaluate;
							break;
							}
							
						request->req_operation = req_return;
						// fall thru
					default:
						//RSE_close(tdbb, (RecordSource*) node->nod_arg[e_for_rsb]);
						((RecordSource*) node->nod_arg[e_for_rsb])->close(request);
						node = node->nod_parent;
					}
				break;

			case nod_dcl_cursor:
				if (request->req_operation == req_evaluate) 
					{
					const USHORT number = (USHORT) (IPTR) node->nod_arg[e_dcl_cursor_number];
					// set up the cursors vector
					request->req_cursors = vec::newVector(*request->req_pool, request->req_cursors, number + 1);
					// store RSB in the vector
					(*request->req_cursors)[number] = node->nod_arg[e_dcl_cursor_rsb];
					request->req_operation = req_return;
					}
					
				node = node->nod_parent;
				break;

			case nod_cursor_stmt:
				{
				const UCHAR op = (UCHAR) (IPTR) node->nod_arg[e_cursor_stmt_op];
				const USHORT number = (USHORT) (IPTR) node->nod_arg[e_cursor_stmt_number];
				// get RSB and the impure area
				fb_assert(request->req_cursors && number < request->req_cursors->count());
				RecordSource* rsb = (RecordSource*) (*request->req_cursors)[number];
				IRSB impure = (IRSB) IMPURE (tdbb->tdbb_request, rsb->rsb_impure);
				
				switch (op) 
					{
					case blr_cursor_open:
						if (request->req_operation == req_evaluate) 
							{
							if (impure->irsb_flags & irsb_open) 
								ERR_post(isc_cursor_already_open, 0);

							//RSE_open(tdbb, rsb);
							rsb->open(request);
							request->req_operation = req_return;
							}
						node = node->nod_parent;
						break;
						
					case blr_cursor_close:
						if (request->req_operation == req_evaluate) 
							{
							if (!(impure->irsb_flags & irsb_open)) 
								ERR_post(isc_cursor_not_open, 0);

							//RSE_close(tdbb, rsb);
							rsb->close(request);
							request->req_operation = req_return;
							}
							
						node = node->nod_parent;
						break;
						
					case blr_cursor_fetch:
						switch (request->req_operation) 
							{
							case req_evaluate:
								if (!(impure->irsb_flags & irsb_open)) 
									ERR_post(isc_cursor_not_open, 0);

								if (node->nod_arg[e_cursor_stmt_seek]) 
									{
									node = node->nod_arg[e_cursor_stmt_seek];
									break;
									}
								request->req_records_affected = 0;
								// fall thru
							case req_return:
								if (!request->req_records_affected) 
									{
									// fetch one record
									//if (RSE_get_record(tdbb, rsb,
									if (rsb->get(request,
#ifdef SCROLLABLE_CURSORS
													RSE_get_next))
#else
													RSE_get_forward))
#endif
										{
										node = node->nod_arg[e_cursor_stmt_into];
										request->req_operation = req_evaluate;
										break;
										}
									}
								request->req_operation = req_return;
								//fall thru
							default:
								node = node->nod_parent;
							}
					break;
					}
				}
				break;

			case nod_abort:
				switch (request->req_operation) 
					{
					case req_evaluate:
						{
						PsqlException* xcp_node = reinterpret_cast<PsqlException*>(node->nod_arg[e_xcp_desc]);
						
						if (xcp_node)
							/* XCP is defined, so throw an exception */
							set_error(tdbb, &xcp_node->xcp_rpt[0], node->nod_arg[e_xcp_msg]);
						else if (!request->req_last_xcp->success())
							/* XCP is undefined, but there was a known exception before, so re-initiate it */
							set_error(tdbb, NULL, NULL);
						else
							/* XCP is undefined and there weren't any exceptions before, so just do nothing */
							request->req_operation = req_return;
						}

					default:
						node = node->nod_parent;
					}
				break;

			case nod_user_savepoint:
				switch (request->req_operation) 
					{
					case req_evaluate:
						if (transaction != dbb->dbb_sys_trans) 
							{
							UCHAR operation = (UCHAR) (long) node->nod_arg[e_sav_operation];
							TEXT * node_savepoint_name = (TEXT*) node->nod_arg[e_sav_name]; 
							// Skip the savepoint created by EXE_start
							Savepoint* savepoint = transaction->tra_save_point->sav_next;
							Savepoint* previous = transaction->tra_save_point;
							bool found = false;
							
							// Find savepoint
							
							while (true) 
								{
								if (!savepoint || !(savepoint->sav_flags & SAV_user))
									break;

								if (!strcmp(node_savepoint_name, savepoint->sav_name)) 
									{
									found = true;
									break;
									}

								previous = savepoint;
								savepoint = savepoint->sav_next;
								}
								
							if (!found && operation != blr_savepoint_set) 
								ERR_post(isc_invalid_savepoint,
									isc_arg_string, node_savepoint_name, 0);

							if (operation == blr_savepoint_set) 
								{
								// Release the savepoint
								if (found) 
									{
									previous->sav_next = savepoint->sav_next;
									Savepoint* current = transaction->tra_save_point;
									transaction->tra_save_point = savepoint;
									VERB_CLEANUP;
									transaction->tra_save_point = current;
									}

								// Use the savepoint created by EXE_start
								transaction->tra_save_point->sav_flags |= SAV_user;
								strcpy(transaction->tra_save_point->sav_name, node_savepoint_name);
								}
							else if (operation == blr_savepoint_release_single) 
								{
								// Release the savepoint
								previous->sav_next = savepoint->sav_next;
								Savepoint* current = transaction->tra_save_point;
								transaction->tra_save_point = savepoint;
								VERB_CLEANUP;
								transaction->tra_save_point = current;
								}
							else if (operation == blr_savepoint_release) 
								{
								SLONG sav_number = savepoint->sav_number;

								// Release the savepoint and all subsequent ones
								while (transaction->tra_save_point &&
									transaction->tra_save_point->sav_number >= sav_number) 
									{
									VERB_CLEANUP;
									}

								// Restore the savepoint initially created by EXE_start
								VIO_start_save_point(tdbb, transaction);
								}
							else if (operation == blr_savepoint_undo) 
								{
								SLONG sav_number = savepoint->sav_number;

								// Undo the savepoint
								while (transaction->tra_save_point &&
									transaction->tra_save_point->sav_number >= sav_number) 
									{
									transaction->tra_save_point->sav_verb_count++;
									VERB_CLEANUP;
									}

								// Now set the savepoint again to allow to return to it later
								VIO_start_save_point(tdbb, transaction);
								transaction->tra_save_point->sav_flags |= SAV_user;
								strcpy(transaction->tra_save_point->sav_name, node_savepoint_name);
								}
							else 
								BUGCHECK(232);
						}
					default:
						node = node->nod_parent;
						request->req_operation = req_return;
					}
				break;

			case nod_start_savepoint:
				switch (request->req_operation) 
					{
					case req_evaluate:
						/* Start a save point */

						if (transaction != dbb->dbb_sys_trans)
							VIO_start_save_point(tdbb, transaction);

					default:
						node = node->nod_parent;
						request->req_operation = req_return;
					}
				break;

			case nod_end_savepoint:
				switch (request->req_operation) 
					{
					case req_evaluate:
					case req_unwind:
						/* If any requested modify/delete/insert
						ops have completed, forget them */
						
						if (transaction != dbb->dbb_sys_trans) 
							{
							/* If an error is still pending when the savepoint is 
							supposed to end, then the application didn't handle the
							error and the savepoint should be undone. */
							
							if (error_pending) 
								++transaction->tra_save_point->sav_verb_count;
								
							VERB_CLEANUP;
							}

					default:
						node = node->nod_parent;
						request->req_operation = req_return;
					}
				break;

			case nod_handler:
				switch (request->req_operation) 
					{
					case req_evaluate:
						node = node->nod_arg[0];
						break;

					case req_unwind:
						if (!request->req_label)
							request->req_operation = req_return;

					default:
						node = node->nod_parent;
					}
				break;

			case nod_block:
				switch (request->req_operation) 
				{
					SLONG count;
					Savepoint* save_point;

				case req_evaluate:
					if (transaction != dbb->dbb_sys_trans) 
						{
						VIO_start_save_point(tdbb, transaction);
						save_point = transaction->tra_save_point;
						count = save_point->sav_number;
						MOVE_FAST(&count, IMPURE (request, node->nod_impure), sizeof(SLONG));
						}
					node = node->nod_arg[e_blk_action];
					break;

				case req_unwind:
					{
					JRD_NOD *ptr, *end;

					if (request->req_flags & req_leave)
						{
						// Although the req_operation is set to req_unwind,
						// it's not an error case if req_leave bit is set.
						// req_leave bit indicates that we hit an EXIT or
						// BREAK/LEAVE statement in the SP/trigger code.
						// Do not perform the error handling stuff.
						
						if (transaction != dbb->dbb_sys_trans) 
							{
							MOVE_FAST (IMPURE (request, node->nod_impure), &count, sizeof(SLONG));
							
							for (save_point = transaction->tra_save_point;
								 save_point && count <= save_point->sav_number;
								 save_point = transaction->tra_save_point)
								VERB_CLEANUP;
							}
						node = node->nod_parent;
						break;
						}
						
					if (transaction != dbb->dbb_sys_trans)
						{
						MOVE_FAST (IMPURE (request, node->nod_impure), &count, sizeof(SLONG));
						
						/* Since there occurred an error (req_unwind), undo all savepoints
						up to, but not including, the savepoint of this block.  The
						savepoint of this block will be dealt with below. */
						
						for (save_point = transaction->tra_save_point;
							 save_point && count < save_point->sav_number;
							 save_point = transaction->tra_save_point) 
							{
							++transaction->tra_save_point->sav_verb_count;
							VERB_CLEANUP;
							}
						}

					JRD_NOD handlers = node->nod_arg[e_blk_handlers];
					
					if (handlers)
						{
						ULONG prev_req_error_handler;
						node = node->nod_parent;
						
						for (ptr = handlers->nod_arg, end = ptr + handlers->nod_count; ptr < end; ptr++)
							{
							const PsqlException* xcp_node = reinterpret_cast<PsqlException*>((*ptr)->nod_arg[e_err_conditions]);
							
							if (test_and_fixup_error(tdbb, xcp_node, request))
								{
								request->req_operation = req_evaluate;
								node = (*ptr)->nod_arg[e_err_action];
								error_pending = false;
								catch_disabled = true;

								/* On entering looper old_request etc. are saved.
								   On recursive calling we will loose the actual old
								   request for that invocation of looper. Avoid this. */

								tdbb->tdbb_default = old_pool;
								tdbb->tdbb_request = old_request;
								fb_assert(request->req_caller == old_request);
								request->req_caller = NULL;

								/* Save the previous state of req_error_handler
								   bit. We need to restore it later. This is
								   necessary if the error handler is deeply 
								   nested. */

								prev_req_error_handler = request->req_flags & req_error_handler;
								request->req_flags |= req_error_handler;
								node = looper(tdbb, request, node);
								request->req_flags &= ~(req_error_handler);
								request->req_flags |= prev_req_error_handler;

								/* Note: Previously the above call
								"node = looper (tdbb, request, node);"
								never returned back till the node tree
								was executed completely. Now that the looper
								has changed its behaviour such that it
								returns back after handling error. This 
								makes it necessary that the jmpbuf be reset
								so that looper can proceede with the 
								processing of execution tree. If this is
								not done then anymore errors will take the
								engine out of looper there by abruptly
								terminating the processing. */

								catch_disabled = false;
								tdbb->tdbb_default = request->req_pool;
								tdbb->tdbb_request = request;
								fb_assert(request->req_caller == NULL);
								request->req_caller = old_request;

								/* The error is dealt with by the application, cleanup
								this block's savepoint. */

								if (transaction != dbb->dbb_sys_trans)
									{
									for (save_point = transaction->tra_save_point;
										 save_point && count <= save_point->sav_number;
										 save_point = transaction->tra_save_point)
										{
										VERB_CLEANUP;
										}
									}
								}
							}
						}
					else
						node = node->nod_parent;

					/* If the application didn't have an error handler, then
					the error will still be pending.  Undo the block by
					using its savepoint. */

					if (error_pending && transaction != dbb->dbb_sys_trans) 
						{
						++transaction->tra_save_point->sav_verb_count;
						VERB_CLEANUP;
						}
					}
					break;

				case req_return:
					if (transaction != dbb->dbb_sys_trans) 
						{
						MOVE_FAST (IMPURE (request, node->nod_impure), &count, sizeof(SLONG));
						
						for (save_point = transaction->tra_save_point;
							 save_point && count <= save_point->sav_number;
							 save_point = transaction->tra_save_point)
							VERB_CLEANUP;
						}
				default:
					node = node->nod_parent;
				}
				break;

			case nod_error_handler:
				if (request->req_flags & req_error_handler && !error_pending) 
				{
					fb_assert(request->req_caller == old_request);
					request->req_caller = NULL;
					return node;
				}
				node = node->nod_parent;
				node = node->nod_parent;
				
				if (request->req_operation == req_unwind)
					node = node->nod_parent;
					
				request->req_last_xcp->clear();
				break;

			case nod_label:
				switch (request->req_operation) 
					{
					case req_evaluate:
						node = node->nod_arg[e_lbl_statement];
						break;

					case req_unwind:
						if ((request->req_label == (USHORT)(long) node->nod_arg[e_lbl_label]) &&
								(request->req_flags & (req_leave | req_error_handler))) 
							{
							request->req_flags &= ~req_leave;
							request->req_operation = req_return;
							}

					default:
						node = node->nod_parent;
					}
				break;

			case nod_leave:
				request->req_flags |= req_leave;
				request->req_operation = req_unwind;
				request->req_label = (USHORT)(long) node->nod_arg[0];
				node = node->nod_parent;
				break;

			case nod_list:
				impure = (impure_state*) IMPURE (request, node->nod_impure);
				switch (request->req_operation) 
					{
					case req_evaluate:
						impure->sta_state = 0;
					case req_return:
					case req_sync:
						if (impure->sta_state < node->nod_count) 
							{
							request->req_operation = req_evaluate;
							node = node->nod_arg[impure->sta_state++];
							break;
							}
							
						request->req_operation = req_return;
					default:
						node = node->nod_parent;
					}
				break;

			case nod_loop:
				switch (request->req_operation) 
					{
					case req_evaluate:
					case req_return:
						node = node->nod_arg[0];
						request->req_operation = req_evaluate;
						break;

					default:
						node = node->nod_parent;
					}
				break;

			case nod_if:
				if (request->req_operation == req_evaluate)
					if (EVL_boolean(tdbb, node->nod_arg[e_if_boolean])) 
						{
						node = node->nod_arg[e_if_true];
						request->req_operation = req_evaluate;
						break;
						}
					else if (node->nod_arg[e_if_false]) 
						{
						node = node->nod_arg[e_if_false];
						request->req_operation = req_evaluate;
						break;
						}
					else
						request->req_operation = req_return;
						
				node = node->nod_parent;
				break;

			case nod_modify:
				impure = (impure_state*) IMPURE (request, node->nod_impure);
				
				if ((request->req_operation == req_return) &&
					(!impure->sta_state) && (node->nod_arg[e_mod_sub_mod])) 
					{
					if (!top_node) 
						{
						top_node = node;
						which_mod_trig = PRE_TRIG;
						}
						
					prev_node = node;
					node = modify(tdbb, node, which_mod_trig);
					
					if (which_mod_trig == PRE_TRIG) 
						{
						node = prev_node->nod_arg[e_mod_sub_mod];
						node->nod_parent = prev_node;
						}
					if (top_node == prev_node && which_mod_trig == POST_TRIG) 
						{
						top_node = NULL;
						which_mod_trig = ALL_TRIGS;
						}
					else 
						request->req_operation = req_evaluate;
					}
				else 
					{
					prev_node = node;
					node = modify(tdbb, node, ALL_TRIGS);
					
					if (!(prev_node->nod_arg[e_mod_sub_mod]) && which_mod_trig == PRE_TRIG)
						which_mod_trig = POST_TRIG;
					}

				break;

			case nod_nop:
				request->req_operation = req_return;
				node = node->nod_parent;
				break;

			case nod_receive:
				node = receive_msg(tdbb, node);
				break;
				
#if 0
			case nod_exec_sql:
				if (request->req_operation == req_unwind) 
					{
					node = node->nod_parent;
					break;
					}
					
				exec_sql(tdbb, request, EVL_expr(tdbb, node->nod_arg[0]));
				
				if (request->req_operation == req_evaluate)
					request->req_operation = req_return;
					
				node = node->nod_parent;
				break;
#endif

			case nod_exec_into: 
				{
				ExecStatement **pExec = (ExecStatement**)  IMPURE (request, node->nod_impure);
				if (!*pExec)
					request->getExecStatement(pExec);
				ExecStatement *exec = *pExec;
				
				switch (request->req_operation) 
					{
					case req_evaluate:
						exec->prepare(node->nod_arg[0], !node->nod_arg[1]);
						exec->execute(node->nod_arg[2]);
					case req_return:
					case req_sync:
						if (exec->fetch(node->nod_arg[2])) 
							{
							if (node->nod_arg[1])
								{
								request->req_operation = req_evaluate;
								node = node->nod_arg[1];
								}
							else
								{
								request->req_operation = req_return;
								node = node->nod_parent;
								}
							break;
							}
						request->req_operation = req_return;
					default:
						// if have active opened request - close it
						exec->close();
						node = node->nod_parent;
						}
					}
				break;

			case nod_post:
				{
				DeferredWork* work = DFW_post_work(transaction, dfw_post_event,
										EVL_expr(tdbb, node->nod_arg[0]), 0);
				if (node->nod_arg[1])
					DFW_post_work_arg(transaction, work,
									EVL_expr(tdbb, node->nod_arg[1]), 0);
				}

				/* for an autocommit transaction, events can be posted
				* without any updates */

				if (transaction->tra_flags & TRA_autocommit)
					transaction->tra_flags |= TRA_perform_autocommit;
				
			case nod_message:
				if (request->req_operation == req_evaluate)
					request->req_operation = req_return;
					
				node = node->nod_parent;
				break;

			case nod_stall:
				node = stall(tdbb, node);
				break;

			case nod_select:
				node = selct(tdbb, node);
				break;

			case nod_send:
				node = send_msg(tdbb, node);
				break;

			case nod_store:
				impure = (impure_state*) IMPURE (request, node->nod_impure);
				
				if ((request->req_operation == req_return) &&
					(!impure->sta_state) && (node->nod_arg[e_sto_sub_store])) 
					{
					if (!top_node) 
						{
						top_node = node;
						which_sto_trig = PRE_TRIG;
						}
						
					prev_node = node;
					node = store(tdbb, node, which_sto_trig);
					
					if (which_sto_trig == PRE_TRIG) 
						{
						node = prev_node->nod_arg[e_sto_sub_store];
						node->nod_parent = prev_node;
						}
						
					if (top_node == prev_node && which_sto_trig == POST_TRIG) 
						{
						top_node = NULL;
						which_sto_trig = ALL_TRIGS;
						}
					else
						request->req_operation = req_evaluate;
					}
				else 
					{
					prev_node = node;
					node = store(tdbb, node, ALL_TRIGS);
					
					if (!(prev_node->nod_arg[e_sto_sub_store]) && which_sto_trig == PRE_TRIG)
						which_sto_trig = POST_TRIG;
					}
				break;

#ifdef SCROLLABLE_CURSORS
			case nod_seek:
				node = seek_rse(tdbb, request, node);
				break;
#endif


			case nod_set_generator:
				if (request->req_operation == req_evaluate) 
					{
					DSC *desc = EVL_expr(tdbb, node->nod_arg[e_gen_value]);
					DPM_gen_id(tdbb, (int) (long) node->nod_arg[e_gen_id], 1,
									MOV_get_int64(desc, 0));
					request->req_operation = req_return;
					}
					
				node = node->nod_parent;
				break;

			case nod_set_generator2:
				if (request->req_operation == req_evaluate) 
					{
					DSC *desc = EVL_expr(tdbb, node->nod_arg[e_gen_value]);
					DPM_gen_id(tdbb, (long) node->nod_arg[e_gen_id], 1,
									MOV_get_int64(desc, 0));
					request->req_operation = req_return;
					}
					
				node = node->nod_parent;
				break;

			default:
				BUGCHECK(168);		/* msg 168 looper: action not yet implemented */
			}

//#if defined(DEBUG_GDS_ALLOC) && defined(PROD_BUILD)
//		memory_count++;
//		if ((memory_count % memory_debug) == 0) {
//			ALL_check_memory();
//		}
//#endif
		}
	catch (OSRIException& exception) 
		{
		// If we already have a handled error, and took another, simply pass the buck.
		
		if (catch_disabled) 
			throw;

		/* If the database is already bug-checked, then get out. */
		
		if (dbb->dbb_flags & DBB_bugcheck) 
			throw;

		/* Since an error happened, the current savepoint needs to be undone. */	
			
		if (transaction != dbb->dbb_sys_trans) 
			{
			++transaction->tra_save_point->sav_verb_count;
			VERB_CLEANUP;
			}

		error_pending = true;
		pendingException = exception;
		request->req_operation = req_unwind;
		request->req_label = 0;
		
		if (! (tdbb->tdbb_flags & TDBB_stack_trace_done) ) 
		{
			stuff_stack_trace(request, pendingException);
			tdbb->tdbb_flags |= TDBB_stack_trace_done;
		}
		}
	} // while()

/* if there is no node, assume we have finished processing the 
   request unless we are in the middle of processing an asynchronous message */

#ifdef SHARED_CACHE
	Sync sync(&dbb->syncCmpClone, "EXE_recieve");
#endif
	

	if (!node
#ifdef SCROLLABLE_CURSORS
		&& !(request->req_flags & req_async_processing)
#endif
		)
		{
		// close active cursors
		if (request->req_cursors) 
			{
			for (vec::iterator ptr = request->req_cursors->begin(), 
				 end = request->req_cursors->end(); ptr < end; ptr++)
				if (*ptr)
					//RSE_close(tdbb, (RecordSource*) *ptr);
					((RecordSource*) *ptr)->close(request);
			}

#ifdef SHARED_CACHE
		sync.lock(Exclusive);
#endif
		request->req_flags &= ~(req_active | req_reserved);
		request->req_timestamp = 0;
		release_blobs(tdbb, request);
		}

	request->req_next = node;
	tdbb->tdbb_default = old_pool;
	tdbb->tdbb_transaction = (tdbb->tdbb_request = old_request) ? old_request->req_transaction : NULL;
	fb_assert(request->req_caller == old_request);
	request->req_caller = NULL;

	// in the case of a pending error condition (one which did not
	// result in a exception to the top of looper), we need to
	// delete the last savepoint
	
	if (error_pending) 
		{
		if (transaction != dbb->dbb_sys_trans) 
			{
			for (Savepoint* save_point = transaction->tra_save_point; 
				 ((save_point) && (save_point_number <= save_point->sav_number));
				 save_point = transaction->tra_save_point) 
				{
				++transaction->tra_save_point->sav_verb_count;
				VERB_CLEANUP;
				}
			}

		throw pendingException;
		}

	// if the request was aborted, assume that we have already
	// longjmp'ed to the top of looper, and therefore that the
	// last savepoint has already been deleted

	if (request->req_flags & req_abort) {
		ERR_post(isc_req_sync, 0);
	}

	return node;
}


static JRD_NOD modify(thread_db* tdbb, JRD_NOD node, SSHORT which_trig)
{
/**************************************
 *
 *	m o d i f y
 *
 **************************************
 *
 * Functional description
 *	Execute a MODIFY statement.
 *
 **************************************/
	Database* dbb = tdbb->tdbb_database;
	Request* request = tdbb->tdbb_request;
	Transaction* transaction = request->req_transaction;
	impure_state* impure = (impure_state*) IMPURE (request, node->nod_impure);

	SSHORT org_stream = (USHORT)(long) node->nod_arg[e_mod_org_stream];
	record_param* org_rpb = &request->req_rpb[org_stream];
	Relation* relation = org_rpb->rpb_relation;

	SSHORT new_stream = (USHORT)(long) node->nod_arg[e_mod_new_stream];
	record_param* new_rpb = &request->req_rpb[new_stream];


	/* If the stream was sorted, the various fields in the rpb are
	   probably junk.  Just to make sure that everything is cool,
	   refetch and release the record. */

	if (org_rpb->rpb_stream_flags & RPB_s_refetch) 
		{
		SLONG tid_fetch = org_rpb->rpb_transaction;
		
		if ((!DPM_get(tdbb, org_rpb, LCK_read)) ||
			(!VIO_chase_record_version(tdbb, org_rpb, transaction, tdbb->tdbb_default, FALSE)))
			ERR_post(isc_deadlock, isc_arg_gds, isc_update_conflict, 0);

		VIO_data(tdbb, org_rpb, tdbb->tdbb_request->req_pool);

		/* If record is present, and the transaction is read committed,
		 * make sure the record has not been updated.  Also, punt after
		 * VIO_data () call which will release the page.
		 */

		if ((transaction->tra_flags & TRA_read_committed) && (tid_fetch != org_rpb->rpb_transaction))
				ERR_post(isc_deadlock, isc_arg_gds, isc_update_conflict, 0);

		org_rpb->rpb_stream_flags &= ~RPB_s_refetch;
		}

	switch (request->req_operation) 
		{
		case req_evaluate:
			break;

		case req_return:
			if (impure->sta_state) 
				{
				impure->sta_state = 0;
				Record* org_record = org_rpb->rpb_record;
				Record* new_record = new_rpb->rpb_record;
				MOVE_FASTER(new_record->rec_data, org_record->rec_data,
							new_record->rec_length);
				request->req_operation = req_evaluate;
				return node->nod_arg[e_mod_statement];
				}

			/* CVC: This call made here to clear the record in each NULL field and
			   varchar field whose tail may contain garbage. */
			   
			cleanup_rpb(tdbb, new_rpb);

			if (transaction != dbb->dbb_sys_trans)
				++transaction->tra_save_point->sav_verb_count;

			PreModifyEraseTriggers(tdbb, relation, &relation->rel_pre_modify,
									which_trig, org_rpb, new_rpb->rpb_record, 
									req_trigger_update);

			if (node->nod_arg[e_mod_validate]) 
				validate(tdbb, node->nod_arg[e_mod_validate]);

			if (relation->rel_file)
				EXT_modify(tdbb, org_rpb, new_rpb, reinterpret_cast<int*>(transaction));
			else if (!relation->rel_view_rse)
				{
				USHORT bad_index;
				Relation* bad_relation;
				VIO_modify(tdbb, org_rpb, new_rpb, transaction);
				IDX_E error_code = IDX_modify(tdbb, org_rpb, new_rpb, transaction, &bad_relation, &bad_index);
				
				if (error_code) 
					{
					VIO_bump_count(tdbb, DBB_update_count, bad_relation, true);
					ERR_duplicate_error(error_code, bad_relation, bad_index);
					}
				}

			Request* trigger;
			
			if (relation->rel_post_modify && which_trig != PRE_TRIG &&
				(trigger = execute_triggers(tdbb, relation, &relation->rel_post_modify,
											org_rpb->rpb_record,
											new_rpb->rpb_record,
											req_trigger_update)))
				{
				VIO_bump_count(tdbb, DBB_update_count, relation, true);
				trigger_failure(tdbb, trigger);
				}

			/* now call IDX_modify_check_constrints after all post modify triggers 
				have fired.  This is required for cascading referential integrity, 
				which can be implemented as post_erase triggers */

			if (!relation->rel_file && !relation->rel_view_rse)
				{
				USHORT bad_index;
				Relation* bad_relation;
				IDX_E error_code = IDX_modify_check_constraints(tdbb, org_rpb, new_rpb, transaction, &bad_relation, &bad_index);

				if (error_code) 
					{
					VIO_bump_count(tdbb, DBB_update_count, relation, true);
					ERR_duplicate_error(error_code, bad_relation, bad_index);
					}
				}

			if (transaction != dbb->dbb_sys_trans) 
				--transaction->tra_save_point->sav_verb_count;


			/* CVC: Increment the counter only if we called VIO/EXT_modify() and
					we were successful. */
					
			if (!(request->req_view_flags & req_first_modify_return)) 
				{
				request->req_view_flags |= req_first_modify_return;
				
				if (relation->rel_view_rse) 
					request->req_top_view_modify = relation;
				}
				
			if (relation == request->req_top_view_modify) 
				{
				if (which_trig == ALL_TRIGS || which_trig == POST_TRIG) 
					{
					request->req_records_updated++;
					request->req_records_affected++;
					}
				}
			else if (relation->rel_file || !relation->rel_view_rse) 
				{
				request->req_records_updated++;
				request->req_records_affected++;
				}

			if (which_trig != PRE_TRIG) 
				{
				Record* org_record = org_rpb->rpb_record;
				org_rpb->rpb_record = new_rpb->rpb_record;
				new_rpb->rpb_record = org_record;
				}

		default:
			return node->nod_parent;
		}

	impure->sta_state = 0;
	RLCK_reserve_relation(tdbb, transaction, relation, TRUE, TRUE);

	/* Fall thru on evaluate to set up for modify before executing sub-statement.
	   This involves finding the appropriate format, making sure a record block
	   exists for the stream and is big enough, and copying fields from the
	   original record to the new record. */

	Format* new_format = new_rpb->rpb_relation->getCurrentFormat(tdbb);
	Record* new_record = VIO_record(tdbb, new_rpb, new_format, tdbb->tdbb_default);
	new_rpb->rpb_address = new_record->rec_data;
	new_rpb->rpb_length = new_format->fmt_length;
	new_rpb->rpb_format_number = new_format->fmt_version;

	Format* org_format;
	Record* org_record = org_rpb->rpb_record;
	
	if (!org_record) 
		{
		org_record = VIO_record(tdbb, org_rpb, new_format, tdbb->tdbb_default);
		org_format = org_record->rec_format;
		org_rpb->rpb_address = org_record->rec_data;
		org_rpb->rpb_length = org_format->fmt_length;
		org_rpb->rpb_format_number = org_format->fmt_version;
		}
	else
		org_format = org_record->rec_format;

	/* Copy the original record to the new record.  If the format hasn't changed,
	   this is a simple move.  If the format has changed, each field must be
	   fetched and moved separately, remembering to set the missing flag. */

	if (new_format->fmt_version == org_format->fmt_version)
		MOVE_FASTER(org_record->rec_data, new_rpb->rpb_address, new_rpb->rpb_length);
	else 
		{
		DSC org_desc, new_desc;

		for (SSHORT i = 0; i < new_format->fmt_count; i++) 
			{
			/* In order to "map a null to a default" value (in EVL_field()), 
			 * the relation block is referenced. 
			 * Reference: Bug 10116, 10424 
			 */
			CLEAR_NULL(new_record, i);
			
			if (EVL_field(new_rpb->rpb_relation, new_record, i, &new_desc)) 
				{
				if (EVL_field (org_rpb->rpb_relation, org_record, i, &org_desc)) 
					MOV_move(&org_desc, &new_desc);
				else 
					{
					SET_NULL(new_record, i);
					
					if (new_desc.dsc_dtype) 
						{
						UCHAR *p;
						USHORT n;

						p = new_desc.dsc_address;
						n = new_desc.dsc_length;
						do
							*p++ = 0;
						while (--n);
						}
					}				/* if (org_record) */
				}					/* if (new_record) */
			}						/* for (fmt_count) */
		}

	if (node->nod_arg[e_mod_map_view]) 
		{
		impure->sta_state = 1;
		return node->nod_arg[e_mod_map_view];
		}

	return node->nod_arg[e_mod_statement];
}

static JRD_NOD receive_msg(thread_db* tdbb, JRD_NOD node)
{
/**************************************
 *
 *	r e c e i v e _ m s g
 *
 **************************************
 *
 * Functional description
 *	Execute a RECEIVE statement.  This can be entered either
 *	with "req_evaluate" (ordinary receive statement) or
 *	"req_proceed" (select statement).  In the latter case,
 *	the statement isn't every formalled evaluated.
 *
 **************************************/
	Request *request;

	SET_TDBB(tdbb);
	request = tdbb->tdbb_request;
	BLKCHK(node, type_nod);

	switch (request->req_operation) {
	case req_evaluate:
		request->req_operation = req_receive;
		request->req_message = node->nod_arg[e_send_message];
		request->req_flags |= req_stall;
		return node;

	case req_proceed:
		request->req_operation = req_evaluate;
		return (node->nod_arg[e_send_statement]);

	default:
		return (node->nod_parent);
	}
}


static void release_blobs(thread_db* tdbb, Request *request)
{
/**************************************
 *
 *	r e l e a s e _ b l o b s
 *
 **************************************
 *
 * Functional description
 *	Release temporary blobs assigned by this request.
 *
 **************************************/
	Transaction* transaction;

	if ( (transaction = request->req_transaction) ) 
		{
#ifdef SHARED_CACHE
		Sync sync (&transaction->syncObject, "release_blobs");
		sync.lock (Exclusive);
#endif

		/* Release blobs assigned by this request */

		for (blb** blob = &transaction->tra_blobs; *blob;) 
			if ((*blob)->blb_request == request)
				BLB_cancel(tdbb, *blob);
			else
				blob = &(*blob)->blb_next;

		/* Release arrays assigned by this request */

		for (ArrayField** array = &transaction->tra_arrays; *array;) 
			if ((*array)->arr_request == request)
				BLB_release_array(*array);
			else
				array = &(*array)->arr_next;
		}
}


static void release_proc_save_points(Request *request)
{
/**************************************
 *
 *	r e l e a s e _ p r o c _ s a v e _ p o i n t s
 *
 **************************************
 *
 * Functional description
 *	Release temporary blobs assigned by this request.
 *
 **************************************/
	Savepoint* sav_point = request->req_proc_sav_point;

	if (request->req_transaction) {
		while (sav_point) {
			Savepoint* temp_sav_point = sav_point->sav_next;
			delete sav_point;
			sav_point = temp_sav_point;
		}
	}
	request->req_proc_sav_point = NULL;
}


#ifdef SCROLLABLE_CURSORS
static JRD_NOD seek_rse(thread_db* tdbb, Request *request, JRD_NOD node)
{
/**************************************
 *
 *      s e e k _ r s e
 *
 **************************************
 *
 * Functional description
 *	Execute a nod_seek, which specifies 
 *	a direction and offset in which to 
 *	scroll a record selection expression.
 *
 **************************************/
	USHORT direction;
	SLONG offset;
	RecordSelExpr* rse;

	SET_TDBB(tdbb);
	DEV_BLKCHK(node, type_nod);

	if (request->req_operation == req_proceed) {
		/* get input arguments */

		direction = MOV_get_long(EVL_expr(tdbb,
										  node->nod_arg[e_seek_direction]),
								 0);
		offset =
			MOV_get_long(EVL_expr(tdbb, node->nod_arg[e_seek_offset]), 0);

		rse = (RecordSelExpr*) node->nod_arg[e_seek_rse];

		seek_rsb(tdbb, request, rse->rse_rsb, direction, offset);

		request->req_operation = req_return;
	}

	return node->nod_parent;
}
#endif


#ifdef SCROLLABLE_CURSORS
static void seek_rsb(
					 thread_db* tdbb,
					 Request *request, RecordSource* rsb, USHORT direction, SLONG offset)
{
/**************************************
 *
 *      s e e k _ r s b
 *
 **************************************
 *
 * Functional description
 *	Allow scrolling through a stream as defined 
 *	by the input rsb.  Handles cracks, refresh 
 *	ranges, and multiple seeking.  Uses RSE_get_record ()
 *	to do the actual work.
 *
 **************************************/
	USHORT crack_flag = 0;
	IRSB impure;
	IRSB next_impure;

	SET_TDBB(tdbb);
	DEV_BLKCHK(rsb, type_rsb);
	impure = (IRSB) IMPURE (request, rsb->rsb_impure);

/* look past any boolean to the actual stream */

	if (rsb->rsb_type == rsb_boolean) {
		seek_rsb(tdbb, request, rsb->rsb_next, direction, offset);

		/* set the backwards flag */

		next_impure = (IRSB) IMPURE (request, rsb->rsb_next->rsb_impure);

		if (next_impure->irsb_flags & irsb_last_backwards)
			impure->irsb_flags |= irsb_last_backwards;
		else
			impure->irsb_flags &= ~irsb_last_backwards;
		return;
	}

/* do simple boundary checking for bof and eof */

	switch (direction) {
	case blr_forward:
		if (impure->irsb_flags & irsb_eof)
			ERR_post(isc_stream_eof, 0);
		break;

	case blr_backward:
		if (impure->irsb_flags & irsb_bof)
			ERR_post(isc_stream_bof, 0);
		break;

	case blr_bof_forward:
	case blr_eof_backward:
		break;

	default:
		BUGCHECK(232);
	}

/* the actual offset to seek may be one less because the next time 
   through the blr_for loop we will seek one record--flag the fact 
   that a fetch is required on this stream in case it doesn't happen 
   (for example when GPRE generates BLR which does not stall prior to 
   the blr_for, as DSQL does) */

	if (offset > 0)
		switch (direction) {
		case blr_forward:
		case blr_bof_forward:
			if (!(impure->irsb_flags & irsb_last_backwards)) {
				offset--;
				if (!(impure->irsb_flags & irsb_bof))
					request->req_flags |= req_fetch_required;
			}
			break;

		case blr_backward:
		case blr_eof_backward:
			if (impure->irsb_flags & irsb_last_backwards) {
				offset--;
				if (!(impure->irsb_flags & irsb_eof))
					request->req_flags |= req_fetch_required;
			}
			break;
		}

/* now do the actual seek */

	switch (direction) {
	case blr_forward:			/* go forward from the current location */


		/* the rsb_backwards flag is used to indicate the direction to seek in; 
		   this is sticky in the sense that after the user has seek'ed in the 
		   backward direction, the next retrieval from a blr_for loop will also 
		   be in the backward direction--this allows us to continue scrolling 
		   without constantly sending messages to the engine */

		impure->irsb_flags &= ~irsb_last_backwards;

		while (offset) {
			offset--;
			if (!(RSE_get_record(tdbb, rsb, RSE_get_next)))
				break;
		}
		break;

	case blr_backward:			/* go backward from the current location */
		impure->irsb_flags |= irsb_last_backwards;

		while (offset) 
			{
			offset--;
			
			if (!(RSE_get_record(tdbb, rsb, RSE_get_next)))
				break;
			}
		break;

	case blr_bof_forward:		/* go forward from the beginning of the stream */

		RSE_close(tdbb, rsb);
		RSE_open(tdbb, rsb);

		impure->irsb_flags &= ~irsb_last_backwards;

		while (offset) {
			offset--;
			if (!(RSE_get_record(tdbb, rsb, RSE_get_next)))
				break;
		}
		break;

	case blr_eof_backward:		/* go backward from the end of the stream */

		RSE_close(tdbb, rsb);
		RSE_open(tdbb, rsb);

		/* if this is a stream type which uses bof and eof flags, 
		   reverse the sense of bof and eof in this case */

		if (impure->irsb_flags & irsb_bof) {
			impure->irsb_flags &= ~irsb_bof;
			impure->irsb_flags |= irsb_eof;
		}

		impure->irsb_flags |= irsb_last_backwards;

		while (offset) {
			offset--;
			if (!(RSE_get_record(tdbb, rsb, RSE_get_next)))
				break;
		}
		break;

	default:
		BUGCHECK(232);
	}

}
#endif


static JRD_NOD selct(thread_db* tdbb, JRD_NOD node)
{
/**************************************
 *
 *	s e l e c t
 *
 **************************************
 *
 * Functional description
 *	Execute a SELECT statement.  This is more than a little
 *	obscure.  We first set up the SELECT statement as the
 *	"message" and stall on receive (waiting for user send).
 *	EXE_send will then loop thru the sub-statements of select
 *	looking for the appropriate RECEIVE statement.  When (or if)
 *	it finds it, it will set it up the next statement to be
 *	executed.  The RECEIVE, then, will be entered with the
 *	operation "req_proceed."
 *
 **************************************/
	Request *request;

	SET_TDBB(tdbb);
	request = tdbb->tdbb_request;
	BLKCHK(node, type_nod);

	switch (request->req_operation) {
	case req_evaluate:
		request->req_message = node;
		request->req_operation = req_receive;
		request->req_flags |= req_stall;
		return node;

	default:
		return node->nod_parent;
	}
}



static JRD_NOD send_msg(thread_db* tdbb, JRD_NOD node)
{
/**************************************
 *
 *	s e n d _ m s g
 *
 **************************************
 *
 * Functional description
 *	Execute a SEND statement.
 *
 **************************************/
	Request *request;

	SET_TDBB(tdbb);
	request = tdbb->tdbb_request;
	BLKCHK(node, type_nod);

	switch (request->req_operation) {
	case req_evaluate:
		return (node->nod_arg[e_send_statement]);

	case req_return:
		request->req_operation = req_send;
		request->req_message = node->nod_arg[e_send_message];
		request->req_flags |= req_stall;
		return node;

	case req_proceed:
		request->req_operation = req_return;
		return node->nod_parent;

	default:
		return (node->nod_parent);
	}
}




static void set_error(thread_db* tdbb, const xcp_repeat* exception, JRD_NOD msg_node)
{
/**************************************
 *
 *	s e t _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	Set status vector according to specified error condition
 *	and jump to handle error accordingly.
 *
 **************************************/
	TEXT message[XCP_MESSAGE_LENGTH + 1];

	// since temp used as vary, we need size of vary::vary_length 
	// (USHORT) extra chars
	TEXT temp[XCP_MESSAGE_LENGTH + sizeof(USHORT)]; 

	Request *request = tdbb->tdbb_request;
	if (!exception) 
		{
		// retrieve the status vector and punt
		request->req_last_xcp->copyTo(tdbb->tdbb_status_vector);
		request->req_last_xcp->clear();
		ERR_punt();
		}

	USHORT length = 0;
	
	if (msg_node)
		{
		const char* string = 0;
		
		// evaluate exception message and convert it to string
		
		DSC* desc = EVL_expr(tdbb, msg_node);
		
		if (desc && !(request->req_flags & req_null))
			{
			length = MOV_make_string(desc,
									 ttype_none,
									 &string,
									 reinterpret_cast<VARY*>(temp),
									 sizeof(temp));
			length = MIN(length, sizeof(message) - 1);

			/* dimitr: or should we throw an error here, i.e.
					   replace the above assignment with the following lines:

			if (length > sizeof(message) - 1)
				ERR_post(isc_imp_exc, isc_arg_gds, isc_blktoobig, 0);
			*/

			memcpy(message, string, length);
			}
		else
			length = 0;
		
		}
	message[length] = 0;

	const TEXT *s;
	SqlIdentifier name;
	SqlIdentifier relation_name;

	switch (exception->xcp_type) 
		{
		case xcp_sql_code:
			ERR_post(isc_sqlerr, isc_arg_number, exception->xcp_code, 0);

		case xcp_gds_code:
			if (exception->xcp_code == isc_check_constraint) 
				{
				MET_lookup_cnstrt_for_trigger(tdbb, name, relation_name,
											  request->req_trg_name);
				// const CAST
				s = (name[0]) ? name : "";
				const TEXT* r = (relation_name[0]) ? relation_name : (TEXT*) "";
				ERR_post(exception->xcp_code,
						 isc_arg_string, s,
						 isc_arg_string, r, 0);
				}
			else
				ERR_post(exception->xcp_code, 0);

		case xcp_xcp_code:
			MET_lookup_exception(tdbb, exception->xcp_code, name, temp);
			
			if (message[0])
				s = message;
			else if (temp[0])
				s = temp;
			else if (name[0])
				s = name;
			else
				s = NULL;
				
			if (s)
				ERR_post(isc_except,
						 isc_arg_number, exception->xcp_code,
						 isc_arg_gds, isc_random, isc_arg_string, s,
						 0);
			else
				ERR_post(isc_except, isc_arg_number, exception->xcp_code, 0);
		}
}



static JRD_NOD stall(thread_db* tdbb, JRD_NOD node)
{
/**************************************
 *
 *	s t a l l 
 *
 **************************************
 *
 * Functional description
 *	Execute a stall statement.
 *	This is like a blr_receive, except that there is no
 *	need for a gds__send () from the user (i.e. EXE_send () in the engine).
 *	A gds__receive () will unblock the user.
 *
 **************************************/
	Request *request;

	SET_TDBB(tdbb);
	request = tdbb->tdbb_request;
	BLKCHK(node, type_nod);

	switch (request->req_operation) {
	case req_sync:
		return node->nod_parent;

	case req_proceed:
		request->req_operation = req_return;
		return node->nod_parent;

	default:
		request->req_message = node;
		request->req_operation = req_return;
		request->req_flags |= req_stall;
		return node;
	}
}


static JRD_NOD store(thread_db* tdbb, JRD_NOD node, SSHORT which_trig)
{
/**************************************
 *
 *	s t o r e
 *
 **************************************
 *
 * Functional description
 *	Execute a STORE statement.
 *
 **************************************/

	Request *trigger;
	Format* format;
	SSHORT n;
	Record* record;
	UCHAR *p;

	//SET_TDBB(tdbb);
	DBB dbb = tdbb->tdbb_database;
	BLKCHK(node, type_nod);

	Request* request = tdbb->tdbb_request;
	Transaction* transaction = request->req_transaction;
	impure_state* impure = (impure_state*) IMPURE (request, node->nod_impure);
	SSHORT stream = (USHORT)(long) node->nod_arg[e_sto_relation]->nod_arg[e_rel_stream];
	record_param* rpb = &request->req_rpb[stream];
	Relation* relation = rpb->rpb_relation;

	switch (request->req_operation) 
		{
		case req_evaluate:
			impure->sta_state = 0;
			RLCK_reserve_relation(tdbb, transaction, relation, TRUE, TRUE);
			break;

		case req_return:
			if (impure->sta_state)
				return node->nod_parent;
				
			record = rpb->rpb_record;
			format = record->rec_format;

			if (transaction != dbb->dbb_sys_trans)
				++transaction->tra_save_point->sav_verb_count;

			if (relation->rel_pre_store &&
				 (which_trig != POST_TRIG) &&
				 (trigger = execute_triggers(tdbb, relation, &relation->rel_pre_store,
											 0, record, req_trigger_insert)))
				trigger_failure(tdbb, trigger);

			if (node->nod_arg[e_sto_validate]) 
				validate(tdbb, node->nod_arg[e_sto_validate]);

			/* For optimum on-disk record compression, zero all unassigned
			   fields. In addition, zero the tail of assigned varying fields
			   so that previous remnants don't defeat compression efficiency. */

			/* CVC: The code that was here was moved to its own routine: cleanup_rpb()
					and replaced by the call shown above. */

			cleanup_rpb(tdbb, rpb);

			if (relation->rel_file) 
				EXT_store(dbb, rpb, reinterpret_cast < int *>(transaction));
			else if (!relation->rel_view_rse)
				{
				USHORT bad_index;
				Relation* bad_relation;

				VIO_store(tdbb, rpb, transaction);
				IDX_E error_code = IDX_store(tdbb,
											rpb,
											transaction,
											&bad_relation,
											&bad_index);
				if (error_code) 
					{
					VIO_bump_count(tdbb, DBB_insert_count, bad_relation, true);
					ERR_duplicate_error(error_code, bad_relation, bad_index);
					}
				}

			if (relation->rel_post_store &&
				(which_trig != PRE_TRIG) &&
				(trigger = execute_triggers(tdbb, relation, &relation->rel_post_store,
											0, record, req_trigger_insert)))
				{
				VIO_bump_count(tdbb, DBB_insert_count, relation, true);
				trigger_failure(tdbb, trigger);
				}

			/* CVC: Increment the counter only if we called VIO/EXT_store() and
					we were successful. */
					
			if (!(request->req_view_flags & req_first_store_return)) 
				{
				request->req_view_flags |= req_first_store_return;
				if (relation->rel_view_rse) 
					request->req_top_view_store = relation;
				}
				
			if (relation == request->req_top_view_store) 
				{
				if (which_trig == ALL_TRIGS || which_trig == POST_TRIG) 
					{
					request->req_records_inserted++;
					request->req_records_affected++;
					}
				}
			else if (relation->rel_file || !relation->rel_view_rse) 
				{
				request->req_records_inserted++;
				request->req_records_affected++;
				}

			if (transaction != dbb->dbb_sys_trans) 
				--transaction->tra_save_point->sav_verb_count;

			if (node->nod_arg[e_sto_statement2]) 
				{
				impure->sta_state = 1;
				request->req_operation = req_evaluate;
				
				return node->nod_arg[e_sto_statement2];
				}

		default:
			return node->nod_parent;
		}

	/* Fall thru on evaluate to set up for store before executing sub-statement.
	   This involves finding the appropriate format, making sure a record block
	   exists for the stream and is big enough, and initialize all null flags
	   to "missing." */

	format = relation->getCurrentFormat(tdbb);
	record = VIO_record(tdbb, rpb, format, tdbb->tdbb_default);

	rpb->rpb_address = record->rec_data;
	rpb->rpb_length = format->fmt_length;
	rpb->rpb_format_number = format->fmt_version;

	/* CVC: This small block added by Ann Harrison to
			start with a clean empty buffer and so avoid getting
			new record buffer with misleading information. Fixes
			bug with incorrect blob sharing during insertion in
			a stored procedure. */

	memset (record->rec_data, 0, rpb->rpb_length);
	/***
	p = record->rec_data;
	UCHAR *data_end = p + rpb->rpb_length;
	
	while (p < data_end)
		*p++ = 0;
	***/
	
	/* Initialize all fields to missing */

	p = record->rec_data;
	n = (format->fmt_count + 7) >> 3;
	
	if (n) 
		do {
			*p++ = 0xff;
		} while (--n);

	return node->nod_arg[e_sto_statement];
}



static bool test_and_fixup_error(thread_db* tdbb, const PsqlException* conditions, Request *request)
{
/**************************************
 *
 *	t e s t _ a n d _ f i x u p _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	Test for match of current state with list of error conditions.
 *  Fix type and code of the exception.
 *
 **************************************/
	SET_TDBB(tdbb);

	ISC_STATUS* status_vector = tdbb->tdbb_status_vector;
	SSHORT sqlcode = gds__sqlcode(status_vector);

	bool found = false;

	for (USHORT i = 0; i < conditions->xcp_count; i++)
	{
		switch (conditions->xcp_rpt[i].xcp_type)
		{
		case xcp_sql_code:
			if (sqlcode == conditions->xcp_rpt[i].xcp_code)
			{
				found = true;
			}
			break;

		case xcp_gds_code:
			if (status_vector[1] == conditions->xcp_rpt[i].xcp_code)
			{
				found = true;
			}
			break;

		case xcp_xcp_code:
			if ((status_vector[1] == isc_except) &&
				(status_vector[3] == conditions->xcp_rpt[i].xcp_code))
			{
				found = true;
			}
			break;

		case xcp_default:
			found = true;
			break;
		}

		if (found)
		{
			request->req_last_xcp->init(status_vector);
			status_vector[0] = 0;
			status_vector[1] = 0;
			break;
		}
    }

	return found;
}


static void trigger_failure(thread_db* tdbb, Request *trigger)
{
/**************************************
 *
 *	t r i g g e r _ f a i l u r e
 *
 **************************************
 *
 * Functional description
 *	Trigger failed, report error.
 *
 **************************************/

	//EXE_unwind(tdbb, trigger);
	trigger->unwind();
	trigger->req_attachment = NULL;
	trigger->req_flags &= ~req_in_use;
	trigger->req_timestamp = 0;

	if (trigger->req_flags & req_leave)
		{
		trigger->req_flags &= ~req_leave;
		const TEXT* msg;
		
		if (!trigger->req_trg_name.IsEmpty() &&
			 (msg = MET_trigger_msg(tdbb, trigger->req_trg_name, trigger->req_label)))
			{
			if (trigger->req_flags & req_sys_trigger)
				{
				ISC_STATUS code = PAR_symbol_to_gdscode(msg);
				
				if (code)
					ERR_post(isc_integ_fail, isc_arg_number, trigger->req_label,
							 isc_arg_gds, code, 0);
				}
				
			ERR_post(isc_integ_fail, isc_arg_number, trigger->req_label,
					 isc_arg_gds, isc_random, isc_arg_string, msg, 0);
			}
		else
			ERR_post(isc_integ_fail, isc_arg_number, trigger->req_label, 0);
		}
	else
		ERR_punt();
}


static void validate(thread_db* tdbb, JRD_NOD list)
{
/**************************************
 *
 *	v a l i d a t e
 *
 **************************************
 *
 * Functional description
 *	Execute a list of validation expressions.
 *
 **************************************/

	//SET_TDBB(tdbb);
	//BLKCHK(list, type_nod);
	JRD_NOD *ptr1, *ptr2;

	for (ptr1 = list->nod_arg, ptr2 = ptr1 + list->nod_count; ptr1 < ptr2; ptr1++)
		{
		if (!EVL_boolean(tdbb, (*ptr1)->nod_arg[e_val_boolean]))
			{
			/* Validation error -- report result */

			//JRD_NOD			node;
			//VEC			vector;
			//JRD_REL			relation;
			//Request*			request;
			//JRD_FLD			field;
			const char*	value;
			TEXT		temp[128];
			const TEXT*	name;
			//USHORT stream, id;

			JRD_NOD node = (*ptr1)->nod_arg[e_val_value];
			Request *request = tdbb->tdbb_request;
			int length = MOV_make_string(EVL_expr(tdbb, node),
									 ttype_dynamic,
									 &value,
									 reinterpret_cast<VARY*>(temp),
									 sizeof(temp));

			if (request->req_flags & req_null || request->req_flags & req_clone_data_from_default_clause)
				value = "*** null ***";
			else if (!length)
				value = "";
			else
				value = ERR_string(value, length);

			if (node->nod_type == nod_field)
				{
				int stream = (USHORT)(long) node->nod_arg[e_fld_stream];
				int id = (USHORT)(long) node->nod_arg[e_fld_id];
				Relation *relation = request->req_rpb[stream].rpb_relation;
				Field *field = relation->findField(id);
				
				if (field)
					name = field->fld_name;
				}

			if (!name)
				name = "*** unknown ***";

			ERR_post(isc_not_valid, isc_arg_string, name,
					 isc_arg_string, value, 0);
			}
		}
}


/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		req.h
 *	DESCRIPTION:	Request block definitions
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
 * 2002.09.28 Dmitry Yemanov: Reworked internal_info stuff, enhanced
 *                            exception handling in SPs/triggers,
 *                            implemented ROWS_AFFECTED system variable
 */

#include <memory.h>
#include "firebird.h"
#include "common.h"
#include "../common/classes/alloc.h"
#include "Request.h"
#include "Database.h"
#include "jrd.h"
#include "req.h"
#include "iberror.h"
#include "err_proto.h"
#include "inf_proto.h"
#include "thd_proto.h"
#include "cmp_proto.h"
#include "../jrd/tra.h"
#include "../jrd/blb.h"
#include "../jrd/val.h"
#include "../jrd/blb_proto.h"
#include "ExecStatement.h"

Request::Request(JrdMemoryPool* pool, int rpbCount, int impureSize) : req_invariants(pool), req_fors(pool)
{
	init();
	req_rpb = new record_param [rpbCount];
	memset (req_rpb, 0, sizeof (record_param) * rpbCount);
	req_impure_size = impureSize;
	req_impure = new UCHAR [req_impure_size];
	memset (req_impure, 0, req_impure_size);
	req_last_xcp = new StatusXcp;
}


Request::Request(Request* request) : req_invariants(request->req_pool)
{
	init();
	req_attachment = request->req_attachment;
	req_count = request->req_count;
	req_pool = request->req_pool;
	req_impure_size = request->req_impure_size;
	req_top_node = request->req_top_node;
	req_trg_name = request->req_trg_name;
	req_flags = request->req_flags & REQ_FLAGS_CLONE_MASK;
	//req_last_xcp = request->req_last_xcp;
	req_invariants.join(request->req_invariants);
	req_last_xcp = new StatusXcp;
	req_rpb = new record_param [req_count];
	memset (req_rpb, 0, sizeof (record_param) * req_count);
	req_impure = new UCHAR [req_impure_size];
	memset (req_impure, 0, req_impure_size);
	execStatements = NULL;
	
	for (record_param* rpb1 = req_rpb, *end = rpb1 + req_count, *rpb2 = request->req_rpb; rpb1 < end; rpb1++, rpb2++) 
		{
		if (rpb2->rpb_stream_flags & RPB_s_update)
			rpb1->rpb_stream_flags |= RPB_s_update;
		rpb1->rpb_relation = rpb2->rpb_relation;
		}
}

Request::~Request(void)
{
	delete req_last_xcp;
	delete [] req_rpb;
	delete [] req_impure;
	reset();

	for (RecordSource *rsb; rsb = rsbs;)
		{
		rsbs = rsb->nextInRequest;
		delete rsb;
		}
}


Request* Request::getInstantiatedRequest(int instantiation)
{
	Request *request = findInstantiatedRequest(instantiation);
		
	if (!request)
		ERR_post(isc_req_sync, 0);

	return request;
}

Request* Request::findInstantiatedRequest(int instantiation)
{
	if (instantiation == 0)
		return this;

	const vec* vector = req_sub_requests;
		
	if (!vector || instantiation >= vector->count())
		return NULL;
		
	return (Request*)(*vector)[instantiation];
}

int Request::getRequestInfo(thread_db* threadData, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer)
{
	return INF_request_info(threadData, this, items, itemsLength,buffer,bufferLength);
}

void Request::release(void)
{
	thread_db* tdbb = GET_THREAD_DATA;
	release(tdbb);
}

void Request::release(thread_db* tdbb)
{
	CMP_release(tdbb, this);
}

void Request::unwind(void)
{
	//DBB dbb = req_attachment->att_database;
	//thread_db *tdbb = req_tdbb;
	reset();
	
	if (req_flags & req_active) 
		{
		if (req_fors.getCount())
			{
			JrdMemoryPool *old_pool = req_tdbb->tdbb_default;
			req_tdbb->tdbb_default = req_pool;
			Request *old_request = req_tdbb->tdbb_request;
			req_tdbb->tdbb_request = this;
			Transaction* old_transaction = req_tdbb->tdbb_transaction;
			req_tdbb->tdbb_transaction = req_transaction;
			RecordSource** ptr = req_fors.begin();
			
			for (const RecordSource* const* const end = req_fors.end(); ptr < end; ptr++)
				if (*ptr)
					//RSE_close(tdbb, *ptr);
					(*ptr)->close(this);

			req_tdbb->tdbb_default = old_pool;
			req_tdbb->tdbb_request = old_request;
			req_tdbb->tdbb_transaction = old_transaction;
			}
			
		releaseBlobs();
		}

	if (req_proc_sav_point && (req_flags & req_proc_fetch))
		releaseProcedureSavePoints();

#ifdef SHARED_CACHE
	if (req_attachment)
		{
		Sync sync(&req_attachment->att_database->syncClone, "Request::unwind");
		sync.lock(Exclusive);
		req_flags &= ~(req_active | req_proc_fetch | req_reserved);
		req_flags |= req_abort | req_stall;
		req_timestamp = 0;
		req_tdbb = NULL;
		
		return;
		}
#endif

	req_flags &= ~(req_active | req_proc_fetch | req_reserved);
	req_flags |= req_abort | req_stall;
	req_timestamp = 0;
	req_tdbb = NULL;
}

void Request::releaseBlobs(void)
{
	if (req_transaction) 
		{
#ifdef SHARED_CACHE
		Sync sync (&req_transaction->syncObject, "Request::releaseBlobs");
		sync.lock (Exclusive);
#endif

		/* Release blobs assigned by this request */

		for (blb** blob = &req_transaction->tra_blobs; *blob;) 
			if ((*blob)->blb_request == this)
				BLB_cancel(req_tdbb, *blob);
			else
				blob = &(*blob)->blb_next;

		/* Release arrays assigned by this request */

		for (ArrayField** array = &req_transaction->tra_arrays; *array;) 
			if ((*array)->arr_request == this)
				BLB_release_array(*array);
			else
				array = &(*array)->arr_next;
		}
}

void Request::releaseProcedureSavePoints(void)
{
	Savepoint* sav_point = req_proc_sav_point;

	if (req_transaction) 
		while (sav_point) 
			{
			Savepoint* temp_sav_point = sav_point->sav_next;
			delete sav_point;
			sav_point = temp_sav_point;
			}
		
	req_proc_sav_point = NULL;
}

void Request::setThread(thread_db* tdbb)
{
	req_tdbb = tdbb;
	req_attachment = tdbb->tdbb_attachment;
}

ExecStatement* Request::getExecStatement(void)
{
	ExecStatement *exec = new ExecStatement(this);
	exec->next = execStatements;
	execStatements = exec;
	
	return exec;
}

void Request::init(void)
{
	req_attachment = NULL;		// database attachment
	req_count = 0;			// number of streams
	req_incarnation = 0;	// incarnation number
	req_impure_size = 0;	// size of impure area
	req_pool = NULL;
	req_sub_requests = NULL;	// vector of sub-requests
	req_transaction = NULL;
	req_request = NULL;		// next request in dbb
	req_caller = NULL;			// Caller of this request
	req_access = NULL;			// Access items to be checked
	req_resources = NULL;		// Resources (relations and indices)
	req_message = NULL;		// Current message for send/receive
	
#ifdef SCROLLABLE_CURSORS
	req_async_message = NULL;	// Asynchronous message (used in scrolling)
#endif

	req_refresh_ranges = NULL;	// Vector of refresh_ranges 
	req_procedure = NULL;		// procedure, if any 
	req_length = 0;			// message length for send/receive 
	req_nmsgs = 0;			// number of message types 
	req_mmsg = 0;			// highest message type 
	req_msend = 0;			// longest send message 
	req_mreceive = 0;		// longest receive message 

	req_records_selected = 0;	/* count of records selected by request (meeting selection criteria) */
	req_records_inserted = 0;	/* count of records inserted by request */
	req_records_updated = 0;	/* count of records updated by request */
	req_records_deleted = 0;	/* count of records deleted by request */

	req_records_affected = 0;	/* count of records affected by the last statement */

	req_view_flags = 0;			/* special flags for virtual ops on views */
	req_top_view_store = NULL;		/* the top view in store(), if any */
	req_top_view_modify = NULL;	/* the top view in modify(), if any */
	req_top_view_erase = NULL;		/* the top view in erase(), if any */

	req_top_node = NULL;			/* top of execution tree */
	req_next = NULL;				/* next node for execution */
	req_cursors = NULL;			/* Vector of named cursors, if any */
	req_label = 0;				/* label for leave */
	req_flags = 0;				/* misc request flags */
	req_proc_sav_point = NULL;		/* procedure savepoint list */
	req_timestamp = 0;			/* Start time of request */
    req_last_xcp = NULL;			/* last known exception */
	req_rpb = NULL;				/* record parameter blocks */
	req_impure = NULL;
	req_tdbb = NULL;
	rsbs = NULL;
	execStatements = NULL;
}

void Request::reset(void)
{
	for (ExecStatement *exec; exec = execStatements;)
		{
		execStatements = exec->next;
		delete exec;
		}
}

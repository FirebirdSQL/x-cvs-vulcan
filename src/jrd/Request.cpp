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
#include "jrd.h"
#include "req.h"
#include "iberror.h"
#include "err_proto.h"
#include "inf_proto.h"
#include "thd_proto.h"
#include "cmp_proto.h"

Request::Request(JrdMemoryPool* pool, int rpbCount, int impureSize) : req_invariants(pool), req_fors(pool)
{
	req_rpb = new record_param [rpbCount];
	memset (req_rpb, 0, sizeof (record_param) * rpbCount);
	req_impure_size = impureSize;
	req_impure = new UCHAR [req_impure_size];
	memset (req_impure, 0, req_impure_size);
	req_last_xcp = new StatusXcp;
	rsbs = NULL;
}


Request::Request(Request* request) : req_invariants(request->req_pool)
{
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

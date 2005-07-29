/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		RsbAggregate.cpp
 *	DESCRIPTION:	Run time record fetching
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
 * Refactored July 28, 2005 by James A. Starkey
 */
 
#include "firebird.h"
#include "RsbAggregate.h"
#include "jrd.h"
#include "rse.h"
#include "Request.h"
#include "CompilerScratch.h"
#include "req.h"
#include "../jrd/vio_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/cmp_proto.h"

RsbAggregate::RsbAggregate(CompilerScratch *csb, RecordSource* prior_rsb, jrd_nod *mapNode) : RecordSource(csb, rsb_aggregate)
{
	rsb_next = prior_rsb;
	map = mapNode;
	rsb_impure = CMP_impure(csb, sizeof(struct irsb));
}

RsbAggregate::~RsbAggregate(void)
{
}

void RsbAggregate::open(Request* request)
{
	thread_db *tdbb = request->req_tdbb;
	record_param* rpb = &request->req_rpb[rsb_stream];
	IRSB impure = (IRSB) IMPURE (request, rsb_impure);
	impure->irsb_count = 3;
	VIO_record(request->req_tdbb, rpb, rsb_format, tdbb->tdbb_default);
}

bool RsbAggregate::get(Request* request, RSE_GET_MODE mode)
{
	IRSB impure = (IRSB) IMPURE (request, rsb_impure);

	if ( (impure->irsb_count = EVL_group(request->req_tdbb, rsb_next, map, impure->irsb_count)) ) 
		return true;
	
	return false;
}

void RsbAggregate::close(Request* request)
{
	rsb_next->close(request);
}


void RsbAggregate::findRsbs(StreamStack* stream_list, RsbStack* rsb_list)
{
	stream_list->push(rsb_stream);
		
	if (rsb_list) 
		rsb_list->push(this);
}

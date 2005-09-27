/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		RsbFirst.cpp
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
 * Refactored July 29, 2005 by James A. Starkey
 */
 
#include "fbdev.h"
#include "ibase.h"
#include "RsbFirst.h"
#include "jrd.h"
#include "rse.h"
#include "Request.h"
#include "CompilerScratch.h"
#include "req.h"
#include "ExecutionPathInfoGen.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/mov_proto.h"

RsbFirst::RsbFirst(CompilerScratch *csb, RecordSource* prior_rsb, jrd_nod* node) : RecordSource(csb, rsb_first)
{
	rsb_next = prior_rsb;
	valueNode = node;
	rsb_impure = CMP_impure(csb, sizeof(struct irsb_first_n));
}

RsbFirst::~RsbFirst(void)
{
}

void RsbFirst::open(Request* request)
{
	thread_db *tdbb = request->req_tdbb;
	IRSB_FIRST impure = (IRSB_FIRST) IMPURE (request, rsb_impure);
	impure->irsb_count = MOV_get_int64(EVL_expr(tdbb, valueNode), 0);

	if (impure->irsb_count < 0)
		ERR_post(isc_bad_limit_param, 0);

	rsb_next->open(request);
}

bool RsbFirst::get(Request* request, RSE_GET_MODE mode)
{
	thread_db *tdbb = request->req_tdbb;
	IRSB_FIRST impure = (IRSB_FIRST) IMPURE (request, rsb_impure);

	switch(mode) 
		{
		case RSE_get_forward:
			if (impure->irsb_count <= 0)
				return FALSE;
				
			impure->irsb_count--;
			
			if (!rsb_next->get(request, mode))
				return FALSE;
				
			break;

		case RSE_get_current:
			if (impure->irsb_count <= 0)
				return FALSE;
				
			if (!rsb_next->get(request, mode))
				return FALSE;
				
			break;

		case RSE_get_backward:
			impure->irsb_count++;
			
			if (!rsb_next->get(request, mode))
				return FALSE;
			break;
		}

	return true;
}

bool RsbFirst::getExecutionPathInfo(Request* request, ExecutionPathInfoGen* infoGen)
{
	if (!infoGen->putBegin())
		return false;

	if (!infoGen->putType(isc_info_rsb_first))
		return false;

	if (rsb_next)
		if (!rsb_next->getExecutionPathInfo(request, infoGen))
			return false;

	return infoGen->putEnd();
}

void RsbFirst::close(Request* request)
{
	rsb_next->close(request);
}

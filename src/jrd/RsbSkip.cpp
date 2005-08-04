/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		RsbSkip.cpp
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
 
#include "firebird.h"
#include "ibase.h"
#include "RsbSkip.h"
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

RsbSkip::RsbSkip(CompilerScratch *csb, RecordSource* prior_rsb, jrd_nod* node) : RecordSource(csb, rsb_skip)
{
	rsb_next = prior_rsb;
	valueNode = node;
	rsb_impure = CMP_impure(csb, sizeof(struct irsb_skip_n));
}

RsbSkip::~RsbSkip(void)
{
}

void RsbSkip::open(Request* request)
{
	thread_db *tdbb = request->req_tdbb;
	IRSB_SKIP impure = (IRSB_SKIP) IMPURE (request, rsb_impure);
	impure->irsb_count = MOV_get_int64(EVL_expr(tdbb, valueNode), 0);

	if (impure->irsb_count < 0)
		ERR_post(isc_bad_limit_param, 0);

	rsb_next->open(request);
}

bool RsbSkip::get(Request* request, RSE_GET_MODE mode)
{
	thread_db *tdbb = request->req_tdbb;
	IRSB_SKIP impure = (IRSB_SKIP) IMPURE (request, rsb_impure);

	switch(mode) 
		{
		case RSE_get_backward:
			if (impure->irsb_count > 0)
				return FALSE;
				
			if (impure->irsb_count == 0) 
				{
				impure->irsb_count++;
				rsb_next->get(request, mode);
				return FALSE;
				}
				
			impure->irsb_count++;
			
			if (!rsb_next->get(request, mode))
				return FALSE;
				
			break;

		case RSE_get_forward:
			while(impure->irsb_count > 1) 
				{
				impure->irsb_count--;
				
				if (!rsb_next->get(request, mode))
					return FALSE;
				}
				
			impure->irsb_count--;
			
			if (!rsb_next->get(request, mode))
				return FALSE;
			break;

		case RSE_get_current:
			if (impure->irsb_count >= 1)
				return FALSE;
			else if (!rsb_next->get(request, mode))
				return FALSE;
		}

	return true;
}

bool RsbSkip::getExecutionPathInfo(Request* request, ExecutionPathInfoGen* infoGen)
{
	if (!infoGen->putBegin())
		return false;

	if (!infoGen->putType(isc_info_rsb_skip))
		return false;

	if (rsb_next)
		if (!rsb_next->getExecutionPathInfo(request, infoGen))
			return false;

	return infoGen->putEnd();
}

void RsbSkip::close(Request* request)
{
	rsb_next->close(request);
}

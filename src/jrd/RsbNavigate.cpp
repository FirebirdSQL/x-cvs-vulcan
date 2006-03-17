/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		RsbIndexed.cpp
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
 * Refactored July 25, 2005 by James A. Starkey
 */
 
#include "fbdev.h"
#include "ibase.h"
#include "RsbNavigate.h"
#include "jrd.h"
#include "rse.h"
#include "Request.h"
#include "CompilerScratch.h"
#include "Relation.h"
#include "req.h"
#include "btr.h"
#include "ExecutionPathInfoGen.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/nav_proto.h"


RsbNavigate::RsbNavigate(CompilerScratch *csb, int stream, Relation *relation, str *alias, jrd_nod *node, int key_length)
		: RsbIndexed(csb, rsb_navigate, stream, relation, alias, NULL)
{
	retrievalInversion = node;
	keyLength = key_length;
	rsb_impure = CMP_impure(csb, computeImpureSize(key_length));
}

RsbNavigate::~RsbNavigate(void)
{
}

void RsbNavigate::open(Request* request)
{
	IRSB impure = (IRSB) IMPURE (request, rsb_impure);
	impure->irsb_flags |= irsb_first;
#ifdef SCROLLABLE_CURSORS
	impure->irsb_flags |= irsb_bof;
	impure->irsb_flags &= ~irsb_eof;
#endif

	reserveRelation(request);
}

bool RsbNavigate::get(Request* request, RSE_GET_MODE mode)
{
	IRSB_NAV impure = (IRSB_NAV) IMPURE (request, rsb_impure);
	thread_db *tdbb = request->req_tdbb;
	record_param* rpb = &request->req_rpb[rsb_stream];

#ifdef SCROLLABLE_CURSORS
		if (impure->irsb_flags & irsb_bof)
			rpb->rpb_number = -1;
#endif // SCROLLABLE_CURSORS

	return NAV_get_record(tdbb, this, impure, rpb, mode);
}

bool RsbNavigate::getExecutionPathInfo(Request* request, ExecutionPathInfoGen* infoGen)
{
	if (!infoGen->putBegin())
		return false;

	if (!infoGen->putRelation(rsb_relation, rsb_alias))
		return false;

	if (!infoGen->putType(isc_info_rsb_navigate))
		return false;

	if (!RsbIndexed::getExecutionPathInfo(request, infoGen, retrievalInversion))
		return false;

	// dimitr:	here we report indicies used to limit
	//			the navigational-based retrieval	
	if (inversion) 
		{
		if (!infoGen->putType(isc_info_rsb_indexed))
			return false;

		if (!RsbIndexed::getExecutionPathInfo(request, infoGen, inversion))
			return false;
		}

	if (rsb_next)
		if (!rsb_next->getExecutionPathInfo(request, infoGen))
			return false;

	return infoGen->putEnd();
}

void RsbNavigate::close(Request* request)
{
}

int RsbNavigate::computeImpureSize(int key_length)
{
	int size = sizeof(struct irsb_nav);
	
#ifdef SCROLLABLE_CURSORS
	/* allocate extra impure area to hold the current key, 
	   plus an upper and lower bound key value, for a total 
	   of three times the key length for the index */
	   
	size +=  3 * key_length;
#else

	size += 2 * key_length;
	
#endif

	size = FB_ALIGN(size, ALIGNMENT);
	indexOffset = size;
	size += sizeof(index_desc);
	
	return size;
}

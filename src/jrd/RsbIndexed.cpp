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
 * Refactored July 22, 2005 by James A. Starkey
 */
 
#include "firebird.h"
#include "RsbIndexed.h"
#include "jrd.h"
#include "rse.h"
#include "Request.h"
#include "CompilerScratch.h"
#include "Relation.h"
#include "req.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/vio_proto.h"
#include "../jrd/evl_proto.h"


RsbIndexed::RsbIndexed(CompilerScratch *csb, int stream, Relation *relation, str *alias, jrd_nod *node)
		: RsbSequential(csb, rsb_indexed, stream, relation, alias)
{
	rsb_impure = CMP_impure(csb, sizeof(struct irsb_index));
	inversion = node;
}

RsbIndexed::RsbIndexed(CompilerScratch* csb, RSB_T type, int stream, Relation* relation, str* alias, jrd_nod *node) 
		: RsbSequential(csb, type, stream, relation, alias)
{
	inversion = node;
}

RsbIndexed::~RsbIndexed(void)
{
}

void RsbIndexed::open(Request* request)
{
	thread_db *tdbb = request->req_tdbb;
	//DBB dbb = tdbb->tdbb_database;
	IRSB_INDEX impure = (IRSB_INDEX) IMPURE (request, rsb_impure);
	impure->irsb_bitmap = EVL_bitmap(tdbb, inversion);
	impure->irsb_prefetch_number = -1;
	reserveRelation(request);
}

bool RsbIndexed::get(Request* request, RSE_GET_MODE mode)
{
	if (request->req_flags & req_abort)
		return FALSE;

	if (!request->req_transaction)
		return FALSE;

	IRSB_INDEX impure = (IRSB_INDEX) IMPURE (request, rsb_impure);
	
	if (impure->irsb_flags & irsb_singular_processed)
		return FALSE;

	record_param* rpb = request->req_rpb + rsb_stream;
	thread_db *tdbb = request->req_tdbb;
	RecordBitmap **pbitmap = impure->irsb_bitmap;
	RecordBitmap *bitmap;
	
	if (!pbitmap || !(bitmap = *pbitmap))
		return false;

	bool result = false;

	// NS: Original code was the following:
	// while (SBM_next(*bitmap, &rpb->rpb_number, mode))
	// We assume mode = RSE_get_forward because we do not support
	// scrollable cursors at the moment.
	
	if (rpb->rpb_number.isBof() ? bitmap->getFirst() : bitmap->getNext()) 
		do {
			rpb->rpb_number.setValue(bitmap->current());
			
#ifdef SUPERSERVER_V2
		/* Prefetch next set of data pages from bitmap. */

		if (rpb->rpb_number > impure->irsb_prefetch_number && (mode == RSE_get_forward))
			impure->irsb_prefetch_number =
				DPM_prefetch_bitmap(tdbb, rpb->rpb_relation, *bitmap, rpb->rpb_number);

#endif // SUPERSERVER_V2

		if (VIO_get(tdbb, rpb, request->req_transaction, request->req_pool))
			{
			result = true;
			break;
			}
		} while (bitmap->getNext());

	return result;
}

bool RsbIndexed::getExecutionPathInfo(Request* request, ExecutionPathInfoGen* infoGen)
{
	return false;
}

void RsbIndexed::close(Request* request)
{
	IRSB_INDEX impure = (IRSB_INDEX) IMPURE (request, rsb_impure);
	
	if (!(impure->irsb_flags & irsb_open))
		return;

	impure->irsb_flags &= ~irsb_open;
}

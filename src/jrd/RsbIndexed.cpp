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
 
#include "fbdev.h"
#include "ibase.h"
#include "RsbIndexed.h"
#include "dsc.h"
#include "btr.h"
#include "jrd.h"
#include "rse.h"
#include "Request.h"
#include "CompilerScratch.h"
#include "Relation.h"
#include "req.h"
#include "ExecutionPathInfoGen.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/vio_proto.h"


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
	if (!infoGen->putBegin())
		return false;

	if (!infoGen->putRelation(rsb_relation, rsb_alias))
		return false;

	if (!infoGen->putType(isc_info_rsb_indexed))
		return false;

	if (!getExecutionPathInfo(request, infoGen, inversion))
		return false;

	if (rsb_next)
		if (!rsb_next->getExecutionPathInfo(request, infoGen))
			return false;

	return infoGen->putEnd();
}

bool RsbIndexed::getExecutionPathInfo(Request* request, ExecutionPathInfoGen* infoGen, const jrd_nod* node)
{
	switch (node->nod_type)
		{
		case nod_bit_and:
			if (!infoGen->putByte(isc_info_rsb_and))
				return false;
			break;
		case nod_bit_or:
		case nod_bit_in:
			if (!infoGen->putByte(isc_info_rsb_or))
				return false;
			break;
		case nod_bit_dbkey:
			if (!infoGen->putByte(isc_info_rsb_dbkey))
				return false;
			break;
		case nod_index:
			if (!infoGen->putByte(isc_info_rsb_index))
				return false;
			break;
		}

	// dump sub-nodes or the actual index info
	if ((node->nod_type == nod_bit_and) ||
		(node->nod_type == nod_bit_or) ||
		(node->nod_type == nod_bit_in))
		{
		if (!getExecutionPathInfo(request, infoGen, node->nod_arg[0]))
			return false;
		
		if (!getExecutionPathInfo(request, infoGen, node->nod_arg[1]))
			return false;
		}
	else if (node->nod_type == nod_index) 
		{
		SqlIdentifier index_name;
		IndexRetrieval* retrieval = (IndexRetrieval*) node->nod_arg[e_idx_retrieval];
		MET_lookup_index(infoGen->threadData, index_name, retrieval->irb_relation->rel_name,
						 (USHORT) (retrieval->irb_index + 1));
		int length = strlen(index_name);
		if (!infoGen->putString(index_name, length))
			return false;
		}

	return true;
}

void RsbIndexed::close(Request* request)
{
	IRSB_INDEX impure = (IRSB_INDEX) IMPURE (request, rsb_impure);
	
	if (!(impure->irsb_flags & irsb_open))
		return;

	impure->irsb_flags &= ~irsb_open;
}

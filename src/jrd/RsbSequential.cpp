/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		RsbSequential.cpp
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
#include "RsbSequential.h"
#include "jrd.h"
#include "rse.h"
#include "Request.h"
#include "CompilerScratch.h"
#include "PageCache.h"
#include "Relation.h"
#include "RsbSort.h"
#include "req.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dpm_proto.h"
#include "../jrd/rlck_proto.h"
#include "../jrd/vio_proto.h"

RsbSequential::RsbSequential(CompilerScratch *csb, int stream, Relation *relation, str *alias) : RecordSource(csb, rsb_sequential)
{
	rsb_impure = CMP_impure(csb, sizeof(struct irsb));
	rsb_relation = relation;
	rsb_alias = alias;
	rsb_stream = stream;
}

RsbSequential::RsbSequential(CompilerScratch* csb, RSB_T type, int stream, Relation* relation, str* alias) : RecordSource(csb, type)
{
	rsb_relation = relation;
	rsb_alias = alias;
	rsb_stream = stream;
}

RsbSequential::~RsbSequential(void)
{
}

void RsbSequential::open(Request* request)
{
	thread_db *tdbb = request->req_tdbb;
	DBB dbb = tdbb->tdbb_database;
	//SINT64 first_records = -1, skip_records = 0;
	IRSB impure = (IRSB) IMPURE (request, rsb_impure);
	impure->irsb_flags |= irsb_first | irsb_open;
	impure->irsb_flags &= ~(irsb_singular_processed | irsb_checking_singular);
	record_param* rpb = &request->req_rpb[rsb_stream];
	rpb->rpb_window.win_flags = 0;

	/* Unless this is the only attachment, limit the cache flushing
	   effect of large sequential scans on the page working sets of
	   other attachments. */

	Attachment* attachment = request->req_attachment;
	
	if (attachment &&  (attachment != dbb->dbb_attachments || attachment->att_next))
		{
		/* If the relation has more data pages than the number of
		   pages in the buffer cache then mark the input window
		   block as a large scan so that a data page is released
		   to the LRU tail after its last record is fetched.

		   A database backup treats everything as a large scan
		   because the cumulative effect of scanning all relations
		   is equal to that of a single large relation. */

		if (attachment->att_flags & ATT_gbak_attachment ||
			DPM_data_pages(tdbb,  rpb->rpb_relation) > (SLONG) dbb->pageCache->bcb_count)
			{
			rpb->rpb_window.win_flags = WIN_large_scan;
			rpb->rpb_org_scans = rpb->rpb_relation->rel_scan_count++;
			}
		}

	reserveRelation(request);
}

bool RsbSequential::get(Request* request, RSE_GET_MODE mode)
{
	if (request->req_flags & req_abort)
		return FALSE;

	if (!request->req_transaction)
		return FALSE;

	IRSB impure = (IRSB) IMPURE (request, rsb_impure);
	
	if (impure->irsb_flags & irsb_singular_processed)
		return FALSE;

	record_param* rpb = request->req_rpb + rsb_stream;
	thread_db *tdbb = request->req_tdbb;
	//DBB dbb = tdbb->tdbb_database;
	//SINT64 first_records = -1, skip_records = 0;

	if (impure->irsb_flags & irsb_bof)
		rpb->rpb_number.setValue(BOF_NUMBER);

	if (!VIO_next_record(tdbb, rpb, this, request->req_transaction, request->req_pool,
							(mode == RSE_get_backward) ? TRUE : FALSE, FALSE))
			return FALSE;
	
	return true;
}

void RsbSequential::close(Request* request)
{
	IRSB_SORT impure = (IRSB_SORT) IMPURE (request, rsb_impure);
	
	if (!(impure->irsb_flags & irsb_open))
		return;

	impure->irsb_flags &= ~irsb_open;
	record_param* rpb = &request->req_rpb[rsb_stream];
	
	if (rpb->rpb_window.win_flags & WIN_large_scan && rpb->rpb_relation->rel_scan_count)
		--rpb->rpb_relation->rel_scan_count;
}

void RsbSequential::reserveRelation(Request* request)
{
	thread_db *tdbb = request->req_tdbb;
	record_param* rpb = &request->req_rpb[rsb_stream];

	RLCK_reserve_relation(tdbb, request->req_transaction, rpb->rpb_relation, FALSE, TRUE);
	rpb->rpb_number.setValue(BOF_NUMBER);
}


void RsbSequential::findRsbs(StreamStack* stream_list, RsbStack* rsb_list)
{
	stream_list->push(rsb_stream);
}

/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		RsbSort.cpp
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
 * Refactored July 5, 2005 by James A. Starkey
 */
 
#include "firebird.h"
#include "ibase.h"
#include "RsbSort.h"
#include "jrd.h"
#include "rse.h"
#include "Request.h"
#include "Procedure.h"
#include "Relation.h"
#include "req.h"
#include "sort.h"
#include "ExecutionPathInfoGen.h"
#include "../jrd/mov_proto.h"
#include "../jrd/sort_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/vio_proto.h"
#include "../jrd/intl.h"
#include "../jrd/intl_proto.h"
#include "../jrd/btr.h"					// this really doesn't belong here!

RsbSort::RsbSort(CompilerScratch *csb, RecordSource *source, SortMap *sortMap) : RecordSource(csb, rsb_sort)
{
	rsb_next = source;
	map = sortMap;
}

RsbSort::~RsbSort(void)
{
}

void RsbSort::open(Request* request)
{
	thread_db* tdbb = request->req_tdbb;
	IRSB_SORT impure = (IRSB_SORT) IMPURE (request, rsb_impure);
	
#ifdef SCROLLABLE_CURSORS
	impure->irsb_flags |= irsb_bof;
	impure->irsb_flags &= ~irsb_eof;
#endif
	// dimitr:	we can avoid reading and sorting the entire
	//			record set, if there's actually nothing to return
	
	/***
	if (!first_records) 
		{
		((IRSB_SORT) impure)->irsb_sort_handle = NULL;
		return;
		}
	***/
	
	//UINT64 max_records = (first_records < 0) ? 0 : (UINT64) first_records + skip_records);
	UINT64 max_records = 0;

	UCHAR *data, flag;
	DSC *from, to, temp;
	record_param* rpb;
	smb_repeat *item, *end_item;

	rsb_next->open(request);
	ULONG records = 0;

	// Get rid of the old sort areas if this request has been used already

	if (impure->irsb_sort_handle &&
		impure->irsb_sort_handle->scb_impure == impure)
		SORT_fini(impure->irsb_sort_handle, request->req_attachment);

	// Initialize for sort. If this is really a project operation,
	// establish a callback routine to reject duplicate records.

	sort_context* handle = SORT_init(tdbb,
						   map->smb_length,
						   map->smb_keys,
						   map->smb_keys,
						   map->smb_key_desc,
         				   ((map->smb_flags & SMB_project) ? reject : NULL), 0,
						   tdbb->tdbb_attachment, max_records);

	if (!(impure->irsb_sort_handle = handle))
		ERR_punt();

	// Mark scb with the impure area pointer

	handle->scb_impure = impure;

	// Pump the input stream dry while pushing records into sort. For
	// each record, map all fields into the sort record. The reverse
	// mapping is done in get_sort().

	//while (get_record(request, tdbb, rsb->rsb_next, NULL, RSE_get_forward)) 
	while (rsb_next->get(request, RSE_get_forward)) 
		{
		records++;

		// "Put" a record to sort. Actually, get the address of a place
		// to build a record.

		SORT_put(tdbb, impure->irsb_sort_handle, (ULONG **) &data);

		// Zero out the sort key. This solve a multitude of problems.

		MOVE_CLEAR(data, (SLONG) map->smb_length);

		// Loop thru all field (keys and hangers on) involved in the sort.
		// Be careful to null field all unused bytes in the sort key.

		end_item = map->smb_rpt + map->smb_count;
		
		for (item = map->smb_rpt; item < end_item; item++) 
			{
			to = item->smb_desc;
			to.dsc_address = data + (long) to.dsc_address;
			flag = FALSE;
			
			if (item->smb_node) 
				{
				from = EVL_expr(tdbb, item->smb_node);
				if (request->req_flags & req_null)
					flag = TRUE;
				}
			else 
				{
				from = &temp;
				rpb = &request->req_rpb[item->smb_stream];
				if (item->smb_field_id < 0) 
					{
					if (item->smb_field_id == SMB_TRANS_ID)
						*(SLONG *) (to.dsc_address) = rpb->rpb_transaction;
					else
						*(SINT64 *) (to.dsc_address) = rpb->rpb_number.getValue();
					continue;
					}
				if (!EVL_field
					(rpb->rpb_relation, rpb->rpb_record, item->smb_field_id,
					 from)) flag = TRUE;
				}
			*(data + item->smb_flag_offset) = flag;
			
			if (!flag) 
				{
				// If moving a TEXT item into the KEY portion of the sort record,
				// then want to sort by language dependent order.

				if (IS_INTL_DATA(&item->smb_desc) &&
					(USHORT)(long) item->smb_desc.dsc_address < map->smb_key_length * sizeof(ULONG)) 
					INTL_string_to_key(tdbb, INTL_INDEX_TYPE(&item->smb_desc), from, &to, FALSE);
				else
					MOV_move(from, &to);
				}
			}
		}

	SORT_sort(tdbb, impure->irsb_sort_handle);

	// For the sake of prudence, set all record parameter blocks to contain
	// the most recent format. This is will guarentee that all fields mapped
	// back to records by get_sort() have homes in the target record.

	if (!records)
		return;

	SSHORT stream = -1;

	for (item = map->smb_rpt; item < end_item; item++) 
		{
		if (item->smb_node && item->smb_node->nod_type != nod_field)
			continue;
			
		if (item->smb_stream == stream)
			continue;
			
		stream = item->smb_stream;
		rpb = &request->req_rpb[stream];
		
		if (rpb->rpb_relation)
			VIO_record(tdbb, rpb, rpb->rpb_relation->getCurrentFormat(tdbb), tdbb->tdbb_default);
		}
}

bool RsbSort::get(Request* request,RSE_GET_MODE mode)
{
	thread_db* tdbb = request->req_tdbb;
	IRSB_SORT impure = (IRSB_SORT) IMPURE (request, rsb_impure);
	UCHAR *data;

#ifdef SCROLLABLE_CURSORS
	/* any attempt to get a record takes us off of bof or eof */

	impure->irsb_flags &= ~(irsb_bof | irsb_eof);
	SORT_get(tdbb-, impure->irsb_sort_handle,(ULONG **) & data, mode);
#else
	SORT_get(tdbb, impure->irsb_sort_handle, (ULONG **) & data);
#endif

	if (!data)
		{
#ifdef SCROLLABLE_CURSORS

		if (mode == RSE_get_forward)
			impure->irsb_flags |= irsb_eof;
		else
			impure->irsb_flags |= irsb_bof;
#endif
		return FALSE;
		}

	mapSortData(request, map, data);

#ifdef SCROLLABLE_CURSORS
	/* fix up the sort data in case we need to retrieve it again */

	unget_sort(tdbb, this, data);
#endif

	return true;
}

bool RsbSort::getExecutionPathInfo(Request* request, ExecutionPathInfoGen* infoGen)
{
	if (!infoGen->putBegin())
		return false;

	if (!infoGen->putType(isc_info_rsb_sort))
		return false;

	if (rsb_next)
		if (!rsb_next->getExecutionPathInfo(request, infoGen))
			return false;

	return infoGen->putEnd();
}

void RsbSort::close(Request* request)
{
	IRSB_SORT impure = (IRSB_SORT) IMPURE (request, rsb_impure);
	SORT_fini(impure->irsb_sort_handle, request->req_attachment);
	impure->irsb_sort_handle = NULL;
	rsb_next->close(request);
}

bool RsbSort::reject(const UCHAR* record_a, const UCHAR* record_b, void* user_arg)
{
	return true;
}

void RsbSort::pushRecords(Request* request)
{
	SSHORT i, streams[128];
	smb_repeat * item, *end_item;

	for (i = 0; i < request->req_count; i++)
		streams[i] = 0;
		
	end_item = map->smb_rpt + map->smb_count;
	
	for (item = map->smb_rpt; item < end_item; item++)
		streams[item->smb_stream] = 1;
		
	for (i = 0; i < request->req_count; i++)
		if (streams[i]) 
			{
			record_param *rpb = request->req_rpb + i;
			saveRecord(request, rpb);
			}
}

void RsbSort::popRecords(Request* request)
{
	SSHORT i, streams[128];
	smb_repeat * item, *end_item;

	for (i = 0; i < request->req_count; i++)
		streams[i] = 0;
		
	end_item = map->smb_rpt + map->smb_count;
	
	for (item = map->smb_rpt; item < end_item; item++)
		streams[item->smb_stream] = 1;
		
	for (i = 0; i < request->req_count; i++)
		if (streams[i]) 
			{
			record_param *rpb = request->req_rpb + i;
			restoreRecord(rpb);
			}
}

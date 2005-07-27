/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		RecordSource.cpp
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
 * $Id$
 *
 * 2001.07.28: John Bellardo: Implemented rse_skip and made rse_first work with
 *                              seekable streams.
 * 2002.02.22 Claudio Valderrama: Fix SF Bugs #225283, #518279, #514186 & #221925.
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */
 
#include <errno.h>
#include "firebird.h"
#include "RecordSource.h"
#include "Request.h"
#include "Relation.h"
#include "PageCache.h"
//#include "sort_mem.h"
#include "jrd.h"
#include "rse.h"
#include "req.h"
//#include "sort.h"
//#include "../jrd/btr.h"					// this really doesn't belong here!
#include "../jrd/intl.h"
//#include "../jrd/sort_proto.h"
#include "../jrd/ext_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/dpm_proto.h"
#include "../jrd/rlck_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/vio_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/nav_proto.h"
#include "RsbMerge.h"
#include "RsbSort.h"
#include "RsbLeftCross.h"
#include "CompilerScratch.h"

#if defined(WIN_NT)
#include <io.h> // close
#endif

#ifdef SMALL_FILE_NAMES
#define SCRATCH         "fb_m"
#else
#define SCRATCH         "fb_merge_"
#endif

/***
static void close_procedure(Request *request, thread_db* tdbb, RecordSource* rsb);
static void open_procedure(Request *request, thread_db* tdbb, RecordSource* rsb, IRSB_PROCEDURE impure);
static BOOLEAN get_procedure(Request *request,
							 thread_db*			tdbb,
							 RecordSource*		rsb,
							 IRSB_PROCEDURE		impure,
							 record_param*		rpb);
***/

//static void open_sort(Request *request, thread_db* tdbb, RecordSource* rsb, IRSB_SORT impure, UINT64 max_records);
//static UCHAR *get_sort(Request *request, thread_db* tdbb, RecordSource* rsb, RSE_GET_MODE mode);
//static void map_sort_data(JRD_REQ request, SortMap* map, UCHAR * data);
							 
static void close_merge(Request *request, thread_db* tdbb, RecordSource* rsb, IRSB_MRG impure);
static void open_merge(Request *request, thread_db* tdbb, RecordSource* rsb, IRSB_MRG impure);
static bool reject(const UCHAR* record_a, const UCHAR* record_b, void* user_arg);
static BOOLEAN fetch_record(Request *request, thread_db* tdbb, RecordSource* rsb, SSHORT n, RSE_GET_MODE mode);

static BOOLEAN get_record(Request *request, thread_db*	tdbb,
						  RecordSource*	rsb,
						  RecordSource*	parent_rsb,
						  RSE_GET_MODE	mode);

/***
static void proc_assignment(DSC * from_desc,
							DSC * flag_desc,
							UCHAR * msg,
							DSC * to_desc, SSHORT to_id, Record* record);
***/
							 
static BOOLEAN get_union(Request *request, thread_db* tdbb, RecordSource* rsb, IRSB impure);
//static BOOLEAN fetch_left(Request *request, thread_db* tdbb, RecordSource* rsb, IRSB impure, RSE_GET_MODE mode);
static void push_rpbs(thread_db* tdbb, JRD_REQ request, RecordSource* rsb);
static void pop_rpbs(JRD_REQ request, RecordSource* rsb);
//static void join_to_nulls(Request *request, thread_db* tdbb, RecordSource* rsb, StreamStack* stream);
static void restore_record(record_param * rpb);
static void save_record(thread_db* tdbb, record_param * rpb);

/***
static void write_merge_block(thread_db* tdbb, MergeFile* mfb, ULONG block);
static ULONG read_merge_block(thread_db* tdbb, MergeFile* mfb, ULONG block);
static BOOLEAN get_merge_fetch(Request *request, thread_db* tdbb, RecordSource* rsb, SSHORT stream, RSE_GET_MODE mode);
static UCHAR *get_merge_data(thread_db* tdbb, MergeFile* mfb, SLONG record);
static SLONG get_merge_record(Request *request, thread_db* tdbb,RecordSource* rsb, irsb_mrg::irsb_mrg_repeat * impure, RSE_GET_MODE mode);
static BOOLEAN get_merge_join(Request *request, thread_db* tdbb, RecordSource* rsb, IRSB_MRG impure, RSE_GET_MODE mode);
static SSHORT compare(Request *request, thread_db* tdbb, jrd_nod* node1, jrd_nod* node2);
static SSHORT compare_longs(const SLONG* p, const SLONG* q, USHORT count);
***/

RecordSource::RecordSource(CompilerScratch *compilerScratch)
{
	init();
	compilerScratch->addRsb(this);
}

RecordSource::RecordSource(CompilerScratch *compilerScratch, RSB_T type)
{
	init();
	rsb_type = type;
	compilerScratch->addRsb(this);
}

void RecordSource::init(void)
{
	rsb_left_inner_streams = NULL;
	rsb_left_streams = NULL;
	rsb_left_rsbs = NULL;
	rsb_next = NULL;
	rsb_flags = NULL;
	rsb_impure = NULL;
	rsb_relation = NULL;
	rsb_alias = NULL;
}

RecordSource::~RecordSource(void)
{
}

void RecordSource::open(Request* request)
{
	thread_db *tdbb = request->req_tdbb;
	SINT64 first_records = -1, skip_records = 0;
	IRSB_INDEX impure = (IRSB_INDEX) IMPURE (request, rsb_impure);
	impure->irsb_flags |= irsb_first | irsb_open;
	impure->irsb_flags &= ~(irsb_singular_processed | irsb_checking_singular);
	record_param* rpb = &request->req_rpb[rsb_stream];
	rpb->rpb_window.win_flags = 0;

	switch (rsb_type) 
		{
		case rsb_indexed:
			impure->irsb_bitmap = EVL_bitmap(tdbb, (JRD_NOD) rsb_arg[0]);
			impure->irsb_prefetch_number = -1;

#ifdef OBSOLETE
		case rsb_navigate:
		case rsb_sequential:
#ifdef SCROLLABLE_CURSORS
			if (rsb_type == rsb_navigate) 
				{
				impure->irsb_flags |= irsb_bof;
				impure->irsb_flags &= ~irsb_eof;
				}
#endif // SCROLLABLE_CURSORS
			if (rsb_type == rsb_sequential) 
				{
				DBB dbb = tdbb->tdbb_database;

				/* Unless this is the only attachment, limit the cache flushing
				   effect of large sequential scans on the page working sets of
				   other attachments. */

				Attachment* attachment = tdbb->tdbb_attachment;
				
				if (attachment && 
					(attachment != dbb->dbb_attachments || attachment->att_next))
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
						rpb->rpb_org_scans =
							rpb->rpb_relation->rel_scan_count++;
						}
					}
				}

			RLCK_reserve_relation(tdbb, request->req_transaction,
								rpb->rpb_relation, FALSE, TRUE);

			rpb->rpb_number.setValue(BOF_NUMBER);
			return;
#endif // OBSOLETE

		/***	
		case rsb_cross:
			return;
		***/
		
		/***
		case rsb_sort:
#ifdef SCROLLABLE_CURSORS
			impure->irsb_flags |= irsb_bof;
			impure->irsb_flags &= ~irsb_eof;
#endif // SCROLLABLE_CURSORS
			// dimitr:	we can avoid reading and sorting the entire
			//			record set, if there's actually nothing to return
			
			if (first_records) 
				{
				open_sort(request, tdbb, this, (IRSB_SORT) impure,
					(first_records < 0) ? 0 : (UINT64) first_records + skip_records);
				}
			else 
				((IRSB_SORT) impure)->irsb_sort_handle = NULL;
			return;
		***/
		
		/***
		case rsb_procedure:
			open_procedure(request, tdbb, this, (IRSB_PROCEDURE) impure);
			return;
		***/
		
		case rsb_first:
			first_records = ((IRSB_FIRST) impure)->irsb_count =
				MOV_get_int64(EVL_expr(tdbb, (JRD_NOD) rsb_arg[0]), 0);

			if (((IRSB_FIRST) impure)->irsb_count < 0)
				ERR_post(isc_bad_limit_param, 0);

			//rsb = rsb_next;
			rsb_next->open(request);
			return;

		case rsb_skip:
			skip_records = ((IRSB_SKIP) impure)->irsb_count =
				MOV_get_int64(EVL_expr(tdbb, (JRD_NOD) rsb_arg[0]), 0);

			if (((IRSB_SKIP) impure)->irsb_count < 0)
				ERR_post(isc_bad_skip_param, 0);
			((IRSB_SKIP) impure)->irsb_count++;

			//rsb = rsb_next;
			rsb_next->open(request);
			return;
	
		/***
		case rsb_boolean:
			//rsb = rsb_next;
			rsb_next->open(request);
			return;
		***/

		/***
		case rsb_union:
			{
			((IRSB) impure)->irsb_count = 0;
			VIO_record(tdbb, rpb, rsb_format, tdbb->tdbb_default);

			// Initialize the record number of each stream in the union

			RecordSource** ptr = &rsb_arg[rsb_count];
			
			for (RecordSource** const end = ptr + (USHORT)(long) *ptr;  ++ptr <= end;) 
				request->req_rpb[(USHORT)(long) * ptr].rpb_number.setValue(BOF_NUMBER);

			//rsb = rsb_arg[0];
			rsb_arg[0]->open(request);
			}
			break;
		***/
		
		case rsb_aggregate:
			((IRSB) impure)->irsb_count = 3;
			VIO_record(tdbb, rpb, rsb_format, tdbb->tdbb_default);
			return;

		/***
		case rsb_merge:
			open_merge(request, tdbb, this, (IRSB_MRG) impure);
			return;
		***/
		
		case rsb_ext_sequential:
		case rsb_ext_indexed:
		case rsb_ext_dbkey:
			EXT_open(tdbb, this);
			return;

#ifdef OBSOLETE
		case rsb_left_cross:
			{
			//RSE_open(tdbb, rsb_arg[RSB_LEFT_outer]);
			rsb_arg[RSB_LEFT_outer]->open(request);
			impure->irsb_flags &=
				~(irsb_first | irsb_in_opened | irsb_join_full);
			impure->irsb_flags |= irsb_mustread;

			/* Allocate a record block for each union/aggregate/procedure
			   stream in the right sub-stream.  The block will be needed
			   if we join to nulls before opening the rsbs */

			for (RsbStack::iterator stack(*(rsb_left_rsbs));  stack.hasData(); ++stack)
				VIO_record(tdbb,
						&request->req_rpb[stack.object()->rsb_stream],
						stack.object()->rsb_format, tdbb->tdbb_default);

			return;
			}
#endif // OBSOLETE

		default:
			BUGCHECK(166);		/* msg 166 invalid rsb type */
		}
}

bool RecordSource::get(Request* request, RSE_GET_MODE mode)
{
	//SET_TDBB(tdbb);
	//JRD_REQ request = tdbb->tdbb_request;
	thread_db *tdbb = request->req_tdbb;
	IRSB impure = (IRSB) IMPURE (request, rsb_impure);

#ifdef SCROLLABLE_CURSORS
	/* The mode RSE_get_next is a generic mode which requests that 
	   we continue on in the last direction we were going.  Oblige 
	   by converting the mode to the appropriate direction. */

	if (mode == RSE_get_next)
		mode = (impure->irsb_flags & irsb_last_backwards) ? RSE_get_backward : RSE_get_forward;

	request->req_flags &= ~req_fetch_required;
#endif // SCROLLABLE_CURSORS

	/* Turn off the flag so that records at a 
	   lower level will not be counted. */

	const bool count = (request->req_flags & req_count_records) != 0;
	request->req_flags &= ~req_count_records;
	BOOLEAN result;
	
	while (result = get_record(request, tdbb, this, NULL, mode))
		{
		if (rsb_flags & rsb_writelock)
			{
			// Lock record if we were asked for it
			Transaction* transaction = request->req_transaction;
			RecordSource* test_rsb;
			
			if (rsb_type == rsb_boolean)
				test_rsb = rsb_next;
			else
				test_rsb = this;

			record_param* org_rpb = request->req_rpb + test_rsb->rsb_stream;
			Relation* relation = org_rpb->rpb_relation;
			
			if (relation && !relation->rel_view_rse && !relation->rel_file) 
				{
				RLCK_reserve_relation(tdbb, transaction, relation, TRUE, TRUE);
				
				// Fetch next record if current was deleted before being locked
				if (!VIO_writelock(tdbb, org_rpb, this, transaction)) 
					continue;
				}
			}
		
		if (count) 
			{
			request->req_records_selected++;
			request->req_records_affected++;
			}
		break;
		}

	/* reset the flag to whatever it was */

	if (count)
		request->req_flags |= req_count_records;

	return result;
}

void RecordSource::close(Request* request)
{
	IRSB_SORT impure = (IRSB_SORT) IMPURE (request, rsb_impure);
	if (!(impure->irsb_flags & irsb_open))
		return;

	impure->irsb_flags &= ~irsb_open;

	switch (rsb_type) 
		{
		/***
		case rsb_indexed:
		case rsb_navigate:
			return;

		case rsb_sequential:
			{
			record_param* rpb = &request->req_rpb[rsb_stream];
			if (rpb->rpb_window.win_flags & WIN_large_scan &&
				rpb->rpb_relation->rel_scan_count)
					--rpb->rpb_relation->rel_scan_count;
			return;
			}
		***/
		
		case rsb_first:
		case rsb_skip:
		//case rsb_boolean:
		case rsb_aggregate:
			//rsb = rsb_next;
			rsb_next->close(request);
			return;

		/***
		case rsb_cross:
			{
			RecordSource** ptr = rsb_arg;
			for (RecordSource** const end = ptr + rsb_count; ptr < end; ptr++)
				//RSE_close(tdbb, *ptr);
				(*ptr)->close(request);
			return;
			}
		***/
	
		/***	
		case rsb_left_cross:
			//RSE_close(tdbb, rsb_arg[RSB_LEFT_outer]);
			//RSE_close(tdbb, rsb_arg[RSB_LEFT_inner]);
			rsb_arg[RSB_LEFT_outer]->close(request);
			rsb_arg[RSB_LEFT_inner]->close(request);
			return;
		***/
		
		/***
		case rsb_procedure:
			close_procedure(request, tdbb, this);
			return;
		***/
		
		/***
		case rsb_merge:
			close_merge(request, tdbb, this, (IRSB_MRG) impure);
			return;
		***/
		
		/***
		case rsb_sort:
			SORT_fini(impure->irsb_sort_handle, tdbb->tdbb_attachment);
			impure->irsb_sort_handle = NULL;
			//rsb = rsb_next;
			rsb_next->close(request);
			return;
		***/

		/***		
		case rsb_union:
			{
			const USHORT i = ((IRSB) impure)->irsb_count;
			if (i >= rsb_count)
				return;
			//rsb = rsb_arg[i];
			rsb_arg[i]->close(request);
			}
			return;
		***/
		
		case rsb_ext_sequential:
		case rsb_ext_indexed:
		case rsb_ext_dbkey:
			EXT_close(request->req_tdbb, this);
			return;

		default:
			BUGCHECK(166);		/* msg 166 invalid rsb type */
		}
}

static bool reject(const UCHAR* record_a, const UCHAR* record_b, void* user_arg)
{
/**************************************
 *
 *	r e j e c t
 *
 **************************************
 *
 * Functional description
 *	Callback routine used by project to reject duplicate records.
 *	Particularly dumb routine -- always returns true;
 *
 **************************************/

	return true;
}

static BOOLEAN get_record(Request *request, thread_db*	tdbb,
						  RecordSource*	rsb,
						  RecordSource*	parent_rsb,
						  RSE_GET_MODE	mode)
{
/**************************************
 *
 *	g e t _ r e c o r d
 *
 **************************************
 *
 * Functional description
 *	Get a record from an open record stream.
 *	The mode parameter specifies whether to
 *	go forward one record, go backward one
 *	record, or fetch the current record.
 *
 **************************************/
	/* check request flags for special processing */

	if (request->req_flags & req_abort)
		return FALSE;

	if (!request->req_transaction)
		return FALSE;

	IRSB impure = (IRSB) IMPURE (request, rsb->rsb_impure);
	
	if (impure->irsb_flags & irsb_singular_processed)
		return FALSE;

	record_param* rpb = request->req_rpb + rsb->rsb_stream;

#ifdef SCROLLABLE_CURSORS
	/* do bof and eof handling for streams which may be navigated */

	if (rsb->rsb_type == rsb_sequential ||
		rsb->rsb_type == rsb_navigate || rsb->rsb_type == rsb_sort)
		if (((mode == RSE_get_forward) && (impure->irsb_flags & irsb_eof)) ||
			((mode == RSE_get_backward) && (impure->irsb_flags & irsb_bof)))
				return FALSE;
#endif // SCROLLABLE_CURSORS

	switch (rsb->rsb_type)
	{
	case rsb_sequential:
		if (impure->irsb_flags & irsb_bof)
			rpb->rpb_number.setValue(BOF_NUMBER);

		if (!VIO_next_record(tdbb,
							rpb,
							rsb,
							request->req_transaction,
							request->req_pool,
							(mode == RSE_get_backward) ? TRUE : FALSE,
							FALSE))
				return FALSE;
		break;

	case rsb_indexed:
		{
		RecordBitmap **pbitmap = ((IRSB_INDEX) impure)->irsb_bitmap;
		RecordBitmap *bitmap;
		if (!pbitmap || !(bitmap = *pbitmap))
			return false;

		bool result = false;

		// NS: Original code was the following:
		// while (SBM_next(*bitmap, &rpb->rpb_number, mode))
		// We assume mode = RSE_get_forward because we do not support
		// scrollable cursors at the moment.
		if (rpb->rpb_number.isBof() ? bitmap->getFirst() : bitmap->getNext()) do {
			rpb->rpb_number.setValue(bitmap->current());
#ifdef SUPERSERVER_V2
			/* Prefetch next set of data pages from bitmap. */

			if (rpb->rpb_number >
				((IRSB_INDEX) impure)->irsb_prefetch_number &&
				(mode == RSE_get_forward))
			{
				((IRSB_INDEX) impure)->irsb_prefetch_number =
					DPM_prefetch_bitmap(tdbb, rpb->rpb_relation,
										*bitmap, rpb->rpb_number);
			}
#endif // SUPERSERVER_V2
			if (VIO_get(tdbb, rpb, rsb, request->req_transaction,
						request->req_pool))
			{
				result = true;
				break;
			}
		} while (bitmap->getNext());

		if (result)
			break;
			
		return false;
		}

	/***
	case rsb_navigate:
	
#ifdef SCROLLABLE_CURSORS
		if (impure->irsb_flags & irsb_bof)
			rpb->rpb_number = -1;
#endif // SCROLLABLE_CURSORS

		if (!NAV_get_record(tdbb, rsb, (IRSB_NAV) impure, rpb, mode))
			return FALSE;
		break;
	***/
	
#ifdef OBSOLETE
	case rsb_boolean:
		{
		int result;
		SSHORT select_value;	/* select for ANY/ALL processing */
		JRD_NOD select_node;	/* ANY/ALL select node pointer */
		JRD_NOD column_node;	/* ANY/ALL column node pointer */

		/* For ANY and ALL clauses (ALL is handled as a negated ANY),
			we must first detect them, and then make sure that the returned
			results are correct.   This mainly entails making sure that
			there are in fact records in the source stream to test against.
			If there were none, the response must be FALSE.
			Also, if the result of the column comparison is always
			NULL, this must also be returned as NULL.   (Note that normally,
			an AND of a NULL and a FALSE would be FALSE, not NULL).

			This all depends on evl.c putting the unoptimized expression
			in the rsb.   The unoptimized expression always has the
			select expression on the left, and the column comparison
			on the right. */

		column_node = (JRD_NOD) rsb->rsb_any_boolean;
		
		if (column_node && (request->req_flags & (req_ansi_all | req_ansi_any)))
			{
			/* see if there's a select node to work with */

			if (column_node->nod_type == nod_and)
				{
				select_node = column_node->nod_arg[0];
				column_node = column_node->nod_arg[1];
				}
			else
				select_node = NULL;
			}
			
		if (column_node && (request->req_flags & req_ansi_any))
			{
			SSHORT any_null;	/* some records null for ANY/ALL */
			SSHORT any_true;	/* some records true for ANY/ALL */
			request->req_flags &= ~req_ansi_any;
			
			if (request->req_flags & req_ansi_not)
				{
				request->req_flags &= ~req_ansi_not;

				/* do NOT ANY */
				/* if the subquery was the empty set
					(numTrue + numFalse + numUnknown = 0)
					or if all were false
					(numTrue + numUnknown = 0),
					NOT ANY is true */

				any_null = FALSE;
				any_true = FALSE;
				
				//while (get_record(request, tdbb, rsb->rsb_next, rsb, mode))
				while (rsb->rsb_next->get(request, mode))
					{
					if (EVL_boolean(tdbb, (JRD_NOD) rsb->rsb_arg[0]))
						{
						/* found a TRUE value */

						any_true = TRUE;
						break;
						}

					/* check for select stream and nulls */

					if (!select_node)
						{
						if (request->req_flags & req_null)
							{
							any_null = TRUE;
							break;
							}
						}
					else
						{
						request->req_flags &= ~req_null;
						select_value = EVL_boolean(tdbb, select_node);

						/* see if any record in select stream */

						if (select_value)
							{
							/* see if any nulls */

							request->req_flags &= ~req_null;
							EVL_boolean(tdbb, column_node);

							/* see if any record is null */

							if (request->req_flags & req_null) 
								{
								any_null = TRUE;
								break;
								}
							}
						}
					}
					
				request->req_flags &= ~req_null;
				
				if (any_null || any_true)
					result = TRUE;
				else
					return FALSE;
					
				break;
				}
			else
				{
				/* do ANY */
				/* if the subquery was true for any comparison, ANY is true */

				result = FALSE;
				
				//while (get_record(request, tdbb, rsb->rsb_next, rsb, mode))
				while (rsb->rsb_next->get(request, mode))
					{
					if (EVL_boolean(tdbb, (JRD_NOD) rsb->rsb_arg[0])) 
						{
						result = TRUE;
						break;
						}
					}
					
				request->req_flags &= ~req_null;
				
				if (result)
					break;
					
				return FALSE;
				}
			}
		else if (column_node && (request->req_flags & req_ansi_all))
			{
			SSHORT any_false;	/* some records false for ANY/ALL */
			request->req_flags &= ~req_ansi_all;
			
			if (request->req_flags & req_ansi_not)
				{
				request->req_flags &= ~req_ansi_not;

				/* do NOT ALL */
				/* if the subquery was false for any comparison, NOT ALL is true */

				any_false = FALSE;
				
				//while (get_record(request, tdbb, rsb->rsb_next, rsb, mode))
				while (rsb->rsb_next->get(request, mode))
					{
					request->req_flags &= ~req_null;

					/* look for a FALSE (and not null either) */

					if (!EVL_boolean(tdbb, (JRD_NOD) rsb->rsb_arg[0]) && !(request->req_flags & req_null))
						{

						/* make sure it wasn't FALSE because there's
							no select stream record */

						if (select_node) 
							{
							request->req_flags &= ~req_null;
							select_value = EVL_boolean(tdbb, select_node);
							
							if (select_value) 
								{
								any_false = TRUE;
								break;
								}
							}
						else 
							{
							any_false = TRUE;
							break;
							}
						}
					}
					
				request->req_flags &= ~req_null;
				
				if (any_false)
					return FALSE;
					
				result = TRUE;
				break;
				}
			else
				{
				/* do ALL */
				/* if the subquery was the empty set (numTrue + numFalse + numUnknown = 0)
					or if all were true (numFalse + numUnknown = 0), ALL is true */

				any_false = FALSE;
				
				//while (get_record(request, tdbb, rsb->rsb_next, rsb, mode))
				while (rsb->rsb_next->get(request, mode))
					{
					request->req_flags &= ~req_null;

					/* look for a FALSE or null */

					if (!EVL_boolean(tdbb, (JRD_NOD) rsb->rsb_arg[0]))
						{
						/* make sure it wasn't FALSE because there's
							no select stream record */

						if (select_node) 
							{
							request->req_flags &= ~req_null;
							select_value = EVL_boolean(tdbb, select_node);
							
							if (select_value) 
								{
								any_false = TRUE;
								break;
								}
							}
						else 
							{
							any_false = TRUE;
							break;
							}
						}
					}
					
				request->req_flags &= ~req_null;
				
				if (any_false)
					return FALSE;
					
				result = TRUE;
				break;
				}
			}
		else
			{
			UCHAR flag = FALSE;
			result = FALSE;
			
			//while (get_record(request, tdbb, rsb->rsb_next, rsb, mode))
			while (rsb->rsb_next->get(request, mode))
				{
				if (EVL_boolean(tdbb, (JRD_NOD) rsb->rsb_arg[0])) 
					{
					result = TRUE;
					break;
					}

				if (request->req_flags & req_null)
					flag = TRUE;
				}

			if (flag)
				request->req_flags |= req_null;
				
			if (result)
				break;
				
			return FALSE;
			}
		}
#endif // OBSOLETE

    /******
        ***     IMPORTANT!!
        *
        *   If the RSB list contains both a rsb_first node and a rsb_skip node
        *     the rsb_skip node MUST be after the rsb_first node in the list.
        *     The reason is the rsb_skip calls get_record in a loop to skip
        *     over the first n records in the stream.  If the rsb_first node
        *     was down stream the counter associated with rsb_first would
        *     be decremented by the calls to get_record that never return a
        *     record to the user.  Possible symptoms of this are erroneous
        *     empty result sets (when skip >= first) and too small result sets
        *     (when first > skip, first - skip records will be returned).
        *******/
        
		case rsb_first:
			switch(mode) 
				{
				case RSE_get_forward:
					if (((IRSB_FIRST) impure)->irsb_count <= 0)
						return FALSE;
						
					((IRSB_FIRST) impure)->irsb_count--;
					
					//if (!get_record(request, tdbb, rsb->rsb_next, NULL, mode))
					if (!rsb->rsb_next->get(request, mode))
						return FALSE;
					break;

				case RSE_get_current:
					if (((IRSB_FIRST) impure)->irsb_count <= 0)
						return FALSE;
						
					//if (!get_record(request, tdbb, rsb->rsb_next, NULL, mode))
					if (!rsb->rsb_next->get(request, mode))
						return FALSE;
						
					break;

				case RSE_get_backward:
					((IRSB_FIRST) impure)->irsb_count++;
					
					//if (!get_record(request, tdbb, rsb->rsb_next, NULL, mode))
					if (!rsb->rsb_next->get(request, mode))
						return FALSE;
					break;
				}
				break;

		case rsb_skip:
			switch(mode) 
				{
				case RSE_get_backward:
					if (((IRSB_SKIP) impure)->irsb_count > 0)
						return FALSE;
						
					if (((IRSB_SKIP) impure)->irsb_count == 0) 
						{
						((IRSB_SKIP) impure)->irsb_count++;
						//get_record(request, tdbb, rsb->rsb_next, NULL, mode);
						rsb->rsb_next->get(request, mode);
						return FALSE;
						}
						
					((IRSB_SKIP) impure)->irsb_count++;
					
					//if (!get_record(request, tdbb, rsb->rsb_next, NULL, mode))
					if (!rsb->rsb_next->get(request, mode))
						return FALSE;
					break;

				case RSE_get_forward:
					while(((IRSB_SKIP) impure)->irsb_count > 1) 
						{
						((IRSB_SKIP) impure)->irsb_count--;
						
						//if (!get_record(request, tdbb, rsb->rsb_next, NULL, mode))
						if (!rsb->rsb_next->get(request, mode))
							return FALSE;
						}
						
					((IRSB_SKIP) impure)->irsb_count--;
					
					//if (!get_record(request, tdbb, rsb->rsb_next, NULL, mode))
					if (!rsb->rsb_next->get(request, mode))
						return FALSE;
					break;

				case RSE_get_current:
					if (((IRSB_SKIP) impure)->irsb_count >= 1)
						return FALSE;
					//else if (!get_record(request, tdbb, rsb->rsb_next, NULL, mode))
					else if (!rsb->rsb_next->get(request, mode))
						return FALSE;
				}
			break;

		/***
		case rsb_merge:
			if (!get_merge_join(request, tdbb, rsb, (IRSB_MRG) impure, mode))
				return FALSE;
			break;
		***/

		/***
		case rsb_procedure:
			if (!get_procedure(request, tdbb, rsb, (IRSB_PROCEDURE) impure, rpb))
				return FALSE;
			break;
		***/

#ifdef OBSOLETE
		case rsb_sort:
			{
			UCHAR *data;

#ifdef SCROLLABLE_CURSORS
			/* any attempt to get a record takes us off of bof or eof */

			impure->irsb_flags &= ~(irsb_bof | irsb_eof);
#endif // SCROLLABLE_CURSORS

			if (!(data = get_sort(request, tdbb, rsb, mode)))
				{
#ifdef SCROLLABLE_CURSORS

				if (mode == RSE_get_forward)
					impure->irsb_flags |= irsb_eof;
				else
					impure->irsb_flags |= irsb_bof;
#endif // SCROLLABLE_CURSORS
				return FALSE;
				}

			RecordSource::mapSortData(request, (SortMap*) rsb->rsb_arg[0], data);

#ifdef SCROLLABLE_CURSORS
			/* fix up the sort data in case we need to retrieve it again */

			unget_sort(tdbb, rsb, data);
#endif // SCROLLABLE_CURSORS
			}
			break;
#endif //OBSOLETE

#ifdef OBSOLETE		
		case rsb_cross:
			if (impure->irsb_flags & irsb_first) 
				{
				SSHORT i;

				for (i = 0; i < (SSHORT) rsb->rsb_count; i++) 
					{
					//RSE_open(tdbb, rsb->rsb_arg[i]);
					rsb->rsb_arg[i]->open(request);
					
					if (!fetch_record(request, tdbb, rsb, i, mode))
						return FALSE;
					}
					
				impure->irsb_flags &= ~irsb_first;
				break;
				}

			/* in the case of a project which has been mapped to an index, 
			we need to make sure that we only return a single record for 
			each of the leftmost records in the join */

			if (rsb->rsb_flags & rsb_project) 
				{
				if (!fetch_record(request, tdbb, rsb, 0, mode))
					return FALSE;
				}
			else if (!fetch_record(request, tdbb, rsb, rsb->rsb_count - 1, mode))
				return FALSE;
				
			break;
#endif // OBSOLETE

/***
		case rsb_union:
			if (!get_union(request, tdbb, rsb, impure))
				return FALSE;
			break;
***/

		case rsb_aggregate:
			if ( (impure->irsb_count = EVL_group(tdbb, rsb->rsb_next,
											(JRD_NOD) rsb->rsb_arg[0],
											impure->irsb_count)) ) 
				break;
			return FALSE;

		case rsb_ext_sequential:
		case rsb_ext_indexed:
		case rsb_ext_dbkey:
			if (!EXT_get(tdbb, rsb))
				return FALSE;
			break;

		/***
		case rsb_left_cross:
			if (!fetch_left(request, tdbb, rsb, impure, mode))
				return FALSE;
			break;
		***/
		
		default:
			BUGCHECK(166);			/* msg 166 invalid rsb type */
		}

	/* Check to see if we need to update the record_count. This record 
	   count is used in NAV_get_record and needs to be updated before
	   checking for singularity. Note that in our check for singularity
	   we call get_record which calls NAV_get_record where this count
	   is used. Bug # 8415, 8416 */

	if (rsb->rsb_type == rsb_boolean)
		rsb->rsb_next->rsb_record_count++;
	else if (!parent_rsb || parent_rsb->rsb_type != rsb_boolean)
		rsb->rsb_record_count++;

	if (rsb->rsb_flags & rsb_singular && !(impure->irsb_flags & irsb_checking_singular)) 
		{
		push_rpbs(tdbb, request, rsb);
		impure->irsb_flags |= irsb_checking_singular;
		
		//if (get_record(request, tdbb, rsb, parent_rsb, mode)) 
		if (rsb->get(request, mode))
			{
			impure->irsb_flags &= ~irsb_checking_singular;
			ERR_post(isc_sing_select_err, 0);
			}
			
		pop_rpbs(request, rsb);
		impure->irsb_flags &= ~irsb_checking_singular;
		impure->irsb_flags |= irsb_singular_processed;
		}

	return TRUE;
}

static BOOLEAN fetch_record(Request *request, thread_db* tdbb, RecordSource* rsb, SSHORT n, RSE_GET_MODE mode)
{
/**************************************
 *
 *	f e t c h _ r e c o r d
 *
 **************************************
 *
 * Functional description
 *	Get a record for a sub-stream of a cross.  If the
 *	sub-stream is dry, close it and recurse to get the
 *	next record from the previous stream.  If there isn't
 *	a previous stream, then the cross is finished, so
 *	return FALSE.
 *
 **************************************/
	//SET_TDBB(tdbb);
#ifndef SCROLLABLE_CURSORS
	mode = RSE_get_forward;
#endif // SCROLLABLE_CURSORS

	RecordSource* sub_rsb = rsb->rsb_arg[n];

	//if (get_record(request, tdbb, sub_rsb, NULL, mode))
	if (sub_rsb->get(request, mode))
		return TRUE;

	/* we have exhausted this stream, so close it; if there is 
	   another candidate record from the n-1 streams to the left, 
	   then reopen the stream and start again from the beginning */

	while (true)
		{
		//RSE_close(tdbb, sub_rsb);
		sub_rsb->close(request);
		
		if (n == 0 || !fetch_record(request, tdbb, rsb, n - 1, mode))
			return FALSE;
			
		//RSE_open(tdbb, sub_rsb);
		sub_rsb->open(request);

		//if (get_record(request, tdbb, sub_rsb, NULL, mode))
		if (sub_rsb->get(request, mode))
			return TRUE;
		}
}




#ifdef OBSOLETE
static BOOLEAN get_union(Request *request, thread_db* tdbb, RecordSource* rsb, IRSB impure)
{
/**************************************
 *
 *	g e t _ u n i o n
 *
 **************************************
 *
 * Functional description
 *	Get next record in a union.
 *
 **************************************/
	//JRD_NOD map, *ptr, *end;
	//SET_TDBB(tdbb);
	RecordSource** rsb_ptr = rsb->rsb_arg + impure->irsb_count;

	/* March thru the sub-streams (tributaries?) looking for a record */

	//while (!get_record(request, tdbb, *rsb_ptr, NULL, RSE_get_forward)) 
	while (!(*rsb_ptr)->get(request, RSE_get_forward))
		{
		//RSE_close(tdbb, *rsb_ptr);
		(*rsb_ptr)->close(request);
		impure->irsb_count += 2;
		
		if (impure->irsb_count >= rsb->rsb_count)
			return FALSE;
			
		rsb_ptr += 2;
		//RSE_open(tdbb, *rsb_ptr);
		(*rsb_ptr)->open(request);
		}

	/* We've got a record, map it into the target record */

	JRD_NOD map = (JRD_NOD) rsb_ptr[1];

	for (JRD_NOD *ptr = map->nod_arg, *end = ptr + map->nod_count; ptr < end; ptr++)
		EXE_assignment(tdbb, *ptr);

	return TRUE;
}
#endif // OBSOLETE

#ifdef OBSOLETE
#ifdef SCROLLABLE_CURSORS
static BOOLEAN fetch_left(thread_db* tdbb, RecordSource* rsb, IRSB impure, RSE_GET_MODE mode)
{
/**************************************
 *
 *	f e t c h _ l e f t ( I B _ V 4 _ 1 )
 *
 **************************************
 *
 * Functional description
 *	Get records for a left outer join.  Records are read
 *	from the left sub-stream when the right sub-stream is
 *	dry or when it is not yet open.  When the left 
 *	sub-stream's boolean is true, open the right sub-stream
 *	and read a record.  When the right sub-stream becomes dry,
 *	close it, and if nothing has been joined to the left
 *	sub-stream's current record, join a null valued right
 *	sub-stream record.  When the left sub-stream is dry,
 *	the outer join is finished, so return FALSE.
 *
 **************************************/
	SET_TDBB(tdbb);

/* loop through the outer join in either the forward or the backward direction; 
   the various modes indicate what state of the join we are in */

	while (true)
		{
		if (!(impure->irsb_flags & irsb_join_full))
			{
			/* mustread indicates to get the next record from the outer stream */

			if (impure->irsb_flags & irsb_mustread)
				{
				//if (!get_record(tdbb, rsb->rsb_arg[RSB_LEFT_outer], NULL, mode))
				if (!rsb->rsb_arg[RSB_LEFT_outer]->get(request, mode);
					{
					if (mode == RSE_get_backward)
						return FALSE;
					else if (!rsb->rsb_arg[RSB_LEFT_inner_streams])
						return FALSE;

					/* We have a full outer join.  Open up the inner stream
					   one more time. */

					//RSE_close(tdbb, rsb->rsb_arg[RSB_LEFT_outer]);
					rsb->rsb_arg[RSB_LEFT_outer]->close(request, rdbb);
					impure->irsb_flags |= irsb_join_full;
					//RSE_open(tdbb, rsb->rsb_arg[RSB_LEFT_inner]);
					rsb->rsb_arg[RSB_LEFT_outer]->open(request, rdbb);
					continue;
					}

				/* check if the outer record qualifies for the boolean */

				if (rsb->rsb_arg[RSB_LEFT_boolean] &&
					!EVL_boolean(tdbb, rsb->rsb_arg[RSB_LEFT_boolean]))
					{
					/* The boolean pertaining to the left sub-stream is false
					   so just join sub-stream to a null valued right sub-stream */
					join_to_nulls(request, tdbb, rsb, RSB_LEFT_streams);
					return TRUE;
					}

				impure->irsb_flags &= ~(irsb_mustread | irsb_joined);
				impure->irsb_flags |= irsb_in_opened;
				//RSE_open(tdbb, rsb->rsb_arg[RSB_LEFT_inner]);
				rsb->rsb_arg[RSB_LEFT_inner]->open(request);
				}

			/* fetch records from the inner stream until exhausted */

			//while (get_record(tdbb, rsb->rsb_arg[RSB_LEFT_inner], NULL, mode))
			while(rsb->rsb_arg[RSB_LEFT_inner]->get(request, mode))
				if (!rsb->rsb_arg[RSB_LEFT_inner_boolean] ||
					EVL_boolean(tdbb, rsb->rsb_arg[RSB_LEFT_inner_boolean]))
					{
					impure->irsb_flags |= irsb_joined;
					return TRUE;
					}

			/* out of inner records, go back to reading the next outer record */

			RSE_close(tdbb, rsb->rsb_arg[RSB_LEFT_inner]);
			impure->irsb_flags |= irsb_mustread;

			/* The right stream did not have any matching records.  Join 
			   the left stream to a null valued right sub-stream */

			if (!(impure->irsb_flags & irsb_joined)) {
				join_to_nulls(request, tdbb, rsb, RSB_LEFT_streams);
				return TRUE;
			}
		}
		else
			{
			/* Continue with a full outer join. */

			RSB full = rsb->rsb_arg[RSB_LEFT_inner];
			full = (full->rsb_type == rsb_boolean) ? full->rsb_next : full;

			if (impure->irsb_flags & irsb_in_opened)
				{
				/* The inner stream was opened at some point.  If it doesn't have a
				   boolean, then all of its records have been returned.  Otherwise,
				   find the records that haven't been. */
				   
				BOOLEAN found;
				
				do {
					//if (!get_record(tdbb, full, NULL, mode)) 
					if (!full->get(request, mode))
						{
						if (mode == RSE_get_forward)
							return FALSE;
						else
							goto return_to_outer;
						}

					//RSE_open(tdbb, rsb->rsb_arg[RSB_LEFT_outer]);
					rsb->rsb_arg[RSB_LEFT_outer]->open(request);
					
					while (found = rsb->rsb_arg[RSB_LEFT_outer]->get(request, mode))
						   //get_record(tdbb, rsb->rsb_arg[RSB_LEFT_outer], NULL, mode))
						{
						if (
							(!rsb->rsb_arg[RSB_LEFT_boolean]
								|| EVL_boolean(tdbb, rsb->rsb_arg[RSB_LEFT_boolean]))
							&& (!rsb->rsb_arg[RSB_LEFT_inner_boolean]
								|| EVL_boolean(tdbb, rsb->rsb_arg [RSB_LEFT_inner_boolean]))
							&& (full == rsb->rsb_arg[RSB_LEFT_inner]
								|| EVL_boolean(tdbb, rsb->rsb_arg[RSB_LEFT_inner]->rsb_arg[0])))
							{
								break;
							}
						}
						
					//RSE_close(tdbb, rsb->rsb_arg[RSB_LEFT_outer]);
					rsb->rsb_arg[RSB_LEFT_outer]->close(request);
				} while (found);
				}
			//else if (!get_record(tdbb, full, NULL, mode))
			else if (!full->get(request, mode))
				{
				if (mode == RSE_get_forward)
					return FALSE;
				else
					goto return_to_outer;
				}

			join_to_nulls(request, tdbb, rsb, RSB_LEFT_inner_streams);
			return TRUE;

return_to_outer:
			impure->irsb_flags &= ~(irsb_join_full | irsb_in_opened);
			impure->irsb_flags |= irsb_mustread;
			//RSE_close(tdbb, rsb->rsb_arg[RSB_LEFT_inner]);
			rsb->rsb_arg[RSB_LEFT_inner]->close(request);
			//RSE_close(tdbb, rsb->rsb_arg[RSB_LEFT_outer]);
			rsb->rsb_arg[RSB_LEFT_outer]->close(request);
			//RSE_open(tdbb, rsb->rsb_arg[RSB_LEFT_outer]);
			rsb->rsb_arg[RSB_LEFT_outer]->open(request);
			}
		}

/* NOTREACHED */
	return TRUE;
}
#else
static BOOLEAN fetch_left(Request *request, thread_db* tdbb, RecordSource* rsb, IRSB impure, RSE_GET_MODE mode)
{
/**************************************
 *
 *	f e t c h _ l e f t
 *
 **************************************
 *
 * Functional description
 *	Get records for a left outer join.  Records are read
 *	from the left sub-stream when the right sub-stream is
 *	dry or when it is not yet open.  When the left 
 *	sub-stream's boolean is true, open the right sub-stream
 *	and read a record.  When the right sub-stream becomes dry,
 *	close it, and if nothing has been joined to the left
 *	sub-stream's current record, join a null valued right
 *	sub-stream record.  When the left sub-stream is dry,
 *	the outer join is finished, so return FALSE.
 *
 **************************************/
	//SET_TDBB(tdbb);

	if (!(impure->irsb_flags & irsb_join_full))
		{
		while (true)
			{
			if (impure->irsb_flags & irsb_mustread)
				{
				//if (!get_record(request, tdbb, rsb->rsb_arg[RSB_LEFT_outer], NULL, RSE_get_forward)) 
				if (!rsb->rsb_arg[RSB_LEFT_outer]->get(request, RSE_get_forward))
					{
					if (rsb->rsb_left_inner_streams->isEmpty())
						return false;

					/* We have a full outer join.  Open up the inner stream
					   one more time. */

					//RSE_close(tdbb, rsb->rsb_arg[RSB_LEFT_outer]);
					rsb->rsb_arg[RSB_LEFT_outer]->close(request);
					impure->irsb_flags |= irsb_join_full;
					//RSE_open(tdbb, rsb->rsb_arg[RSB_LEFT_inner]);
					rsb->rsb_arg[RSB_LEFT_outer]->open(request);
					break;
					}
					
				if (rsb->rsb_arg[RSB_LEFT_boolean] &&
					!EVL_boolean(tdbb, (jrd_nod*) rsb->rsb_arg[RSB_LEFT_boolean])) 
					{
					/* The boolean pertaining to the left sub-stream is false
					   so just join sub-stream to a null valued right sub-stream */
					join_to_nulls(request, tdbb, rsb, rsb->rsb_left_streams);
					return TRUE;
					}
					
				impure->irsb_flags &= ~(irsb_mustread | irsb_joined);
				impure->irsb_flags |= irsb_in_opened;
				//RSE_open(tdbb, rsb->rsb_arg[RSB_LEFT_inner]);
				rsb->rsb_arg[RSB_LEFT_inner]->open(request);
				}

			//while (get_record(request, tdbb, rsb->rsb_arg[RSB_LEFT_inner], NULL, RSE_get_forward))
			while ( rsb->rsb_arg[RSB_LEFT_inner]->get(request, RSE_get_forward))
				if (!rsb->rsb_arg[RSB_LEFT_inner_boolean]
						|| EVL_boolean(tdbb,(JRD_NOD) rsb->rsb_arg[RSB_LEFT_inner_boolean]))
					{
					impure->irsb_flags |= irsb_joined;
					return TRUE;
					}

			//RSE_close(tdbb, rsb->rsb_arg[RSB_LEFT_inner]);
			rsb->rsb_arg[RSB_LEFT_inner]->close(request);
			impure->irsb_flags |= irsb_mustread;
			
			if (!(impure->irsb_flags & irsb_joined))
				{
				/* The current left sub-stream record has not been joined
				   to anything.  Join it to a null valued right sub-stream */
				join_to_nulls(request, tdbb, rsb, rsb->rsb_left_streams);
				return TRUE;
				}
			}
		}

	/* Continue with a full outer join. */

	RecordSource* full = rsb->rsb_arg[RSB_LEFT_inner];
	full = (full->rsb_type == rsb_boolean) ? full->rsb_next : full;

	if (impure->irsb_flags & irsb_in_opened)
		{
		/* The inner stream was opened at some point.  If it doesn't have a
		   boolean, then all of its records have been returned.  Otherwise,
		   find the records that haven't been. */
		BOOLEAN found;
		
		do {
			//if (!get_record(request, tdbb, full, NULL, RSE_get_forward))
			if (!full->get(request, RSE_get_forward))
				return FALSE;
				
			//RSE_open(tdbb, rsb->rsb_arg[RSB_LEFT_outer]);
			rsb->rsb_arg[RSB_LEFT_outer]->open(request);
			
			//while ( (found =get_record(request, tdbb, rsb->rsb_arg[RSB_LEFT_outer], NULL,RSE_get_forward)) )
			while( (found = rsb->rsb_arg[RSB_LEFT_outer]->get(request, RSE_get_forward)) )
				{
				if ((!rsb->rsb_arg[RSB_LEFT_boolean] || EVL_boolean(tdbb,(JRD_NOD) rsb->rsb_arg[RSB_LEFT_boolean]))
					&& (!rsb->rsb_arg[RSB_LEFT_inner_boolean]
						|| EVL_boolean(tdbb,
									   (JRD_NOD)
									   rsb->rsb_arg
									   [RSB_LEFT_inner_boolean]))
					&& (full == rsb->rsb_arg[RSB_LEFT_inner]
						|| EVL_boolean(tdbb,
									   (JRD_NOD)
									   rsb->rsb_arg
									   [RSB_LEFT_inner]->rsb_arg[0])))
					break;
				}
			//RSE_close(tdbb, rsb->rsb_arg[RSB_LEFT_outer]);
			rsb->rsb_arg[RSB_LEFT_outer]->close(request);
			} while (found);
		}
	//else if (!get_record(request, tdbb, full, NULL, RSE_get_forward))
	else if (!full->get(request, RSE_get_forward))
		return FALSE;

	join_to_nulls(request, tdbb, rsb, rsb->rsb_left_inner_streams);

	return TRUE;
}
#endif
#endif // OBSOLETE


static void pop_rpbs(JRD_REQ request, RecordSource* rsb)
{
/**************************************
 *
 *	p o p _ r p b s
 *
 **************************************
 *
 * Functional description
 *	Restore record state to saved copy.
 *
 **************************************/
	record_param* rpb;
	IRSB_MRG impure = (IRSB_MRG) IMPURE (request, rsb->rsb_impure);

	switch (rsb->rsb_type) 
		{
		case rsb_indexed:
		case rsb_sequential:
		case rsb_procedure:
		case rsb_ext_sequential:
		case rsb_ext_indexed:
		case rsb_ext_dbkey:
		case rsb_navigate:
		case rsb_union:
		case rsb_aggregate:
			rpb = request->req_rpb + rsb->rsb_stream;
			restore_record(rpb);
			return;

		case rsb_sort:
			{
			SSHORT i, streams[128];
			SortMap* map;
			smb_repeat * item, *end_item;

			//map = (SortMap*) rsb->rsb_arg[0];
			map = ((RsbSort*) rsb)->map;
			
			for (i = 0; i < (SSHORT) request->req_count; i++)
				streams[i] = 0;
				
			end_item = map->smb_rpt + map->smb_count;
			
			for (item = map->smb_rpt; item < end_item; item++)
				streams[item->smb_stream] = 1;
				
			for (i = 0; i < (SSHORT) request->req_count; i++)
				if (streams[i]) 
					{
					rpb = request->req_rpb + i;
					restore_record(rpb);
					}
					
			return;
			}

		case rsb_merge:
			{
			SSHORT i, streams[128];
			SortMap* map;
			RecordSource* sort_rsb;
			RecordSource** ptr;
			RecordSource** end;
			smb_repeat * item, *end_item;
			irsb_mrg::irsb_mrg_repeat * tail;

			for (i = 0; i < (SSHORT) request->req_count; i++)
				streams[i] = 0;
				
			end = rsb->rsb_arg + rsb->rsb_count * 2;
			impure = (IRSB_MRG) IMPURE (request, rsb->rsb_impure);
			
			for (ptr = rsb->rsb_arg, tail = impure->irsb_mrg_rpt;
				ptr < end; ptr += 2, tail++)
				{
				sort_rsb = *ptr;
				//map = (SortMap*) sort_rsb->rsb_arg[0];
				map = ((RsbSort*) sort_rsb)->map;
				end_item = map->smb_rpt + map->smb_count;
				for (item = map->smb_rpt; item < end_item; item += 2)
					streams[item->smb_stream] = 1;
				}
				
			for (i = 0; i < (SSHORT) request->req_count; i++)
				if (streams[i]) 
					{
					rpb = request->req_rpb + i;
					restore_record(rpb);
					}
			return;
			}

		case rsb_first:
		case rsb_skip:
		case rsb_boolean:
			pop_rpbs(request, rsb->rsb_next);
			return;

		case rsb_cross:
			{
			RecordSource** ptr;
			RecordSource** end;

			/* Bug # 72369: singleton-SELECT in Stored Procedure gives wrong
			* results when there are more than 2 streams in the cross. 
			* rsb_cross can have more than 2 rsb_arg's. Go through each one
			*/
			
			for (ptr = rsb->rsb_arg, end = ptr + rsb->rsb_count; ptr < end; ptr++)
				pop_rpbs(request, *ptr);

			return;
			}

		case rsb_left_cross:
			//pop_rpbs(request, rsb->rsb_arg[RSB_LEFT_outer]);
			//pop_rpbs(request, rsb->rsb_arg[RSB_LEFT_inner]);
			pop_rpbs(request, ((RsbLeftCross*) rsb)->outerRsb);
			pop_rpbs(request, ((RsbLeftCross*) rsb)->innerRsb);
			return;

		default:
			BUGCHECK(166);	/* msg 166 invalid rsb type */
		}
}


static void push_rpbs(thread_db* tdbb, JRD_REQ request, RecordSource* rsb)
{
/**************************************
 *
 *	p u s h _ r p b s
 *
 **************************************
 *
 * Functional description
 *	Save data state for current rsb
 *
 **************************************/
	record_param* rpb;
	IRSB_MRG impure;
	//SET_TDBB(tdbb);

	switch (rsb->rsb_type) 
		{
		case rsb_indexed:
		case rsb_sequential:
		case rsb_procedure:
		case rsb_ext_sequential:
		case rsb_ext_indexed:
		case rsb_ext_dbkey:
		case rsb_navigate:
		case rsb_union:
		case rsb_aggregate:
			rpb = request->req_rpb + rsb->rsb_stream;
			save_record(tdbb, rpb);
			return;

		case rsb_sort:
			{
			SSHORT i, streams[128];
			SortMap* map;
			smb_repeat * item, *end_item;

			//map = (SortMap*) rsb->rsb_arg[0];
			map = ((RsbSort*) rsb)->map;
			
			for (i = 0; i < (SSHORT) request->req_count; i++)
				streams[i] = 0;
				
			end_item = map->smb_rpt + map->smb_count;
			
			for (item = map->smb_rpt; item < end_item; item++)
				streams[item->smb_stream] = 1;
				
			for (i = 0; i < (SSHORT) request->req_count; i++) 
				if (streams[i]) 
					{
					rpb = request->req_rpb + i;
					save_record(tdbb, rpb);
					}

			return;
			}

		case rsb_merge:
			{
			SSHORT i, streams[128];
			SortMap* map;
			RecordSource* sort_rsb;
			RecordSource** ptr;
			RecordSource** end;
			smb_repeat * item, *end_item;
			irsb_mrg::irsb_mrg_repeat * tail;

			for (i = 0; i < (SSHORT) request->req_count; i++)
				streams[i] = 0;
				
			end = rsb->rsb_arg + rsb->rsb_count * 2;
			impure = (IRSB_MRG) IMPURE (request, rsb->rsb_impure);
			
			for (ptr = rsb->rsb_arg, tail = impure->irsb_mrg_rpt;
				ptr < end; ptr += 2, tail++)
				{
				sort_rsb = *ptr;
				//map = (SortMap*) sort_rsb->rsb_arg[0];
				map = ((RsbSort*) sort_rsb)->map;
				end_item = map->smb_rpt + map->smb_count;
				
				for (item = map->smb_rpt; item < end_item; item++)
					streams[item->smb_stream] = 1;
				}
			for (i = 0; i < (SSHORT) request->req_count; i++)
				if (streams[i]) 
					{
					rpb = request->req_rpb + i;
					save_record(tdbb, rpb);
					}
					
			return;
			}

		case rsb_first:
		case rsb_skip:
		case rsb_boolean:
			push_rpbs(tdbb, request, rsb->rsb_next);
			return;

		case rsb_cross:
			{
			RecordSource** ptr;
			RecordSource** end;

			/* Bug # 72369: singleton-SELECT in Stored Procedure gives wrong
			* results when there are more than 2 streams in the cross. 
			* rsb_cross can have more than 2 rsb_arg's. Go through each one
			*/
			
			for (ptr = rsb->rsb_arg, end = ptr + rsb->rsb_count; ptr < end;ptr++)
				push_rpbs(tdbb, request, *ptr);

			return;
			}

			/* BUG #8637: left outer join gives internal gds software consistency
			  check. Added case for rsb_left_cross. */
			  
		case rsb_left_cross:
			//push_rpbs(tdbb, request, rsb->rsb_arg[RSB_LEFT_outer]);
			//push_rpbs(tdbb, request, rsb->rsb_arg[RSB_LEFT_inner]);
			push_rpbs(tdbb, request, ((RsbLeftCross*) rsb)->outerRsb);
			push_rpbs(tdbb, request, ((RsbLeftCross*) rsb)->innerRsb);
			return;

		default:
			BUGCHECK(166);	/* msg 166 invalid rsb type */
		}
}


static void restore_record(record_param * rpb)
{
/**************************************
 *
 *	r e s t o r e _ r e c o r d
 *
 **************************************
 *
 * Functional description
 *	Restore record to prior state
 *
 **************************************/
	Record* rec_copy;
	SaveRecordParam* rpb_copy;

	if ((rpb_copy = rpb->rpb_copy) && 
		 (rec_copy = rpb_copy->srpb_rpb->rpb_record)) 
		{
		Record* record = rpb->rpb_record;
		USHORT size = rec_copy->rec_length;
		
		if (!record || size > record->rec_length)
			/* msg 284 cannot restore singleton select data */
			BUGCHECK(284);
			
		record->rec_format = rec_copy->rec_format;
		record->rec_number = rec_copy->rec_number;
		MOVE_FAST(rec_copy->rec_data, record->rec_data, size);

		MOVE_FAST(rpb_copy->srpb_rpb, rpb, sizeof(struct record_param));
		rpb->rpb_record = record;

		delete rec_copy;
		}
		
	if (rpb_copy)
		delete rpb_copy;

	rpb->rpb_copy = NULL;
}

static void save_record(thread_db* tdbb, record_param * rpb)
{
/**************************************
 *
 *	s a v e _ r e c o r d
 *
 **************************************
 *
 * Functional description
 *	Save current record
 *
 **************************************/
	SaveRecordParam* rpb_copy;
	Record* record = rpb->rpb_record;

	if (record) 
		{
		SLONG size = record->rec_length;
		Record* rec_copy;
		
		if ( (rpb_copy = rpb->rpb_copy) ) 
			{
			if ( (rec_copy = rpb_copy->srpb_rpb->rpb_record) )
				delete rec_copy;
			}
		else
			rpb->rpb_copy = rpb_copy = FB_NEW(*tdbb->tdbb_default) SaveRecordParam(); 

		MOVE_FAST(rpb, rpb_copy->srpb_rpb, sizeof(struct record_param));
		rpb_copy->srpb_rpb->rpb_record = rec_copy =
			FB_NEW_RPT(*tdbb->tdbb_default, size) Record();

		rec_copy->rec_length = size;
		rec_copy->rec_format = record->rec_format;
		rec_copy->rec_number = record->rec_number;
		MOVE_FAST(record->rec_data, rec_copy->rec_data, size);
		}
}

void RecordSource::mapSortData(Request* request, SortMap* map, UCHAR* data)
{
	for (smb_repeat *item = map->smb_rpt, *end_item = item + map->smb_count; item < end_item; item++) 
		{
		UCHAR flag = *(data + item->smb_flag_offset);
		DSC from = item->smb_desc;
		from.dsc_address = data + (long) from.dsc_address;
		JRD_NOD node = item->smb_node;
		
		if (node && node->nod_type != nod_field)
			continue;

		/* if moving a TEXT item into the KEY portion of
		   the sort record, then want to sort by
		   language dependant order */

		/* in the case below a nod_field is being converted to
		   a sort key, there is a later nod_field in the item
		   list that contains the data to send back
		 */
		 
		if (IS_INTL_DATA(&item->smb_desc) &&
			(USHORT)(long) item->smb_desc.dsc_address <map->smb_key_length * sizeof(ULONG)) 
			continue;

		record_param *rpb = &request->req_rpb[item->smb_stream];
		SSHORT id = item->smb_field_id;
		
		if (id < 0) 
			{
			if (id == SMB_TRANS_ID)
				rpb->rpb_transaction = *(SLONG *) (from.dsc_address);
			else
				rpb->rpb_number.setValue(*(SINT64 *) (from.dsc_address));
				
			rpb->rpb_stream_flags |= RPB_s_refetch;
			continue;
			}
			
		Record *record = rpb->rpb_record;

        if (record && !flag && !record->rec_format && record->rec_fmt_bk) 
            record->rec_format = record->rec_fmt_bk; // restore the format

		DSC to;
		EVL_field(0, record, id, &to);

		if (flag)
			SET_NULL(record, id);
		else 
			{
			MOV_move(&from, &to);
			CLEAR_NULL(record, id);
			}
		}
}

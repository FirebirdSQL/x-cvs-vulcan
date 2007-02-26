/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		RsbOuterMerge.cpp
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
 
#include <errno.h>
#include "fbdev.h"
#include "ibase.h"
#include "RsbOuterMerge.h"
#include "RsbMerge.h"
#include "RsbSort.h"
#include "jrd.h"
#include "rse.h"
#include "Request.h"
#include "Procedure.h"
#include "Relation.h"
#include "CompilerScratch.h"
#include "req.h"
#include "sort.h"
#include "sort_mem.h"
#include "ExecutionPathInfoGen.h"
#include "../jrd/mov_proto.h"
#include "../jrd/sort_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/vio_proto.h"
#include "../jrd/intl.h"
#include "../jrd/intl_proto.h"
#include "../jrd/gds_proto.h"

#if defined(WIN_NT)
#include <io.h> // close
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef SMALL_FILE_NAMES
#define SCRATCH         "fb_m"
#else
#define SCRATCH         "fb_merge_"
#endif

RsbOuterMerge::RsbOuterMerge(CompilerScratch *csb, int count, bool leftJoin, jrd_nod *innerBool) : RecordSource(csb, rsb_merge)
{
	rsb_count = count;
	sortRsbs = new (csb->csb_pool) RsbSort* [count];
	sortNodes = new (csb->csb_pool) jrd_nod* [count];
	isLeftJoin = leftJoin;
	innerBoolean = innerBool;
}

RsbOuterMerge::~RsbOuterMerge(void)
{
	delete [] sortNodes;
	delete [] sortRsbs;
}

void RsbOuterMerge::open(Request* request)
{
	thread_db* tdbb = request->req_tdbb;
	IRSB_MRG impure = (IRSB_MRG) IMPURE (request, rsb_impure);
	impure->irsb_flags |= irsb_open;

	/* do two simultaneous but unrelated things in one loop */

	for (int n = 0; n < rsb_count; ++n)
		{
		/* open all the substreams for the sort-merge */

		RsbSort *sortRsb = sortRsbs[n];
		sortRsb->open(request);
		SortMap *map = sortRsb->map;
		irsb_mrg::irsb_mrg_repeat *tail = impure->irsb_mrg_rpt + n;
		
		/* Reset equality group record positions */

		tail->irsb_mrg_equal = -1;
		tail->irsb_mrg_equal_end = -1;
		tail->irsb_mrg_equal_current = -1;
		tail->irsb_mrg_last_fetched = -1;
		tail->irsb_mrg_order = tail - impure->irsb_mrg_rpt;

		MergeFile *mfb = &tail->irsb_mrg_file;
		mfb->mfb_equal_records = 0;
		mfb->mfb_current_block = 0;
		mfb->mfb_record_size = ROUNDUP_LONG(map->smb_length);
		mfb->mfb_block_size = MAX(mfb->mfb_record_size, MERGE_BLOCK_SIZE);
		mfb->mfb_blocking_factor = mfb->mfb_block_size / mfb->mfb_record_size;
		
		if (!mfb->mfb_block_data)
			mfb->mfb_block_data = reinterpret_cast <UCHAR * >(gds__alloc(mfb->mfb_block_size));
		}
}

bool RsbOuterMerge::get(Request* request, RSE_GET_MODE mode)
{
	thread_db* tdbb = request->req_tdbb;
	IRSB_MRG impure = (IRSB_MRG) IMPURE (request, rsb_impure);

	// Outer stream base variables
	RsbSort* outerRsb = sortRsbs[OM_STREAM_OUTER];
	SortMap* outerMap = outerRsb->map;
	irsb_mrg::irsb_mrg_repeat* outerTail = impure->irsb_mrg_rpt + OM_STREAM_OUTER;
	MergeFile* outerMfb = &outerTail->irsb_mrg_file;
	SLONG outerRecord = -1;

	// Inner stream base variables
	RsbSort* innerRsb = sortRsbs[OM_STREAM_INNER];
	SortMap* innerMap = innerRsb->map;
	irsb_mrg::irsb_mrg_repeat* innerTail = impure->irsb_mrg_rpt + OM_STREAM_INNER;
	MergeFile* innerMfb = &innerTail->irsb_mrg_file;
	SLONG innerRecord = -1;


	// See if we've an inner group cache result pending
	if (getMergeFetch(request, tdbb, OM_STREAM_INNER, mode))
		return true;

	// Next look if an outer group cache result pending
	if (getMergeFetch(request, tdbb, OM_STREAM_OUTER, mode))
		{
		// If the inner stream has an equal group cached, prepare 
		// the inner stream to fetch everything all over again.
		if (innerTail->irsb_mrg_equal_end >= 0)
			{
			innerTail->irsb_mrg_equal_current = 0;
			mapSortData(request, innerMap, getMergeData(tdbb, innerMfb, 0));
			}
		return true;
		}

	// Get next or already pending sort-data for the outer stream
	outerRecord = getNextMergeRecord(request, tdbb, outerRsb, outerTail, mode);
		
	// Map outer stream data into target records
	if (outerRecord >= 0)
		mapSortData(request, outerMap, getMergeData(tdbb, outerMfb, outerRecord));
	else
		setSortDataRecordsToNulls(request, outerMap);

	// Get next or already pending sort-data for the inner stream
	innerRecord = getNextMergeRecord(request, tdbb, innerRsb, innerTail, mode);

	// If both streams are dry we're done
	if ((outerRecord < 0) && ((innerRecord < 0) || isLeftJoin))
		return false;	

	// Map inner stream data into target records
	if (innerRecord >= 0)
		mapSortData(request, innerMap, getMergeData(tdbb, innerMfb, innerRecord));
	else
		setSortDataRecordsToNulls(request, innerMap);

	outerTail->irsb_mrg_last_fetched = -1;
	innerTail->irsb_mrg_last_fetched = -1;
	if ((outerRecord >= 0) && (innerRecord >= 0))
		{

		int compareResult = compare(request, tdbb, 
				sortNodes[OM_STREAM_OUTER], sortNodes[OM_STREAM_INNER]);

		while (isLeftJoin && (compareResult > 0) && (innerRecord >= 0))
			{
			innerRecord = getNextMergeRecord(request, tdbb, innerRsb, innerTail, mode);

			if (innerRecord >= 0)
				{
				mapSortData(request, innerMap, getMergeData(tdbb, innerMfb, innerRecord));

				compareResult = compare(request, tdbb, 
					sortNodes[OM_STREAM_OUTER], sortNodes[OM_STREAM_INNER]);

				// When done and last fetched record is not on first position 
				// in the cache, moved it to the first position.
				if ((compareResult <= 0) && (innerRecord > 0))
					{
					innerTail->irsb_mrg_last_fetched = innerRecord;
					innerRecord = getNextMergeRecord(request, tdbb, innerRsb, innerTail, mode);
					}

				}
			else
				compareResult = -1;
			}

		if ((compareResult == 0) && innerBoolean && !EVL_boolean(tdbb, innerBoolean))
			compareResult = -1;	

		if (compareResult < 0)
			{
			// Outer + Inner as NULLs
			setSortDataRecordsToNulls(request, innerMap);

			// next repeating equals Outer
			innerTail->irsb_mrg_last_fetched = innerRecord;
			pushEqualRecordsInMergeCache(request, tdbb, OM_STREAM_OUTER, mode);
			}
		else if (compareResult > 0)
			{
			// Outer as NULLs + Inner
			setSortDataRecordsToNulls(request, outerMap);

			// next repeating equals Inner
			outerTail->irsb_mrg_last_fetched = outerRecord;
			pushEqualRecordsInMergeCache(request, tdbb, OM_STREAM_INNER, mode);
			}
		else 
			{
			// Outer + Inner
			// next repeating equals Inner + 
			// next repeating equals Outer
			pushEqualRecordsInMergeCache(request, tdbb, OM_STREAM_OUTER, mode);
			pushEqualRecordsInMergeCache(request, tdbb, OM_STREAM_INNER, mode);
			}
		}

	return true;
}

void RsbOuterMerge::close(Request* request)
{
	//thread_db* tdbb = request->req_tdbb;
	IRSB_MRG impure = (IRSB_MRG) IMPURE (request, rsb_impure);
	
	if (!(impure->irsb_flags & irsb_open))
		return;

	impure->irsb_flags &= ~irsb_open;

	
	//for (RecordSource** const end = ptr + rsb_count * 2; ptr < end; ptr += 2, tail++)
	for (int n = 0; n < rsb_count; ++n)
		{
		irsb_mrg::irsb_mrg_repeat* tail = impure->irsb_mrg_rpt + n;
		/* close all the substreams for the sort-merge */

		sortRsbs[n]->close(request);
		
		/* Release memory associated with the merge file block
		   and the sort file block. Also delete the merge file
		   if one exists. */

		MergeFile* mfb = &tail->irsb_mrg_file;
		SortWorkFile* sfb = mfb->mfb_sfb;
		
		if (sfb) 
			{
			if (sfb->sfb_file_name) 
				{
				::close(sfb->sfb_file);
				unlink(sfb->sfb_file_name);
				gds__free(sfb->sfb_file_name);
				}
				
			delete sfb->sfb_mem;
			delete sfb;
			mfb->mfb_sfb = 0;
			}
			
		if (mfb->mfb_block_data) 
			{
			gds__free(mfb->mfb_block_data);
			mfb->mfb_block_data = 0;
			}
		}
}

bool RsbOuterMerge::getExecutionPathInfo(Request* request, ExecutionPathInfoGen* infoGen)
{
	if (!infoGen->putBegin())
		return false;

	if (!infoGen->putType(isc_info_rsb_merge))
		return false;

	if (!infoGen->putByte((UCHAR) rsb_count))
		return false;
	
	for (int n = 0; n < rsb_count; ++n)
		if (!sortRsbs[n]->getExecutionPathInfo(request, infoGen))
			return false;

	if (rsb_next)
		if (!rsb_next->getExecutionPathInfo(request, infoGen))
			return false;

	return infoGen->putEnd();
}

bool RsbOuterMerge::getMergeFetch(Request* request, thread_db* tdbb, SSHORT stream, RSE_GET_MODE mode)
{
	IRSB_MRG impure = (IRSB_MRG) IMPURE (request, rsb_impure);
	irsb_mrg::irsb_mrg_repeat* tail = impure->irsb_mrg_rpt + stream;
	SLONG record = tail->irsb_mrg_equal_current;
	++record;

	if (record > tail->irsb_mrg_equal_end) 
		{
		tail->irsb_mrg_equal = tail->irsb_mrg_equal_current;
		return false;
		}

	tail->irsb_mrg_equal_current = record;
	MergeFile* mfb = &tail->irsb_mrg_file;
	RsbSort *sortRsb = sortRsbs[stream];
	mapSortData(tdbb->tdbb_request, sortRsb->map, getMergeData(tdbb, mfb, record));

	return true;
}

UCHAR* RsbOuterMerge::getMergeData(thread_db* tdbb, MergeFile* mfb, SLONG record)
{
	const ULONG merge_block = record / mfb->mfb_blocking_factor;
	
	if (merge_block != mfb->mfb_current_block) 
		mfb->mfb_current_block = readMergeBlock(tdbb, mfb, merge_block);

	const ULONG merge_offset = (record % mfb->mfb_blocking_factor) * mfb->mfb_record_size;
	
	return (mfb->mfb_block_data + merge_offset);
}

int RsbOuterMerge::compare(Request* request, thread_db* tdbb, jrd_nod* node1, jrd_nod* node2)
{
	jrd_nod* const* ptr1 = node1->nod_arg;
	jrd_nod* const* ptr2 = node2->nod_arg;
	
	for (const jrd_nod* const* const end = ptr1 + node1->nod_count; ptr1 < end; ptr1++, ptr2++)
		{
		const dsc* desc1 = EVL_expr(tdbb, *ptr1);
		const ULONG flags = request->req_flags;
		const dsc* desc2 = EVL_expr(tdbb, *ptr2);
		
		if (flags & req_null) 
			{
			if (!(request->req_flags & req_null)) 
				return -1;
			else 
				{
				// AB: When both expression evaluated NULL then
				// we return -1 ( (NULL = NULL) = false).
				return -1;
				}
			}
		else if (request->req_flags & req_null) 
			return 1;
		
		// AB: MOV_compare can't handle NULL parameters
		// therefore check before passing all null flags.
		
		const SSHORT result = MOV_compare(tdbb, desc1, desc2);
		
		if (result != 0) 
			return result;
		}

	return 0;
}

SLONG RsbOuterMerge::getMergeRecord(Request* request, thread_db* tdbb, RsbSort* rsb, irsb_mrg::irsb_mrg_repeat * impure, RSE_GET_MODE mode)
{
	IRSB_SORT sortImpure = (IRSB_SORT) IMPURE (request, rsb->rsb_impure);
	UCHAR *sort_data, *merge_data;

	/* Get address of record from sort.  If the address if null, we
	  ran out of records.  This is known in the trade as "end of file." */

#ifdef SCROLLABLE_CURSORS
	SORT_get(tdbb-, sortImpure->irsb_sort_handle, (ULONG **) &sort_data, mode);
#else
	SORT_get(tdbb, sortImpure->irsb_sort_handle, (ULONG **) &sort_data);
#endif

	if (!sort_data)
		return -1;

	SortMap *map = rsb->map;
	MergeFile* mfb = &impure->irsb_mrg_file;
	SLONG record = mfb->mfb_equal_records;

	ULONG merge_block = record / mfb->mfb_blocking_factor;
	
	if (merge_block != mfb->mfb_current_block)
		{
		writeMergeBlock(tdbb, mfb, mfb->mfb_current_block);
		mfb->mfb_current_block = merge_block;
		}

	ULONG merge_offset = (record % mfb->mfb_blocking_factor) * mfb->mfb_record_size;
	merge_data = mfb->mfb_block_data + merge_offset;

	MOVE_FASTER(sort_data, merge_data, map->smb_length);
	++mfb->mfb_equal_records;

#ifdef SCROLLABLE_CURSORS
/* fix up the sort data in case we need to retrieve it again */

	unget_sort(tdbb, rsb, sort_data);
#endif

	return record;
}

SLONG RsbOuterMerge::getNextMergeRecord(Request* request, thread_db* tdbb, RsbSort* rsb, irsb_mrg::irsb_mrg_repeat* impure, RSE_GET_MODE mode)
{
	MergeFile* mfb = &impure->irsb_mrg_file;

	impure->irsb_mrg_equal = -1;
	impure->irsb_mrg_equal_current = -1;
	impure->irsb_mrg_equal_end = -1;

	// Get next or already pending sort-data for the inner stream
	SLONG record = impure->irsb_mrg_last_fetched;
	if (record >= 0)
		{
		impure->irsb_mrg_last_fetched = -1;
		mfb->mfb_current_block = 0;
		if (record > 0)
			{
			// Move our last fetched sort-data to the first position in cache
			UCHAR* last_data = getMergeData(tdbb, mfb, record);
			UCHAR* first_data = getMergeData(tdbb, mfb, 0);
			MOVE_FASTER(last_data, first_data, rsb->map->smb_length);				
			mfb->mfb_equal_records = 1;
			record = 0;
			}
		}
	else
		record = getMergeRecord(request, tdbb, rsb, impure, mode);

	return record;
}

int RsbOuterMerge::compareLongs(const SLONG* p, const SLONG* q, int count)
{
	for (; count; p++, q++, --count)
		if (*p > *q)
			return 1;
		else if (*p < *q)
			return -1;

	return 0;
}

void RsbOuterMerge::pushEqualRecordsInMergeCache(Request* request, thread_db* tdbb, SSHORT stream, RSE_GET_MODE mode)
{
	// Compute equality group and put in merge cache
	IRSB_MRG impure = (IRSB_MRG) IMPURE (request, rsb_impure);
	ULONG key[64];

	RsbSort* sortRsb = sortRsbs[stream];
	SortMap* map = sortRsb->map;
	irsb_mrg::irsb_mrg_repeat* tail = impure->irsb_mrg_rpt + stream;
	MergeFile* mfb = &tail->irsb_mrg_file;
	ULONG key_length = map->smb_key_length * sizeof(ULONG);
	SLONG record;
	UCHAR* first_data;
	
	if (key_length > sizeof(key))
		first_data = (UCHAR*) gds__alloc(key_length);
	else
		first_data = (UCHAR*) key;
		
	MOVE_FASTER(getMergeData(tdbb, mfb, 0), first_data, key_length);
	tail->irsb_mrg_equal_current = 0;

	while ((record = getMergeRecord(request, tdbb, sortRsb, tail, mode)) >= 0)
		{
		if (compareLongs((SLONG *) first_data,
				(SLONG*) getMergeData(tdbb, mfb, record), map->smb_key_length))
			{
			tail->irsb_mrg_last_fetched = record;
			break;
			}

		tail->irsb_mrg_equal_end = record;
		}

	if (first_data != (UCHAR*) key)
		gds__free(first_data);
		
	if (mfb->mfb_current_block)
		writeMergeBlock(tdbb, mfb, mfb->mfb_current_block);
}

void RsbOuterMerge::setSortDataRecordsToNulls(Request* request, SortMap* map)
{
	StreamStack streams;

	for (smb_repeat *item = map->smb_rpt, *end_item = item + map->smb_count; item < end_item; item++) 
		{
		if (item->smb_node && item->smb_node->nod_type != nod_field)
			continue;

		bool exists = false;
		StreamStack::iterator iter(streams);
		for (;iter.hasData(); ++iter)
			if (item->smb_stream == iter.object())
				{
				exists = true;
				break;
				}

		if (!exists)
			streams.push(item->smb_stream);
		}

	setRecordsToNulls(request, &streams);	
}

void RsbOuterMerge::writeMergeBlock(thread_db* tdbb, MergeFile* mfb, ULONG block)
{
	SortWorkFile* sfb_ = mfb->mfb_sfb;
	
	if (!sfb_) 
		{
		sfb_ = mfb->mfb_sfb = FB_NEW(*getDefaultMemoryPool()) SortWorkFile (tdbb->tdbb_database);
		//memset(sfb_, 0, sizeof(struct sfb));
		}
		
	if (!sfb_->sfb_file_name) 
		{
		TEXT file_name[128];
		sfb_->sfb_file = gds__temp_file(SCRATCH, file_name);
		
		if (sfb_->sfb_file == -1)
			SORT_error(sfb_, "open", isc_io_error, errno);
			
		sfb_->sfb_file_name = (SCHAR*) gds__alloc((ULONG) (strlen(file_name) + 1));
			
		strcpy(sfb_->sfb_file_name, file_name);
		sfb_->sfb_mem = FB_NEW (*getDefaultMemoryPool()) SortMem(sfb_, mfb->mfb_block_size);
		}

	sfb_->sfb_mem->write(mfb->mfb_block_size * block,
						 reinterpret_cast<char*>(mfb->mfb_block_data),
						 mfb->mfb_block_size);
}

ULONG RsbOuterMerge::readMergeBlock(thread_db* tdbb, MergeFile* mfb, ULONG block)
{
	fb_assert(mfb->mfb_sfb && mfb->mfb_sfb->sfb_file_name);

	mfb->mfb_sfb->sfb_mem->read(mfb->mfb_block_size * block,
								reinterpret_cast<char*>(mfb->mfb_block_data),
								mfb->mfb_block_size);

	return block;
}

void RsbOuterMerge::findRsbs(StreamStack* stream_list, RsbStack* rsb_list)
{
	for (int n = 0; n < rsb_count; ++n)
		sortRsbs[n]->findRsbs(stream_list, rsb_list);
}

void RsbOuterMerge::pushRecords(Request* request)
{
	SSHORT i, streams[128];
	smb_repeat * item, *end_item;

	for (i = 0; i < (SSHORT) request->req_count; i++)
		streams[i] = 0;
		
	IRSB_MRG impure = (IRSB_MRG) IMPURE (request, rsb_impure);
	irsb_mrg::irsb_mrg_repeat * tail = impure->irsb_mrg_rpt;
	
	for (i = 0; i <rsb_count; ++i, ++tail)
		{
		RsbSort *sort_rsb = sortRsbs[i];
		SortMap* map = sort_rsb->map;
		end_item = map->smb_rpt + map->smb_count;
		
		for (item = map->smb_rpt; item < end_item; item++)
			streams[item->smb_stream] = 1;
		}
		
	for (i = 0; i < (SSHORT) request->req_count; i++)
		if (streams[i]) 
			{
			record_param *rpb = request->req_rpb + i;
			saveRecord(request, rpb);
			}
					
}

void RsbOuterMerge::popRecords(Request* request)
{
	SSHORT i, streams[128];
	smb_repeat * item, *end_item;

	for (i = 0; i < (SSHORT) request->req_count; i++)
		streams[i] = 0;
		
	IRSB_MRG impure = (IRSB_MRG) IMPURE (request, rsb_impure);
	irsb_mrg::irsb_mrg_repeat * tail = impure->irsb_mrg_rpt;
	
	for (i = 0; i < rsb_count; ++i, ++tail)
		{
		RsbSort *sort_rsb = sortRsbs[i];
		SortMap* map = sort_rsb->map;
		end_item = map->smb_rpt + map->smb_count;
		
		for (item = map->smb_rpt; item < end_item; item += 2)
			streams[item->smb_stream] = 1;
		}
		
	for (i = 0; i < (SSHORT) request->req_count; i++)
		if (streams[i]) 
			{
			record_param *rpb = request->req_rpb + i;
			restoreRecord(rpb);
			}
}

/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		RsbMerge.cpp
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
#include "firebird.h"
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

#ifdef SMALL_FILE_NAMES
#define SCRATCH         "fb_m"
#else
#define SCRATCH         "fb_merge_"
#endif


RsbMerge::RsbMerge(CompilerScratch *csb, int count) : RecordSource(csb, rsb_merge)
{
	rsb_count = count;
	sortRsbs = new (csb->csb_pool) RsbSort* [count];
	sortNodes = new (csb->csb_pool) jrd_nod* [count];
}

RsbMerge::~RsbMerge(void)
{
	delete [] sortNodes;
	delete [] sortRsbs;
}

void RsbMerge::open(Request* request)
{
	thread_db* tdbb = request->req_tdbb;
	IRSB_MRG impure = (IRSB_MRG) IMPURE (request, rsb_impure);

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

bool RsbMerge::get(Request* request, RSE_GET_MODE mode)
{
	thread_db* tdbb = request->req_tdbb;
	IRSB_MRG impure = (IRSB_MRG) IMPURE (request, rsb_impure);
	SLONG record;
	int result;
	UCHAR *first_data, *last_data;
	LLS best_tails;

	/* If there is a record group already formed, fetch the next combination */

	if (getMergeFetch(request, tdbb, rsb_count - 1, mode))
		return TRUE;

	/* Assuming we are done with the current value group, advance each
	   stream one record.  If any comes up dry, we're done. */

	//RsbSort** highest_ptr = sortRsbs;
	int highestPtr = 0;
	int n;
	
	//for (ptr = sortRsbs, tail = impure->irsb_mrg_rpt; ptr < end; ptr += 2, tail++)
	for (n = 0; n < rsb_count; ++n)
		{
		//sort_rsb = *ptr;
		//map = (SortMap*) sort_rsb->rsb_arg[0];
		RsbSort *sort_rsb = sortRsbs[n];
		SortMap *map = sort_rsb->map;
		irsb_mrg::irsb_mrg_repeat *tail = impure->irsb_mrg_rpt + n;
		MergeFile *mfb = &tail->irsb_mrg_file;

		/* reset equality group record positions */

		tail->irsb_mrg_equal = 0;
		tail->irsb_mrg_equal_current = 0;
		tail->irsb_mrg_equal_end = 0;

		/* If there is a record waiting, use it.  Otherwise get another */

		if ((record = tail->irsb_mrg_last_fetched) >= 0) 
			{
			tail->irsb_mrg_last_fetched = -1;
			last_data = getMergeData(tdbb, mfb, record);
			mfb->mfb_current_block = 0;
			first_data = getMergeData(tdbb, mfb, 0);
			
			if (first_data != last_data)
				MOVE_FASTER(last_data, first_data, map->smb_length);
				
			mfb->mfb_equal_records = 1;
			record = 0;
			}
		else 
			{
			mfb->mfb_current_block = 0;
			mfb->mfb_equal_records = 0;
			
			if ((record = getMergeRecord(request, tdbb, sort_rsb, tail, mode)) < 0)
				return FALSE;
			}

		/* Map data into target records and do comparison */

		mapSortData(request, map, getMergeData(tdbb, mfb, record));
		
		/***
		if (ptr != highest_ptr &&
			compare(request, tdbb, (JRD_NOD) highest_ptr[1], (JRD_NOD) ptr[1]) < 0)
			highest_ptr = ptr;
		***/
		
		if (n != highestPtr && compare(request, tdbb, sortNodes[highestPtr], sortNodes[n]))
			highestPtr = n;
		}

	/* Loop thru the streams advancing each up to the target value.  If any
	   exceeds the target value, start over */

	for (;;)
		{
		//for (ptr = sortRsbs, tail = impure->irsb_mrg_rpt; ptr < end; ptr += 2, tail++)
		for (n = 0; n < rsb_count; ++n)
			{
			if (highestPtr != n)
				{
				irsb_mrg::irsb_mrg_repeat *tail = impure->irsb_mrg_rpt + n;
				
				//while ( (result = compare(request, tdbb, (JRD_NOD) highest_ptr[1], (JRD_NOD) ptr[1])) )
				while ( (result = compare(request, tdbb, sortNodes[highestPtr], sortNodes[n])) )
					{
					if (result < 0) 
						{
						highestPtr = n;
						goto recycle;
						}
					
					RsbSort *sort_rsb = sortRsbs[n];	
					MergeFile *mfb = &tail->irsb_mrg_file;
					mfb->mfb_current_block = 0;
					mfb->mfb_equal_records = 0;

					if ((record = getMergeRecord(request, tdbb, sort_rsb, tail, mode)) < 0)
						return FALSE;
						
					mapSortData(request, sort_rsb->map, getMergeData(tdbb, mfb, record));
					}
				}
			}
		break;
		recycle:;
		}

	/* Finally compute equality group for each stream in sort/merge */

	//for (ptr = sortRsbs, tail = impure->irsb_mrg_rpt; ptr < end; ptr += 2, tail++)
	for (n = 0; n < rsb_count; ++n)
		{
		ULONG key[64];

		RsbSort *sort_rsb = sortRsbs[n];
		SortMap *map = sort_rsb->map;
		irsb_mrg::irsb_mrg_repeat *tail = impure->irsb_mrg_rpt + n;
		MergeFile *mfb = &tail->irsb_mrg_file;
		ULONG key_length = map->smb_key_length * sizeof(ULONG);
		
		if (key_length > sizeof(key))
			first_data = (UCHAR*) gds__alloc(key_length);
		else
			first_data = (UCHAR*) key;
			
		MOVE_FASTER(getMergeData(tdbb, mfb, 0), first_data, key_length);

		while ((record = getMergeRecord(request, tdbb, sort_rsb, tail, mode)) >= 0)
			{
			if (compareLongs((SLONG *) first_data,
							  (SLONG *) getMergeData(tdbb, mfb, record),
							  map->smb_key_length))
				{
				tail->irsb_mrg_last_fetched = record;
				break;
				}

			tail->irsb_mrg_equal_end = record;
			}

		if (first_data != (UCHAR *) key)
			gds__free(first_data);
			
		if (mfb->mfb_current_block)
			writeMergeBlock(tdbb, mfb, mfb->mfb_current_block);
		}

	/* Optimize cross product of equivalence groups by ordering the streams
	   from left (outermost) to right (innermost) by descending cardinality
	   of merge blocks. This ordering will vary for each set of equivalence
	   groups and cannot be statically assigned by the optimizer. */

	best_tails = 0;

	//for (irsb_mrg::irsb_mrg_repeat *tail = impure->irsb_mrg_rpt, tail_end = tail + rsb_count; tail < tail_end; tail++)
	for (n = 0; n < rsb_count; ++n)
		{
		irsb_mrg::irsb_mrg_repeat *tail = impure->irsb_mrg_rpt + n;
		irsb_mrg::irsb_mrg_repeat *best_tail;
		ULONG blocks;
		LLS stack;
		ULONG most_blocks = 0;
		
		//for (tail2 = impure->irsb_mrg_rpt; tail2 < tail_end; tail2++)
		for (int n2 = 0; n < rsb_count; ++n)
			{
			irsb_mrg::irsb_mrg_repeat *tail2 = impure->irsb_mrg_rpt + n2;
			
			for (stack = best_tails; stack; stack = stack->lls_next)
				if (stack->lls_object == (BLK) tail2)
					break;
				
			if (stack) 
				continue;

			MergeFile *mfb = &tail2->irsb_mrg_file;
			blocks = mfb->mfb_equal_records / mfb->mfb_blocking_factor;
			
			if (++blocks > most_blocks) 
				{
				most_blocks = blocks;
				best_tail = tail2;
				}
			}

		LLS_PUSH(best_tail, &best_tails);
		tail->irsb_mrg_order = best_tail - impure->irsb_mrg_rpt;
		}

	while (best_tails) 
		LLS_POP(&best_tails);

	return TRUE;
}

void RsbMerge::close(Request* request)
{
	//thread_db* tdbb = request->req_tdbb;
	IRSB_MRG impure = (IRSB_MRG) IMPURE (request, rsb_impure);
	
	//for (RecordSource** const end = ptr + rsb_count * 2; ptr < end; ptr += 2, tail++)
	for (int n = 0; n < rsb_count; ++n)
		{
		irsb_mrg::irsb_mrg_repeat* tail = impure->irsb_mrg_rpt + n;
		/* close all the substreams for the sort-merge */

		//RSE_close(tdbb, *ptr);
		sortRsbs[n]->close(request);
		
		/* Release memory associated with the merge file block
		   and the sort file block. Also delete the merge file
		   if one exists. */

		MergeFile* mfb = &tail->irsb_mrg_file;
		sort_work_file* sfb = mfb->mfb_sfb;
		
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

bool RsbMerge::getExecutionPathInfo(Request* request, ExecutionPathInfoGen* infoGen)
{
	return false;
}

bool RsbMerge::getMergeFetch(Request* request, thread_db* tdbb, SSHORT stream, RSE_GET_MODE mode)
{
	IRSB_MRG impure = (IRSB_MRG) IMPURE (request, rsb_impure);
	irsb_mrg::irsb_mrg_repeat* tail = impure->irsb_mrg_rpt + stream;
	SSHORT m = tail->irsb_mrg_order;
	tail = impure->irsb_mrg_rpt + m;
	RsbSort *sub_rsb = sortRsbs[m];
	SLONG record = tail->irsb_mrg_equal_current;
	++record;

	if (record > tail->irsb_mrg_equal_end) 
		{
		if (stream == 0 || !getMergeFetch(request, tdbb,stream - 1, mode))
			return false;
			
		record = tail->irsb_mrg_equal;
		}

	tail->irsb_mrg_equal_current = record;
	MergeFile* mfb = &tail->irsb_mrg_file;
	mapSortData(tdbb->tdbb_request, sub_rsb->map, getMergeData(tdbb, mfb, record));

	return true;
}

UCHAR* RsbMerge::getMergeData(thread_db* tdbb, MergeFile* mfb, SLONG record)
{
	const ULONG merge_block = record / mfb->mfb_blocking_factor;
	
	if (merge_block != mfb->mfb_current_block) 
		mfb->mfb_current_block = readMergeBlock(tdbb, mfb, merge_block);

	const ULONG merge_offset = (record % mfb->mfb_blocking_factor) * mfb->mfb_record_size;
	
	return (mfb->mfb_block_data + merge_offset);
}

int RsbMerge::compare(Request* request, thread_db* tdbb, jrd_nod* node1, jrd_nod* node2)
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
				// we return 0 ( (NULL = NULL) = true).
				//
				// Currently this (0 and higher) isn't used by the 
				// MERGE procedure, but when we allow MERGE to 
				// handle outer-joins we must not forget this one !!!
				return 0;
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

SLONG RsbMerge::getMergeRecord(Request* request, thread_db* tdbb, RsbSort* rsb, irsb_mrg::irsb_mrg_repeat * impure, RSE_GET_MODE mode)
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

int RsbMerge::compareLongs(const SLONG* p, const SLONG* q, int count)
{
	for (; count; p++, q++, --count)
		if (*p > *q)
			return 1;
		else if (*p < *q)
			return -1;

	return 0;
}

void RsbMerge::writeMergeBlock(thread_db* tdbb, MergeFile* mfb, ULONG block)
{
	sort_work_file* sfb_ = mfb->mfb_sfb;
	
	if (!sfb_) 
		{
		sfb_ = mfb->mfb_sfb = FB_NEW(*getDefaultMemoryPool()) sort_work_file (tdbb->tdbb_database);
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

ULONG RsbMerge::readMergeBlock(thread_db* tdbb, MergeFile* mfb, ULONG block)
{
	fb_assert(mfb->mfb_sfb && mfb->mfb_sfb->sfb_file_name);

	mfb->mfb_sfb->sfb_mem->read(mfb->mfb_block_size * block,
								reinterpret_cast<char*>(mfb->mfb_block_data),
								mfb->mfb_block_size);

	return block;
}

void RsbMerge::findRsbs(StreamStack* stream_list, RsbStack* rsb_list)
{
	for (int n = 0; n < rsb_count; ++n)
		sortRsbs[n]->findRsbs(stream_list, rsb_list);
}

void RsbMerge::pushRecords(Request* request)
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
			//save_record(tdbb, rpb);
			saveRecord(request, rpb);
			}
					
}

void RsbMerge::popRecords(Request* request)
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

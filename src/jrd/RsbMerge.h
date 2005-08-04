/*
 *	PROGRAM:		JRD Access Method
 *	MODULE:			RsbMerge.h
 *	DESCRIPTION:	Record source block definitions
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

#ifndef JRD_RSE_MERGE_H
#define JRD_RSE_MERGE_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "RecordSource.h"

// Merge (equivalence) file block

struct MergeFile 
{
	class sort_work_file* mfb_sfb;				// merge file uses SORT I/O routines
	ULONG mfb_equal_records;			// equality group cardinality
	ULONG mfb_record_size;				// matches sort map length
	ULONG mfb_current_block;			// current merge block in buffer
	ULONG mfb_block_size;				// merge block I/O size
	ULONG mfb_blocking_factor;			// merge equality records per block
	UCHAR *mfb_block_data;				// merge block I/O buffer
};

const ULONG MERGE_BLOCK_SIZE	= 65536;



struct irsb_mrg {
	ULONG irsb_flags;
	USHORT irsb_mrg_count;				// next stream in group
	struct irsb_mrg_repeat {
		SLONG irsb_mrg_equal;			// queue of equal records
		SLONG irsb_mrg_equal_end;		// end of the equal queue
		SLONG irsb_mrg_equal_current;	// last fetched record from equal queue
		SLONG irsb_mrg_last_fetched;	// first sort merge record of next group
		SSHORT irsb_mrg_order;			// logical merge order by substream
		MergeFile irsb_mrg_file;		// merge equivalence file
	} irsb_mrg_rpt[1];
};

typedef irsb_mrg *IRSB_MRG;

class RsbSort;

class RsbMerge : public RecordSource
{
public:
	jrd_nod		**sortNodes;
	RsbSort		**sortRsbs;
	
	RsbMerge(CompilerScratch *csb, int count);
	virtual ~RsbMerge(void);
	virtual void open(Request* request);
	virtual bool get(Request* request, RSE_GET_MODE mode);
	virtual bool getExecutionPathInfo(Request* request, ExecutionPathInfoGen* infoGen);
	virtual void close(Request* request);
	virtual void pushRecords(Request* request);
	virtual void popRecords(Request* request);
	virtual void findRsbs(StreamStack* stream_list, RsbStack* rsb_list);

	bool getMergeFetch(Request* request, thread_db* tdbb, SSHORT stream, RSE_GET_MODE mode);
	UCHAR* getMergeData(thread_db* tdbb, MergeFile* mfb, SLONG record);
	static int compare(Request* request, thread_db* tdbb, jrd_nod* node1, jrd_nod* node2);
	SLONG getMergeRecord(Request* request, thread_db* tdbb, RsbSort* rsb, irsb_mrg::irsb_mrg_repeat * impure, RSE_GET_MODE mode);
	static int compareLongs(const SLONG* p, const SLONG* q, int count);
	static void writeMergeBlock(thread_db* tdbb, MergeFile* mfb, ULONG block);
	static ULONG readMergeBlock(thread_db* tdbb, MergeFile* mfb, ULONG block);
};

#endif


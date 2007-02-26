/*
 *	PROGRAM:		JRD Access Method
 *	MODULE:			RsbOuterMerge.h
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

#ifndef JRD_RSE_OUTER_MERGE_H
#define JRD_RSE_OUTER_MERGE_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "RecordSource.h"
#include "RsbMerge.h"

#define OM_STREAM_OUTER 0
#define OM_STREAM_INNER 1

class SortWorkFile;
class RsbSort;

class RsbOuterMerge : public RecordSource
{
public:
	jrd_nod** sortNodes;
	RsbSort** sortRsbs;
	
	RsbOuterMerge(CompilerScratch* csb, int count, bool leftJoin, jrd_nod *innerBool);
	virtual ~RsbOuterMerge(void);
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
	SLONG getMergeRecord(Request* request, thread_db* tdbb, RsbSort* rsb, irsb_mrg::irsb_mrg_repeat* impure, RSE_GET_MODE mode);
	SLONG getNextMergeRecord(Request* request, thread_db* tdbb, RsbSort* rsb, irsb_mrg::irsb_mrg_repeat* impure, RSE_GET_MODE mode);
	static int compareLongs(const SLONG* p, const SLONG* q, int count);
	void pushEqualRecordsInMergeCache(Request* request, thread_db* tdbb, SSHORT stream, RSE_GET_MODE mode);
	static void setSortDataRecordsToNulls(Request* request, SortMap* map);
	static void writeMergeBlock(thread_db* tdbb, MergeFile* mfb, ULONG block);
	static ULONG readMergeBlock(thread_db* tdbb, MergeFile* mfb, ULONG block);

	jrd_nod* innerBoolean;
	bool isLeftJoin;
};

#endif


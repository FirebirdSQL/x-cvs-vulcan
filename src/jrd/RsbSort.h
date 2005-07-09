/*
 *	PROGRAM:		JRD Access Method
 *	MODULE:			RsbSort.h
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
 * Refactored July 1, 2005 by James A. Starkey
 */

#ifndef JRD_RSE_SORT_H
#define JRD_RSE_SORT_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "RecordSource.h"

struct sort_context;

struct irsb_sort {
	ULONG		  irsb_flags;
	sort_context* irsb_sort_handle;
};

typedef irsb_sort *IRSB_SORT;


class RsbSort : public RecordSource
{
public:
	RsbSort(CompilerScratch *csb, RecordSource *source, SortMap *sortMap);
	virtual ~RsbSort(void);
	virtual void open(Request* request, thread_db* tdbb);
	virtual bool get(Request* request, thread_db* tdbb, RSE_GET_MODE mode);
	virtual void close(Request* request, thread_db* tdbb);
	static bool reject(const UCHAR* record_a, const UCHAR* record_b, void* user_arg);
	
	SortMap		*map;
};

#endif


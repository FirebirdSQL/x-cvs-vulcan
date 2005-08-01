/*
 *	PROGRAM:		JRD Access Method
 *	MODULE:			RsbUnion.h
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
 * Refactored July 25, 2005 by James A. Starkey
 */

#ifndef JRD_RSB_UNION_H
#define JRD_RSB_UNION_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "RecordSource.h"


class RsbUnion : public RecordSource
{
public:
	RsbUnion(CompilerScratch *csb, int streamCount, int clauseCount);
	virtual ~RsbUnion(void);
	
	RecordSource	**rsbs;
	jrd_nod			**maps;
	int				numberStreams;
	UCHAR			*streams;
	virtual void open(Request* request);
	virtual bool get(Request* request, RSE_GET_MODE mode);
	virtual void close(Request* request);
	virtual void findRsbs(StreamStack* stream_list, RsbStack* rsb_list);
	virtual void pushRecords(Request* request);
	virtual void popRecords(Request* request);
};

#endif

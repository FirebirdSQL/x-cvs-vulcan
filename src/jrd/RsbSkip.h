/*
 *	PROGRAM:		JRD Access Method
 *	MODULE:			RecordSource.h
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
 * Refactored July 29, 2005 by James A. Starkey
 */

#ifndef JRD_RSB_SKIP_H
#define JRD_RSB_SKIP_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "RecordSource.h"

struct irsb_skip_n {
    ULONG irsb_flags;
    SLONG irsb_number;
    SINT64 irsb_count;
};

typedef irsb_skip_n *IRSB_SKIP;

class RsbSkip : public RecordSource
{
public:
	RsbSkip(CompilerScratch *csb, RecordSource* prior_rsb, jrd_nod* node);
	virtual ~RsbSkip(void);
	virtual void open(Request* request);
	virtual bool get(Request* request, RSE_GET_MODE mode);
	virtual void close(Request* request);
	
	jrd_nod		*valueNode;
};

#endif

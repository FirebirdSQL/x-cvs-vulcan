/*
 *	PROGRAM:		JRD Access Method
 *	MODULE:			RsbIndexed.h
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
 * Refactored July 22, 2005 by James A. Starkey
 */

#ifndef JRD_RSB_INDEXED_H
#define JRD_RSB_INDEXED_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "RsbSequential.h"
#include "sbm.h"


struct irsb_index {
	ULONG irsb_flags;
	SLONG irsb_number;
	SLONG irsb_prefetch_number;
	RecordBitmap** irsb_bitmap;
};

typedef irsb_index *IRSB_INDEX;

struct jrd_nod;

class RsbIndexed : public RsbSequential
{
public:
	RsbIndexed(CompilerScratch *csb, int stream, Relation *relation, str *alias, jrd_nod *inversion);
	RsbIndexed(CompilerScratch* csb, RSB_T type, int stream, Relation* relation, str* alias, jrd_nod *node);

	virtual ~RsbIndexed(void);
	virtual void open(Request* request);
	virtual bool get(Request* request, RSE_GET_MODE mode);
	virtual bool getExecutionPathInfo(Request* request, ExecutionPathInfoGen* infoGen);
	bool getExecutionPathInfo(Request* request, ExecutionPathInfoGen* infoGen, const jrd_nod* node);
	virtual void close(Request* request);
	
	jrd_nod* inversion;
};

#endif

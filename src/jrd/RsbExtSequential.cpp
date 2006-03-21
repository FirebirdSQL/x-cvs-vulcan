/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		RsbExtSequential.cpp
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
 * Refactored July 29, 2005 by James A. Starkey
 */
 
#include "fbdev.h"
#include "ibase.h"
#include "RsbExtSequential.h"
#include "jrd.h"
#include "rse.h"
#include "Request.h"
#include "CompilerScratch.h"
#include "Relation.h"
#include "req.h"
#include "ExecutionPathInfoGen.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/ext_proto.h"

RsbExtSequential::RsbExtSequential(CompilerScratch *csb, int stream, Relation *relation, str *alias)
		: RsbSequential(csb, rsb_ext_sequential, stream, relation, alias)
{
	rsb_impure = CMP_impure(csb, sizeof(struct irsb));
}

RsbExtSequential::~RsbExtSequential(void)
{
}

void RsbExtSequential::open(Request* request)
{
	IRSB impure = (IRSB) IMPURE (request, rsb_impure);
	impure->irsb_flags |= irsb_open;
	EXT_open(request->req_tdbb, this);
}

bool RsbExtSequential::get(Request* request, RSE_GET_MODE mode)
{
	return EXT_get(request->req_tdbb, this);
}

bool RsbExtSequential::getExecutionPathInfo(Request* request, ExecutionPathInfoGen* infoGen)
{
	if (!infoGen->putBegin())
		return false;

	if (!infoGen->putType(isc_info_rsb_ext_sequential))
		return false;

	if (rsb_next)
		if (!rsb_next->getExecutionPathInfo(request, infoGen))
			return false;

	return infoGen->putEnd();
}

void RsbExtSequential::close(Request* request)
{
	IRSB impure = (IRSB) IMPURE (request, rsb_impure);

	if (!(impure->irsb_flags & irsb_open))
		return;

	impure->irsb_flags &= ~irsb_open;

	EXT_close(request->req_tdbb, this);
}

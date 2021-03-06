/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		RsbExtDbkey.cpp
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
#include "RsbExtDbkey.h"
#include "jrd.h"
#include "rse.h"
#include "Request.h"
#include "CompilerScratch.h"
#include "Relation.h"
#include "req.h"
#include "ExecutionPathInfoGen.h"
#include "../jrd/cmp_proto.h"

RsbExtDbkey::RsbExtDbkey(CompilerScratch *csb, int stream, Relation *relation, str *alias, jrd_nod *node)
		: RsbIndexed(csb, rsb_ext_dbkey, stream, relation, alias, node)
{
	rsb_impure = CMP_impure(csb, sizeof(struct irsb_index));
}

RsbExtDbkey::~RsbExtDbkey(void)
{
}

void RsbExtDbkey::open(Request* request)
{
}

bool RsbExtDbkey::get(Request* request, RSE_GET_MODE mode)
{
	return false;
}

bool RsbExtDbkey::getExecutionPathInfo(Request* request, ExecutionPathInfoGen* infoGen)
{
	if (!infoGen->putBegin())
		return false;

	if (!infoGen->putType(isc_info_rsb_ext_dbkey))
		return false;

	if (rsb_next)
		if (!rsb_next->getExecutionPathInfo(request, infoGen))
			return false;

	return infoGen->putEnd();
}

void RsbExtDbkey::close(Request* request)
{
}

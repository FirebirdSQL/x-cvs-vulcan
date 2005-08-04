/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		RsbExtIndexed.cpp
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
 
#include "firebird.h"
#include "RsbExtIndexed.h"
#include "jrd.h"
#include "rse.h"
#include "Request.h"
#include "CompilerScratch.h"
#include "Relation.h"
#include "req.h"
#include "../jrd/cmp_proto.h"

RsbExtIndexed::RsbExtIndexed(CompilerScratch *csb, int stream, Relation *relation, str *alias, jrd_nod *node)
		: RsbIndexed(csb, rsb_ext_indexed, stream, relation, alias, node)
{
	rsb_impure = CMP_impure(csb, sizeof(struct irsb_index));
}

RsbExtIndexed::~RsbExtIndexed(void)
{
}

void RsbExtIndexed::open(Request* request)
{
}

bool RsbExtIndexed::get(Request* request, RSE_GET_MODE mode)
{
	return false;
}

bool RsbExtIndexed::getExecutionPathInfo(Request* request, ExecutionPathInfoGen* infoGen)
{
	return false;
}

void RsbExtIndexed::close(Request* request)
{
}

/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		opt.cpp
 *	DESCRIPTION:	Optimizer / record selection expression compiler
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
 *  Split out from exe.h July 5, 2005 by James A. Starkey
 *
 */

// AB:Sync FB 1.236

#include "firebird.h"
#include "common.h"
#include <string.h>
#include "../jrd/ibase.h"
#include "../jrd/jrd.h"
#include "CompilerScratch.h"
#include "err_proto.h"
#include "RecordSource.h"
#include "rse.h"

CompilerScratch::CompilerScratch(MemoryPool& p, size_t len)
	:   csb_dependencies(p),
		csb_fors(p),
		csb_invariants(&p),
		csb_current_rses(&p),
		//csb_pool(p),
		csb_rpt(&p, len)

{
	csb_pool = &p;
	csb_blr = 0;
	csb_running = 0;
	csb_node = 0;
	csb_access = 0;
	csb_resources = 0;
#ifdef SCROLLABLE_CURSORS
	csb_current_rse = 0;
#endif
	csb_async_message = 0;
	//csb_count = 0;
	csb_n_stream = 0;
	csb_msg_number = 0;
	csb_impure = 0;
	csb_g_flags = 0;
	rsbs = NULL;
}

CompilerScratch::~CompilerScratch(void)
{
	for (RecordSource *rsb; rsb = rsbs;)
		{
		rsbs = rsb->nextInRequest;
		delete rsb;
		}
}

int CompilerScratch::nextStream(bool check)
{
	if (csb_n_stream >= MAX_STREAMS && check)
		ERR_post(isc_too_many_contexts, 0);
		
	return csb_n_stream++;
}

CompilerScratch* CompilerScratch::newCsb(MemoryPool & p, size_t len)
{
	return FB_NEW(p) CompilerScratch(p, len);
}

void CompilerScratch::addRsb(RecordSource* rsb)
{
	rsb->nextInRequest = rsbs;
	rsbs = rsb;
}

RecordSource* CompilerScratch::stealRsbs(void)
{
	RecordSource *temp = rsbs;
	rsbs = NULL;
	
	return temp;
}

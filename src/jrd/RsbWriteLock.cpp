/*
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
 *
 *     Software distributed under the License is distributed on 
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *     express or implied.  See the License for the specific 
 *     language governing rights and limitations under the License.
 *
 *     The contents of this file or any work derived from this file
 *     may not be distributed under any other license whatsoever 
 *     without the express prior written permission of the original 
 *     author.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 2005 James A. Starkey
 *  All Rights Reserved.
 */

#include "firebird.h"
#include "RsbWriteLock.h"
#include "jrd.h"
#include "rse.h"
#include "Request.h"
#include "CompilerScratch.h"
#include "Relation.h"
#include "req.h"
#include "../jrd/vio_proto.h"
#include "../jrd/rlck_proto.h"

RsbWriteLock::RsbWriteLock(CompilerScratch *csb, RecordSource *next, int stream) : RecordSource(csb, rsb_write_lock)
{
	rsb_stream = stream;
	rsb_next = next;
}

RsbWriteLock::~RsbWriteLock(void)
{
}

void RsbWriteLock::open(Request* request)
{
	rsb_next->open(request);
}

bool RsbWriteLock::get(Request* request, RSE_GET_MODE mode)
{
	for(;;)
		{
		if (!rsb_next->get(request, mode))
			return false;
		
		thread_db *tdbb = request->req_tdbb;
		Transaction* transaction = request->req_transaction;
		record_param* org_rpb = request->req_rpb + rsb_stream;
		Relation* relation = org_rpb->rpb_relation;
		
		if (relation && !relation->rel_view_rse && !relation->rel_file) 
			{
			RLCK_reserve_relation(tdbb, transaction, relation, TRUE, TRUE);
			
			// Fetch next record if current was deleted before being locked
			if (!VIO_writelock(tdbb, org_rpb, this, transaction)) 
				continue;
			}
		
		break;
		}
		
	return true;
}

void RsbWriteLock::close(Request* request)
{
	rsb_next->close(request);
}

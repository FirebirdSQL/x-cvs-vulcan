/*
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
 * April 11, 2004	Created by James A. Starkey
 *
 */
 
#include "firebird.h"
#include "common.h"
#include "Trigger.h"
#include "../jrd/jrd.h"
#include "exe.h"
#include "flags.h"
#include "Request.h"
#include "req.h"
#include "../jrd/par_proto.h"
#include "../jrd/cmp_proto.h"

Trigger::Trigger(void)
{
	blr = NULL;
	relation = NULL;
	request = NULL;
	sys_trigger = false;
	compile_in_progress = false;
	flags = 0;
}

Trigger::~Trigger(void)
{
	delete blr;
}

void Trigger::compile(thread_db* _tdbb)
{
	if (request)
		return;
	
	DBB dbb = _tdbb->tdbb_database;
#ifdef SHARED_CACHE
	Sync sync (&dbb->syncSysTrans, "trig::compile");
	sync.lock(Exclusive);
#endif
	
	if (!request && !compile_in_progress)
		{
		compile_in_progress = TRUE;
		JrdMemoryPool *old_pool = _tdbb->tdbb_default;
		JrdMemoryPool *new_pool = JrdMemoryPool::createPool(dbb);
		_tdbb->tdbb_default = new_pool;
		
		try 
			{
			PAR_blr(_tdbb, relation, blr->str_data,  NULL, NULL, &request, TRUE,
					(flags & TRG_ignore_perm ? csb_ignore_perm : 0));
			_tdbb->tdbb_default = old_pool;
			}
		catch (OSRIException& exception) 
			{
			_tdbb->tdbb_default = old_pool;
			compile_in_progress = FALSE;
			exception;
			
			if (request) 
				{
				CMP_release(_tdbb,request);
				request = NULL;
				} 
			else 
				JrdMemoryPool::deletePool(new_pool);
				
			throw;
			}
			
		_tdbb->tdbb_default = old_pool;
		
		if (!name.IsEmpty())
			request->req_trg_name = name;
			
		if (sys_trigger)
			request->req_flags |= req_sys_trigger;
			
		if (flags & TRG_ignore_perm)
			request->req_flags |= req_ignore_perm;

		compile_in_progress = FALSE;
		}
}

BOOLEAN Trigger::release(thread_db* _tdbb)
{
	if (!blr/*sys_trigger*/ || !request || CMP_clone_is_active(request)) 
		return FALSE;
	
	CMP_release(_tdbb, request);
	request = NULL;
	
	return TRUE;
}

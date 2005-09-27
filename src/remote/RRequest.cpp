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
 *
 */

#include <memory.h>
#include "fbdev.h"
#include "common.h"
#include "ibase.h"
#include "RRequest.h"
#include "remote.h"
#include "Sync.h"


RRequest::RRequest(RDatabase *database, int count)
{
	rrq_rdb = database;
	rrq_rtr = NULL;
	rrq_rpt = new rrq_repeat [count];
	memset (rrq_rpt, 0, count * sizeof (rrq_repeat));
	rrq_next = NULL;
	rrq_levels = NULL;
	rrq_handle = NULL_HANDLE;
	rrq_id = 0;
	rrq_max_msg = count;
	rrq_level = 0;
}

RRequest::~RRequest(void)
{
	if (rrq_rdb)
		rrq_rdb->releaseRequest (this);
		
	delete [] rrq_rpt;
}

RRequest* RRequest::clone(void)
{
	RRequest *request = new RRequest (rrq_rdb, rrq_max_msg);
	request->rrq_rtr = rrq_rtr;
	memcpy (request->rrq_rpt, rrq_rpt, sizeof (rrq_repeat) * rrq_max_msg);
	
	return request;
}

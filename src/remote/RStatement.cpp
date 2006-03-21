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

#include "fbdev.h"
#include "ibase.h"
#include "RStatement.h"
#include "remote.h"
#include "remot_proto.h"

#define ALLR_RELEASE(x)		ALLR_release ((struct blk *) (x))

RStatement::RStatement(RDatabase *database)
{
	rsr_rdb = database;
	rsr_rtr = NULL;
	rsr_handle = NULL_HANDLE;
	rsr_message = NULL;
	rsr_buffer = NULL;
	rsr_flags = 0;
	rsr_rows_pending = 0;
	rsr_msgs_waiting = 0;
	rsr_reorder_level = 0;
	rsr_batch_count = 0;
	rsr_id = 0;
}

RStatement::~RStatement(void)
{
	if (rsr_message)		
		REMOTE_release_messages(rsr_message);
	
	if (rsr_rdb)
		rsr_rdb->releaseStatement (this);
}

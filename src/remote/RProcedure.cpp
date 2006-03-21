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
#include "RProcedure.h"
#include "remote.h"
#include "remot_proto.h"

#define ALLR_RELEASE(x)		ALLR_release ((struct blk *) (x))

RProcedure::RProcedure(RDatabase *database)
{
	rpr_rdb = database;
	rpr_rtr = NULL;
	rpr_in_msg = NULL;
	rpr_out_msg = NULL;
	rpr_in_format = NULL;
	rpr_out_format = NULL;
	rpr_flags = 0;
	rpr_handle = NULL_HANDLE;
}

RProcedure::~RProcedure(void)
{
	clear();
}

void RProcedure::clear(void)
{
	if (rpr_in_msg) 
		{
		rpr_in_msg->release();
		rpr_in_msg = NULL;
		}
		
	if (rpr_out_msg) 
		{
		rpr_out_msg->release();
		rpr_out_msg = NULL;
		}
}

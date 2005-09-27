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
#include "common.h"
#include "RTransaction.h"
#include "remote.h"
#include "Sync.h"


RTransaction::RTransaction(RDatabase *database)
{
	rtr_rdb = database;
	rtr_blobs = NULL;
	rtr_handle = NULL_HANDLE;
	rtr_flags = 0;
	rtr_id = 0;
}

RTransaction::~RTransaction(void)
{
	while (rtr_blobs)
		delete rtr_blobs;

	rtr_rdb->releaseTransaction (this);
	
	if (rtr_rdb && rtr_id)
		rtr_rdb->rdb_port->releaseObjectId (rtr_id);
}

void RTransaction::releaseBlob(RBlob* blob)
{
	Sync sync (&syncObject, "RTransaction::releaseBlob");
	sync.lock (Exclusive);
	
	for (RBlob **ptr = &rtr_blobs; *ptr; ptr = &(*ptr)->rbl_next)
		if (*ptr == blob)
			{
			*ptr = blob->rbl_next;
			break;
			}
}

RBlob* RTransaction::createBlob(int size)
{
	RBlob *blob = new RBlob (this, size);
	Sync sync (&syncObject, "RTransaction::createBlob");
	sync.lock (Exclusive);
	blob->rbl_next = rtr_blobs;
	rtr_blobs = blob;
	
	return blob;
}

void RTransaction::cleanup(void)
{
}

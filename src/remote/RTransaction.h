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

#ifndef RTRANSACTION_H
#define RTRANSACTION_H

#define RTR_limbo	1

#include "SyncObject.h"

class RDatabase;
class RBlob;

class RTransaction
{
public:
	RTransaction(RDatabase *database);
	virtual ~RTransaction(void);

	//struct blk	rtr_header;
	RDatabase*		rtr_rdb;
	RTransaction*	rtr_next;
	RBlob*			rtr_blobs;
	isc_tr_handle 	rtr_handle;
	USHORT			rtr_flags;
	USHORT			rtr_id;
	
	SyncObject		syncObject;
	void releaseBlob(RBlob* blob);
	RBlob* createBlob(int size);
	void cleanup(void);
};

#endif


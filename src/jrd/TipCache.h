/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		tpc.h
 *	DESCRIPTION:	Transaction Inventory Page Cache
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
 */

#ifndef JRD_TIP_CACHE_H
#define JRD_TIP_CACHE_H

#include "SyncObject.h"

class Database;
class tpc;
struct tdbb;
struct tip;

class TipCache
{
public:
	TipCache(Database *dbb);
	~TipCache(void);
	
	Database	*database;
	SyncObject	syncObject;
	tpc			*cache;
	int			transactionsPerTip;
	
	int			getCacheState(tdbb* tdbb, int number);
	void		initialize(tdbb* tdbb, int number);
	void		updateCache(tdbb* tdbb, tip* tip_page, int sequence);
	int			snapshotState(tdbb* tdbb, int number);
	void		setState(tdbb* tdbb, int number, int state);

protected:
	int			extendCache(tdbb* tdbb, int number);
	void		cacheTransactions(tdbb* tdbb, tpc** tip_cache_ptr, ULONG oldest);
	tpc*		allocateTpc(tdbb* tdbb, ULONG base);
};

#endif


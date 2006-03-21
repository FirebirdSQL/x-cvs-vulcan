/* $Id$ */
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
struct thread_db;
struct tx_inv_page;

class TipCache
{
public:
	TipCache(Database *dbb);
	~TipCache(void);
	
	Database	*database;
#ifdef SHARED_CACHE
	SyncObject	syncObject;
	SyncObject	syncObjectInitialize;
#endif
	tpc			*cache;
	int			transactionsPerTip;
	
	int			getCacheState(thread_db* tdbb, int number);
	void		initialize(thread_db* tdbb, int number);
	void		updateCache(thread_db* tdbb, tx_inv_page* tip_page, int sequence);
	int			snapshotState(thread_db* tdbb, int number);
	void		setState(thread_db* tdbb, int number, int state);

protected:
	int			extendCache(thread_db* tdbb, int number);
	void		cacheTransactions(thread_db* tdbb, tpc** tip_cache_ptr, ULONG oldest);
	tpc*		allocateTpc(thread_db* tdbb, ULONG base);
};

#endif


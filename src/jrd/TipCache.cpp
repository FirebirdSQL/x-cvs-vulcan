/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		tpc.cpp
 *	DESCRIPTION:	tx_inv_page* Cache for DBB
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

#include <memory>
#include <string.h>
#include "fbdev.h"
#include "common.h"
#include "TipCache.h"
#include "../jrd/jrd.h"
#include "../jrd/tpc.h"
#include "../jrd/ods.h"
#include "../jrd/tra.h"
#include "../jrd/lck.h"
#include "../jrd/pag.h"
#include "gen/iberror.h"
#include "../jrd/iberr.h"
//#include "../jrd/all_proto.h"
#include "../jrd/cch_proto.h"
//#include "../jrd/err_proto.h"
//#include "../jrd/gds_proto.h"
#include "../jrd/lck_proto.h"
#include "../jrd/tra_proto.h"
#include "PageCache.h"
#include "Sync.h"

TipCache::TipCache(Database *dbb)
{
	database = dbb;
	cache = NULL;
	transactionsPerTip = 0;
}

TipCache::~TipCache(void)
{
	for (tpc *tipCache; tipCache = cache;)
		{
		cache = tipCache->tpc_next;
		delete tipCache;
		}
}

/**************************************
 *
 *	T P C _ c a c h e _ s t a t e
 *
 **************************************
 *
 * Functional description
 *	Get the current state of a transaction in the cache.
 *
 **************************************/

int TipCache::getCacheState(thread_db* tdbb, int number)
{
	if (!cache) 
		initialize(tdbb, number);

	if (number && database->dbb_pc_transactions.size())
		if (TRA_precommited(tdbb, number, number))
			return tra_precommitted;

#ifdef SHARED_CACHE
	Sync sync (&syncObject, "TipCache::getCacheState");
	sync.lock(Shared);
#endif
	
	/* if the transaction is older than the oldest
	   transaction in our tip cache, it must be committed */

	if (number < cache->tpc_base)
		return tra_committed;

	/* locate the specific tx_inv_page* cache block for the transaction */

	for (tpc *tip_cache = cache; tip_cache; tip_cache = tip_cache->tpc_next)
		if (number < (SLONG) (tip_cache->tpc_base + transactionsPerTip)) 
			return TRA_state(tip_cache->tpc_transactions, tip_cache->tpc_base, number);

	/* Cover all possibilities by returning active */

	return tra_active;
}

/**************************************
 *
 *	T P C _ i n i t i a l i z e _ t p c
 *
 **************************************
 *
 * Functional description
 *	At transaction startup, intialize the tip cache up to
 *	number.  This is used at TRA_start () time.
 *
 **************************************/

void TipCache::initialize(thread_db* tdbb, int number)
{
	TPC *tip_cache_ptr, tip_cache;
#ifdef SHARED_CACHE
	Sync syncInitialize (&syncObjectInitialize, "TipCache::initialize");
	syncInitialize.lock(Exclusive);

	Sync sync (&syncObject, "TipCache::initialize");
	sync.lock(Exclusive);
#endif

	if (!cache) 
		{
		cacheTransactions(tdbb, NULL, 0);
		return;
		}

	/* If there is already a cache, extend it if required.
	 * find the end of the linked list, and cache
	 * all transactions from that point up to the
	 * most recent transaction
	 */

	for (tip_cache_ptr = &cache; *tip_cache_ptr; tip_cache_ptr = &(*tip_cache_ptr)->tpc_next)
		tip_cache = *tip_cache_ptr;

	if (number < (SLONG)(tip_cache->tpc_base + transactionsPerTip))
		return;

	cacheTransactions(tdbb, tip_cache_ptr, tip_cache->tpc_base + transactionsPerTip);
}

/**************************************
 *
 *	c a c h e _ t r a n s a c t i o n s
 *
 **************************************
 *
 * Functional description
 *	Cache the state of all the transactions since
 *	the last time this routine was called, or since
 *	the oldest interesting transaction.
 *
 **************************************/

void TipCache::cacheTransactions(thread_db* tdbb, tpc **tip_cache_ptr, ULONG oldest)
{
	/* check the header page for the oldest and 
	   newest transaction numbers */

#ifdef SUPERSERVER_V2
	top = database->dbb_next_transaction;
	oldest = MAX(oldest, database->dbb_oldest_transaction);
#else
	WIN window(HEADER_PAGE);
	header_page* header = (header_page*) CCH_FETCH(tdbb, &window, LCK_read, pag_header);
	ULONG top = header->hdr_next_transaction;
	oldest = MAX(oldest, (ULONG) header->hdr_oldest_transaction);
	CCH_RELEASE(tdbb, &window);
#endif

	/* allocate tpc blocks to hold all transaction states --
	   assign one tpc block per page to simplify cache maintenance */

	if (!transactionsPerTip)
		transactionsPerTip = database->dbb_pcontrol->pgc_tpt;
	
	if (!tip_cache_ptr)
		tip_cache_ptr = &cache;

	for (ULONG base = oldest - oldest % transactionsPerTip; base <= top; base += transactionsPerTip) 
		{
		*tip_cache_ptr = allocateTpc(tdbb, base);
		tip_cache_ptr = &(*tip_cache_ptr)->tpc_next;
		}

	/* now get the inventory of all transactions, which will
	   automatically fill in the tip cache pages */

	TRA_get_inventory(tdbb, NULL, oldest, top);
}

/**************************************
 *
 *	a l l o c a t e _ t p c
 *
 **************************************
 *
 * Functional description
 *	Create a tip cache block to hold the state
 *	of all transactions on one page.
 *
 **************************************/

tpc* TipCache::allocateTpc(thread_db* tdbb, ULONG base)
{
	if (!transactionsPerTip)
		transactionsPerTip = database->dbb_pcontrol->pgc_tpt;

	/* allocate a tx_inv_page* cache block with enough room for 
	   all desired transactions */

	TPC tip_cache = FB_NEW_RPT(*database->dbb_permanent, transactionsPerTip / 4) tpc();
	tip_cache->tpc_base = base;

	return tip_cache;
}

/**************************************
 *
 *	T P C _ u p d a t e _ c a c h e
 *
 **************************************
 *
 * Functional description
 *	A tx_inv_page* page has been fetched into memory,
 *	so we should take the opportunity to update
 *	the tx_inv_page* cache with the state of all transactions
 *	on that page.
 *
 **************************************/

void TipCache::updateCache(thread_db* tdbb, tx_inv_page* tip_page, int sequence)
{
	TPC tip_cache;
	USHORT l;
	SLONG first_trans = sequence * transactionsPerTip;
#ifdef SHARED_CACHE
	Sync sync (&syncObject, "TipCache::updateCache");
	sync.lock(Exclusive);
#endif

	/* while we're in the area we can check to see if there are 
	   any tip cache pages we can release--this is cheaper and 
	   easier than finding out when a tx_inv_page* page is dropped */

	while ( (tip_cache = cache) )
		if (database->dbb_oldest_transaction >= tip_cache->tpc_base + transactionsPerTip) 
			{
			cache = tip_cache->tpc_next;
			delete tip_cache;
			}
		else
			break;

	/* find the appropriate page in the tx_inv_page* cache and assign all transaction
	   bits -- it's not worth figuring out which ones are actually used */

	for (; tip_cache; tip_cache = tip_cache->tpc_next)
		if (first_trans == tip_cache->tpc_base) 
			{
			l = TRANS_OFFSET(transactionsPerTip);
			MOVE_FAST(tip_page->tip_transactions, tip_cache->tpc_transactions, l);
			break;
			}

	/* note that a potential optimization here would be to extend the cache
	   if the fetched page is not already in cache; it would involve a little
	   extra effort to make sure the pages remained in order, and since there
	   is a fetched page passed to us we can't fetch any other pages in this
	   routine, so I just decided not to do it - djb */
}

/**************************************
 *
 *	T P C _ s n a p s h o t _ s t a t e
 *
 **************************************
 *
 * Functional description
 *	Get the current state of a transaction.
 *	Look at the tx_inv_page* cache first, but if it
 *	is marked as still alive we must do some 
 *	further checking to see if it really is.
 *
 *  This routine is used by the system transaction and
 *  read committed transactions.
 *
 **************************************/

int TipCache::snapshotState(thread_db* tdbb, int number)
{
#ifdef SHARED_CACHE
	Sync sync (&syncObject, "TipCache::snapshotState");
	sync.lock(Shared);
#else
	bool exclusive = 0;
#endif

	if (!cache)
		{
#ifdef SHARED_CACHE
		sync.unlock();
		sync.lock(Exclusive);
#else
		exclusive = 1;
#endif
		
		if (!cache)
			cacheTransactions(tdbb, NULL, 0);
		}

	if (number && database->dbb_pc_transactions.size()) 
		if (TRA_precommited(tdbb, number, number)) 
			return tra_precommitted;

	/* if the transaction is older than the oldest
	   transaction in our tip cache, it must be committed */

	if (number < cache->tpc_base) 
		return tra_committed;

	/* locate the specific tx_inv_page* cache block for the transaction */

	for (;;)
		{
		for (TPC tip_cache = cache; tip_cache; tip_cache = tip_cache->tpc_next)
			{
			if (number < (SLONG) (tip_cache->tpc_base + transactionsPerTip))
				{
				const USHORT state = TRA_state(	tip_cache->tpc_transactions,
												tip_cache->tpc_base, number);

				/* committed or dead transactions always stay that 
				way, so no need to check their current state */

				if (state == tra_committed || state == tra_dead) 
					return state;

				// see if we can get a lock on the transaction; if we can't
				// then we know it is still active
				// We need to create this one in a pool since the
				// receiver of this (ptr) checks its type.
				// Please review this. This lock has _nothing_ to do in the
				// permamnent pool!
				
				std::auto_ptr<Lock> temp_lock(FB_NEW_RPT(*database->dbb_permanent, 0) Lock);

				//temp_lock.blk_type = type_lck;
				temp_lock->lck_dbb = database;
				temp_lock->lck_type = LCK_tra;
				temp_lock->lck_owner_handle = LCK_get_owner_handle(tdbb, LCK_tra);
				temp_lock->lck_parent = database->dbb_lock;
				temp_lock->lck_length = sizeof(SLONG);
				temp_lock->lck_key.lck_long = number;

				/* If we can't get a lock on the transaction, it must be active. */

				if (!LCK_lock_non_blocking(tdbb, temp_lock.get(), LCK_read, FALSE)) 
					{
					INIT_STATUS(tdbb->tdbb_status_vector);
					return tra_active;
					}

				INIT_STATUS(tdbb->tdbb_status_vector);
				LCK_release(temp_lock.get());
#ifdef SHARED_CACHE
				sync.unlock();
#endif
				
				/* as a last resort we must look at the tx_inv_page* page to see
				  whether the transaction is committed or dead; to minimize 
				  having to do this again we will check the state of all 
				  other transactions on that page */

				return TRA_fetch_state(tdbb, number);
				}
			}
			
#ifdef SHARED_CACHE
		if (sync.state == Exclusive)
#else
		if (exclusive)
#endif
			break;
					
#ifdef SHARED_CACHE
		sync.unlock();
		sync.lock(Exclusive);
#endif
		}

	/* if the transaction has been started since we
	   last looked, extend the cache upward */

	return extendCache(tdbb, number);
}

/**************************************
 *
 *	e x t e n d _ c a c h e
 *
 **************************************
 *
 * Functional description
 *	Extend the transaction inventory page
 *	cache to include at least all transactions 
 *	up to the passed transaction, and return 
 *	the state of the passed transaction.
 *
 **************************************/

int TipCache::extendCache(thread_db* tdbb, int number)
{
	TPC *tip_cache_ptr, tip_cache;

	/* find the end of the linked list, and cache
	   all transactions from that point up to the
	   most recent transaction */

	for (tip_cache_ptr = &cache; *tip_cache_ptr; tip_cache_ptr = &(*tip_cache_ptr)->tpc_next)
		tip_cache = *tip_cache_ptr;
		
	cacheTransactions(tdbb, tip_cache_ptr, tip_cache->tpc_base + transactionsPerTip);

	/* find the right block for this transaction and return the state */

	for (tip_cache = cache; tip_cache; tip_cache = tip_cache->tpc_next) 
		if (number < (SLONG) (tip_cache->tpc_base + transactionsPerTip)) 
			 return TRA_state(tip_cache->tpc_transactions, tip_cache->tpc_base, number);

	/* we should never get to this point, but if we do the
	   safest thing to do is return active */

	return tra_active;
}

/**************************************
 *
 *	T P C _ s e t _ s t a t e
 *
 **************************************
 *
 * Functional description
 *	Set the state of a particular transaction
 *	in the tx_inv_page* cache.
 *
 **************************************/

void TipCache::setState(thread_db* tdbb, int number, int state)
{
#ifdef SHARED_CACHE
	Sync sync (&syncObject, "TipCache::setState");
	sync.lock(Exclusive);
#endif
	ULONG byte = TRANS_OFFSET(number % transactionsPerTip);
	SSHORT shift = TRANS_SHIFT(number);

	for (TPC tip_cache = cache; tip_cache; tip_cache = tip_cache->tpc_next) 
		if (number < (SLONG)(tip_cache->tpc_base + transactionsPerTip)) 
			{
			UCHAR *address = tip_cache->tpc_transactions + byte;
			*address &= ~(TRA_MASK << shift);
			*address |= state << shift;
			break;
			}

	/* right now we don't set the state of a transaction on a page
	   that has not already been cached -- this should probably be done */
}

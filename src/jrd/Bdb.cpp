/*
 *	PROGRAM:		JRD access method
 *	MODULE:			Bdb.h
 *	DESCRIPTION:	Buffer Descriptor Block
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

#include "fbdev.h"
#include "common.h"
#include "Bdb.h"
#include "jrd.h"
#include "ods.h"
#include "lck.h"
#include "Interlock.h"
#include "ThreadData.h"
#include "PageCache.h"
#include "lck_proto.h"
#include "Interlock.h"

#define PAGE_DUMP

#ifdef PAGE_DUMP
#include "dmp_proto.h"
#endif

Bdb::Bdb(thread_db* tdbb, pag* memory)
{
	Database *database = tdbb->tdbb_database;
	init();
	bdb_dbb = database;
	bdb_buffer = memory;
	
	bdb_lock = FB_NEW_RPT(*database->dbb_bufferpool, sizeof(SLONG)) Lock;
	bdb_lock->lck_type = LCK_bdb;
	bdb_lock->lck_owner_handle = LCK_get_owner_handle(tdbb, LCK_bdb);
	bdb_lock->lck_length = sizeof(SLONG);
	bdb_lock->lck_dbb = database;
	bdb_lock->lck_parent = database->dbb_lock;
	bdb_lock->lck_ast = PageCache::bdbBlockingAst;
	bdb_lock->lck_object = reinterpret_cast<blk*>(this);
}


Bdb::Bdb(void)
{
	init();
}

Bdb::~Bdb(void)
{
	delete bdb_lock;
}


void Bdb::init(void)
{
	bdb_dbb = NULL;
	bdb_buffer = NULL;
	bdb_lock = NULL;
	QUE_INIT(bdb_higher);
	QUE_INIT(bdb_lower);
	QUE_INIT(bdb_waiters);
	//QUE_INIT(bdb_dirty);
	bdb_use_count = 0;
	bdb_incarnation = 0;
	bdb_length = 0;
	bdb_flags = 0;
	//bdb_left = NULL;
	//bdb_right = NULL;
	//bdb_parent = NULL;
	bdb_ast_flags = 0;
	bdb_buffer = NULL;
	bdb_expanded_buffer = NULL;
	bdb_blocked = NULL;
	bdb_incarnation = 0;
	bdb_transactions = 0;
	//bdb_mark_transaction = 0;
	bdb_exclusive = NULL;
	bdb_io = NULL;
	bdb_scan_count = 0;
	bdb_write_direction = 0;
	bdb_difference_page = 0;
	bdb_diff_generation = 0;
	bdb_pending_page = 0;
	bdb_mark_sequence = 0;
	//bdb_lru_chained = false;
	exclusive = false;
	writers = 0;
}


void Bdb::init(thread_db* tdbb, Lock *lock, pag* buffer)
{
	bdb_dbb = tdbb->tdbb_database;
	bdb_buffer = buffer;
	bdb_lock = lock;
	bdb_lock->lck_ast = PageCache::bdbBlockingAst;
	bdb_lock->lck_object = reinterpret_cast<blk*>(this);
}

USHORT Bdb::computeChecksum(void)
{
#ifdef NO_CHECKSUM
	return DUMMY_CHECKSUM;
#else
#ifdef WIN_NT
/* ODS_VERSION8 for NT was shipped before page checksums
   were disabled on other platforms. Continue to compute
   checksums for ODS_VERSION8 databases but eliminate them
   for ODS_VERSION9 databases. The following code can be
   deleted when development on ODS_VERSION10 begins and
   NO_CHECKSUM is defined for all platforms. */

	if (bdb_dbb->dbb_ods_version >= ODS_VERSION9)
		return DUMMY_CHECKSUM;
#endif

	pag* page = bdb_buffer;

	const ULONG* const end = (ULONG *) ((SCHAR *) page + bdb_dbb->dbb_page_size);
	const USHORT old_checksum = page->pag_checksum;
	page->pag_checksum = 0;
	const ULONG* p = (ULONG *) page;
	ULONG checksum = 0;

	do {
		checksum += *p++;
		checksum += *p++;
		checksum += *p++;
		checksum += *p++;
		checksum += *p++;
		checksum += *p++;
		checksum += *p++;
		checksum += *p++;
		} while (p < end);

	page->pag_checksum = old_checksum;

	if (checksum)
		return (USHORT) checksum;

	/* If the page is all zeros, return an artificial checksum */

	for (p = (ULONG *) page; p < end;) 
		{
		if (*p++)
			return (USHORT) checksum;
		}

	/* Page is all zeros -- invent a checksum */

	return 12345;
#endif
}

int Bdb::blockingAstBdb(void* argument)
{
	return PageCache::bdbBlockingAst(argument);
}

void Bdb::addRef(thread_db* tdbb, LockType lType)
{
#ifdef SHARED_CACHE
	syncPage.lock (NULL, lType);
#endif
	incrementUseCount();
	
	if (lType == Exclusive)
		{
		exclusive = true;
		bdb_exclusive = tdbb;
		++writers;
		}
		
	if (tdbb)
		tdbb->registerBdb (this);
}

bool Bdb::addRefConditional(thread_db* tdbb, LockType lType)
{
#ifdef SHARED_CACHE
	if (!syncPage.lockConditional(lType))
		return false;
#endif
		
	incrementUseCount();
	
	if (lType == Exclusive)
		{
		exclusive = true;
		bdb_exclusive = tdbb;
		++writers;
		}
		
	if (tdbb)
		tdbb->registerBdb (this);
	
	return true;
}

void Bdb::release(thread_db* tdbb)
{
#ifdef SHARED_CACHE
	int oldState = syncPage.getState();
#endif
	fb_assert (!(bdb_flags & BDB_marked) || writers > 1);
	decrementUseCount();
	
	if (exclusive)
		{
		bdb_exclusive = bdb_io = NULL;
		if (--writers == 0)
			exclusive = false;
#ifdef SHARED_CACHE
		syncPage.unlock (NULL, Exclusive);
#endif
		}
#ifdef SHARED_CACHE
	else
		syncPage.unlock(NULL, Shared);
#endif
	
	if (tdbb)
		tdbb->clearBdb (this);

}

INTERLOCK_TYPE Bdb::incrementUseCount(void)
{
#ifdef SHARED_CACHE
	return INTERLOCKED_INCREMENT (bdb_use_count);
#else
	bdb_use_count++;
	return bdb_use_count;
#endif
}

INTERLOCK_TYPE Bdb::decrementUseCount(void)
{
	if (bdb_use_count <= 0)
		bugcheck();
	
#ifdef SHARED_CACHE
	return INTERLOCKED_DECREMENT (bdb_use_count);
#else
	bdb_use_count--;
	return bdb_use_count;
#endif
}

void Bdb::downGrade(LockType lType)
{
	if (lType == Shared && !exclusive)
		return;

	if (writers > 1)
		bugcheck();
	
	--writers;
	exclusive = false;
	
	if (exclusive)
		bdb_exclusive = NULL;
		
#ifdef SHARED_CACHE
	syncPage.downGrade (lType);
#endif
}

void Bdb::bugcheck(void)
{
}

bool Bdb::isLocked(void)
{
#ifdef SHARED_CACHE
	return syncPage.isLocked();
#else
	return bdb_use_count;
#endif
}

void Bdb::validate(void)
{
}

bool Bdb::ourExclusiveLock(void)
{
	if (!exclusive)
		return false;
	
#ifdef SHARED_CACHE
	return syncPage.ourExclusiveLock();
#else
	return true;
#endif
}

int Bdb::clearFlags(int flags)
{
#ifdef SHARED_CACHE
	volatile INTERLOCK_TYPE oldFlags;

	for(;;)
		{
		oldFlags = bdb_flags;
		volatile INTERLOCK_TYPE newFlags = oldFlags & ~flags;
		if (COMPARE_EXCHANGE(&bdb_flags, oldFlags, newFlags))
			break;
		}
#else
	int oldFlags = bdb_flags;
	bdb_flags &= ~flags;
#endif
	return oldFlags;
}

int Bdb::setFlags(int flags)
{
#ifdef SHARED_CACHE
	volatile INTERLOCK_TYPE oldFlags;

	for(;;)
		{
		oldFlags = bdb_flags;
		volatile INTERLOCK_TYPE newFlags = oldFlags | flags;
		if (COMPARE_EXCHANGE(&bdb_flags, oldFlags, newFlags))
			break;
		}
#else
	int oldFlags = bdb_flags;
	bdb_flags |= flags;
#endif
	
	return oldFlags;
}

int Bdb::setFlags(int setBits, int clearBits)
{
#ifdef SHARED_CACHE
	volatile INTERLOCK_TYPE oldFlags;
	
	for(;;)
		{
		oldFlags = bdb_flags;
		volatile INTERLOCK_TYPE newFlags = oldFlags & ~clearBits;
		newFlags |= setBits;
		if (COMPARE_EXCHANGE(&bdb_flags, oldFlags, newFlags))
			break;
		}
#else
	int oldFlags = bdb_flags;
	bdb_flags &= ~clearBits;
	bdb_flags |= setBits;
#endif
	return oldFlags;
}

void Bdb::printPage()
{
#ifdef PAGE_DUMP
	DMP_fetched_page(bdb_buffer, bdb_page, 0, bdb_dbb->dbb_page_size);
#endif
}

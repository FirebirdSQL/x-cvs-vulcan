/*
 *	PROGRAM:		JRD access method
 *	MODULE:			PageCache.cpp
 *	DESCRIPTION:	Cache manager 
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

#define NEW_PAGE_CACHE

#include <stdio.h>
#include "firebird.h"
#include "common.h"
#include "PageCache.h"
#include "jrd.h"
#include "ods.h"
#include "cch.h"
#include "Sync.h"
#include "Bdb.h"
#include "iberror.h"
#include "err_proto.h"
#include "OSRIBugcheck.h"
#include "nbak.h"
#include "sch_proto.h"
#include "thd_proto.h"
#include "../jrd/os/pio_proto.h"
#include "../jrd/jrd_time.h"
#include "sdw_proto.h"
#include "pag_proto.h"
#include "lck_proto.h"
#include "tra_proto.h"
#include "shut_proto.h"
#include "isc_proto.h"
#include "nav_proto.h"
#include "iberr_proto.h"
#include "sdw.h"
#include "ThreadData.h"
#include "status.h"
#include "tra.h"
#include "jrd_proto.h"
#include "sbm_proto.h"
#include "gds_proto.h"
#include "BdbSort.h"
#include "Interlock.h"
#include "CommitManager.h"

#undef CACHE_WRITER

#define JOURNAL_PAGE		-1
#define SHADOW_PAGE		-2
#define FREE_PAGE		-3
#define CHECKPOINT_PAGE		-4
#define MIN_PAGE_NUMBER		-5

#define PRE_SEARCH_LIMIT	100
#define PRE_EXISTS		-1
#define PRE_UNKNOWN		-2

#define BLOCK(fld_ptr, type, fld) ((type)((SCHAR*) fld_ptr - OFFSET (type, fld)))

PageCache::PageCache(Database *dbb)
{
	database = dbb;
	bdbs = NULL;
	hashTable = NULL;
	bcb_count = 0;
	bcb_flags = 0;
	bcb_prefetch = NULL;
	bcb_memory = NULL;
	markSequence = 0;
	bcb_lru_chain = 0;
	getShadows = false;
	QUE_INIT(bcb_dirty);
}

PageCache::~PageCache(void)
{
	for (void *memory; memory = bcb_memory;)
		{
		bcb_memory = *(void**) memory;
		delete [] (UCHAR*) memory;
		}
	
	for (int n = 0; n < bcb_count; ++n)
		delete bdbs [n];
		
	delete [] bdbs;
	delete [] hashTable;
}

void PageCache::initialize(thread_db* tdbb, int numberBuffers)
{
	SLONG count;
	int number = numberBuffers;
	
	/* Check for database-specific page buffers */

	if (database->dbb_page_buffers)
		number = database->dbb_page_buffers;

	/* Enforce page buffer cache constraints */

	if (number < MIN_PAGE_BUFFERS)
		number = MIN_PAGE_BUFFERS;
		
	if (number > MAX_PAGE_BUFFERS)
		number = MAX_PAGE_BUFFERS;

	count = number;

	/* Allocate and initialize buffers control block */

	QUE_INIT(bcb_in_use);
	QUE_INIT(bcb_pending);
	QUE_INIT(bcb_empty);
	QUE_INIT(bcb_free_lwt);
	QUE_INIT(bcb_dirty);
	bdbs = new Bdb* [number];
	hashTable = new Que [number];
	UCHAR *memory = NULL;
	UCHAR *end = NULL;
	int pageSize = database->dbb_page_size;
	
	for (int n = 0; n < number; ++n, memory += pageSize)
		{
		QUE_INIT (hashTable [n]);
		
		if ((end - memory) <= pageSize)
			{
			int l = (number - n + 1) * pageSize;
			for(;;)
				{
				try
					{
					end = memory = new UCHAR [l];
					if (memory)
						{
						end += l;
						*(void**) memory = bcb_memory;
						bcb_memory = (void*) memory;
						memory = (UCHAR*) (((U_IPTR) memory + pageSize) & ~(pageSize - 1));
						//remaining = end - memory;
						break;
						}
					}
				catch (...)
					{
					}
				l >>= 1;
				if (l  < pageSize * 2)
					ERR_post (isc_virmemexh, 0);
				}
			}
			
		Bdb *bdb = bdbs[n] = new Bdb(tdbb, (pag*) memory);
		QUE_INSERT(bcb_empty, bdb->bdb_que);
		//bdb->init (tdbb, lock, (pag*) memory);
		}
		
	/* initialization of memory is system-specific */

	bcb_count = number; //memory_init(tdbb, bcb_, number);
	bcb_free_minimum = (SSHORT) MIN(bcb_count / 4, 128);

	if (bcb_count < MIN_PAGE_BUFFERS)
		ERR_post(isc_cache_too_small, 0);

	/* Log if requested number of page buffers could not be allocated. */

	/***
	if (count != (SLONG) bcb_count)
		gds__log
			("Database: %s\n\tAllocated %ld page buffers of %ld requested",
			 (const char*) tdbb->tdbb_attachment->att_filename, bcb_count, count);
	***/

	if (database->dbb_lock->lck_logical != LCK_EX)
		database->dbb_ast_flags |= DBB_assert_locks;
	
#ifdef CACHE_READER
	AsyncEvent* event = dbb->dbb_reader_event;
	ISC_event_init(event, 0, 0);
	count = ISC_event_clear(event);

	if (gds__thread_start
		(reinterpret_cast < FPTR_INT_VOID_PTR > (cache_reader), dbb,
		 THREAD_high, 0, 0))
		ERR_bugcheck_msg("cannot start thread");

	ISC_event_wait(1, &event, &count, 5 * 1000000, NULL, 0);
#endif

#ifdef CACHE_WRITER
	if (!(dbb->dbb_flags & DBB_read_only)) {
		AsyncEvent* event = dbb->dbb_writer_event_init;
		/* Initialize initialization event */
		ISC_event_init(event, 0, 0);
		count = ISC_event_clear(event);

		if (gds__thread_start (FPTR_INT_VOID_PTR) cache_writer, dbb,THREAD_high, 0, 0)) 
			ERR_bugcheck_msg("cannot start thread");

		ISC_event_wait(1, &event, &count, 5 * 1000000, NULL, 0);
		/* Clean up initialization event */
		ISC_event_fini(event);
	}
#endif
}

void PageCache::expandBuffers(thread_db* tdbb, int expandedSize)
{
	if (expandedSize <= bcb_count || expandedSize > MAX_PAGE_BUFFERS)
		return;

	Sync sync (&syncObject, "PageCache::expandBuffers");
	sync.lock (Exclusive);
	Bdb **newBdbs = NULL;
	UCHAR *memory = NULL;
	Que *newHashTable = NULL;
	int pageSize = database->dbb_page_size;
	int n;
	
	// Try to allocate necessary memory and structures.  If we can't get it, punt!
	
	try
		{
		newHashTable = new Que [expandedSize];
		memory = new UCHAR[(expandedSize - bcb_count + 1) * pageSize];
		*(void**) memory = bcb_memory;
		bcb_memory = memory;
		newBdbs = new Bdb* [expandedSize];
		memcpy(newBdbs, bdbs, bcb_count * sizeof(Bdb*));
		memset(newBdbs + bcb_count, 0, (expandedSize - bcb_count) * sizeof(Bdb*));
		memory = (UCHAR*) (((U_IPTR) memory + pageSize) & ~(pageSize - 1));
		
		for (n = bcb_count; n < expandedSize; ++n, memory += pageSize)
			newBdbs [n] = new Bdb(tdbb, (pag*) memory);
		}
	catch (...)
		{
		if (newBdbs)
			{
			for (n = bcb_count; n < expandedSize; ++n)
				delete newBdbs[n];
			delete [] newBdbs;
			}
				
		delete [] newHashTable;
		}
	
	// We're got everything we need.  Integrate the structures and delete the old.
	// Start by rebuilding the hash table
	
	for (n = 0; n < expandedSize; ++n)
		QUE_INIT (newHashTable [n]);

	for (n = 0; n < bcb_count; ++n)
		while (QUE_NOT_EMPTY(hashTable[n]))
			{
			Que *que = hashTable[n].que_forward;
			Bdb *bdb = BLOCK(que, Bdb*, bdb_que);
			QUE_DELETE(bdb->bdb_que);
			Que *modQue = newHashTable + bdb->bdb_page % expandedSize;
			QUE_INSERT(*modQue, bdb->bdb_que);
			}
	
	for (n = bcb_count; n < expandedSize; ++n)
		{
		Bdb *bdb = newBdbs[n];
		QUE_INSERT(bcb_empty, bdb->bdb_que);
		}
		
	delete [] bdbs;
	delete [] hashTable;
	bdbs = newBdbs;
	hashTable = newHashTable;
	bcb_count = expandedSize;
}

/**************************************
 *
 *	w r i t e _ p a g e
 *
 **************************************
 *
 * Functional description
 *	Do actions required when writing a database page,
 *	including journaling, shadowing.
 *
 **************************************/

bool PageCache::writePage(thread_db* tdbb, Bdb* bdb, bool write_thru, bool inAst)
{
	Sync sync (&syncPageWrite, "PageCache::writePage");
	sync.lock(Exclusive);

	if (!((bdb->bdb_flags & BDB_dirty || (write_thru && bdb->bdb_flags & BDB_db_dirty)) &&
		 !(bdb->bdb_flags & BDB_marked)))
		 return true;
	
	FIL file;
	//ISC_STATUS statusVector [20];
	
	if (bdb->bdb_flags & BDB_not_valid)
		ERR_post (isc_buf_invalid, isc_arg_number, bdb->bdb_page, isc_arg_end);

	bool result = true;
	pag* page = bdb->bdb_buffer;

	/* Before writing db header page, make sure that the next_transaction > oldest_active
	  transaction */
	  
	if (bdb->bdb_page == HEADER_PAGE) 
		{
		header_page* header = (header_page*) page;
		if (header->hdr_next_transaction) 
			{
			if (header->hdr_oldest_active > header->hdr_next_transaction)
				BUGCHECK(266);	/*next transaction older than oldest active */
			if (header->hdr_oldest_transaction > header->hdr_next_transaction)
				BUGCHECK(267);	/* next transaction older than oldest transaction */
			}
		}

	page->pag_generation++;
	
	if (bdb->bdb_flags & BDB_journal) 
		journalBuffer(bdb);

	if (true || write_thru) 
		{
		AST_CHECK;
		database->dbb_writes++;

		/* write out page to main database file, and to any
		   shadows, making a special case of the header page */

		if (bdb->bdb_page >= 0) 
			{
			if (bdb->bdb_write_direction == BDB_write_undefined)
				throw OSRIBugcheck (isc_bug_check, isc_arg_string, "Undefined page write direction", isc_arg_end);

			page->pag_checksum = bdb->computeChecksum();

			if (bdb->bdb_write_direction == BDB_write_diff ||
				(bdb->bdb_write_direction == BDB_write_both 
#ifndef SUPERSERVER
				 && bdb->bdb_diff_generation == database->backup_manager->get_current_generation()
#endif
				)) 
				{
#ifdef NBAK_DEBUG
				// We cannot call normal trace functions here as they are signal-unsafe
				char buffer[1000], *ptr = buffer;
				strcpy(ptr, "NBAK,Write page "); ptr += strlen(ptr);
				gds__ulstr(ptr, bdb->bdb_page, 0, 0); ptr += strlen(ptr);
				strcpy(ptr, " at offset "); ptr += strlen(ptr);
				gds__ulstr(ptr, bdb->bdb_difference_page, 0, 0); ptr += strlen(ptr);
				strcpy(ptr, " in difference file");
				gds__trace(buffer);
#endif
				if (!database->backup_manager->write_difference(NULL, bdb->bdb_difference_page, bdb->bdb_buffer)) 
					{
					bdb->bdb_flags |= BDB_io_error;
					database->dbb_flags |= DBB_suspend_bgio;
					return false;
					}
				}
				
			if (bdb->bdb_write_direction == BDB_write_diff) 
				{
				// We finished. Adjust transaction accounting and get ready for exit
				if (bdb->bdb_page == HEADER_PAGE)
					database->dbb_last_header_write =
						((header_page*) page)->hdr_next_transaction;
				setWriteDirection (tdbb, bdb, BDB_write_undefined);
				}
			else 
				{
				// We need to write our pages to main database files
				file = database->dbb_file;
				
				while (!PIO_write(file, bdb, page,  NULL)) 
					{
					if (!rolloverToShadow(tdbb, file, inAst)) 
						{
						bdb->bdb_flags |= BDB_io_error;
						database->dbb_flags |= DBB_suspend_bgio;
						return false;
						}
					file = database->dbb_file;
					}

				if (bdb->bdb_page == HEADER_PAGE)
					{
					database->dbb_last_header_write = ((header_page*) page)->hdr_next_transaction;
					database->commitManager->headerWritten(((header_page*) page)->hdr_next_transaction);
					}
					
				if (database->dbb_shadow) 
					result = writeAllShadows(tdbb, NULL, bdb, 0, inAst);
				
				setWriteDirection (tdbb, bdb, BDB_write_undefined);
				}
			}

#ifdef SUPERSERVER
		if (result) 
			{
#ifdef CACHE_WRITER
			if (bdb->bdb_flags & BDB_checkpoint)
				--database->dbb_bcb->bcb_checkpoint;
#endif
			//bdb->bdb_flags &= ~(BDB_db_dirty | BDB_checkpoint);
			bdb->clearFlags(BDB_db_dirty | BDB_checkpoint);
			}
#endif
		AST_CHECK;
		}

	if (!result) 
		{
		/* If there was a write error then idle background threads
		   so that they don't spin trying to write these pages. This
		   usually results from device full errors which can be fixed
		   by someone freeing disk space. */

		bdb->bdb_flags |= BDB_io_error;
		database->dbb_flags |= DBB_suspend_bgio;
		}
	else 
		{
		/* clear the dirty bit vector, since the buffer is now
		   clean regardless of which transactions have modified it */

		bdb->bdb_transactions = 0;
		//bdb->bdb_mark_transaction = 0;

		/***
		//if (!(bcb_flags & BCB_keep_pages) && (bdb->bdb_parent || bdb == database->pageCache->bcb_btree))
		if (!(bcb_flags & BCB_keep_pages) && bdb->bdb_mark_sequence)
			removeDirtyPage(bdb);
		***/
		
		if (bdb->bdb_mark_sequence)
			removeDirtyPage(bdb);

		//bdb->bdb_flags &= ~(BDB_dirty | BDB_must_write | BDB_system_dirty);
		bdb->clearFlags(BDB_dirty | BDB_must_write | BDB_system_dirty);
		
		if (bdb->bdb_flags & BDB_io_error) 
			{
			/* If a write error has cleared, signal background threads
			   to resume their regular duties. If someone has freed up
			   disk space these errors will spontaneously go away. */

			bdb->bdb_flags &= ~BDB_io_error;
			database->dbb_flags &= ~DBB_suspend_bgio;
			}
		}
	
	return result;
}

void PageCache::journalBuffer(Bdb* bdb)
{
}

bool PageCache::setWriteDirection(thread_db* tdbb, Bdb* bdb, int direction)
{
#ifdef SUPERSERVER
	NBAK_TRACE(("set_write_direction page=%d old=%d new=%d", bdb->bdb_page,
				bdb->bdb_write_direction, direction));
				
	if (bdb->bdb_write_direction == BDB_write_normal ||
		bdb->bdb_write_direction == BDB_write_both) 
		{
		if (direction != BDB_write_normal && direction != BDB_write_both)		
			database->backup_manager->release_sw_database_lock(tdbb);
		}
	else 
		if (direction == BDB_write_normal || direction == BDB_write_both)		
			database->backup_manager->get_sw_database_lock(tdbb, true);
			
	bdb->bdb_write_direction = direction;
	
#else
	//LCK_ast_inhibit();
	
	switch(bdb->bdb_write_direction) 
		{
		case BDB_write_normal:
		case BDB_write_both:
			switch (direction) 
				{
				case BDB_write_diff:
					database->backup_manager->increment_diff_use_count();
					database->backup_manager->release_sw_database_lock();
					break;
					
				case BDB_write_undefined:
					database->backup_manager->release_sw_database_lock();
					break;
				}
			break;
			
		case BDB_write_diff:
			switch(direction) 
				{
				case BDB_write_normal:
				case BDB_write_both:
					database->backup_manager->decrement_diff_use_count();
					bdb->bdb_write_direction = direction;
					// We ask this function to enable signals
					if (!database->backup_manager->get_sw_database_lock(true)) 
						{
						bdb->bdb_write_direction = BDB_write_undefined;
						return false;
						}
					return true;
					
				case BDB_write_undefined:
					database->backup_manager->decrement_diff_use_count();
				}
			break;
			
		case BDB_write_undefined:
			switch (direction) 
				{
				case BDB_write_diff:
					database->backup_manager->increment_diff_use_count();
					break;
								
				case BDB_write_normal:
				case BDB_write_both:
					bdb->bdb_write_direction = direction;
					// We ask this function to enable signals
					if (!database->backup_manager->get_sw_database_lock(true)) 
						{
						bdb->bdb_write_direction = BDB_write_undefined;
						return false;
						}
					return true;
				}
			break;
		}
		
	bdb->bdb_write_direction = direction;
	//LCK_ast_enable();
#endif
	return true;
}

bool PageCache::rolloverToShadow(thread_db* tdbb, fil* file, bool inAst)
{
	/* Is the shadow subsystem yet initialized */
	
	if (!database->dbb_shadow_lock)
		return false;
		
	/* notify other process immediately to ensure all read from sdw
	   file instead of db file */
	   
	return SDW_rollover_to_shadow(tdbb, file, inAst);
}

bool PageCache::writeAllShadows(thread_db* tdbb, sdw* shadow, Bdb* bdb, int checksum, bool inAst)
{
	SLONG last, *spare_buffer = NULL;
	FIL next_file, shadow_file;
	header_page* header;
	PAG old_buffer, page;
	UCHAR *q;
	ISC_STATUS statusVector [20];
	SDW sdw = shadow ? shadow : database->dbb_shadow;
			

	if (!sdw) 
		return true;

	bool result = true;

	try 
		{
		if (bdb->bdb_page == HEADER_PAGE)
			{
			/* allocate a spare buffer which is large enough,
			   and set up to release it in case of error */

			spare_buffer =
				(SLONG *) database->dbb_bufferpool->allocate(database->dbb_page_size,0
#ifdef DEBUG_GDS_ALLOC
			  ,__FILE__,__LINE__
#endif
				);
			page = (PAG) spare_buffer;
			MOVE_FAST((UCHAR *) bdb->bdb_buffer, (UCHAR *) page, HDR_SIZE);
			old_buffer = bdb->bdb_buffer;
			bdb->bdb_buffer = page;
			}
		else 
			{
			page = bdb->bdb_buffer;
			if (checksum)
				page->pag_checksum = bdb->computeChecksum();
			}

		for (; sdw; sdw = sdw->sdw_next) 
			{
			/* don't bother to write to the shadow if it is no longer viable */

			/* Fix for bug 7925. drop_gdb fails to remove secondary file if
			   the shadow is conditional. Reason being the header page not
			   being correctly initialized.

			   The following block was not being performed for a conditional
			   shadow since SDW_INVALID expanded to include conditional shadow

			   -Sudesh  07/10/95
			   old code --> if (sdw->sdw_flags & SDW_INVALID)
			 */

			if ((sdw->sdw_flags & SDW_INVALID) &&
				!(sdw->sdw_flags & SDW_conditional)) 
				continue;

			if (bdb->bdb_page == HEADER_PAGE) 
				{
				/* fixup header for shadow file */
				shadow_file = sdw->sdw_file;
				header = (header_page*) page;
				q = (UCHAR *) database->dbb_file->fil_string;
				header->hdr_data[0] = HDR_end;
				header->hdr_end = HDR_SIZE;
				header->hdr_next_page = 0;

				PAG_add_header_entry(tdbb, header, HDR_root_file_name,
									 (USHORT) strlen((char *) q), q);

				if ( (next_file = shadow_file->fil_next) ) 
					{
					q = (UCHAR *) next_file->fil_string;
					last = next_file->fil_min_page - 1;
					PAG_add_header_entry(tdbb, header, HDR_file,
										 (USHORT) strlen((char *) q), q);
					PAG_add_header_entry(tdbb, header, HDR_last_page, sizeof(last),
										 (UCHAR *) & last);
					}

				header->hdr_flags |= hdr_active_shadow;
				header->hdr_header.pag_checksum = bdb->computeChecksum();
				}

			/* This condition makes sure that PIO_write is performed in case of
			   conditional shadow only if the page is Header page

				-Sudesh 07/10/95
			*/

			if ((sdw->sdw_flags & SDW_conditional) &&
				(bdb->bdb_page != HEADER_PAGE)) 
				continue;

			/* if a write failure happens on an AUTO shadow, mark the
			   shadow to be deleted at the next available opportunity when we
			   know we don't have a page fetched */

			if (!PIO_write(sdw->sdw_file, bdb, page, statusVector))
				if (sdw->sdw_flags & SDW_manual)
					result = false;
				else 
					{
					sdw->sdw_flags |= SDW_delete;
					if (!inAst && SDW_check_conditional(tdbb)) 
						if (SDW_lck_update(database, 0)) 
							{
							SDW_notify(tdbb);
							unwind(tdbb, FALSE);
							SDW_dump_pages(tdbb);
							ERR_post(isc_deadlock, 0);
							}
					}

			/* If shadow was specified, break out of loop */

			if (shadow)
				break;
			}

		if (bdb->bdb_page == HEADER_PAGE)
			bdb->bdb_buffer = old_buffer;

		if (spare_buffer)
			database->dbb_bufferpool->deallocate(spare_buffer);

		}	// try

	catch (...) 
		{
		if (spare_buffer) 
			database->dbb_bufferpool->deallocate(spare_buffer);
		
		throw;
		}

	return result;
}

void PageCache::removeDirtyPage(Bdb* bdb)
{
	if (bdb->bdb_mark_sequence)
		{
		Sync sync (&syncDirtyBdbs, "PageCache::removeDirtyPage");
		sync.lock(Exclusive);
		
		if (bdb->bdb_mark_sequence)
			{
			QUE_DELETE(bdb->bdb_dirty);
			bdb->bdb_mark_sequence = 0;
			}
		}
	
#ifdef OBSOLETE
	Bdb *ptr;
	Bdb	*new_child, *bdb_parent;

	/* engage in a little defensive programming to make
	   sure the node is actually in the tree */

	if ((!bcb_btree) ||
		(!bdb->bdb_parent && !bdb->bdb_left && !bdb->bdb_right && (bcb_btree != bdb))) 
		{
		if ((bdb->bdb_flags & BDB_must_write) || !(bdb->bdb_flags & BDB_dirty)) 
			/* Must writes aren't worth the effort */
			return;
		bugcheck(211);	/* msg 211 attempt to remove page from dirty page list when not there */
		}

	/* make a new child out of the left and right children */

	if (new_child = bdb->bdb_left) 
		{
		for (ptr = new_child; ptr->bdb_right; ptr = ptr->bdb_right)
			;
		if (ptr->bdb_right = bdb->bdb_right)
			ptr->bdb_right->bdb_parent = ptr;
		}
	else
		new_child = bdb->bdb_right;

	/* link the parent with the child node --
	   if no parent place it at the root */

	if (!(bdb_parent = bdb->bdb_parent))
		bcb_btree = new_child;
	else if (bdb_parent->bdb_left == bdb)
		bdb_parent->bdb_left = new_child;
	else
		bdb_parent->bdb_right = new_child;

	if (new_child)
		new_child->bdb_parent = bdb_parent;

	/* initialize the node for next usage */

	bdb->bdb_left = bdb->bdb_right = bdb->bdb_parent = NULL;
#endif
}

void PageCache::unwind(thread_db* tdbb, bool punt)
{
	/* CCH_unwind is called when any of the following occurs:
		- IO error
		- journaling error
		- bad page checksum
		- wrong page type
		- page locking (not latching) deadlock (doesn't happen on Netware) */

	if (tdbb->tdbb_flags & TDBB_no_cache_unwind) 
		{
		if (punt)	
			ERR_punt();
		return;
		}

	/* A cache error has occurred. Scan the cache for buffers
	   which may be in use and release them. */

	for (int n = 0; n < MAX_THREAD_BDBS; ++n)
		{
		Bdb *bdb = tdbb->tdbb_bdbs [n];
		if (bdb)
			{
			if (bdb->exclusive)
				//bdb->bdb_flags &= ~(BDB_writer | BDB_faked | BDB_must_write);
				bdb->clearFlags(BDB_writer | BDB_faked | BDB_must_write);
			bdb->release(tdbb);
			}
		}
		
#ifdef PRIOR_VERSION
	for (int n = 0; n < bcb_count; ++n)
		{
		Bdb *bdb = bdbs + n;

#ifndef SUPERSERVER
		if (bdb->bdb_length || !bdb->bdb_use_count) 
			continue;
	
		if (bdb->bdb_flags & BDB_marked) 
			bugcheck(268);	// msg 268 buffer marked during cache unwind 
		
		//bdb->bdb_flags &= ~BDB_writer;
		bdb->clearFlags(BDB_writer);
		
		while (bdb->bdb_use_count) 
			release(tdbb, bdb, TRUE, FALSE, FALSE);
		
		PAG page = bdb->bdb_buffer;

		if ((page->pag_type == pag_header) ||
			(page->pag_type == pag_transactions))
			{
			bdb->incrementUseCount();
			// Adjust backup page locks

			if (bdb->bdb_flags & BDB_dirty)
				set_write_direction(database, bdb, BDB_write_undefined);

			//bdb->bdb_flags &= ~(BDB_dirty | BDB_writer | BDB_marked | BDB_faked | BDB_db_dirty);
			bdb->clearFlags(BDB_dirty | BDB_writer | BDB_marked | BDB_faked | BDB_db_dirty);
			LCK_release(bdb->bdb_lock);
			bdb->decrementUseCount();
			}
#else
		if (!bdb->bdb_use_count)
			continue;

		/***	
		if (bdb->bdb_io == tdbb)
			release(tdbb, bdb, TRUE, FALSE, FALSE);

		if (bdb->bdb_exclusive == tdbb) 
			{
			if ((bdb->bdb_flags & BDB_marked))
				bugcheck(268);	// msg 268 buffer marked during cache unwind 

			bdb->bdb_flags &= ~(BDB_writer | BDB_faked | BDB_must_write);
			release(tdbb, bdb, TRUE, FALSE, FALSE);
			}

		for (int i = 0; i < BDB_max_shared; ++i) 
			if (bdb->bdb_shared[i] == tdbb) 
				release(tdbb, bdb, TRUE, FALSE, FALSE);
		***/
#endif
		}
#endif

	if (punt)
		ERR_punt();
}

/**************************************
 *
 *	r e l e a s e _ b d b
 *
 **************************************
 *
 * Functional description
 *	Decrement the use count of a bdb, reposting
 *	blocking AST if required.
 *	If rel_mark_latch is TRUE, the the value of downgrade_latch is ignored.
 *
 **************************************/

void PageCache::releaseLatch(thread_db* tdbb, Bdb* bdb, bool repost, bool downgrade_latch, bool rel_mark_latch)
{
	bdb->incrementUseCount();

	if (rel_mark_latch)
		bdb->bdb_io = 0;
	else if (downgrade_latch)
		bdb->bdb_exclusive = 0;
	else if (bdb->bdb_exclusive == tdbb) 
		{
		//bdb->release();
		if (bdb->decrementUseCount() == 0)
			bdb->bdb_exclusive = bdb->bdb_io = 0;
		else if (bdb->bdb_io) 
			{	/* This is a release for an io or an exclusive latch */
			if (bdb->bdb_io == tdbb) 
				{	/* We have an io latch */

				/* The BDB_must_write flag causes the system to latch for io, in addition
				   to the already owned latches.  Make sure not to disturb an already existing
				   exclusive latch. */
				if (!(bdb->bdb_flags & BDB_marked))
					bdb->bdb_io = 0;
				}
			else if (bdb->bdb_use_count == 1)
				/* This must be a release for our exclusive latch */
				bdb->bdb_exclusive = 0;
			}
		}
	else
		/* If the exclusive latch is not held, then things have to behave much nicer. */
		{
		--bdb->bdb_use_count;
		bdb->bdb_io = 0;
		}

	bdb->decrementUseCount();

	if (bdb->bdb_use_count || !repost)
		return;

	if (bdb->bdb_ast_flags & BDB_blocking)
		LCK_re_post(bdb->bdb_lock);
}

void PageCache::bugcheck(int msgNumber)
{
	BUGCHECK(msgNumber);
}

void PageCache::flushDatabase(thread_db* tdbb)
{
#ifdef SUPERSERVER
	// This is called on architectures with shared buffer cache (like SuperServer)
	// Redirect pages to the difference file
	// Need to reconsider this protection for possible deadlocks when MT-safety is implemented

	for (ULONG i = 0; i < bcb_count; i++) 
		{
		Bdb *bdb = bdbs[i];
		if (bdb->bdb_length)
			continue;
		if (bdb->bdb_write_direction != BDB_write_normal &&
			bdb->bdb_write_direction != BDB_write_both)
			continue;
		NBAK_TRACE(("Redirect page=%d use=%d flags=%d", bdb->bdb_page, bdb->bdb_use_count, bdb->bdb_flags));
		updateWriteDirection(tdbb, bdb);
		}

#else		
	/* Do some fancy footwork to make sure that pages are
	   not removed from the btc tree at AST level.  Then
	   restore the flag to whatever it was before. */

	const bool keep_pages = bcb_flags & BCB_keep_pages;
	database->dbb_bcb->bcb_flags |= BCB_keep_pages;
	
	for (ULONG i = 0; (bcb = database->dbb_bcb) && i < bcb_count; i++) 
		{
		Bdb* bdb = bcb_rpt[i].bcb_bdb;
		
		if (bdb->bdb_length)
			continue;
			
		if (!(bdb->bdb_flags & (BDB_dirty | BDB_db_dirty)) || 
			bdb->bdb_write_direction == BDB_write_diff)
			continue;

		down_grade(tdbb, bdb);
		}
		
	if (!keep_pages)
		bcb_flags &= ~BCB_keep_pages;
#endif
}

void PageCache::updateWriteDirection(thread_db* tdbb, Bdb* bdb)
{
	SSHORT write_direction;
	BackupManager *backupManager = database->backup_manager;
	
	if (!backupManager->lock_state(tdbb, true)) 
		invalidateAndRelease(tdbb, bdb);

#ifndef SUPERSERVER
	bdb->bdb_diff_generation = backupManager->get_current_generation();
#endif

	// SCN of header page is adjusted in nbak.cpp	
	
	if (bdb->bdb_page != HEADER_PAGE) 
		bdb->bdb_buffer->pag_scn = backupManager->get_current_scn(); 
		
	// Set SCN for the page

	int backup_state = backupManager->get_state();

	switch (backup_state) 
		{
		case nbak_state_normal:
			write_direction = BDB_write_normal; 
			break;

		case nbak_state_stalled:
			write_direction = BDB_write_diff;
			break;

		case nbak_state_merge:
			if (tdbb->tdbb_flags & TDBB_backup_merge || 
				bdb->bdb_page < backupManager->get_backup_pages())
				write_direction = BDB_write_normal;
			else
				write_direction = BDB_write_both;
			break;
		}

	switch (write_direction)
		{
		case BDB_write_diff:
			if (!backupManager->lock_alloc(tdbb, true)) 
				{
				backupManager->unlock_state(tdbb);
				invalidateAndRelease(tdbb, bdb);
				}

			bdb->bdb_difference_page = backupManager->get_page_index(bdb->bdb_page);
			backupManager->unlock_alloc(tdbb);

			if (!bdb->bdb_difference_page) 
				{
				if (!backupManager->lock_alloc_write(tdbb, true)) 
					{
					backupManager->unlock_state(tdbb);
					invalidateAndRelease(tdbb, bdb);
					}
				bdb->bdb_difference_page = backupManager->allocate_difference_page(tdbb, bdb->bdb_page);
				backupManager->unlock_alloc_write(tdbb);

				if (!bdb->bdb_difference_page) 
					{
					backupManager->unlock_state(tdbb);
					invalidateAndRelease(tdbb, bdb);
					}

				NBAK_TRACE(("Allocate difference page %d for database page %d", 
					bdb->bdb_difference_page, bdb->bdb_page));
				} 
			else 
				NBAK_TRACE(("Map existing difference page %d to database page %d", 
					bdb->bdb_difference_page, bdb->bdb_page));
			break;

		case BDB_write_both:
			if (!backupManager->lock_alloc(tdbb, true)) 
				{
				backupManager->unlock_state(tdbb);
				invalidateAndRelease(tdbb, bdb);
				}

			bdb->bdb_difference_page = backupManager->get_page_index(bdb->bdb_page);
			backupManager->unlock_alloc(tdbb);

			if (bdb->bdb_difference_page) 
				NBAK_TRACE(("Map existing difference page %d to database page %d (write_both)", 
					bdb->bdb_difference_page, bdb->bdb_page));
			else 
				// This may really happen. Database file can grow while in merge mode
				write_direction = BDB_write_normal;
			
			break;
		}

	if (!setWriteDirection(tdbb, bdb, write_direction))
		{
		backupManager->unlock_state(tdbb);
		invalidateAndRelease(tdbb, bdb);
		}

	backupManager->unlock_state(tdbb);
}

void PageCache::invalidateAndRelease(thread_db* tdbb, Bdb* bdb)
{
	//bdb->bdb_flags |= BDB_not_valid;
	bdb->setFlags(BDB_not_valid);
	//bdb->bdb_flags &= ~BDB_dirty;
	bdb->clearFlags(BDB_dirty);
	setWriteDirection(tdbb,  bdb, BDB_write_undefined);
	TRA_invalidate(database, bdb->bdb_transactions);
	bdb->bdb_transactions = 0;
	//release(tdbb, bdb, FALSE, FALSE, FALSE);
	unwind(tdbb, TRUE);
}

int PageCache::downGradeDbb(void* database)
{
	return ((Database*) database)->pageCache->downGradeDatabase();
}

int PageCache::downGradeDatabase(void)
{
	/* Since this routine will be called asynchronously, we must establish
	   a thread context. */
	
	ISC_STATUS_ARRAY statusVector;
	ThreadData threadData (statusVector, database);
	database->dbb_ast_flags |= DBB_blocking;

	/* Database shutdown will release the database lock; just return. */

	if (SHUT_blocking_ast(threadData, database)) 
		{
		database->dbb_ast_flags &= ~DBB_blocking;
		return 0;
		}

	/* If we are already shared, there is nothing more we can do.
	   If any case, the other guy probably wants exclusive access,
	   and we can't give it anyway */
	   
	LCK lock = database->dbb_lock;

	if ((lock->lck_logical == LCK_SW) || (lock->lck_logical == LCK_SR)) 
		return 0;

	if (database->dbb_flags & DBB_bugcheck) 
		{
		LCK_convert(threadData, lock, LCK_SW, LCK_WAIT);
		database->dbb_ast_flags &= ~DBB_blocking;
		return 0;
		}

	/* If we are supposed to be exclusive, stay exclusive */

	if (database->dbb_flags & DBB_exclusive) 
		return 0;

	/* Assert any page locks that have been requested, but not asserted */

	ISC_ast_enter();
	database->dbb_ast_flags |= DBB_assert_locks;

	for (int n = 0; n < bcb_count; ++n)
		{
		Bdb *bdb = bdbs[n];
		LCK_assert(threadData, bdb->bdb_lock);
		}

	/* Down grade the lock on the database itself, letting the cache manage in first */

	int lockType = (lock->lck_physical == LCK_EX) ? LCK_PW : 
						(database->dbb_flags & DBB_cache_manager) ? LCK_SR : LCK_SW;
	LCK_convert(threadData, lock, lockType, LCK_WAIT);
	database->dbb_ast_flags &= ~DBB_blocking;
	ISC_ast_exit();

	/* Restore the prior thread context */

	return 0;
}

void PageCache::insertDirtyPage(Bdb* bdb)
{
	if (bdb->bdb_mark_sequence)
		return;

	Sync sync(&syncDirtyBdbs, "PageCache::insertDirtyPage");
	sync.lock(Exclusive);
	
	if (bdb->bdb_mark_sequence)
		return;
		
	if (!(bdb->bdb_flags & BDB_dirty))
		bugcheck(-1);
		
	bdb->bdb_mark_sequence = ++markSequence;
	QUE_APPEND(bcb_dirty, bdb->bdb_dirty);
	
#ifdef OBSOLETE
	/* if the page is already in the tree (as in when it is
	   written out as a dependency while walking the tree),
	   just leave well enough alone -- this won't check if
	   it's at the root but who cares then */

	if (bdb->bdb_parent)
		return;

	/* if the tree is empty, this is now the tree */

	Bdb* node = bcb_btree;
	
	if (!node) 
		{
		bcb_btree = bdb;
		return;
		}

	/* insert the page sorted by page number;
	   do this iteratively to minimize call overhead */

	for (SLONG page = bdb->bdb_page; page != node->bdb_page;) 
		{
		if (page < node->bdb_page) 
			{
			if (!node->bdb_left) 
				{
				node->bdb_left = bdb;
				bdb->bdb_parent = node;
				break;
				}
			else
				node = node->bdb_left;
			}
		else 
			{
			if (!node->bdb_right) 
				{
				node->bdb_right = bdb;
				bdb->bdb_parent = node;
				break;
				}
			else
				node = node->bdb_right;
			}
		}
#endif
}

/**************************************
 *
 * Functional description
 *	Fake a fetch to a page.  Rather than reading it, however,
 *	zero it in memory.  This is used when allocating a new page.
 *
 * input
 *	latch_wait:	1 => Wait as long as necessary to get the latch.
 *				This can cause deadlocks of course.
 *			0 => If the latch can't be acquired immediately,
 *				or an IO would be necessary, then give
 *				up and return 0.
 *	      		<negative number> => Latch timeout interval in seconds.
 *
 * return
 *	pag pointer if successful.
 *	NULL pointer if timeout occurred (only possible if latch_wait <> 1).
 *	NULL pointer if latch_wait=0 and the faked page would have to be
 *			before reuse.
 *
 **************************************/

pag* PageCache::fake(thread_db* tdbb, win* window)
{
	/* if there has been a shadow added recently, go out and
	   find it before we grant any more write locks */

	if (getShadows)
		SDW_get_shadows(tdbb);

	Bdb *bdb = getBuffer(tdbb, window->win_page, LCK_write);

	/* If a dirty orphaned page is being reused - better write it first
	   to clear current precedences and checkpoint state. This would also
	   update the bcb_free_pages field appropriately. */

	if (bdb->bdb_flags & (BDB_dirty | BDB_db_dirty)) 
		{
		if (!writeBuffer (tdbb, bdb, bdb->bdb_page, true, tdbb->tdbb_status_vector, true))
			unwind(tdbb, TRUE);
		}
	else if (QUE_NOT_EMPTY(bdb->bdb_lower)) 
		clearPrecedenceSync(bdb);	// Clear residual precedence left over from AST-level I/O.

	//bdb->bdb_flags = (BDB_writer | BDB_faked);
	bdb->setFlags(BDB_writer | BDB_faked, ~BDB_lru_chained);
	bdb->bdb_scan_count = 0;

	lockBuffer(tdbb, bdb, LCK_WAIT, pag_undefined);

	MOVE_CLEAR(bdb->bdb_buffer, database->dbb_page_size);
	window->win_buffer = bdb->bdb_buffer;
	window->win_expanded_buffer = NULL;
	window->win_bdb = bdb;
	window->win_flags = 0;
	mark(tdbb, window, 0);

	return bdb->bdb_buffer;
}

/**************************************
 *
 *	g e t _ b u f f e r
 *
 **************************************
 *
 * Functional description
 *	Get a buffer.  If possible, get a buffer already assigned
 *	to the page.  Otherwise get one from the free list or pick
 *	the least recently used buffer to be reused.
 *	Note the following special page numbers:
 *	     -1 indicates that a buffer is required for journaling
 *	     -2 indicates a special scratch buffer for shadowing
 *
 * input
 *	page:		page to get
 *	latch:		type of latch to acquire on the page.
 *	latch_wait:	1 => Wait as long as necessary to get the latch.
 *				This can cause deadlocks of course.
 *			0 => If the latch can't be acquired immediately,
 *				give up and return 0;
 *	      		<negative number> => Latch timeout interval in seconds.
 *
 * return
 *	bdb pointer if successful.
 *	NULL pointer if timeout occurred (only possible is latch_wait <> 1).
 *		     if cache manager doesn't have any pages to write anymore.
 *
 **************************************/

Bdb* PageCache::getBuffer(thread_db* tdbb, SLONG page, int lock_type)
{
	QUE que;
	Bdb *oldest;
	PRE precedence;
	int walk = bcb_free_minimum;
	LockType lockType = (lock_type == LCK_read) ? Shared : Exclusive;
	//LatchType latch = (lock_type == LCK_read) ? LATCH_shared : LATCH_exclusive;
	Sync sync (&syncObject, "PageCache::getBuffer");
	sync.lock(Shared);
	Bdb *bdb = findBuffer(page);
	
	if (bdb)
		{
		bdb->incrementUseCount();
		bdb->bdb_sequence = database->dbb_fetches++;
		sync.unlock();
		
		if (!(bdb->bdb_flags & BDB_free_pending))
			recentlyUsed(bdb);
			
		bdb->addRef (tdbb, lockType);
		bdb->decrementUseCount();
		
		return bdb;
		}
	
	sync.unlock();
	sync.lock (Exclusive);
	Sync lru(&syncLRU, "PageCache::getBuffer");
	
	while (true) 
		{
		//validate();
		// If page is already in cache, everything is easy
		
		if (page >= 0) 
			if (bdb = findBuffer(page))
				{
				bdb->bdb_sequence = database->dbb_fetches++;
				bdb->incrementUseCount();
				sync.unlock();
				
				if (!(bdb->bdb_flags & BDB_free_pending))
					recentlyUsed(bdb);
					
				bdb->addRef (tdbb, lockType);
				bdb->decrementUseCount();
				
				return bdb;
				}
			
#ifdef CACHE_WRITER

		else if ((page == FREE_PAGE) || (page == CHECKPOINT_PAGE)) 
			{
			/* This code is only used by the background I/O threads:
			   cache writer, cache reader and garbage collector. */

			lru.lock(Exclusive);
			
			for (que = bcb_in_use.que_backward; que != &bcb_in_use; que = que->que_backward)
				{
				bdb = BLOCK(que, Bdb*, bdb_in_use);
				if (page == FREE_PAGE) 
					{
					if (bdb->bdb_use_count || bdb->bdb_flags & BDB_free_pending) 
						continue;
					if (bdb->bdb_flags & BDB_db_dirty) 
						return bdb;
					if (!--walk) 
						{
						bcb_flags &= ~BCB_free_pending;
						break;
						}
					}
				else if (bdb->bdb_flags & BDB_checkpoint) 
					return bdb;
				}

			return NULL;
			}
#endif

		/* If there is an empty buffer sitting around, allocate it */

		if (QUE_NOT_EMPTY(bcb_empty)) 
			{
			que = bcb_empty.que_forward;
			QUE_DELETE((*que));
			bdb = BLOCK(que, Bdb*, bdb_que);
			
			if (page >= 0) 
				{
				QUE mod_que = hashTable + (page % bcb_count);
				QUE_INSERT((*mod_que), (*que));
				lru.lock(Exclusive);
				QUE_INSERT(bcb_in_use, bdb->bdb_in_use);
				}

			/* This correction for bdb_use_count below is needed to
				avoid a deadlock situation in latching code.  It's not
				clear though how the bdb_use_count can get < 0 for a bdb
				in bcb_empty queue */
				
			if (bdb->bdb_use_count < 0)
				bugcheck(301);	/* msg 301 Non-zero use_count of a buffer in the empty que */

			bdb->bdb_page = page;
			bdb->bdb_flags = BDB_read_pending;		// we have buffer exclusively, this is safe
			bdb->bdb_scan_count = 0;
			
			if (page >= 0)
				bdb->bdb_lock->lck_logical = LCK_none;
			else
				LCK_release(bdb->bdb_lock);

			bdb->bdb_sequence = database->dbb_fetches++;
			
			/***
			bdb->incrementUseCount();
			//validate();
			sync.unlock();
			bdb->addRef (tdbb, Exclusive);
			bdb->decrementUseCount();
			***/
			
			bdb->addRef (tdbb, Exclusive);
			sync.unlock();
			
			return bdb;
			}
		
		// Find the oldest available buffer and try that

		lru.lock(Exclusive);
		
		if (bcb_lru_chain)
			requeueRecentlyUsed();
			
		for (que = bcb_in_use.que_backward; que != &bcb_in_use || QUE_NOT_EMPTY(bcb_empty);
			 que = que->que_backward) 
			{
			/* get the oldest buffer as the least recently used -- note
			   that since there are no empty buffers this queue cannot be empty */

			if (bcb_in_use.que_forward == &bcb_in_use)
				bugcheck(213);	/* msg 213 insufficient cache size */

			oldest = BLOCK(que, Bdb*, bdb_in_use);
			
			if (oldest->bdb_use_count || (oldest->bdb_flags & BDB_free_pending) || !writeable(oldest))
				continue;
			
			if (oldest->bdb_flags & BDB_lru_chained)
				continue;

#ifdef CACHE_WRITER
			if (oldest->bdb_flags & (BDB_dirty | BDB_db_dirty)) 
				{
				bcb_flags |= BCB_free_pending;
				
				if (bcb_flags & BCB_cache_writer && !(bcb_flags & BCB_writer_active))
					database->dbb_writer_event->post();
					
				if (walk) 
					{
					if (!--walk)
						break;
					continue;
					}
				}
#endif
			lru.unlock();
			bdb = oldest;
			sync.unlock();
			recentlyUsed(bdb);
			bdb->setFlags(BDB_free_pending);
			bdb->bdb_pending_page = page;

			if (bdb->bdb_page >= 0)
				{
				QUE_DELETE(bdb->bdb_que);
				QUE_INSERT(bcb_pending, bdb->bdb_que);
				}
			
			bdb->addRef(tdbb, Exclusive);
			
			/* If the buffer selected is dirty, arrange to have it written. */

			if (bdb->bdb_flags & (BDB_dirty | BDB_db_dirty)) 
				{
#ifdef SUPERSERVER
				if (!writeBuffer (tdbb, bdb, bdb->bdb_page, true, tdbb->tdbb_status_vector,	true))
#else
				if (!writeBuffer (tdbb, bdb, bdb->bdb_page, false, tdbb->tdbb_status_vector, true))
#endif
					{
					bdb->clearFlags(BDB_free_pending);
					bdb->release(tdbb);
					unwind(tdbb, TRUE);
					}
					
				//sync.lock(Exclusive, "PageCache::getBuffer");
				}

			/* If the buffer is still in the dirty tree, remove it.
			   In any case, release any lock it may have. */

			if (bdb->bdb_mark_sequence)
				removeDirtyPage(bdb);
			
			/* if the page has an expanded index buffer, release it */

			/***
			if (bdb->bdb_expanded_buffer) 
				{
				delete bdb->bdb_expanded_buffer;
				bdb->bdb_expanded_buffer = NULL;
				}
			***/
			
			/* Cleanup any residual precedence blocks.  Unless something is
			   screwed up, the only precedence blocks that can still be hanging
			   around are ones cleared at AST level. */

			if (QUE_NOT_EMPTY(bdb->bdb_higher) || QUE_NOT_EMPTY(bdb->bdb_lower)) 
				{
				Sync syncPrec(&syncPrecedence, "PageCache::getBuffer");
				syncPrec.lock(Exclusive);
				
				while (QUE_NOT_EMPTY(bdb->bdb_higher)) 
					{
					QUE que2 = bdb->bdb_higher.que_forward;
					precedence = BLOCK(que2, PRE, pre_higher);
					deletePrecedence(precedence);
					}
					
				clearPrecedence(bdb);
				}
			
#ifdef OBSOLETE
			/* remove the buffer from the "mod" queue and place it
			   in it's new spot, provided it's not a negative (scratch) page */

			if (bdb->bdb_page >= 0)
				{
				QUE_DELETE(bdb->bdb_que);		// bcb_pending
				//QUE_INSERT(bcb_pending, bdb->bdb_que);
				}
				
			QUE_INSERT(bcb_empty, bdb->bdb_que);
			QUE_DELETE(bdb->bdb_in_use)
			bdb->bdb_page = JOURNAL_PAGE;
			//release(tdbb, bdb, FALSE, FALSE, FALSE);
			//bdb->release();
			break;
#endif
			
			if (page >= 0) 
				{
				QUE_DELETE(bdb->bdb_que);		// bcb_pending
				QUE mod_que = hashTable + (page % bcb_count);
				QUE_INSERT((*mod_que), bdb->bdb_que);
				}

			/* This correction for bdb_use_count below is needed to
				avoid a deadlock situation in latching code.  It's not
				clear though how the bdb_use_count can get < 0 for a bdb
				in bcb_empty queue */
				
			if (bdb->bdb_use_count < 0)
				bugcheck(301);	/* msg 301 Non-zero use_count of a buffer in the empty Que */

			bdb->bdb_page = page;
			bdb->setFlags(BDB_read_pending, ~BDB_lru_chained);
			bdb->bdb_scan_count = 0;
				
			if (page >= 0)
				bdb->bdb_lock->lck_logical = LCK_none;
			else
				LCK_release(bdb->bdb_lock);
			
			return bdb;
			}

		if (que == &bcb_in_use)
			expandBuffers(tdbb, bcb_count + 75);
		}
}

void PageCache::clearPrecedenceSync(Bdb* bdb)
{
	if (QUE_EMPTY(bdb->bdb_lower))
		return;
		
	Sync sync(&syncPrecedence, "");
	sync.lock(Exclusive);
	clearPrecedence(bdb);
}

void PageCache::clearPrecedence(Bdb* bdb)
{
	/* Loop thru lower precedence buffers.  If any can be downgraded,
	   by all means down grade them. */

	while (QUE_NOT_EMPTY(bdb->bdb_lower)) 
		{
		QUE que = bdb->bdb_lower.que_forward;
		PRE precedence = BLOCK(que, PRE, pre_lower);
		Bdb *low_bdb = precedence->pre_low;
		int flags = precedence->pre_flags;
		deletePrecedence(precedence);
		
		//precedence->pre_hi = (Bdb*) bcb_free;
		//bcb_free = precedence;
		
		//if (!(precedence->pre_flags & PRE_cleared)) 
		if (!(flags & PRE_cleared) && (low_bdb->bdb_ast_flags & BDB_blocking))
			LCK_re_post(low_bdb->bdb_lock);
		}
}

/**************************************
 *
 *	w r i t e _ b u f f e r
 *
 **************************************
 *
 * Functional description
 *	Write a dirty buffer.  This may recurse due to
 *	precedence problems.
 *
 * input:  write_this_page
 *		= true if the input page needs to be written
 *			before returning.  (normal case)
 *		= false if the input page is being written
 *			because of precedence.  Only write
 *			one page and return so that the caller
 *			can re-establish the need to write this
 *			page.
 *
 * return: 0 = Write failed.
 *         1 = Page is written.  Page was written by this
 *     		call, or was written by someone else, or the
 *		cache buffer is already reassigned.
 *	   2 = Only possible if write_this_page is false.
 *		This input page is not written.  One
 *		page higher in precedence is written
 *		though.  Probable action: re-establich the
 * 		need to write this page and retry write.
 *
 **************************************/

int PageCache::writeBuffer(thread_db* tdbb, Bdb* bdb, SLONG page, bool write_thru, ISC_STATUS* status, bool write_this_page)
{
	QUE que;
	PRE precedence;
	Bdb	*hi_bdb;
	SLONG hi_page;
	int write_status;

	//latchBdb(tdbb, LATCH_io, bdb, page);
	/***
	if (!bdb->syncObject.isLocked())
		printf ("bdb not locked\n");
	***/
		
	bdb->bdb_io = tdbb;

	if ((bdb->bdb_flags & BDB_marked) && !(bdb->bdb_flags & BDB_faked))
		bugcheck(217);	/* msg 217 buffer marked for update */

	if (!(bdb->bdb_flags & BDB_dirty) && !(write_thru && bdb->bdb_flags & BDB_db_dirty))
		{
		clearPrecedenceSync(bdb);
		//release(tdbb, bdb, TRUE, FALSE, FALSE);
		return 1;
		}

	/* If there are buffers that must be written first, write them now. */

	if (QUE_NOT_EMPTY(bdb->bdb_higher)) 
		{
		Sync sync(&syncPrecedence, "PageCache::writeBuffer");
		
		for (;;)
			{
			sync.lock(Exclusive);
			
			if (QUE_EMPTY(bdb->bdb_higher))
				break;

			que = bdb->bdb_higher.que_forward;
			precedence = BLOCK(que, PRE, pre_higher);
			
			if (precedence->pre_flags & PRE_cleared) 
				{
				deletePrecedence(precedence);
				sync.unlock();
				}
			else 
				{
				hi_bdb = precedence->pre_hi;
				hi_page = hi_bdb->bdb_page;
				sync.unlock();
				
				if (hi_bdb->ourExclusiveLock())
					write_status = writeBuffer(tdbb, hi_bdb, hi_page, write_thru, status, false);
				else
					{
					hi_bdb->addRef (NULL, Shared);
					write_status = writeBuffer(tdbb, hi_bdb, hi_page, write_thru, status, false);
					hi_bdb->release(NULL);
					}
				
				if (write_status == 0)
					return 0;		/* return IO error */
					
#ifdef SUPERSERVER
				if (!write_this_page)
					return 2;		/* caller wants to re-establish the need for this write after one precedence write */
#endif

				//latchBdb(tdbb, LATCH_io, bdb, page);
				bdb->bdb_io = tdbb;
				}
			}
		}

	/* Unless the buffer has been faked (recently re-allocated), write
	   out the page */

	bool result = true;
	
	if ((bdb->bdb_flags & BDB_dirty || (write_thru && bdb->bdb_flags & BDB_db_dirty)) &&
		 !(bdb->bdb_flags & BDB_marked))
		{
		if (result = writePage(tdbb, bdb, write_thru, false))
			clearPrecedenceSync(bdb);
		}
	else
		clearPrecedenceSync(bdb);

	//bdb->release(tdbb);
	//release(tdbb, bdb, TRUE, FALSE, FALSE);

	if (!result)
		return 0;
#ifdef SUPERSERVER
	else if (!write_this_page)
		return 2;
#endif
	else
		return 1;
}

/**************************************
 *
 *	Get a lock on page for a buffer.  If the lock ever slipped
 *	below READ, indicate that the page must be read.
 *
 * input:
 *	wait: LCK_WAIT = TRUE = 1	=> Wait as long a necessary to get the lock.
 *	      LCK_NO_WAIT = FALSE = 0	=> If the lock can't be acquired immediately,
 *						give up and return -1.
 *	      <negative number>		=> Lock timeout interval in seconds.
 *
 * return: 0  => buffer locked, page is already in memroy.
 *	   1  => buffer locked, page needs to be read from disk.
 *	   -1 => timeout on lock occurred, see input parameter 'wait'.
 *
 **************************************/
 
LockState PageCache::lockBuffer(thread_db* tdbb, Bdb* bdb, int wait, int page_type)
{
	ISC_STATUS_ARRAY alt_status;
	ISC_STATUS *status;
	TEXT errmsg[MAX_ERRMSG_LEN + 1];

	int lock_type = (bdb->bdb_flags & (BDB_dirty | BDB_writer)) ? LCK_write : LCK_read;
	LCK lock = bdb->bdb_lock;

	if (lock->lck_logical >= lock_type)
		return lsLockedHavePage;

	status = NULL;

	if (lock->lck_logical == LCK_none) 
		{
		/* Prevent header and TIP pages from generating blocking AST
		   overhead. The promise is that the lock will unconditionally
		   be released when the buffer use count indicates it is safe
		   to do so. */

		/***
		if (page_type == pag_header || page_type == pag_transactions) 
			{
			//fb_assert(lock->lck_ast == bdbBlockingAst);
			//fb_assert(lock->lck_object == reinterpret_cast<blk*>(bdb));
			lock->lck_ast = 0;
			lock->lck_object = NULL;
			}
		else
			fb_assert(lock->lck_ast != NULL);
		***/
		
		lock->lck_key.lck_long = bdb->bdb_page;
		
		if (LCK_lock_opt(tdbb, lock, lock_type, wait)) 
			{
			if (!lock->lck_ast) 
				{
				/* Restore blocking AST to lock block if it was swapped
				   out. Flag the bdb so that the lock is released when
				   the buffer is released. */

				fb_assert(page_type == pag_header || page_type == pag_transactions);
				lock->lck_ast = bdbBlockingAst;
				lock->lck_object = reinterpret_cast<blk*>(bdb);
				//bdb->bdb_flags |= BDB_no_blocking_ast;
				bdb->setFlags(BDB_no_blocking_ast);
				}
			return lsLocked;
			}

		if (!lock->lck_ast) 
			{
			fb_assert(page_type == pag_header || page_type == pag_transactions);
			lock->lck_ast = bdbBlockingAst;
			lock->lck_object = reinterpret_cast<blk*>(bdb);
			}

		/* Case: a timeout was specified, or the caller didn't want to wait,
		   return the error. */

		if ((wait == LCK_NO_WAIT) || ((wait < 0) && (status[1] == isc_lock_timeout))) 
			{
			//release(tdbb, bdb, FALSE, FALSE, FALSE);
			bdb->release(tdbb);
			return lsLockTimeout;
			}

		/* Case: lock manager detected a deadlock, probably caused by locking the
		   bdb's in an unfortunate order.  Nothing we can do about it, return the
		   error, and log it to firebird.log. */

		gds__msg_format(0, JRD_BUGCHK, 215, sizeof(errmsg), errmsg,
						(TEXT *) (long) bdb->bdb_page, 
						(TEXT *) (long) page_type, 0, 0, 0);
		IBERR_append_status(status, isc_random, isc_arg_string,
							ERR_cstring(errmsg), 0);
		ERR_log (JRD_BUGCHK, 215, errmsg);	/* msg 215 page %ld, page type %ld lock conversion denied */

		/* CCH_unwind releases all the bdb's and calls ERR_punt()
		   ERR_punt will longjump. */

		unwind (tdbb, TRUE);
		}

	/* Lock requires an upward conversion.  Sigh.  Try to get the conversion.
	   If it fails, release the lock and re-seize. Save the contents of the
	   status vector just in case */

	LockState must_read = (lock->lck_logical < LCK_read) ? lsLocked : lsLockedHavePage;
	memcpy(alt_status, tdbb->tdbb_status_vector, sizeof(alt_status));

	if (LCK_convert_opt(tdbb, lock, lock_type))
		return must_read;
		
	if (wait == LCK_NO_WAIT) 
		{
		//release(tdbb, bdb, TRUE, FALSE, FALSE);
		bdb->release(tdbb);
		return lsLockTimeout;
		}

	//memcpy(tdbb->tdbb_status_vector, alt_status, sizeof(alt_status));

	if (LCK_lock(tdbb, lock, lock_type, wait))
		return lsLocked;

	/* Case: a timeout was specified, or the caller didn't want to wait,
	   return the error. */

	if ((wait < 0) && (status[1] == isc_lock_timeout)) 
		{
		//release(tdbb, bdb, FALSE, FALSE, FALSE);
		bdb->release(tdbb);
		return lsLockTimeout;
		}

	/* Case: lock manager detected a deadlock, probably caused by locking the
	   bdb's in an unfortunate order.  Nothing we can do about it, return the
	   error, and log it to firebird.log. */

	gds__msg_format(0, JRD_BUGCHK, 216, sizeof(errmsg), errmsg,
					(TEXT*) (long) bdb->bdb_page, 
					(TEXT*) (long) page_type, 0, 0, 0);
	IBERR_append_status(status, isc_random, isc_arg_string, errmsg, 0);
	ERR_log(JRD_BUGCHK, 216, errmsg);	/* msg 216 page %ld, page type %ld lock denied */
	unwind(tdbb, TRUE);
	
	return lsError;					/* Added to get rid of Compiler Warning */
}

void PageCache::mark(thread_db* tdbb, win* window, int mark_system)
{
	Transaction* transaction;
	ULONG trans_bucket;
	SLONG number;

	database->dbb_marks++;
	Bdb *bdb = window->win_bdb;

	/* A LATCH_mark is needed before the bdb can be marked.
	   This prevents a write while the page is being modified. */

	//latchBdb(tdbb, LATCH_mark, bdb, bdb->bdb_page);
	bdb->bdb_io = tdbb;
	bdb->bdb_incarnation = ++database->dbb_page_incarnation;

	if (!(bdb->bdb_flags & BDB_writer))
		BUGCHECK(208);			/* msg 208 page not accessed for write */

	/* mark the dirty bit vector for this specific transaction,
	   if it exists; otherwise mark that the system transaction
	   has updated this page */

	int newFlags = 0;
	
	if ((transaction = tdbb->tdbb_transaction) && (number = transaction->tra_number)) 
		{
		if (!(tdbb->tdbb_flags & TDBB_sweeper)) 
			{
			trans_bucket = number & (BITS_PER_LONG - 1);
			bdb->bdb_transactions |= (1L << trans_bucket);
			
			/*** unused feature
			if (number > bdb->bdb_mark_transaction)
				bdb->bdb_mark_transaction = number;
			***/
			}
		}
	else
		newFlags |= BDB_system_dirty;

	if (mark_system)
		newFlags |= BDB_system_dirty;

#ifdef SUPERSERVER
	//bdb->bdb_flags |= BDB_db_dirty;
	newFlags |= BDB_db_dirty;
#endif

	newFlags |= (BDB_dirty | BDB_marked);
	bdb->setFlags(newFlags);
	
	if (!(tdbb->tdbb_flags & TDBB_sweeper) || bdb->bdb_flags & BDB_system_dirty)
		//if (!bdb->bdb_parent && bdb != bcb_btree)
			insertDirtyPage (bdb);


	updateWriteDirection(tdbb, bdb);
}

/**************************************
 *
 *	l a t c h _ b d b
 *
 **************************************
 *
 * Functional description
 *
 * input
 *	type:		LATCH_none, LATCH_exclusive, LATCH_io, LATCH_shared, or LATCH_mark.
 *	bdb:		object to acquire latch on.
 *	page:		page of bdb, for verification.
 *	latch_wait:	1 => Wait as long as necessary to get the latch.
 *				This can cause deadlocks of course.
 *			0 => If the latch can't be acquired immediately,
 *				give up and return 1.
 *	      		<negative number> => Latch timeout interval in seconds.
 *
 * return
 *	0:	latch successfully acquired.
 *	1:	latch not acquired due to a (permitted) timeout.
 *	-1:	latch not acquired, bdb doesn't corresponds to page.
 *
 **************************************/

int PageCache::latchBdb(thread_db* tdbb, LatchType type, Bdb* bdb, SLONG page)
{
	//bdb->incrementUseCount();

	switch (type) 
		{
		case LATCH_shared:
			break;
			
		case LATCH_exclusive:
			bdb->bdb_exclusive = tdbb;
			break;
			
		case LATCH_io:
			bdb->bdb_io = tdbb;
			break;
			
		case LATCH_mark:
			bdb->bdb_io = tdbb;
			
		case LATCH_none:
			//bdb->decrementUseCount();
			break;
		}

	return 0;
}

/**************************************
 *
 *	w r i t e a b l e
 *
 **************************************
 *
 * Functional description
 *	See if a buffer is writeable.  A buffer is writeable if
 *	neither it nor any of it's higher precedence cousins are
 *	marked for write.
 *
 **************************************/

bool PageCache::writeable(Bdb* bdblock)
{
	if (bdblock->bdb_flags & BDB_marked)
		return false;

	/* If there are buffers that must be written first, check them, too. */

	for (const que* queue = bdblock->bdb_higher.que_forward;
		 queue != &bdblock->bdb_higher; queue = queue->que_forward)
		{
		const pre* precedence = BLOCK(queue, PRE, pre_higher);
		
		if (!(precedence->pre_flags & PRE_cleared) && !writeable(precedence->pre_hi))
			return false;
		}

	return true;
}


/**************************************
 *
 *	b l o c k i n g _ a s t _ b d b
 *
 **************************************
 *
 * Functional description
 *	Blocking AST for buffer control blocks.  This is called at
 *	AST (signal) level to indicate that a lock is blocking another
 *	process.  If the BDB is in use, set a flag indicating that the
 *	lock is blocking and return.  When the bdb is released at normal
 *	level the lock will be down graded.  If the BDB is not in use,
 *	write the page if dirty then downgrade lock.  Things would be
 *	much hairier if UNIX supported asynchronous IO, but it doesn't.
 *	WHEW!
 *
 **************************************/

int PageCache::bdbBlockingAst(void* ast_argument)
{
	Bdb *bdb = (Bdb*) ast_argument;
	
	return bdb->bdb_dbb->pageCache->blockingAst (bdb);
}

int PageCache::blockingAst(Bdb* bdb)
{
	ISC_STATUS_ARRAY ast_status;
	ThreadData threadData (ast_status, database);
	ISC_ast_enter();

	/* Do some fancy footwork to make sure that pages are
	   not removed from the btc tree at AST level.  Then
	   restore the flag to whatever it was before. */

	bool keep_pages = bcb_flags & BCB_keep_pages;
	bcb_flags |= BCB_keep_pages;
	ast_status[1] = 0;

	downGrade(threadData, bdb);

	if (!keep_pages)
		bcb_flags &= ~BCB_keep_pages;

	if (ast_status[1])
		gds__log_status(database->dbb_file->fil_string, ast_status);

	ISC_ast_exit();

    return 0;
}

/**************************************
 *
 *	d o w n _ g r a d e
 *
 **************************************
 *
 * Functional description
 *	A lock on a page is blocking another process.  If possible, downgrade
 *	the lock on the buffer.  This may be called from either AST or
 *	regular level.  Return TRUE if the down grade was successful.  If the
 *	down grade was deferred for any reason, return FALSE.
 *
 **************************************/

void PageCache::downGrade(thread_db* tdbb, Bdb* bdb)
{
	QUE que;
	int in_use, invalid;

	//printf ("downGrade bdb page %d\n", bdb->bdb_page);
	bdb->bdb_ast_flags |= BDB_blocking;
	LCK lock = bdb->bdb_lock;

	if (database->dbb_flags & DBB_bugcheck) 
		{
		LCK_release(bdb->bdb_lock);
		bdb->bdb_ast_flags &= ~BDB_blocking;
		
		// Release backup pages lock as buffer is no longer dirty
		
		if (bdb->bdb_flags & BDB_dirty) 
			{
			//bdb->bdb_flags &= ~BDB_dirty;
			bdb->clearFlags(BDB_dirty);
			setWriteDirection(tdbb, bdb, BDB_write_undefined);
			}
			
		return;
		}

	/* If the BDB is in use and, being written or already
	   downgraded to read, mark it as blocking and exit. */

	if (bdb->bdb_use_count || bdb->isLocked())
		return;

	bdb->incrementUseCount();
	bdb->bdb_io = tdbb;

	/* If the page isn't dirty, the lock can be quietly downgraded. */

	if (!(bdb->bdb_flags & BDB_dirty)) 
		{
		bdb->bdb_ast_flags &= ~BDB_blocking;
#ifdef VMS
		if (lock->lck_logical == LCK_write)
			LCK_convert(tdbb, lock, LCK_read, LCK_WAIT);
		else
			LCK_release(bdb->bdb_lock);
#else
		int level = LCK_downgrade(tdbb, lock);
#endif

		bdb->decrementUseCount();
			
		return;
		}

	in_use = invalid = FALSE;

	if (bdb->bdb_flags & BDB_not_valid)
		invalid = TRUE;

	/* If there are higher precedence guys, see if they can be written. */

	for (que = bdb->bdb_higher.que_forward; que != &bdb->bdb_higher; que = que->que_forward)
		{
		PRE precedence = BLOCK(que, PRE, pre_higher);
		
		if (precedence->pre_flags & PRE_cleared)
			continue;
			
		if (invalid) 
			{
			precedence->pre_flags |= PRE_cleared;
			continue;
			}
			
		Bdb	*blocking_bdb = precedence->pre_hi;
		
		if (blocking_bdb->bdb_flags & BDB_dirty) 
			{
			downGrade(tdbb, blocking_bdb);
			
			if (blocking_bdb->bdb_flags & BDB_dirty)
				in_use = TRUE;
				
			if (blocking_bdb->bdb_flags & BDB_not_valid) 
				{
				invalid = TRUE;
				in_use = FALSE;
				que = bdb->bdb_higher.que_forward;
				}
			}
		}

	/* If any higher precedence buffer can't be written, mark this buffer
	   as blocking and exit. */

	if (in_use) 
		{
		bdb->decrementUseCount();
		return; 
		}

	/* Everything is clear to write this buffer.  Do so and reduce the lock */

	if (invalid || !writePage(tdbb, bdb, false, true))
		{
		//bdb->bdb_flags |= BDB_not_valid;
		bdb->setFlags(BDB_not_valid);
		// Release backup pages lock
		//bdb->bdb_flags &= ~BDB_dirty;
		bdb->clearFlags(BDB_dirty);
		setWriteDirection(tdbb, bdb, BDB_write_undefined);
		bdb->bdb_ast_flags &= ~BDB_blocking;
		TRA_invalidate(database, bdb->bdb_transactions);
		bdb->bdb_transactions = 0;
		LCK_release(bdb->bdb_lock);
		}
	else 
		{
		bdb->bdb_ast_flags &= ~BDB_blocking;
#ifdef VMS
		LCK_convert(tdbb, lock, LCK_read, LCK_WAIT);
#else
		LCK_downgrade(tdbb, lock);
#endif
		}

	/* Clear precedence relationships to lower precedence buffers.  Since it
	   isn't safe to tweak the que pointers from AST level, just mark the
	   precedence links as cleared.  Somebody else will clean up the precedence
	   blocks. */

	for (que = bdb->bdb_lower.que_forward; que != &bdb->bdb_lower; que = que->que_forward)
		{
		PRE precedence = BLOCK(que, PRE, pre_lower);
		Bdb	*blocking_bdb = precedence->pre_low;
		
		if (bdb->bdb_flags & BDB_not_valid)
			//blocking_bdb->bdb_flags |= BDB_not_valid;
			bdb->setFlags(BDB_not_valid);
			
		precedence->pre_flags |= PRE_cleared;
		
		if (blocking_bdb->bdb_flags & BDB_not_valid ||
			blocking_bdb->bdb_ast_flags & BDB_blocking)
			downGrade(tdbb, blocking_bdb);
		}

	//bdb->bdb_flags &= ~BDB_not_valid;
	bdb->clearFlags(BDB_not_valid);
	bdb->decrementUseCount();

	return;
}

/**************************************
 *
 *	C C H _ e x c l u s i v e
 *
 **************************************
 *
 * Functional description
 *	Get exclusive access to a database.  If we get it, return TRUE.
 *	If the wait flag is FALSE, and we can't get it, give up and
 *	return FALSE. There are two levels of database exclusivity: LCK_PW
 *	guarantees there are  no normal users in the database while LCK_EX
 *	additionally guarantes background database processes like the
 *	shared cache manager have detached.
 *
 **************************************/

bool PageCache::getExclusive(thread_db* tdbb, int level, int wait_flag)
{
#ifdef SUPERSERVER
	if (!getExclusiveAttachment(tdbb, level, wait_flag))
		return FALSE;
#endif

	LCK lock = database->dbb_lock;
	
	if (!lock)
		return FALSE;

	database->dbb_flags |= DBB_exclusive;

	switch (level) 
		{
		case LCK_PW:
			if ((lock->lck_physical >= LCK_PW) || LCK_convert(tdbb, lock, LCK_PW, wait_flag))
				return TRUE;
			break;

		case LCK_EX:
			if (lock->lck_physical == LCK_EX || LCK_convert(tdbb, lock, LCK_EX, wait_flag))
				return TRUE;
			break;

		default:
			break;
		}

	/* If we are supposed to wait (presumably patiently),
	  but can't get the lock, generate an error */

	if (wait_flag == LCK_WAIT)
		ERR_post(isc_deadlock, 0);

	database->dbb_flags &= ~DBB_exclusive;

	return FALSE;
}

/**************************************
 *
 *	C C H _ e x c l u s i v e _ a t t a c h m e n t
 *
 **************************************
 *
 * Functional description
 *	Get exclusive access to a database. If we get it, return TRUE.
 *	If the wait flag is FALSE, and we can't get it, give up and
 *	return FALSE.
 *
 **************************************/

bool PageCache::getExclusiveAttachment(thread_db* tdbb, int level, int wait_flag)
{
#define CCH_EXCLUSIVE_RETRY_INTERVAL	1	/* retry interval in seconds */

	Attachment* attachment = tdbb->tdbb_attachment;
	Database *dbb = attachment->att_database;
	Sync sync(&dbb->syncAttachments, "PageCache::getExclusiveAttachment");
	sync.lock(Exclusive);
	
	if (attachment->att_flags & ATT_exclusive)
		return TRUE;

	attachment->att_flags |= (level == LCK_none) ? ATT_attach_pending : ATT_exclusive_pending;

	const SLONG timeout = (SLONG) (wait_flag < 0) ? -wait_flag :
			((wait_flag == LCK_WAIT) ? 1L << 30 : CCH_EXCLUSIVE_RETRY_INTERVAL);

	/* If requesting exclusive database access, then re-position attachment as the
	   youngest so that pending attachments may pass. */

	if (level != LCK_none) 
		{
		for (Attachment** ptr = &database->dbb_attachments; *ptr; ptr = &(*ptr)->att_next) 
			if (*ptr == attachment) 
				{
				*ptr = attachment->att_next;
				break;
				}

		attachment->att_next = database->dbb_attachments;
		database->dbb_attachments = attachment;
		}

	for (SLONG remaining = timeout; remaining > 0; remaining -= CCH_EXCLUSIVE_RETRY_INTERVAL)
		{
		if (tdbb->tdbb_attachment->att_flags & ATT_shutdown)
			break;

		bool found = false;

		for (attachment = tdbb->tdbb_attachment->att_next; attachment; attachment = attachment->att_next)
			{
			if (attachment->att_flags & ATT_shutdown)
				continue;

			if (level == LCK_none) 
				{	/* Wait for other attachments requesting exclusive access */
				if (attachment->att_flags & 
					(ATT_exclusive | ATT_exclusive_pending))
					{
					found = true;
					break;
					}
				}
			else 				/* Requesting exclusive database access */
				{
				found = true;

				if (attachment->att_flags & ATT_exclusive_pending) 
					{
					tdbb->tdbb_attachment->att_flags &= ~ATT_exclusive_pending;

					if (wait_flag == LCK_WAIT)
						ERR_post(isc_deadlock, 0);
					else
						return FALSE;
					}
					
				break;
				}
			}

		if (!found) 
			{
			tdbb->tdbb_attachment->att_flags &=
				~(ATT_exclusive_pending | ATT_attach_pending);

			if (level != LCK_none)
				tdbb->tdbb_attachment->att_flags |= ATT_exclusive;

			return TRUE;
			}

		/* Our thread needs to sleep for CCH_EXCLUSIVE_RETRY_INTERVAL seconds. */

		if (remaining > CCH_EXCLUSIVE_RETRY_INTERVAL)
			{
			sync.unlock();
			THREAD_SLEEP(CCH_EXCLUSIVE_RETRY_INTERVAL * 1000);
			sync.lock(Exclusive);
			}

#ifdef CANCEL_OPERATION
		if (tdbb->tdbb_attachment->att_flags & ATT_cancel_raise) 
			if (JRD_reschedule(tdbb, 0, false))
				{
				tdbb->tdbb_attachment->att_flags &=
					~(ATT_exclusive_pending | ATT_attach_pending);
				ERR_punt();
				}
#endif
		}

	tdbb->tdbb_attachment->att_flags &= ~(ATT_exclusive_pending | ATT_attach_pending);
		
	return FALSE;
}

/**************************************
 *
 *	C C H _ f e t c h
 *
 **************************************
 *
 * Functional description
 *	Fetch a specific page.  If it's already in cache,
 *	so much the better.
 *
 * input
 *	latch_wait:	1 => Wait as long as necessary to get the latch.
 *				This can cause deadlocks of course.
 *			0 => If the latch can't be acquired immediately,
 *				give up and return 0.
 *	      		<negative number> => Latch timeout interval in seconds.
 *
 * return
 *	PAG if successful.
 *	NULL pointer if timeout occurred (only possible if latch_wait <> 1).
 *
 **************************************/

pag* PageCache::fetch(thread_db* tdbb, win* window, int lock_type, int page_type, int checksum, int latch_wait, int read_shadow)
{
	LockState lockState = fetchLock (tdbb, window, lock_type, LCK_WAIT, page_type);
	Bdb *bdb = window->win_bdb;
	LockType lockType = (lock_type >= LCK_write) ? Exclusive : Shared;
	
	switch (lockState)
		{
		case lsLocked:
			fetchPage(tdbb, window, checksum, read_shadow);
			if (lockType != Exclusive)
				bdb->downGrade (lockType);
			break;
		
		case lsLockTimeout:
		//case lsLatchTimeout:
			return NULL;
		}

	/* If a page was read or prefetched on behalf of a large scan
	   then load the window scan count into the buffer descriptor.
	   This buffer scan count is decremented by releasing a buffer
	   with CCH_RELEASE_TAIL.

	   Otherwise zero the buffer scan count to prevent the buffer
	   from being queued to the LRU tail. */

	if (window->win_flags & WIN_large_scan) 
		{
		if (lockState == lsLocked || bdb->bdb_flags & BDB_prefetch || bdb->bdb_scan_count < 0)
			bdb->bdb_scan_count = window->win_scans;
		}
	else if (window->win_flags & WIN_garbage_collector) 
		{
		if (lockState == lsLocked)
			bdb->bdb_scan_count = -1;
		if (bdb->bdb_flags & BDB_garbage_collect)
			window->win_flags |= WIN_garbage_collect;
		}
	else if (window->win_flags & WIN_secondary) 
		{
		if (lockState == lsLocked)
			bdb->bdb_scan_count = -1;
		}
	else 
		{
		bdb->bdb_scan_count = 0;
		if (bdb->bdb_flags & BDB_garbage_collect) 
			//bdb->bdb_flags &= ~BDB_garbage_collect;
			bdb->clearFlags(BDB_garbage_collect);
		}

	/* Validate the fetched page matches the expected type */

	if (bdb->bdb_buffer->pag_type != (SCHAR) page_type && page_type != pag_undefined)
		pageValidationError(tdbb, window, page_type);

	return window->win_buffer;
}

/**************************************
 *
 *	C C H _ f e t c h _ l o c k
 *
 **************************************
 *
 * Functional description
 *	Fetch a lock for a specific page.
 *	Return TRUE if the page needs to be
 *	read and FALSE if not. If a timeout
 *	was passed (wait < 0) and the lock
 *	could not be granted return wait.
 *
 * input
 *
 *	wait: LCK_WAIT = TRUE = 1	=> Wait as long a necessary to get the lock.
 *	      LCK_NO_WAIT = FALSE = 0	=> If the lock can't be acquired immediately,
 *						give up and return -1.
 *	      <negative number>		=> Lock timeout interval in seconds.
 *
 *	latch_wait:	1 => Wait as long as necessary to get the latch.
 *				This can cause deadlocks of course.
 *			0 => If the latch can't be acquired immediately,
 *				give up and return -2.
 *	      		<negative number> => Latch timeout interval in seconds.
 *
 * return
 *	0:	fetch & lock were successful, page doesn't need to be read.
 *	1:	fetch & lock were successful, page needs to be read from disk.
 *	-1:	lock timed out, fetch failed.
 *	-2:	latch timed out, fetch failed, lock not attempted.
 *
 **************************************/

LockState PageCache::fetchLock(thread_db* tdbb, win* window, int lock_type, int wait, int page_type)
{
	/* if there has been a shadow added recently, go out and
	   find it before we grant any more write locks */

	if (getShadows)
		SDW_get_shadows(tdbb);

	/* Look for the page in the cache. */

	Bdb *bdb = getBuffer(tdbb, window->win_page, lock_type);
					  
	if (lock_type >= LCK_write)
		//bdb->bdb_flags |= BDB_writer;
		bdb->setFlags(BDB_writer);

	/* the expanded index buffer is only good when the page is
	   fetched for read; if it is ever fetched for write, it must
	   be discarded */

	/***
	if (bdb->bdb_expanded_buffer && (lock_type > LCK_read)) 
		{
		delete bdb->bdb_expanded_buffer;
		bdb->bdb_expanded_buffer = NULL;
		}
	***/
	
	window->win_bdb = bdb;
	window->win_buffer = bdb->bdb_buffer;
	window->win_expanded_buffer = bdb->bdb_expanded_buffer;

	/* lock_buffer returns 0 or 1 or -1. */
	
	return lockBuffer(tdbb, bdb, wait, page_type);
}

/**************************************
 *
 *	C C H _ f e t c h _ p a g e
 *
 **************************************
 *
 * Functional description
 *	Fetch a specific page.  If it's already in cache,
 *	so much the better.  When "compute_checksum" is 1, compute
 * 	the checksum of the page.  When it is 2, compute
 *	the checksum only when the page type is nonzero.
 *
 **************************************/

void PageCache::fetchPage(thread_db* tdbb, win* window, int compute_checksum, bool read_shadow)
{
	SSHORT retryCount;
	Bdb	*bdb = window->win_bdb;
	ISC_STATUS *status = NULL;
	bdb->bdb_incarnation = ++database->dbb_page_incarnation;

	AST_CHECK;
	++database->dbb_reads;
	PAG page = bdb->bdb_buffer;
	FIL file = database->dbb_file;
	retryCount = 0;

	/* We will read a page, and if there is an I/O error we will try to
	   use the shadow file, and try reading again, for a maximum of
	   3 tries, before it gives up.

	   The read_shadow flag is set to false only in the call to
	   FETCH_NO_SHADOW, which is only called from validate
	   code.

	   read_shadow = FALSE -> IF an I/O error occurs give up (exit
	   the loop, clean up, and return). So the caller,
	   validate in most cases, can know about it and attempt
	   to remedy the situation.

	   read_shadow = TRUE -> IF an I/O error occurs attempt
	   to rollover to the shadow file.  If the I/O error is
	   persistant (more than 3 times) error out of the routine by
	   calling CCH_unwind, and eventually punting out. */
   
	BackupManager *backupManager = database->backup_manager;
	
	if (!backupManager->lock_state(tdbb, false)) 
		{
		LCK_release(bdb->bdb_lock);
		unwind(tdbb, TRUE);
		}
		
	int bak_state = backupManager->get_state();
	ULONG diff_page = 0;
	
	if (bak_state == nbak_state_stalled || bak_state == nbak_state_merge) 
		{
		if (!backupManager->lock_alloc(tdbb, false)) 
			{
			LCK_release(bdb->bdb_lock);
			backupManager->unlock_state(tdbb);
			unwind(tdbb, TRUE);
			}
			
		diff_page = backupManager->get_page_index(bdb->bdb_page);
		backupManager->unlock_alloc(tdbb);
		NBAK_TRACE(("Reading page %d, state=%d, diff page=%d", bdb->bdb_page, bak_state, diff_page));
		}

	if (bak_state == nbak_state_normal || 
		 (bak_state == nbak_state_stalled && !diff_page) ||
		 // In merge mode, if we are reading past beyond old end of file and page is in .delta file
		 // then we maintain actual page in difference file. Always read it from there.
		 (bak_state == nbak_state_merge && 
			(!diff_page || (bdb->bdb_page < database->backup_manager->get_backup_pages())))) 
		{
		NBAK_TRACE(("Reading page %d, state=%d, diff page=%d from DISK",
					bdb->bdb_page, bak_state, diff_page));

		while (!PIO_read(file, bdb, page, status)) 
			{
			if (!read_shadow) 
				break;
			if (!rolloverToShadow(tdbb, file, false)) 
				{
				LCK_release(bdb->bdb_lock);
				backupManager->unlock_state(tdbb);
				unwind(tdbb, TRUE);
				}
				
			if (file != database->dbb_file)
				file = database->dbb_file;
			else if (retryCount++ == 3) 
				{
				//ib_fprintf(ib_stderr, "IO error loop Unwind to avoid a hang\n");
				LCK_release(bdb->bdb_lock);
				backupManager->unlock_state(tdbb);
				unwind(tdbb, TRUE);
				}
			}
		}

	backupManager->unlock_state(tdbb);
	//bdb->bdb_flags &= ~(BDB_not_valid | BDB_read_pending);
	bdb->clearFlags(BDB_not_valid | BDB_read_pending);
	window->win_buffer = bdb->bdb_buffer;
}

/**************************************
 *
 *	p a g e _ v a l i d a t i o n _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	We've detected a validation error on fetch.  Generally
 *	we've detected that the type of page fetched didn't match the
 *	type of page we were expecting.  Report an error and
 *	get out.
 *	This function will only be called rarely, as a page validation
 *	error is an indication of on-disk database corruption.
 *
 **************************************/

void PageCache::pageValidationError(thread_db* tdbb, win* window, int type)
{
	Bdb *bdb = window->win_bdb;
	PAG page = bdb->bdb_buffer;
	unwind(tdbb, false);

	ERR_post (isc_db_corrupt,
				isc_arg_string, "",
				isc_arg_gds, isc_page_type_err,
				isc_arg_gds, isc_badpagtyp,
				isc_arg_number, (SLONG) bdb->bdb_page,
				isc_arg_number, (SLONG) type,
				isc_arg_number, (SLONG) page->pag_type, 0);
					   
}

/**************************************
 *
 *	C C H _ h a n d o f f
 *
 **************************************
 *
 * Functional description
 *	Follow a pointer handing off the lock.  Fetch the new page
 *	before retiring the old page lock.
 *
 * input
 *	latch_wait:	1 => Wait as long as necessary to get the latch.
 *				This can cause deadlocks of course.
 *			0 => If the latch can't be acquired immediately,
 *				give up and return 0.
 *	      		<negative number> => Latch timeout interval in seconds.
 *
 * return
 *	PAG if successful.
 *	0 if a latch timeout occurred (only possible if latch_wait <> 1).
 *		The latch on the fetched page is downgraded to shared.
 *		The fetched page is unmarked.
 *
 **************************************/

pag* PageCache::handoff(thread_db* tdbb, win* window, SLONG page, int lock, int page_type, int latch_wait, int release_tail)
{
	Bdb *bdb = window->win_bdb;

	/* The update, if there was one, of the input page is complete.
	   The cache buffer can be 'unmarked'.  It is important to
	   unmark before CCH_unwind is (might be) called. */

	//unmark(tdbb, window);
	if (bdb->exclusive && bdb->writers == 1 && (bdb->bdb_flags & BDB_marked))
		bdb->clearFlags(BDB_marked);

	/* If the 'from-page' and 'to-page' of the handoff are the
	   same and the latch requested is shared then downgrade it. */

	if (window->win_page == page && lock == LCK_read) 
		{
		//release(tdbb, window->win_bdb, FALSE, TRUE, FALSE);
		if (window->win_bdb->exclusive)
			window->win_bdb->downGrade(Shared);
			
		return window->win_buffer;
		}

	WIN temp = *window;
	window->win_page = page;
	//validate();
	LockState must_read = fetchLock(tdbb, window, lock, LCK_WAIT, page_type);

	/* Latch or lock timeout, return failure. */

	//if (must_read == lsLatchTimeout || must_read == lsLockTimeout) 
	if (must_read == lsLockTimeout) 
		{
		*window = temp;
		release (tdbb, window);
		return NULL;
		}

	release(tdbb, &temp, release_tail != 0);

	if (must_read != lsLockedHavePage)
		fetchPage (tdbb, window, TRUE, TRUE);

	bdb = window->win_bdb;

	/* If a page was read or prefetched on behalf of a large scan
	   then load the window scan count into the buffer descriptor.
	   This buffer scan count is decremented by releasing a buffer
	   with CCH_RELEASE_TAIL.

       Otherwise zero the buffer scan count to prevent the buffer
       from being queued to the LRU tail. */

	if (window->win_flags & WIN_large_scan) 
		{
		if (must_read == lsLocked || bdb->bdb_flags & BDB_prefetch || bdb->bdb_scan_count < 0)
			bdb->bdb_scan_count = window->win_scans;
		}
	else if (window->win_flags & WIN_garbage_collector) 
		{
		if (must_read == lsLocked)
			bdb->bdb_scan_count = -1;
		if (bdb->bdb_flags & BDB_garbage_collect)
			window->win_flags |= WIN_garbage_collect;
		}
	else if (window->win_flags & WIN_secondary) 
		{
		if (must_read == lsLocked)
			bdb->bdb_scan_count = -1;
		}
	else 
		{
		bdb->bdb_scan_count = 0;
		if (bdb->bdb_flags & BDB_garbage_collect) 
			//bdb->bdb_flags &= ~BDB_garbage_collect;
			bdb->clearFlags(BDB_garbage_collect);
		}

	/* Validate the fetched page matches the expected type */

	if (bdb->bdb_buffer->pag_type != (SCHAR) page_type && page_type != pag_undefined)
		pageValidationError(tdbb, window, page_type);

	return window->win_buffer;
}

/**************************************
 *
 *	u n m a r k
 *
 **************************************
 *
 * Functional description
 *	Unmark a bdb.  Called when the update of a page is
 *	complete and delaying the 'unmarking' could cause
 *	problems.
 *
 **************************************/

void PageCache::unmark(thread_db* tdbb, win* window)
{
	Bdb *bdb = window->win_bdb;

	if (bdb->bdb_use_count == 1) 
		{
		//bool marked = (bdb->bdb_flags & BDB_marked) ? TRUE : FALSE;
		//bdb->bdb_flags &= ~BDB_marked;
		int oldFlags = bdb->clearFlags(BDB_marked);
		
		/*** ???
		if (marked)
			release(tdbb, bdb, FALSE, FALSE, TRUE);
		***/
		}
}

/**************************************
 *
 *	C C H _ r e l e a s e
 *
 **************************************
 *
 * Functional description
 *	Release a window. If the release_tail
 *	flag is TRUE then make the buffer
 *	least-recently-used.
 *
 **************************************/

void PageCache::release(thread_db* tdbb, win* window, bool release_tail)
{
	Bdb *bdb = window->win_bdb;

	/***
	Sync bdbSync (&bdb->syncBdb, "PageCache::getBuffer");
	if (!bdb->exclusive)
		bdbSync.lock(Exclusive);
	***/
		
	ISC_STATUS statusVector [20];

	/* if an expanded buffer has been created, retain it
	   for possible future use */

	bdb->bdb_expanded_buffer = window->win_expanded_buffer;
	window->win_expanded_buffer = NULL;

	/* A large sequential scan has requested that the garbage
	   collector garbage collect. Mark the buffer so that the
	   page isn't released to the LRU tail before the garbage
	   collector can process the page. */

	if (window->win_flags & WIN_large_scan && window->win_flags & WIN_garbage_collect)
		{
		//bdb->bdb_flags |= BDB_garbage_collect;
		bdb->setFlags(BDB_garbage_collect);
		window->win_flags &= ~WIN_garbage_collect;
		}

	//  void PageCache::release(thread_db* tdbb, Bdb* bdb, bool repost, bool downgrade_latch, bool rel_mark_latch)

	//if (bdb->bdb_use_count == 1)
	if (bdb->exclusive && bdb->writers == 1)
		{
		//bool marked = bdb->bdb_flags & BDB_marked;
		//bdb->bdb_flags &= ~(BDB_writer | BDB_marked | BDB_faked);
		int oldFlags = bdb->clearFlags(BDB_writer | BDB_marked | BDB_faked);
		/***			
		if (marked)
			release(tdbb, bdb, FALSE, FALSE, TRUE);
		***/
		
		if (oldFlags & BDB_must_write)
			{
			/* Downgrade exclusive latch to shared to allow concurrent share access
			   to page during I/O. */

			bdb->downGrade (Shared);
			
			if (!writeBuffer (tdbb, bdb, bdb->bdb_page, false, statusVector, true))
				{
				insertDirtyPage(bdb);	/* Don't lose track of must_write */
				unwind(tdbb, TRUE);
				}
			}
			
		if (bdb->bdb_flags & BDB_no_blocking_ast)
			{
			if (bdb->bdb_flags & (BDB_db_dirty | BDB_dirty) &&
				 !writeBuffer(tdbb, bdb, bdb->bdb_page, false, statusVector, true))
				{
				/* Reassert blocking AST after write failure with dummy lock convert
					to same level. This will re-enable blocking AST notification. */

				LCK_convert_opt(tdbb, bdb->bdb_lock, bdb->bdb_lock->lck_logical);
				unwind(tdbb, TRUE);
				}

			LCK_release(bdb->bdb_lock);
			bdb->clearFlags(BDB_no_blocking_ast);
			bdb->bdb_ast_flags &= ~BDB_blocking;
			}

		/* Make buffer the least-recently-used by queueing it
		   to the LRU tail. */

		if (release_tail)
			{
			if ((window->win_flags & WIN_large_scan &&
				 bdb->bdb_scan_count > 0 &&
				 !(--bdb->bdb_scan_count) &&
				 !(bdb->bdb_flags & BDB_garbage_collect)) ||
				(window->win_flags & WIN_garbage_collector &&
				 bdb->bdb_flags & BDB_garbage_collect &&
				 !(bdb->bdb_scan_count)))
				{
				if (window->win_flags & WIN_garbage_collector)
					//bdb->bdb_flags &= ~BDB_garbage_collect;
					bdb->clearFlags(BDB_garbage_collect);
					
				recentlyUsed(bdb);
				bdb->bdb_sequence = 0;
#ifdef CACHE_WRITER
				if (bdb->bdb_flags & (BDB_dirty | BDB_db_dirty))
					{
					bcb_flags |= BCB_free_pending;

					if (bcb_flags & BCB_cache_writer && !(bcb_flags & BCB_writer_active))
						ISC_event_post(database->dbb_writer_event);
					}
#endif
				}
			}
		}

	/***		
	release(tdbb, bdb, FALSE, FALSE, FALSE);
	int use_count = bdb->bdb_use_count;

	if (use_count < 0)
		BUGCHECK(209);			// msg 209 attempt to release page not acquired
	***/

	bdb->release(tdbb);
	
	if (bdb->bdb_use_count == 0 && (bdb->bdb_ast_flags & BDB_blocking))
		LCK_re_post(bdb->bdb_lock);
	
	window->win_bdb = NULL;
}

/**************************************
 *
 *	C C H _ p r e c e d e n c e
 *
 **************************************
 *
 * Functional description
 *	Given a window accessed for write and a page number,
 *	establish a precedence relationship such that the
 *	specified page will always be written before the page
 *	associated with the window.
 *
 *	If the "page number" is negative, it is really a transaction
 *	id.  In this case, the precedence relationship is to the
 *	database header page from which the transaction id was
 *	obtained.  If the header page has been written since the
 *	transaction id was assigned, no precedence relationship
 *	is required.
 *
 **************************************/

void PageCache::declarePrecedence(thread_db* tdbb, win* window, SLONG page)
{
	/* If the page is zero, the caller isn't really serious */

	if (page == 0)
		return;

	/* If this is really a transaction id, sort things out */

	if (page < 0)
		{
		if (-page <= database->dbb_last_header_write)
			return;
		page = HEADER_PAGE;
		}

	/* Start by finding the buffer containing the high priority page */

	Sync sync (&syncObject, "PageCache::declarePrecedence");
	sync.lock(Shared);
	QUE	mod_que = hashTable + (page % bcb_count);
	Bdb *high = NULL;
	QUE que;

	for (que = mod_que->que_forward; que != mod_que; que = que->que_forward)
		if ((high = BLOCK(que, Bdb*, bdb_que))->bdb_page == page)
			break;

	if (que == mod_que) 
		return;

	/* Found the higher precedence buffer.  If it's not dirty, don't sweat it.
	   If it's the same page, ditto.  */

	if (!(high->bdb_flags & BDB_dirty) || (high->bdb_page == window->win_page)) 
		return;

	Bdb *low = window->win_bdb;

	if ((low->bdb_flags & BDB_marked) && !(low->bdb_flags & BDB_faked))
		bugcheck(212);	/* msg 212 CCH_precedence: block marked */

	/* If already related, there's nothing more to do. If the precedence
	   search was too complex to complete, just write the high page and
	   forget about about establishing the relationship. */

	sync.unlock();
	Sync syncPrec(&syncPrecedence, "PageCache::declarePrecedence");

	if (QUE_NOT_EMPTY(high->bdb_lower)) 
		{
		syncPrec.lock(Shared);
		int relationship = related(low, high, PRE_SEARCH_LIMIT);
		syncPrec.unlock();
		
		if (relationship == PRE_EXISTS) 
			return;
			
		if (relationship == PRE_UNKNOWN) 
			{
			SLONG high_page = high->bdb_page;
			
			if (!writeBuffer (tdbb, high, high_page, false, tdbb->tdbb_status_vector, true))
				unwind(tdbb, TRUE);
				
			return;
			}
		}

	/* Check to see if we're going to create a cycle or the precedence search
	   was too complex to complete.  If so, force a write of the "after"
	   (currently fetched) page.  Assuming everyone obeys the rules and calls
	   precedence before marking the buffer, everything should be ok */

	if (QUE_NOT_EMPTY(low->bdb_lower)) 
		{
		syncPrec.lock(Shared);
		int relationship = related(high, low, PRE_SEARCH_LIMIT);
		syncPrec.unlock();
		
		if (relationship == PRE_EXISTS || relationship == PRE_UNKNOWN) 
			{
			SLONG low_page = low->bdb_page;
			
			if (!writeBuffer (tdbb, low, low_page, false, tdbb->tdbb_status_vector, true))
				unwind(tdbb, TRUE);
			}
		}

	/* We're going to establish a new precedence relationship.  Get a block,
	   fill in the appropriate fields, and insert it into the various ques */

	syncPrec.lock(Exclusive);
	PRE precedence = new pre;
	precedence->pre_low = low;
	precedence->pre_hi = high;
	precedence->pre_flags = 0;
	QUE_INSERT(low->bdb_higher, precedence->pre_higher);
	QUE_INSERT(high->bdb_lower, precedence->pre_lower);
}

/**************************************
 *
 *	r e l a t e d
 *
 **************************************
 *
 * Functional description
 *	See if there are precedence relationships linking two buffers.
 *	Since precedence graphs can become very complex, limit search for
 *	precedence relationship by visiting a presribed limit of higher
 *	precedence blocks.
 *
 **************************************/

int PageCache::related(Bdb* low, Bdb* high, int limit)
{
	QUE base = &low->bdb_higher;

	for (QUE que = base->que_forward; que != base; que = que->que_forward) 
		{
		if (!--limit)
			return PRE_UNKNOWN;
			
		PRE precedence = BLOCK(que, PRE, pre_higher);
		
		if (!(precedence->pre_flags & PRE_cleared)) 
			{
			if (precedence->pre_hi == high)
				return PRE_EXISTS;
				
			limit = related(precedence->pre_hi, high, limit);
			
			if (limit == PRE_EXISTS || limit == PRE_UNKNOWN)
				return limit;
			}
		}

	return limit;
}

/**************************************
 *
 *	C C H _ p r e f e t c h
 *
 **************************************
 *
 * Functional description
 *	Given a vector of pages, set corresponding bits
 *	in global prefetch bitmap. Initiate an asynchronous
 *	I/O and get the cache reader reading in our behalf
 *	as well.
 *
 **************************************/

void PageCache::prefetch(thread_db* tdbb, SLONG* pages, int count)
{
#ifdef CACHE_READER
	SLONG page, first_page, *end;
	JrdMemoryPool *old_pool;

	SET_TDBB(tdbb);
	dbb = tdbb->tdbb_database;
	bcb = dbb->dbb_bcb;

	if (!count || !(bcb->bcb_flags & BCB_cache_reader))	/* Caller isn't really serious. */
		return;

/* Switch default pool to permanent pool for setting bits in
   prefetch bitmap. */

	old_pool = tdbb->tdbb_default;
	tdbb->tdbb_default = dbb->dbb_bufferpool;

/* The global prefetch bitmap is the key to the I/O coalescense
   mechanism which dovetails all thread prefetch requests to
   minimize sequential I/O requests. It also enables multipage
   I/O by implicitly sorting page vector requests. */

	first_page = 0;
	for (end = pages + count; pages < end;)
		if (page = *pages++) {
			SBM_set(tdbb, &bcb->bcb_prefetch, page);
			if (!first_page)
				first_page = page;
		}

/* Not likely that the caller's page vector was
   empty but check anyway. */

	if (first_page) {
		prf prefetch;

		prefetch_init(&prefetch, tdbb);
		prefetch_prologue(&prefetch, &first_page);
		prefetch_io(&prefetch, tdbb->tdbb_status_vector);
		prefetch_epilogue(&prefetch, tdbb->tdbb_status_vector);
	}

	tdbb->tdbb_default = old_pool;
#endif
}

/**************************************
 *
 *	C C H _ m a r k _ m u s t _ w r i t e
 *
 **************************************
 *
 * Functional description
 *	Mark a window as dirty and must_write.
 *	This will prevent the page from being
 *	inserted in the dirty binary tree when
 *	the intention is to write the page
 *	immediately any way.
 *
 **************************************/

void PageCache::markMustWrite(thread_db* tdbb, win* window)
{
	Bdb *bdb = window->win_bdb;

	if (!(bdb->bdb_flags & BDB_writer))
		BUGCHECK(208);			/* msg 208 page not accessed for write */

	//bdb->bdb_flags |= (BDB_dirty | BDB_must_write);
	bdb->setFlags(BDB_dirty | BDB_must_write);
	mark(tdbb, window, 0);
	updateWriteDirection(tdbb, bdb);
}

/**************************************
 *
 *	C C H _ f l u s h
 *
 **************************************
 *
 * Functional description
 *	Flush all buffers.  If the release flag is set,
 *	release all locks.
 *
 **************************************/

void PageCache::flush(thread_db* tdbb, int flush_flag, int tra_number)
{
	ISC_STATUS *status = tdbb->tdbb_status_vector;

	/* note that some of the code for btc_flush()
	   replicates code in the for loop
	   to minimize call overhead -- changes should
	   be made in both places */

	SLONG transaction_mask = 0;
	bool sys_only = false;
	
	if (flush_flag & (FLUSH_TRAN | FLUSH_SYSTEM)) 
		{
		transaction_mask = (tra_number) ? 1 << (tra_number & (BITS_PER_LONG - 1)) : 0;
		sys_only = !transaction_mask && (flush_flag & FLUSH_SYSTEM);
		}
	else 
		{
		if (flush_flag & (FLUSH_ALL | FLUSH_SWEEP))
			transaction_mask = -1;
		
		/***
		const bool all_flag = (flush_flag & FLUSH_ALL) != 0;
		const bool sweep_flag = (flush_flag & FLUSH_SWEEP) != 0;
		Sync sync (&syncObject, "PageCache::flush");
		sync.lock(Shared);
		
		for (int i = 0; i < bcb_count; i++) 
			{
			Bdb* bdb = bdbs[i]; 
			
			if (bdb->bdb_length)
				continue;
				
			if (!(bdb->bdb_flags & (BDB_dirty | BDB_db_dirty)))
				continue;
			
			if (bdb->bdb_flags & BDB_db_dirty) 
				{
				if (all_flag || (sweep_flag && !bdb->bdb_mark_sequence))	// ???
					{
					bdb->addRef(tdbb, Shared);
					
					if (!(bdb->bdb_flags & BDB_dirty))
						{
						bdb->release(tdbb);
						continue;
						}

					int ret = writeBuffer (tdbb, bdb, bdb->bdb_page, false, status, true);
					bdb->release(tdbb);
					
					if (!ret)
						unwind(tdbb, TRUE);
					}
				}

			}
		***/
		}
		
	flushDirtyPages(tdbb, transaction_mask, sys_only, status);

	// 
	// Check if flush needed
	//
	
	int max_unflushed_writes = tdbb->tdbb_database->maxUnflushedWrites; //Config::getMaxUnflushedWrites();
	time_t max_unflushed_write_time = tdbb->tdbb_database->maxUnflushedWriteTime; //Config::getMaxUnflushedWriteTime();
	bool max_num = (max_unflushed_writes >= 0);
	bool max_time = (max_unflushed_write_time >= 0);

	bool doFlush = false;

	if (!(database->dbb_file->fil_flags & FIL_force_write) && (max_num || max_time))
		{
		const time_t now = time(0);
		Sync sync(&database->syncFlushCount, "PageCache::flush");
		sync.lock(Exclusive);
		
		// If this is the first commit set last_flushed_write to now
		
		if (!database->last_flushed_write)
			database->last_flushed_write = now;

		// test max_num condition and max_time condition
		max_num = max_num && (database->unflushed_writes == max_unflushed_writes);
		max_time = max_time && (now - database->last_flushed_write > max_unflushed_write_time);

		if (max_num || max_time)
			{
			doFlush = true;
			database->unflushed_writes = 0;
			database->last_flushed_write = now;
			}
		else
			database->unflushed_writes++;

		}

	if (doFlush)
		{
		PIO_flush(database->dbb_file);
		
		if (database->dbb_shadow)
			PIO_flush(database->dbb_shadow->sdw_file);
		}

	/* take the opportunity when we know there are no pages
	   in cache to check that the shadow(s) have not been
	   scheduled for shutdown or deletion */

	SDW_check(tdbb);
}

/**************************************
 *
 *	b t c _ f l u s h
 *
 **************************************
 *
 * Functional description
 *	Walk the dirty page binary tree, flushing all buffers
 *	that could have been modified by this transaction.
 *	The pages are flushed in page order to roughly
 *	emulate an elevator-type disk controller. Iteration
 *	is used to minimize call overhead.
 *
 **************************************/

void PageCache::flushDirtyPages(thread_db* tdbb, int transaction_mask, bool sys_only, ISC_STATUS* status)
{
	Sync sync(&syncDirtyBdbs, "PageCache::flushDirtyPages");
	sync.lock(Shared);
	SLONG priorPage = -1;
	int maxSequence = markSequence;
	
	for (bool again = true; again;)
		{
		again = false;
		for (Que *que = bcb_dirty.que_forward; que != &bcb_dirty; que = que->que_forward)
			{
			Bdb *bdb = BLOCK(que, Bdb*, bdb_dirty);
			
			if ((transaction_mask & bdb->bdb_transactions) ||
				 (bdb->bdb_flags & BDB_system_dirty) ||
				 (!transaction_mask && !sys_only) || (!bdb->bdb_transactions))
				{
				if (bdb->bdb_page < priorPage)
					{
					sync.unlock();
					reorderDirty();
					sync.lock(Shared);
					again = true;
					priorPage = -1;
					break;
					}
					
				priorPage = bdb->bdb_page;
				
				// If the page is marked, we've got a potential deadlock.  Stop what we're doing,
				// get buffer (state unknown), release it, and start over
				
				if ((bdb->bdb_flags & BDB_marked) || !(bdb->bdb_flags & BDB_dirty) ||
					 !(bdb->addRefConditional(tdbb, Shared)))
					{
					sync.unlock();
					bdb->addRef(tdbb, Shared);
					bdb->release(tdbb);
					sync.lock(Shared);
					again = true;
					break;
					}
					
				sync.unlock();
				int ret = writeBuffer(tdbb, bdb, bdb->bdb_page, false, status, true);
				bdb->release(tdbb);
				
				if (!ret) 
					unwind(tdbb, TRUE);
					
				sync.lock(Shared);
				again = true;
				break;
				}
			}
		}
	
}

/**************************************
 *
 *	C C H _ r e l e a s e _ e x c l u s i v e
 *
 **************************************
 *
 * Functional description
 *	Release exclusive access to database.
 *
 **************************************/

void PageCache::releaseExclusive(thread_db* tdbb)
{
	database->dbb_flags &= ~DBB_exclusive;
	Attachment *attachment = tdbb->tdbb_attachment;
	
	if (attachment)
		attachment->att_flags &= ~ATT_exclusive;

	if (database->dbb_ast_flags & DBB_blocking)
		LCK_re_post(database->dbb_lock);
}

/**************************************
 *
 *	C C H _ s h u t d o w n _ d a t a b a s e
 *
 **************************************
 *
 * Functional description
 *	Shutdown database physical page locks.
 *
 **************************************/

void PageCache::shutdownDatabase(thread_db* tdbb)
{
	for (int n = 0; n < bcb_count; ++n)
		{
		Bdb *bdb = bdbs[n];
		
		if (bdb->bdb_length)
			continue;
			
		//bdb->bdb_flags &= ~(BDB_dirty | BDB_db_dirty);
		bdb->clearFlags(BDB_dirty | BDB_db_dirty);
		LCK_release(bdb->bdb_lock);
		}
			
#ifndef SUPERSERVER
	PIO_close(database->dbb_file);
	SDW_close(database);
#endif
}

/**************************************
 *
 *	C C H _ c h e c k s u m
 *
 **************************************
 *
 * Functional description
 *	Compute the checksum of a page.
 *
 **************************************/

int PageCache::checksum(Bdb *bdb)
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

	if (database->dbb_ods_version >= ODS_VERSION9)
		return DUMMY_CHECKSUM;
#endif
	pag* page = bdb->bdb_buffer;

	const ULONG* const end = (ULONG *) ((SCHAR *) page + database->dbb_page_size);
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
	}
	while (p < end);

	page->pag_checksum = old_checksum;

	if (checksum)
		return (USHORT) checksum;

/* If the page is all zeros, return an artificial checksum */

	for (p = (ULONG *) page; p < end;) {
		if (*p++)
			return (USHORT) checksum;
	}

/* Page is all zeros -- invent a checksum */

	return 12345;
#endif
}

/**************************************
 *
 *	C C H _ v a l i d a t e
 *
 **************************************
 *
 * Functional description
 *	Give a page a quick once over looking for unhealthyness.
 *
 **************************************/

bool PageCache::validate(win* window)
{
	Bdb *bdb = window->win_bdb;

/* If page is marked for write, checksum is questionable */

	if ((bdb->bdb_flags & (BDB_dirty | BDB_db_dirty)))
		return TRUE;

	PAG page = window->win_buffer;
	USHORT sum = checksum(bdb);

	if (sum == page->pag_checksum)
		return TRUE;

	return FALSE;
}

/**************************************
 *
 *	C C H _ r e c o v e r _ s h a d o w
 *
 **************************************
 *
 * Functional description
 *	Walk through the sparse bit map created during recovery and
 *	write all changed pages to all the shadows.
 *
 **************************************/

void PageCache::recoverShadow(thread_db* tdbb, SparseBitmap* sbm_rec)
{
	SLONG page_no = -1;
	int result;

	ISC_STATUS *status = tdbb->tdbb_status_vector;

	WIN window(-1);
	
	if (!sbm_rec) 
		{
		/* Now that shadows are initialized after WAL, write the header
		   page with the recover bit to shadow. */

		return;
		}

	result = TRUE;

	if (database->dbb_shadow)
		while (SBM_next(sbm_rec, &page_no, RSE_get_forward)) 
			{
			window.win_page = page_no;
			fetch(tdbb, &window, LCK_write, pag_undefined, 1,1,1);
			result = writeAllShadows (tdbb, 0, window.win_bdb, 1, false);
			release(tdbb, &window);
			}

	if (result == FALSE)
		ERR_punt();
	/*
	 * do 2 control points after a recovery to flush all the pages to the
	 * database and shadow. Note that this has to be doen after the shadows
	 * are updated.
	 */

	flush(tdbb, (USHORT) FLUSH_ALL, 0);

	/* release the bit map */

	SBM_release(sbm_rec);
}

/**************************************
 *
 *	C C H _ f i n i
 *
 **************************************
 *
 * Functional description
 *	Shut down buffer operation.
 *
 **************************************/

void PageCache::fini(thread_db* tdbb)
{
	BOOLEAN flush_error;
#ifdef CACHE_WRITER
	QUE que;
	LWT lwt_;
#endif
	flush_error = FALSE;

	/* If we've been initialized, either flush buffers
	   or release locks, depending on where we've been
	   bug-checked; as a defensive programming measure,
	   make sure that the buffers were actually allocated */
	   
	try 
		{
		if (database->dbb_flags & DBB_bugcheck || flush_error)
			for (int n = 0; n < bcb_count; ++n)
				{
				Bdb *bdb = bdbs[n];
				if (bdb->bdb_length)
					continue;
				if (bdb->bdb_expanded_buffer) 
					{
					//delete bdb->bdb_expanded_buffer;
					bdb->bdb_expanded_buffer = NULL;
					}
				LCK_release(bdb->bdb_lock);
				}
		else if (database->dbb_file)
			flush (tdbb, (USHORT) FLUSH_FINI, (SLONG) 0);



#ifdef CACHE_READER

		/* Shutdown the dedicated cache reader for this database. */

		if ((bcb = dbb->dbb_bcb) && (bcb->bcb_flags & BCB_cache_reader)) 
			{
			SLONG count;
			AsyncEvent* event = dbb->dbb_reader_event;

			bcb->bcb_flags &= ~BCB_cache_reader;
			ISC_event_post(event);
			count = ISC_event_clear(event);
			ISC_event_wait(1, &event, &count, 0, NULL, 0);
			/* Now dispose off the cache reader associated semaphore */
			ISC_event_fini(event);
			}
#endif

#ifdef CACHE_WRITER

	/* Shutdown the dedicated cache writer for this database. */

		if ((bcb = dbb->dbb_bcb) && (bcb->bcb_flags & BCB_cache_writer)) 
			{
			SLONG count;
			AsyncEvent* event = dbb->dbb_writer_event_fini;

			/* initialize initialization event */
			ISC_event_init(event, 0, 0);
			count = ISC_event_clear(event);

			bcb->bcb_flags &= ~BCB_cache_writer;
			ISC_event_post(dbb->dbb_writer_event); /* Wake up running thread */
			ISC_event_wait(1, &event, &count, 0, NULL, 0);
			/* Cleanup initialization event */
			ISC_event_fini(event);
			}
#endif

		/* close the database file and all associated shadow files */

		PIO_close(database->dbb_file);
		SDW_close(database);

		for (void *memory; memory = bcb_memory;)
			{
			bcb_memory = *(void**) memory;
			gds__free (memory);
			}
				
#ifdef CACHE_WRITER
			/* Dispose off any associated latching semaphores */
			while (QUE_NOT_EMPTY(bcb->bcb_free_lwt)) 
				{
				que = bcb->bcb_free_lwt.que_forward;
				QUE_DELETE((*que));
				lwt_ = (LWT) BLOCK(que, LWT, lwt_waiters);
				ISC_event_fini(&lwt_->lwt_event);
				}
#endif
		}
	catch (...)
		{
		if (!flush_error) 
			flush_error = TRUE;
		else 
			throw;
		}
}

Bdb* PageCache::findBuffer(SLONG pageNumber)
{
	QUE mod_que = hashTable + (pageNumber % bcb_count);
	QUE que;
	int n;
	
	for (n = 0, que = mod_que->que_forward; que != mod_que; que = que->que_forward, ++n) 
		{
		Bdb *bdb = BLOCK(que, Bdb*, bdb_que);
		if (bdb->bdb_page == pageNumber)
			return bdb;
		if (n > bcb_count)
			bugcheck(-1);
		}
		
	for (que = bcb_pending.que_forward; que != &bcb_pending; que = que->que_forward)
		{
		Bdb *bdb = BLOCK(que, Bdb*, bdb_in_use);
		if (bdb->bdb_pending_page == pageNumber)
			return bdb;
		}
	
	return NULL;
}

void PageCache::validate(void)
{
	validate(&bcb_pending);
	validate(&bcb_in_use);
	validate(&bcb_empty);
	validate(&bcb_free_lwt);
	validate(&bcb_dirty);

	for (int n = 0; n < bcb_count; ++n)
		validate(hashTable + n);
	
	for (Que *que = bcb_dirty.que_forward; que != &bcb_dirty; que = que->que_forward)
		{
		Bdb *bdb = BLOCK(que, Bdb*, bdb_dirty);
		if (!(bdb->bdb_flags & BDB_dirty))
			bugcheck(-1);
		}
	
}

void PageCache::validate(Que* quePtr)
{
	Que *que;
	int n;
	
	for (n = 0, que = quePtr->que_forward; que != quePtr; que = que->que_forward, ++n)
		if (n > bcb_count)
			bugcheck(-1);
	
	for (n = 0, que = quePtr->que_backward; que != quePtr; que = que->que_backward)
		if (n > bcb_count)
			bugcheck(-1);
}

void PageCache::reorderDirty(void)
{
	Sync sync(&syncDirtyBdbs, "PageCache::reorderDirty");
	sync.lock(Exclusive);
	int count = 0;
	Que *que;
	SLONG priorPage = -1;
	bool reorder = false;
	
	for (que = bcb_dirty.que_forward; que != &bcb_dirty; que = que->que_forward)
		{
		++count;
		Bdb *bdb = BLOCK(que, Bdb*, bdb_dirty);
		if (bdb->bdb_page < priorPage)
			reorder = true;
		priorPage = bdb->bdb_page;
		}
	
	if (!reorder)
		return;
	
	if (count == 2)
		{
		Que *que = bcb_dirty.que_forward;
		QUE_DELETE(*que);
		QUE_APPEND(bcb_dirty, *que);
		return;
		}
	
	BdbSort sort(count);
	int n = 0;
	
	for (que = bcb_dirty.que_forward; que != &bcb_dirty; que = que->que_forward)
		{
		Bdb *bdb = BLOCK(que, Bdb*, bdb_dirty);
		sort.records[n++] = bdb;
		}
	
	sort.sort();
	QUE_INIT(bcb_dirty);
	
	for (n = 0; n < sort.size; ++n)
		{
		Bdb *bdb = sort.records[n];
		QUE_APPEND(bcb_dirty, bdb->bdb_dirty);
		}
}

/*
 * Note: PageCache::recentlyUsed is always called with a lock on bdb->syncBdb
 */
 
void PageCache::recentlyUsed(Bdb* bdb)
{
	int oldFlags = bdb->setFlags(BDB_lru_chained);
	
	if (oldFlags & BDB_lru_chained)
		return;
	
	for (volatile Bdb* chain = bcb_lru_chain; chain; chain = chain->bdb_lru_chain)
		if (chain == bdb)
			bugcheck(-1);
			
	for (;;)
		{
		bdb->bdb_lru_chain = (Bdb*) bcb_lru_chain;
		if (COMPARE_EXCHANGE_POINTER(&bcb_lru_chain, bdb->bdb_lru_chain, bdb))
			break;
		}
		
	/***
	Sync sync(&syncLRU, "PageCache::recentlyUsed");
	sync.lock(Exclusive);
	QUE_DELETE (bdb->bdb_in_use);
	QUE_INSERT (bcb_in_use, bdb->bdb_in_use);
	***/
}

void PageCache::requeueRecentlyUsed(void)
{
	volatile Bdb *chain;
	
	// Let's pick up the LRU pending chain, if any
	
	for (;;)
		{
		chain = bcb_lru_chain;
		if (COMPARE_EXCHANGE_POINTER(&bcb_lru_chain, chain, NULL))
			break;
		}
	
	if (!chain)
		return;

	// Next, let's flip the order
	
	Bdb *bdb;
	Bdb *reversed = NULL;
	
	while (bdb = (Bdb*) chain)
		{
		chain = bdb->bdb_lru_chain;
		bdb->bdb_lru_chain = reversed;
		reversed = bdb;
		}
		
	//Sync sync(&syncLRU, "PageCache::requeueRecentlyUsed");
	//sync.lock(Exclusive);
	
	while (bdb = reversed)
		{
		//Sync sync (&bdb->syncBdb, "PageCache::requeueRecentlyUsed");
		//sync.lock(Exclusive);
		reversed = bdb->bdb_lru_chain;
		QUE_DELETE (bdb->bdb_in_use);
		QUE_INSERT (bcb_in_use, bdb->bdb_in_use);
		//bdb->bdb_lru_chained = false;
		bdb->clearFlags(BDB_lru_chained);
		}
}

void PageCache::deletePrecedence(pre* precedence)
{
	QUE_DELETE(precedence->pre_higher);
	QUE_DELETE(precedence->pre_lower);
	delete precedence;
}

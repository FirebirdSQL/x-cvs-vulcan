/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		cch_proto.h
 *	DESCRIPTION:	Prototype header file for cch.cpp
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

#ifndef JRD_CCH_PROTO_H
#define JRD_CCH_PROTO_H

/***
void		CCH_shutdown_database(TDBB tdbb, dbb*);

USHORT		CCH_checksum(db*);
int			CCH_down_grade_dbb(void *ast_argument);
BOOLEAN		CCH_exclusive(TDBB, USHORT, SSHORT);
BOOLEAN		CCH_exclusive_attachment(TDBB, USHORT, SSHORT);
void		CCH_expand(TDBB, ULONG);
pag*		CCH_fake(TDBB, win*, SSHORT);
pag*		CCH_fetch(TDBB, win*, USHORT, SSHORT, SSHORT, SSHORT, BOOLEAN);
SSHORT		CCH_fetch_lock(TDBB, win*, USHORT, SSHORT, SSHORT, SSHORT);
void		CCH_fetch_page(TDBB, win*, SSHORT, BOOLEAN);
void		CCH_fini(TDBB);
void		CCH_flush(TDBB, USHORT, SLONG);
BOOLEAN		CCH_free_page(TDBB);
SLONG		CCH_get_incarnation(win*);
pag*		CCH_handoff(TDBB, win*, SLONG, SSHORT, SSHORT, SSHORT, SSHORT);
void		CCH_init(TDBB, ULONG);
void		CCH_journal_page(TDBB, win*);
void		CCH_journal_record(TDBB, win*, UCHAR*, USHORT, UCHAR*, USHORT);
void		CCH_mark(TDBB, win*, USHORT);
void		CCH_mark_must_write(TDBB, win*);
void		CCH_must_write(TDBB, win*);
lck*		CCH_page_lock(TDBB);
void		CCH_precedence(TDBB, win*, SLONG);
void		CCH_prefetch(tdbb*, SLONG*, SSHORT);
BOOLEAN		CCH_prefetch_pages(TDBB);
void		CCH_recover_shadow(TDBB, sbm*);
void		CCH_release(TDBB, win*, BOOLEAN);
void		CCH_release_and_free(win*);
void		CCH_release_exclusive(TDBB);
void		CCH_release_journal(TDBB, SLONG);
bool		CCH_rollover_to_shadow(TDBB tdbb, dbb*, fil*, const bool);
void		CCH_unwind(TDBB, BOOLEAN);
BOOLEAN		CCH_validate(win*);
void		CCH_flush_database(TDBB tdbb);
bool		CCH_write_all_shadows(TDBB, sdw*, bdb*,
					 ISC_STATUS*, USHORT, const bool);
***/

/* macros for dealing with cache pages */

#define NEW_PAGE_CACHE
#define DEFAULT_FETCH	1,1
#define NO_CHKSUM	0,1

#ifdef NEW_PAGE_CACHE
#define CACHE(tdbb)	tdbb->tdbb_database->pageCache
#define CCH_FETCH(tdbb, window, lock, type)				  CACHE(tdbb)->fetch (tdbb, window, lock, type, DEFAULT_FETCH)
#define CCH_FETCH_NO_SHADOW(tdbb, window, lock, type)	  CACHE(tdbb)->fetch (tdbb, window, lock, type, 1, 0)
#define CCH_FETCH_NO_CHECKSUM(tdbb, window, lock, type)   CACHE(tdbb)->fetch (tdbb, window, lock, type, 0, 1)
#define CCH_FETCH_TIMEOUT(tdbb, window, lock, type, latch_wait)   CACHE(tdbb)->fetch (tdbb, window, lock, type, 0, 1)
#define CCH_FETCH_LOCK(tdbb, window, lock, wait, latch_wait, type) CACHE(tdbb)->fetchLock (tdbb, window, lock, wait, latch_wait, type)
#define CCH_FETCH_PAGE(tdbb, window, checksum, read_shadow)       CACHE(tdbb)->fetchPage (tdbb, window, checksum, read_shadow)
#define CCH_RELEASE(tdbb, window)                         CACHE(tdbb)->release (tdbb, window, FALSE)
#define CCH_RELEASE_TAIL(tdbb, window)                    CACHE(tdbb)->release (tdbb, window, TRUE)
#define CCH_MARK(tdbb, window)                            CACHE(tdbb)->mark (tdbb, window, 0)
#define CCH_MARK_SYSTEM(tdbb, window)                     CACHE(tdbb)->mark (tdbb, window, 1)
#define CCH_HANDOFF(tdbb, window, page, lock, type)       CACHE(tdbb)->handoff (tdbb, window, page, lock, type, 0)
#define CCH_HANDOFF_TIMEOUT(tdbb, window, page, lock, type, latch_wait)   CACHE(tdbb)->handoff (tdbb, window, page, lock, type, 0)
#define CCH_HANDOFF_TAIL(tdbb, window, page, lock, type)  CACHE(tdbb)->handoff (tdbb, window, page, lock, type, 1)
#define CCH_MARK_MUST_WRITE(tdbb, window)                 CACHE(tdbb)->markMustWrite (tdbb, window)
#define CCH_PREFETCH(tdbb, pages, count)				  CACHE(tdbb)->prefetch (tdbb, pages, count)
#define CCH_PRECEDENCE(tdbb, window, page)				  CACHE(tdbb)->declarePrecedence (tdbb, window, page)
#define CCH_FLUSH(tdbb, flush_flag, tra_number)			  CACHE(tdbb)->flush (tdbb, flush_flag, tra_number)
#define CCH_EXPAND(tdbb, number_buffers)				  CACHE(tdbb)->expandBuffers (tdbb, number_buffers)
#define CCH_EXCLUSIVE(tdbb, level, wait_flag)			  CACHE(tdbb)->getExclusive (tdbb, level, wait_flag)
#define CCH_EXCLUSIVE_ATTACHMENT(tdbb, level, wait_flag)  CACHE(tdbb)->getExclusiveAttachment (tdbb, level, wait_flag)
#define CCH_UNWIND(tdbb, punt)							  CACHE(tdbb)->unwind (tdbb, punt)
#define CCH_FAKE(tdbb, window, latch_wait)				  CACHE(tdbb)->fake (tdbb, window)
#define CCH_DOWN_GRADE_DBB								  PageCache::downGradeDbb
#define CCH_RELEASE_EXCLUSIVE(tdbb)						  CACHE(tdbb)->releaseExclusive (tdbb)
#define CCH_JOURNAL_PAGE(tdbb, window)
#define CCH_SHUTDOWN_DATABASE(tdbb,dbb)					  CACHE(tdbb)->shutdownDatabase(tdbb)
#define CCH_CHECKSUM(bdb)								  CACHE(tdbb)->checksum(bdb)
#define CCH_GET_INCARNATION(window)						  (window->win_bdb->bdb_incarnation)

#else

#define CCH_FETCH(tdbb, window, lock, type)		  CCH_fetch (tdbb, window, lock, type, 1, 1, 1)
#define CCH_FETCH_NO_SHADOW(tdbb, window, lock, type)		  CCH_fetch (tdbb, window, lock, type, 1, 1, 0)
#define CCH_FETCH_NO_CHECKSUM(tdbb, window, lock, type)   CCH_fetch (tdbb, window, lock, type, 0, 1, 1)
#define CCH_FETCH_TIMEOUT(tdbb, window, lock, type, latch_wait)   CCH_fetch (tdbb, window, lock, type, 0, latch_wait, 1)
#define CCH_FETCH_LOCK(tdbb, window, lock, wait, latch_wait, type) CCH_fetch_lock (tdbb, window, lock, wait, latch_wait, type)
#define CCH_FETCH_PAGE(tdbb, window, checksum, read_shadow)       CCH_fetch_page (tdbb, window, checksum, read_shadow)
#define CCH_RELEASE(tdbb, window)                         CCH_release (tdbb, window, FALSE)
#define CCH_RELEASE_TAIL(tdbb, window)                    CCH_release (tdbb, window, TRUE)
#define CCH_MARK(tdbb, window)                            CCH_mark (tdbb, window, 0)
#define CCH_MARK_SYSTEM(tdbb, window)                     CCH_mark (tdbb, window, 1)
#define CCH_HANDOFF(tdbb, window, page, lock, type)       CCH_handoff (tdbb, window, page, lock, type, 1, 0)
#define CCH_HANDOFF_TIMEOUT(tdbb, window, page, lock, type, latch_wait)   CCH_handoff (tdbb, window, page, lock, type, latch_wait, 0)
#define CCH_HANDOFF_TAIL(tdbb, window, page, lock, type)  CCH_handoff (tdbb, window, page, lock, type, 1, 1)
#define CCH_MARK_MUST_WRITE(tdbb, window)                 CCH_mark_must_write (tdbb, window)
#define CCH_PREFETCH(tdbb, pages, count)            CCH_prefetch (tdbb, pages, count)
#endif

/* Flush flags */

#ifndef FLUSH_FLAGS
#define FLUSH_FLAGS
#define FLUSH_ALL	1			/* flush all dirty buffers in cache */
#define FLUSH_RLSE	2			/* release page locks after flush */
#define FLUSH_TRAN	4			/* flush transaction dirty buffers from dirty btree */
#define FLUSH_SWEEP	8			/* flush dirty buffers from garbage collection */
#define FLUSH_SYSTEM	16		/* flush system transaction only from dirty btree */
#define FLUSH_FINI	(FLUSH_ALL | FLUSH_RLSE)
#endif

#endif /* JRD_CCH_PROTO_H */

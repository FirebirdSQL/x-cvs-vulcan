/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		req.h
 *	DESCRIPTION:	Request block definitions
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
 *
 * 2002.09.28 Dmitry Yemanov: Reworked internal_info stuff, enhanced
 *                            exception handling in SPs/triggers,
 *                            implemented ROWS_AFFECTED system variable
 */

#ifndef JRD_REQ_H
#define JRD_REQ_H

#include "../jrd/jrd_blks.h"
#include "../include/fb_blk.h"

#include "../jrd/exe.h"

#include <vector>

/* record parameter block */

typedef struct rpb {
	rpb() : rpb_window(-1) {}
	SLONG rpb_number;			/* record number in relation */
	SLONG rpb_transaction;		/* transaction number */
	Relation *rpb_relation;	/* relation of record */
	class rec *rpb_record;		/* final record block */
	class rec *rpb_prior;		/* prior record block if this is a delta record */
	struct srpb *rpb_copy;		/* rpb copy for singleton verification */
	class rec *rpb_undo;		/* our first version of data if this is a second modification */
	USHORT rpb_format_number;	/* format number in relation */

	SLONG rpb_page;				/* page number */
	USHORT rpb_line;			/* line number on page */

	SLONG rpb_f_page;			/* fragment page number */
	USHORT rpb_f_line;			/* fragment line number on page */

	SLONG rpb_b_page;			/* back page */
	USHORT rpb_b_line;			/* back line */

	UCHAR *rpb_address;			/* address of record sans header */
	USHORT rpb_length;			/* length of record */
	USHORT rpb_flags;			/* record ODS flags replica */
	USHORT rpb_stream_flags;	/* stream flags */
	SSHORT rpb_org_scans;		/* relation scan count at stream open */
	struct win rpb_window;
} RPB;

/* Record flags must be an exact replica of ODS record header flags */

#define rpb_deleted	1
#define rpb_chained	2
#define rpb_fragment	4
#define rpb_incomplete	8
#define rpb_blob	16
#define rpb_delta	32			/* prior version is a differences record */
#define rpb_damaged	128			/* record is busted */
#define rpb_gc_active	256		/* garbage collecting dead record version */

/* Stream flags */

#define RPB_s_refetch	0x1		/* re-fetch required due to sort */
#define RPB_s_update	0x2		/* input stream fetched for update */

#define SET_NULL(record,id)	record->rec_data [id >> 3] |=  (1 << (id & 7))
#define CLEAR_NULL(record,id)	record->rec_data [id >> 3] &= ~(1 << (id & 7))
#define TEST_NULL(record,id)	record->rec_data [id >> 3] &   (1 << (id & 7))

#define MAX_DIFFERENCES		1024	/* Max length of generated Differences string 
									   between two records */

/* Store allocation policy types.  Parameter to DPM_store() */

#define DPM_primary	1			/* New primary record */
#define DPM_secondary	2		/* Chained version of primary record */
#define DPM_other	3			/* Independent (or don't care) record */

/* Record block (holds data, remember data?) */

class rec : public pool_alloc_rpt<SCHAR, type_rec>
{
    public:
	struct fmt *rec_format;		/* what the data looks like */
	struct lls *rec_precedence;	/* stack of higher precedence pages */
	USHORT rec_length;			/* how much there is */
	struct fmt *rec_fmt_bk;
	UCHAR rec_flags;			/* misc record flags */
	SLONG rec_number;			/* original rpb number - used for undoing multiple updates */
	double rec_dummy;			/* this is to force next field to a double boundary */
	UCHAR rec_data[1];			/* THIS VARIABLE MUST BE ALIGNED ON A DOUBLE BOUNDARY */
};
typedef rec *REC;

#define REC_same_tx	1			/* record inserted/updated and deleted by same tx */
#define REC_gc_active	2		/* relation garbage collect record block in use */
#define REC_new_version 4		/* savepoint created new record version and deleted it */

/* save rpb block */

class srpb : public pool_alloc<type_srpb>
{
    public:
	struct rpb srpb_rpb[1];		/* record parameter blocks */
};
typedef srpb *SRPB;

/* request block */

#include "Request.h"
typedef Request	jrd_req;
typedef Request *JRD_REQ;


//#define REQ_SIZE	(sizeof (struct jrd_req) - sizeof (((JRD_REQ) NULL)->req_rpb[0]))

/* Flags for req_flags */
#define req_active				0x1L
#define req_stall				0x2L
#define req_leave				0x4L
#ifdef SCROLLABLE_CURSORS
#define req_async_processing	0x8L
#endif
#define req_null			   0x10L
#define req_broken			   0x20L
#define req_abort			   0x40L
#define req_internal		   0x80L
#define req_warning			  0x100L
#define req_in_use			  0x200L
#define req_sys_trigger		  0x400L	/* request is a system trigger */
#define req_count_records	  0x800L	/* count records accessed */
#define req_proc_fetch		 0x1000L	/* Fetch from procedure in progress */
#define req_ansi_any		 0x2000L	/* Request is processing ANSI ANY */
#define req_same_tx_upd		 0x4000L	/* record was updated by same transaction */
#define req_ansi_all		 0x8000L	/* Request is processing ANSI ANY */
#define req_ansi_not		0x10000L	/* Request is processing ANSI ANY */
#define req_reserved		0x20000L	/* Request reserved for client */
#define req_ignore_perm	        0x40000L	/* ignore permissions checks */
#define req_fetch_required	0x80000L	/* need to fetch next record */
#define req_error_handler      0x100000L	/* looper is called to handle error */
#define req_clone_data_from_default_clause \
                               0x200000L	/* This flag is marked in the
											   **   req_flags if and only if the
											   **   the column that was created or
											   **   added as "DEFAULT xx NOT NULL"
											   **   and no data has been
											   **   inserted/updated to the column.
											   **   The data of this column was
											   **   cloned from the default clause.
											 */
#define req_blr_version4      0x400000L	/* Request is of blr_version4 */
/* Mask for flags preserved in a clone of a request */
#define REQ_FLAGS_CLONE_MASK	(req_sys_trigger | req_internal | req_ignore_perm | req_blr_version4)

/* Mask for flags preserved on initialization of a request */
#define REQ_FLAGS_INIT_MASK	(req_in_use | req_internal | req_sys_trigger | req_ignore_perm | req_blr_version4)

/* Flags for req_view_flags */
enum {
	req_first_store_return = 0x1,
	req_first_modify_return = 0x2,
	req_first_erase_return = 0x4
};


/* Resources */

/* TMN: Had to remove this enum from bein nested in struct rsc since we now use C++,
 * but this enum is used in the C API. Can you say "legacy"? :-(
 */
enum rsc_s {
	rsc_relation,
	rsc_procedure,
	rsc_index
};

class Resource : public pool_alloc<type_rsc>
{
    public:
	class Resource* rsc_next;	/* Next resource in request */
	Relation* rsc_rel;			/* Relation block */
	Procedure* rsc_prc;			/* Relation block */
	USHORT rsc_id;				/* Id of parent */
	enum rsc_s rsc_type;
};

/* Index lock block */

class IndexLock : public pool_alloc<type_idl>
{
    public:
	class IndexLock* idl_next;	/* Next index lock block for relation */
	struct lck*	idl_lock;		/* Lock block */
	Relation*	idl_relation;	/* Parent relation */
	USHORT		idl_id;			/* Index id */
	USHORT		idl_count;		/* Use count */
};


/* Access items */

class AccessItem : public pool_alloc<type_acc>
{
    public:
	class AccessItem* acc_next;
	TEXT* acc_security_name;	/* WRITTEN into by SCL_get_class() */
	SLONG acc_view_id;
	const TEXT*	acc_trg_name;
	const TEXT*	acc_prc_name;
	const TEXT*	acc_name;
	const TEXT*	acc_type;
	USHORT		acc_mask;
};
//typedef acc *ACC;

#endif /* JRD_REQ_H */

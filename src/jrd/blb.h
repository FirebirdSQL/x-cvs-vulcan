/* $Id$ */
/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		blb.h
 *	DESCRIPTION:	Blob handling definitions
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
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "DecOSF" port
 *
 */

#ifndef JRD_BLB_H
#define JRD_BLB_H

#include "BlobID.h"

/* Your basic blob block. */

class blb : public pool_alloc_rpt<UCHAR, type_blb>
{
    public:
	Attachment* blb_attachment;	/* database attachment */
	Relation* blb_relation;	/* Relation, if known */
	class Transaction *blb_transaction;	/* Parent transaction block */
	blb *blb_next;		/* Next blob in transaction */
	UCHAR *blb_segment;			/* Next segment to be addressed */
	struct ctl *blb_filter;		/* Blob filter control block, if any */
	struct bid blb_blob_id;		/* Id of materialized blob */
	Request *blb_request;	/* request that assigned temporary blob */
	vcl *blb_pages;		/* Vector of pages */
#ifdef SHARED_CACHE
    SyncObject syncBlb_pages;
#endif
	USHORT blb_pointers;		/* Max pointer on a page */
	USHORT blb_level;			/* Storage type */
	USHORT blb_max_segment;		/* Longest segment */
	USHORT blb_flags;			/* Interesting stuff (see below) */
	USHORT blb_clump_size;		/* Size of data clump */
	USHORT blb_space_remaining;	/* Data space left */
	USHORT blb_max_pages;		/* Max pages in vector */
	USHORT blb_fragment_size;	/* Residual fragment size */
	USHORT blb_source_interp;	/* source interp (for writing) */
	USHORT blb_target_interp;	/* destination interp (for reading) */
	SSHORT blb_sub_type;		/* Blob's declared sub-type */
	ULONG blb_sequence;			/* Blob page sequence */
	ULONG blb_max_sequence;		/* Number of data pages */
	ULONG blb_count;			/* Number of segments */
	ULONG blb_length;			/* Total length of data sans segments */
	ULONG blb_lead_page;		/* First page number */
	ULONG blb_seek;				/* Seek location */
	SLONG	temporaryId;		/* globally assigned id for temporary blobs */
	/* blb_data must be longword aligned */
	UCHAR blb_data[1];			/* A page's worth of blob */
	blb(thread_db* tdbb, Transaction* transaction);
	void release(void);
	~blb(void);
	int getData(thread_db* tdbb, int bufferLength, UCHAR* buffer);
	int getLength(void);
};

const int BLB_temporary	= 1;			/* Newly created blob */
const int BLB_eof		= 2;			/* This blob is exhausted */
const int BLB_stream	= 4;			/* Stream style blob */
const int BLB_closed	= 8;			/* Temporary blob has been closed */
const int BLB_damaged	= 16;			/* Blob is busted */
const int BLB_seek		= 32;			/* Seek is pending */
const int BLB_user_def	= 64;			/* Blob is user created */
const int BLB_large_scan	= 128;		/* Blob is larger than page buffer cache */

/* Blob levels are:

	0	small blob -- blob "record" is actual data
	1	medium blob -- blob "record" is pointer to pages
	2	large blob -- blob "record" is pointer to pages of pointers
*/

/* mapping blob ids for REPLAY */
class map : public pool_alloc<type_map>
{
    public:
	map *map_next;
	blb *map_old_blob;
	blb *map_new_blob;
};
typedef map* MAP;

#endif /* JRD_BLB_H */


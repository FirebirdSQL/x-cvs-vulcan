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

#ifndef RREQUEST_H
#define RREQUEST_H

#include "SyncObject.h"
#include "RFmt.h"

/* rrq flags */

#define RRQ_backward			1	/* the cache was created in the backward direction */ 
#define RRQ_absolute_backward	2	/* rrq_absolute is measured from the end of the stream */
#define RRQ_last_backward		4	/* last time, the next level up asked for us to scroll in the backward direction */

class RDatabase;
class RTransaction;
class RMessage;
class RFormat;

struct rrq_repeat
{
	RFmt		rrq_format;		/* format for this message */
	RMessage*	rrq_message; 	/* beginning or end of cache, depending on whether it is client or server */
	RMessage*	rrq_xdr;		/* point at which cache is read or written by xdr */ 
#ifdef SCROLLABLE_CURSORS
	RMessage*	rrq_last;		/* last message returned */
	ULONG		rrq_absolute;	/* current offset in result set for record being read into cache */
	USHORT		rrq_flags;
#endif
	USHORT		rrq_msgs_waiting;	/* count of full rrq_messages */
	USHORT		rrq_rows_pending;	/* How many rows in waiting */
	USHORT		rrq_reorder_level;	/* Reorder when rows_pending < this level */
	USHORT		rrq_batch_count;	/* Count of batches in pipeline */

};

class RRequest
{
public:
	RRequest(RDatabase *database, int count);
	virtual ~RRequest(void);

	//struct blk	rrq_header;
	RDatabase*		rrq_rdb;
	RTransaction*	rrq_rtr;
	RRequest*		rrq_next;
	RRequest*		rrq_levels;		/* RRQ block for next level */
	isc_req_handle	rrq_handle;
	USHORT			rrq_id;
	USHORT			rrq_max_msg;
	USHORT			rrq_level;
	ISC_STATUS_ARRAY	rrq_status_vector;
	rrq_repeat		*rrq_rpt;
	
	SyncObject		syncObject;
	
	RRequest* clone(void);
};
#endif


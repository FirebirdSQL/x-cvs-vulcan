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

#ifndef RSTATEMENT_H
#define RSTATEMENT_H

#include "RFmt.h"

#define RSR_fetched		1		/* Cleared by execute, set by fetch */
#define RSR_eof			2		/* End-of-stream encountered */
#define RSR_blob		4		/* Statement relates to blob op */
#define RSR_no_batch	8		/* Do not batch fetch rows */
#define RSR_stream_err	16		/* There is an error pending in the batched rows */

#define STMT_BLOB		1
#define STMT_NO_BATCH	2
#define STMT_OTHER		0


class RDatabase;
class RTransaction;
class RMessage;
class RFormat;

class RStatement
{
public:
	RStatement(RDatabase *database);
	virtual ~RStatement(void);
	
	//struct blk		rsr_header;
	RStatement*		rsr_next;
	RDatabase*		rsr_rdb;
	RTransaction*	rsr_rtr;
	isc_handle		rsr_handle;
	RFmt			rsr_bind_format;		/* Format of bind message */
	RFmt			rsr_select_format;		/* Format of select message */
	RFmt			rsr_user_select_format; /* Format of user's select message */
	RFmt			rsr_format;				/* Format of current message */
	RMessage*		rsr_message;			/* Next message to process */
	RMessage*		rsr_buffer;				/* Next buffer to use */
	ISC_STATUS_ARRAY	rsr_status_vector;	/* saved status for buffered errors */
	USHORT			rsr_id;
	USHORT			rsr_flags;
	USHORT			rsr_fmt_length;

	ULONG			rsr_rows_pending;	/* How many rows are pending */
	USHORT			rsr_msgs_waiting; 	/* count of full rsr_messages */
	USHORT			rsr_reorder_level; 	/* Trigger pipelining at this level */
	USHORT			rsr_batch_count; 	/* Count of batches in pipeline */
};

#endif


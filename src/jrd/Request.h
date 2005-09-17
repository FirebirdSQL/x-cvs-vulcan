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

#ifndef _REQUEST_H_
#define _REQUEST_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

//#include "jrd_blks.h"
//#include "../include/fb_blk.h"
#include "../common/classes/array.h"
#include "SVector.h"
#include "JString.h"

#define IMPURE(request,offset)		(request->req_impure + offset)

enum req_ta {
	req_trigger_insert = 1,
	req_trigger_update = 2,
	req_trigger_delete = 3
	};

enum req_s {
	req_evaluate,
	req_return,
	req_receive,
	req_send,
	req_proceed,
	req_sync,
	req_unwind
	};

class JrdMemoryPool;
class jrd_nod;
class vec;
class Attachment;
class Transaction;
class Relation;
class AccessItem;
class Resource;
class Procedure;
//class rng;
class Savepoint;
class StatusXcp;
class RecordSource;
class ExecStatement;

struct record_param;
struct thread_db;

class Request //: public pool_alloc <type_req>
{
public:
	Request(JrdMemoryPool* pool, int rpbCount, int impureSize);
	Request(Request* request);
	~Request(void);
	
	Attachment		*req_attachment;		// database attachment
	USHORT			req_count;			// number of streams
	USHORT			req_incarnation;	// incarnation number
	ULONG			req_impure_size;	// size of impure area
	JrdMemoryPool*	req_pool;
	vec*			req_sub_requests;	// vector of sub-requests
	Transaction*	req_transaction;
	Request*		req_request;		// next request in dbb
	Request*		req_caller;			// Caller of this request
	AccessItem*		req_access;			// Access items to be checked
	//SVector<jrd_nod*> req_variables;	// Vector of variables, if any
	Resource*		req_resources;		// Resources (relations and indices)
	jrd_nod*		req_message;		// Current message for send/receive
	
#ifdef SCROLLABLE_CURSORS
	jrd_nod			*req_async_message;	// Asynchronous message (used in scrolling)
#endif

	vec*			req_refresh_ranges;	// Vector of refresh_ranges 
	//rng*			req_begin_ranges;	// Vector of refresh_ranges 
	Procedure*		req_procedure;		// procedure, if any 
	JString			req_trg_name;		// name of request (trigger), if any 
	USHORT			req_length;			// message length for send/receive 
	USHORT			req_nmsgs;			// number of message types 
	USHORT			req_mmsg;			// highest message type 
	USHORT			req_msend;			// longest send message 
	USHORT			req_mreceive;		// longest receive message 

	ULONG			req_records_selected;	/* count of records selected by request (meeting selection criteria) */
	ULONG			req_records_inserted;	/* count of records inserted by request */
	ULONG			req_records_updated;	/* count of records updated by request */
	ULONG			req_records_deleted;	/* count of records deleted by request */

	ULONG			req_records_affected;	/* count of records affected by the last statement */

	USHORT			req_view_flags;			/* special flags for virtual ops on views */
	Relation*		req_top_view_store;		/* the top view in store(), if any */
	Relation*		req_top_view_modify;	/* the top view in modify(), if any */
	Relation*		req_top_view_erase;		/* the top view in erase(), if any */

	jrd_nod*		req_top_node;			/* top of execution tree */
	jrd_nod*		req_next;				/* next node for execution */
	firebird::Array<class RecordSource*> req_fors;	/* Vector of for loops, if any */
	vec*			req_cursors;			/* Vector of named cursors, if any */
	firebird::Array<struct jrd_nod*> req_invariants;	/* Vector of invariant nodes, if any */
	USHORT			req_label;				/* label for leave */
	ULONG			req_flags;				/* misc request flags */
	Savepoint*		req_proc_sav_point;		/* procedure savepoint list */
	ULONG			req_timestamp;			/* Start time of request */
	req_ta			req_trigger_action;		/* action that caused trigger to fire */
	req_s			req_operation;			/* operation for next node */
    StatusXcp		*req_last_xcp;			/* last known exception */
	record_param	*req_rpb;				/* record parameter blocks */
	UCHAR			*req_impure;
	thread_db		*req_tdbb;
	RecordSource	*rsbs;
	ExecStatement	*execStatements;
	
	Request* getInstantiatedRequest(int instantiation);
	Request* findInstantiatedRequest(int instantiation);

	int getRequestInfo(thread_db* threadData, int itemsLength, const UCHAR* items, int bufferLength, UCHAR* buffer);
	void release(void);
	void release(thread_db* tdbb);
	void unwind(void);
	void releaseBlobs(void);
	void releaseProcedureSavePoints(void);
	void setThread(thread_db* tdbb);
	ExecStatement* getExecStatement(void);
	void init(void);
};

#endif

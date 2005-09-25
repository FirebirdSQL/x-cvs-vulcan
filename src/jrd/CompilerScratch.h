/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		exe.h
 *	DESCRIPTION:	Execution block definitions
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
 * 2001.07.28: Added rse_skip to struct rse to support LIMIT.
 * 2002.09.28 Dmitry Yemanov: Reworked internal_info stuff, enhanced
 *                            exception handling in SPs/triggers,
 *                            implemented ROWS_AFFECTED system variable
 * 2002.10.21 Nickolay Samofatov: Added support for explicit pessimistic locks
 * 2002.10.29 Nickolay Samofatov: Added support for savepoints
 */

#ifndef _COMPILER_SCRATCH_H_
#define _COMPILER_SCRATCH_H_

#include "sbm.h"
#include "lls.h"
//#include "rse.h"

const int csb_internal			= 1;	/* "csb_g_flag" switch */
const int csb_get_dependencies	= 2;
const int csb_ignore_perm		= 4;	/* ignore permissions checks */
const int csb_blr_version4		= 8;	/* The blr is of version 4 */
const int csb_pre_trigger		= 16;
const int csb_post_trigger		= 32;

const int csb_active		= 1;
const int csb_used			= 2;		/* Context has already been defined (BLR parsing only) */
const int csb_view_update	= 4;		/* View update w/wo trigger is in progress */
const int csb_trigger		= 8;		/* NEW or OLD context in trigger */
const int csb_no_dbkey		= 16;		/* Stream doesn't have a dbkey */
const int csb_validation	= 32;		/* We're in a validation expression (RDB hack) */
const int csb_store			= 64;		/* we are processing a store statement */
const int csb_modify		= 128;		/* we are processing a modify */
const int csb_compute		= 256;		/* compute cardinality for this stream */
const int csb_erase			= 512;		/* we are processing an erase */
const int csb_unmatched		= 1024;		/* stream has conjuncts unmatched by any index */

const int csb_dbkey			= 8192;		/* Dbkey as been requested (Gateway only) */
const int csb_update		= 16384;	/* Erase or modify for relation */
const int csb_made_river	= 32768;	/* stream has been included in a river */

class Resource;
class RecordSelExpr;
class Format;
class RecordSource;

struct AccessItem;
struct vec;
struct IndexDescAlloc;
struct str;

class CompilerScratch : public pool_alloc<type_csb>
{
public:
	~CompilerScratch(void);
public:
	CompilerScratch(MemoryPool& p, size_t len);
	
	const UCHAR*		csb_blr;
	const UCHAR*		csb_running;
	jrd_nod*			csb_node;
	AccessItem*			csb_access;				/* Access items to be checked */
	SVector<jrd_nod*>	csb_variables;
	Resource*			csb_resources;			/* Resources (relations and indexes) */
	NodeStack			csb_dependencies;		/* objects this request depends upon */
	firebird::Array<RecordSource*> csb_fors;	/* stack of fors */
	firebird::Array<struct jrd_nod*> csb_invariants;	/* stack of invariant nodes */
	firebird::Array<RecordSelExpr*> csb_current_rses;	/* rse's within whose scope we are */
#ifdef SCROLLABLE_CURSORS
	RecordSelExpr*		csb_current_rse;		/* this holds the rse currently being processed; 
												   unlike the current_rses stack, it references any expanded view rse */
#endif
	jrd_nod*			csb_async_message;	/* asynchronous message to send to request */
	USHORT				csb_n_stream;		/* Next available stream */
	USHORT				csb_msg_number;		/* Highest used message number */
	SLONG				csb_impure;			/* Next offset into impure area */
	USHORT				csb_g_flags;
	MemoryPool*			csb_pool;			/* Memory pool to be used by csb */
	RecordSource		*rsbs;				/* Known record source block */
	
    struct csb_repeat
	{
		// We must zero-initialize this one
		csb_repeat()
		:	csb_stream(0),
			csb_view_stream(0),
			csb_flags(0),
			csb_indices(0),
			csb_relation(0),
			csb_alias(0),
			csb_procedure(0),
			csb_view(0),
			csb_idx(0),
			csb_message(0),
			csb_format(0),
			csb_fields(0),
			csb_cardinality(0.0f),	// TMN: Non-natural cardinality?!
			csb_plan(0),
			csb_map(0),
			csb_rsb_ptr(0)
		{}

		UCHAR			csb_stream;			// Map user context to internal stream
		UCHAR			csb_view_stream;	// stream number for view relation, below
		USHORT			csb_flags;
		USHORT			csb_indices;		// Number of indices

		Relation*		csb_relation;
		str*			csb_alias;			// SQL alias name for this instance of relation 
		Procedure*		csb_procedure;
		Relation*		csb_view;			// parent view 

		IndexDescAlloc* csb_idx;			// Packed description of indices 
		jrd_nod*		csb_message;		// Msg for send/receive 
		Format*			csb_format;			// Default fmt for stream 
		UInt32Bitmap*	csb_fields;			// Fields referenced 
		float			csb_cardinality;	// Cardinality of relation 
		jrd_nod*		csb_plan;			// user-specified plan for this relation 
		UCHAR*			csb_map;			// Stream map for views 
		RecordSource** csb_rsb_ptr;			// point to rsb for nod_stream 
	};


	typedef csb_repeat* rpt_itr;
	typedef const csb_repeat* rpt_const_itr;
	firebird::HalfStaticArray<csb_repeat, 5> csb_rpt;
	
	int nextStream(bool check);
	static CompilerScratch* newCsb(MemoryPool & p, size_t len);
	void addRsb(RecordSource* rsb);
	RecordSource* stealRsbs(void);
	void postResource(Resource* resource);
};

#endif


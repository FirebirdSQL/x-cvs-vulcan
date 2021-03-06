/*
 *	PROGRAM:		JRD Access Method
 *	MODULE:			RecordSource.h
 *	DESCRIPTION:	Record source block definitions
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
 * 2001.07.28: Added rsb_t.rsb_skip and IRSB_SKIP to support LIMIT functionality.
 */

#ifndef JRD_RECORD_SOURCE_H
#define JRD_RECORD_SOURCE_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "../common/classes/array.h"
#include "../jrd/jrd_blks.h"
#include "../include/fb_blk.h"
#include "lls.h"

enum RSB_T
{
	rsb_boolean,						// predicate (logical condition)
	rsb_cross,							// inner join as a nested loop
	rsb_first,							// retrieve first n records
	rsb_skip,							// skip n records
	rsb_indexed,						// access via an index
	rsb_merge,							// join via a sort merge
	rsb_sequential,						// natural scan access
	rsb_sort,							// sort
	rsb_union,							// union
	rsb_aggregate,						// aggregation
	rsb_ext_sequential,					// external sequential access
	rsb_ext_indexed,					// external indexed access
	rsb_ext_dbkey,						// external DB_KEY access
	rsb_navigate,						// navigational walk on an index
	rsb_left_cross,						// left outer join as a nested loop
	rsb_procedure,						// stored procedure
	rsb_write_lock,						// write lock the record
	rsb_singular,						// check for singleton
	rsb_counter							// count records
};

enum RSE_GET_MODE {
	RSE_get_forward,
	RSE_get_backward,
	RSE_get_current,
	RSE_get_first,
	RSE_get_last,
	RSE_get_next
};

//typedef rse_get_mode RSE_GET_MODE;

// bits for the rsb_flags field

//const USHORT rsb_singular = 1;			// singleton select, expect 0 or 1 records
const USHORT rsb_stream_type = 2;		// rsb is for stream type request
const USHORT rsb_descending = 4;		// an ascending index is being used for a descending sort or vice versa
const USHORT rsb_project = 8;			// projection on this stream is requested
//const USHORT rsb_writelock = 16;		// records should be locked for writing

// special argument positions within the RecordSource

// Impure area formats for the various RSB types

struct irsb {
	ULONG irsb_flags;
	USHORT irsb_count;
};

typedef irsb *IRSB;

// flags for the irsb_flags field

const ULONG irsb_first = 1;
const ULONG irsb_joined = 2;				// set in left join when current record has been joined to something
const ULONG irsb_mustread = 4;				// set in left join when must read a record from left stream
const ULONG irsb_open = 8;					// indicated rsb is open
const ULONG irsb_backwards = 16;			// backwards navigation has been performed on this stream
const ULONG irsb_in_opened = 32;			// set in outer join when inner stream has been opened
const ULONG irsb_join_full = 64;			// set in full join when left join has completed
const ULONG irsb_checking_singular = 128;	// fetching to verify singleton select
const ULONG irsb_singular_processed = 256;	// singleton stream already delivered one record
const ULONG irsb_last_backwards = 512;		// rsb was last scrolled in the backward direction
const ULONG irsb_bof = 1024;				// rsb is at beginning of stream
const ULONG irsb_eof = 2048;				// rsb is at end of stream
const ULONG irsb_key_changed = 32768;		// key has changed since record last returned from rsb

class Relation;
class Procedure;
class Format;
class Request;
class SortMap;
class CompilerScratch;
class ExecutionPathInfoGen;

struct str;
struct jrd_nod;
struct thread_db;
struct record_param;

// Array which stores relative pointers to impure areas of invariant nodes
typedef firebird::SortedArray<SLONG> VarInvariantArray;
typedef firebird::Array<VarInvariantArray*> MsgInvariantArray;

class RecordSource //: public pool_alloc_rpt<class RecordSource*, type_rsb>
{
public:
	RecordSource (CompilerScratch *compilerScratch);
	RecordSource (CompilerScratch *compilerScratch, RSB_T type);
	~RecordSource();
	
	/***
	RecordSource() : rsb_left_inner_streams(0),
		rsb_left_streams(0), rsb_left_rsbs(0) { }
	***/
	RSB_T		rsb_type;			// type of rsb
	UCHAR		rsb_stream;			// stream, if appropriate
	USHORT		rsb_count;			// number of sub arguments
	USHORT		rsb_flags;
	ULONG		rsb_impure;			// offset to impure area
	ULONG		rsb_cardinality;	// estimated cardinality of stream
	ULONG		rsb_record_count;	// count of records returned from rsb (not candidate records processed)
	RecordSource* rsb_next;			// next rsb, if appropriate
	Format*		rsb_format;			// format, if appropriate
	RecordSource *nextInRequest;	// list of rsbs in request

	double		estimatedSelectivity;	// estimated selectivity of stream
	double		estimatedCost;			// estimated cost (page reads) of stream
	
	// AP:	stop saving memory with the price of awful conversions,
	//		later may be union will help this, because no ~ are
	//		needed - pool destroyed as whole entity.
	StreamStack*	rsb_left_inner_streams;
	StreamStack*	rsb_left_streams;
	RsbStack*		rsb_left_rsbs;
	VarInvariantArray *rsb_invariants; // Invariant nodes bound to top-level RSB
	//RecordSource* rsb_arg[1];
	
	virtual void open(Request* request) = 0;
	virtual bool get(Request* request, RSE_GET_MODE mode) = 0;
	virtual bool getExecutionPathInfo(Request* request, ExecutionPathInfoGen* infoGen) = 0;
	virtual void close(Request* request) = 0;
	void init(void);
	static void mapSortData(Request* request, SortMap* map, UCHAR* data);
	static void setRecordsToNulls(Request* request, StreamStack* streams);
	virtual void findRsbs(StreamStack* stream_list, RsbStack* rsb_list);
	virtual void pushRecords(Request* request);
	virtual void popRecords(Request* request);
	virtual void saveRecord(Request* request, record_param* rpb);
	virtual void restoreRecord(record_param* rpb);
	virtual bool getExecutionPathEstimationInfo(ExecutionPathInfoGen* infoGen);
};

#endif


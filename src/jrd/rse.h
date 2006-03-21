/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		rse.h
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

#ifndef JRD_RSE_H
#define JRD_RSE_H

#include "../jrd/jrd_blks.h"
#include "../include/fb_blk.h"

#include "../common/classes/array.h"
#include "../jrd/constants.h"

#include "../jrd/dsc.h"
#include "../jrd/lls.h"
#include "../jrd/sbm.h"
#include "RecordNumber.h"
#include "RecordSource.h"

class Procedure;

// Some optimizer flag bits

static const int OPT_count		= 1;		// generate Rsb to count records
static const int OPT_singular	= 2;		// generate Rsb to check record signularity


// Blocks used to compute optimal join order:
// indexed relationships block (IRL) holds 
// information about potential join orders

class IndexedRelationship : public pool_alloc<type_irl>
{
public:
	class IndexedRelationship* irl_next;	// next irl block for stream
	USHORT irl_stream;						// stream reachable by relation
	USHORT irl_unique;						// is this stream reachable by unique index?
};



// Must be less then MAX_SSHORT. Not used for static arrays.

#define MAX_CONJUNCTS	32000

// Note that MAX_STREAMS currently MUST be <= MAX_UCHAR.
// Here we should really have a compile-time fb_assert, since this hard-coded
// limit is NOT negotiable so long as we use an array of UCHAR, where index 0
// tells how many streams are in the array (and the streams themselves are
// identified by a UCHAR).

#define MAX_STREAMS	255

// This is number of ULONG's needed to store bit-mapped flags for all streams
// OPT_STREAM_BITS = (MAX_STREAMS + 1) / sizeof(ULONG)
// This value cannot be increased simple way. Decrease is possible, but it is also
// hardcoded in several places such as TEST_DEP_ARRAYS macro

#define OPT_STREAM_BITS 8

// Number of streams, conjuncts, indices that will be statically allocated 
// in various arrays. Larger numbers will have to be allocated dynamically

#define OPT_STATIC_ITEMS 16

class CompilerScratch;
struct jrd_nod;

// General optimizer block

class OptimizerBlk : public pool_alloc<type_opt>
{
public:
	CompilerScratch* opt_csb;						// compiler scratch block
	SLONG			opt_combinations;				// number of partial orders considered
	double			opt_best_cost;					// cost of best join order
	SSHORT			opt_base_conjuncts;				// number of conjuncts in our rse, next conjuncts are distributed parent
	SSHORT			opt_base_parent_conjuncts;		// number of conjuncts in our rse + distributed with parent, next are parent
	SSHORT			opt_base_missing_conjuncts;		// number of conjuncts in our and parent rse, but without missing
	USHORT			opt_best_count;					// longest length of indexable streams
	USHORT			opt_g_flags;					// global flags
	
	// 01 Oct 2003. Nickolay Samofatov: this static array takes as much as 256 bytes.
	// This is nothing compared to original Firebird 1.5 Opt structure size of ~180k
	// All other arrays had been converted to dynamic to preserve memory 
	// and improve performance
	
	struct opt_segment {
		// Index segments and their options
		jrd_nod*	opt_lower;			// lower bound on index value
		jrd_nod*	opt_upper;			// upper bound on index value
		jrd_nod*	opt_match;			// conjunct which matches index segment
	} opt_segments[MAX_INDEX_SEGMENTS];
	
	struct opt_conjunct {
		// Conjunctions and their options
		jrd_nod*	opt_conjunct_node;	// conjunction
		// Stream dependencies to compute conjunct
		ULONG		opt_dependencies[(MAX_STREAMS + 1) / 32];
		UCHAR		opt_conjunct_flags;
	};
	struct opt_stream {
		// Streams and their options
		IndexedRelationship* opt_relationships;		// streams directly reachable by index
		double				opt_best_stream_cost;	// best cost of retrieving first n = streams
		USHORT				opt_best_stream;		// stream in best join order seen so far
		USHORT				opt_stream_number;		// stream in position of join order
		UCHAR				opt_stream_flags;
	};
	
	firebird::HalfStaticArray<opt_conjunct, OPT_STATIC_ITEMS> opt_conjuncts;
	firebird::HalfStaticArray<opt_stream, OPT_STATIC_ITEMS> opt_streams;
	OptimizerBlk(JrdMemoryPool* pool) : opt_conjuncts(pool), opt_streams(pool) {}
};

// values for opt_stream_flags

const USHORT opt_stream_used = 1;			// stream is used

// values for opt_conjunct_flags

const USHORT opt_conjunct_used = 1;			// conjunct is used
const USHORT opt_conjunct_matched = 2;		// conjunct matches an index segment

// global optimizer bits used in opt_g_flags

//const USHORT opt_g_stream = 1;				// indicate that this is a blr_stream


// River block - used to hold temporary information about a group of streams
// CVC: River is a "secret" of opt.cpp, maybe a new opt.h would be adequate.

class River : public pool_alloc_rpt<SCHAR, type_riv>
{
public:
	struct RecordSource *riv_rsb;		// record source block for river
	USHORT riv_number;			// temporary number for river
	UCHAR riv_count;			// count of streams
	UCHAR riv_streams[1];		// actual streams
};


// bookmark block, used to hold information about the current position 
// within an index; a pointer to this block is passed to the user as a
// handle to facilitate returning to this position

// AB: AFAIK This is only used by PC_ENGINE define?
/***
class Bookmark : public pool_alloc_rpt<SCHAR, type_bkm>
{
public:
	class Bookmark* bkm_next;
	struct dsc bkm_desc;		// bookmark descriptor describing the bookmark handle
	ULONG bkm_handle;			// bookmark handle containing pointer to this block
	SLONG bkm_number;			// current record number
	SLONG bkm_page;				// current btree page
	SLONG bkm_incarnation;		// last known incarnation number of current btree page
	SLONG bkm_expanded_offset;	// offset into expanded index page (if it exists)
	USHORT bkm_offset;			// offset into current btree page
	USHORT bkm_flags;			// flag values indicated below
	struct dsc bkm_key_desc;	// descriptor containing current key value
	UCHAR bkm_key_data[1];		// current key value
};

const USHORT bkm_bof = 1;
const USHORT bkm_eof = 2;
const USHORT bkm_crack = 4;
const USHORT bkm_forced_crack = 8;
***/


#endif // JRD_RSE_H

/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		opt.cpp
 *	DESCRIPTION:	Optimizer / record selection expression compiler
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
 * 2002.10.12: Nickolay Samofatov: Fixed problems with wrong results produced by 
 *            outer joins
 * 2001.07.28: John Bellardo: Added code to handle rse_skip nodes.
 * 2001.07.17 Claudio Valderrama: Stop crash with indices and recursive calls
 *            of OPT_compile: indicator csb_indices set to zero after used memory is
 *            returned to the free pool.
 * 2001.02.15: Claudio Valderrama: Don't obfuscate the plan output if a selectable
 *             stored procedure doesn't access tables, views or other procedures directly.
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 * 2002.10.30: Arno Brinkman: Changes made to gen_retrieval, OPT_compile and make_inversion.
 *             Procedure sort_indices added. The changes in gen_retrieval are that now
 *             an index with high field-count has priority to build an index from. 
 *             Procedure make_inversion is changed so that it not pick every index
 *             that comes away, this was slow performance with bad selectivity indices
 *             which most are foreign_keys with a reference to a few records.
 * 2002.11.01: Arno Brinkman: Added match_indices for better support of OR handling
 *             in INNER JOIN (gen_join) statements.
 * 2002.12.15: Arno Brinkman: Added find_used_streams, so that inside opt_compile all the
 *             streams are marked active. This causes that more indices can be used for
 *             an retrieval. With this change BUG SF #219525 is solved too. 
 *
 */

// AB:Sync FB 1.236

#include "fbdev.h"
#include "common.h"
#include "../jrd/ib_stdio.h"
#include <string.h>
#include "../jrd/ibase.h"
#include "../jrd/jrd.h"
#include "../jrd/Relation.h"
#include "../jrd/Procedure.h"
#include "../jrd/align.h"
#include "../jrd/val.h"
#include "../jrd/req.h"
#include "../jrd/exe.h"
#include "../jrd/lls.h"
#include "../jrd/ods.h"
//#include "../jrd/btr.h"
#include "../jrd/sort.h"
#include "../jrd/rse.h"
#include "../jrd/intl.h"
#include "../jrd/thd.h"
#include "../jrd/gdsassert.h"
#include "../jrd/all_proto.h"
#include "../jrd/btr_proto.h"
#include "../jrd/cch_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dpm_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/ext_proto.h"
#include "../jrd/intl_proto.h"

#include "../jrd/lck_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/opt_proto.h"
#include "../jrd/par_proto.h"
#include "../jrd/thd_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/dbg_proto.h"
#include "../common/classes/array.h"
#include "../common/classes/objects_array.h"
#include "PageCache.h"
#include "../jrd/Optimizer.h"
#include "RsbProcedure.h"
#include "RsbSort.h"
#include "RsbOuterMerge.h"
#include "RsbMerge.h"
#include "RsbCross.h"
#include "RsbLeftCross.h"
#include "RsbBoolean.h"
#include "RsbSequential.h"
#include "RsbIndexed.h"
#include "RsbUnion.h"
#include "RsbNavigate.h"
#include "RsbWriteLock.h"
#include "RsbAggregate.h"
#include "RsbFirst.h"
#include "RsbSkip.h"
#include "RsbCount.h"
#include "RsbSingular.h"
#include "ExecutionPathInfoGen.h"
#include "Format.h"
#include "Resource.h"
#include "Attachment.h"

#ifdef DEV_BUILD
#define OPT_DEBUG
#endif

static bool augment_stack(jrd_nod*, NodeStack&);
static UINT64 calculate_priority_level(thread_db* tdbb, const OptimizerBlk*, const index_desc*);
static void check_indices(thread_db* tdbb, const CompilerScratch::csb_repeat*);
static bool check_relationship(const OptimizerBlk*, USHORT, USHORT);
static void check_sorts(RecordSelExpr*);
static void class_mask(USHORT, jrd_nod**, ULONG *);
static void clear_bounds(OptimizerBlk*, const index_desc*);
static jrd_nod* compose(thread_db* tdbb, jrd_nod**, jrd_nod*, NOD_T);
static void compute_dependencies(const jrd_nod*, ULONG*);
static void compute_dbkey_streams(const CompilerScratch*, const jrd_nod*, UCHAR*);
static void compute_rse_streams(const CompilerScratch*, const RecordSelExpr*, UCHAR*);
static bool check_for_nod_from(const jrd_nod*);
static SLONG decompose(thread_db*, jrd_nod*, NodeStack&, CompilerScratch*);
static USHORT distribute_equalities(thread_db* tdbb, NodeStack&, CompilerScratch*, USHORT);
static bool dump_index(thread_db* tdbb, const jrd_nod*, UCHAR**, SSHORT*);
static bool dump_rsb(thread_db* tdbb, const jrd_req*, const RecordSource*, UCHAR**, SSHORT*);
static bool estimate_cost(thread_db*, OptimizerBlk*, USHORT, double *, double *);
static bool expression_possible_unknown(const jrd_nod*);
static bool expression_contains_stream(CompilerScratch*, const jrd_nod*, UCHAR, bool*);
static void find_best(thread_db*, OptimizerBlk*, USHORT, USHORT, const UCHAR*,
					const jrd_nod*, double, double);
static void find_index_relationship_streams(thread_db*, OptimizerBlk*, const UCHAR*,
		UCHAR*, UCHAR*);
static jrd_nod* find_dbkey(jrd_nod*, USHORT, SLONG*);
static USHORT find_order(thread_db*, OptimizerBlk*, const UCHAR*, const jrd_nod*);

static void find_used_streams(const RecordSource*, UCHAR*);
static void form_rivers(thread_db*, OptimizerBlk*, const UCHAR*, RiverStack&,
		jrd_nod**, jrd_nod**, jrd_nod*);
static bool form_river(thread_db*, OptimizerBlk*, USHORT, const UCHAR*, UCHAR*,
		RiverStack&, jrd_nod**, jrd_nod**, jrd_nod*);
static RecordSource* gen_aggregate(thread_db*, OptimizerBlk*, jrd_nod*, NodeStack*, UCHAR);
static RecordSource* gen_boolean(thread_db*, OptimizerBlk*, RecordSource*, jrd_nod*);
static void gen_deliver_unmapped(thread_db*, NodeStack*, jrd_nod*, NodeStack*, UCHAR);
static RecordSource* gen_first(thread_db*, OptimizerBlk*, RecordSource*, jrd_nod*);
static void gen_join(thread_db*, OptimizerBlk*, UCHAR*, RiverStack&, jrd_nod**, jrd_nod**, jrd_nod*);
static RecordSource* gen_navigation(thread_db*, OptimizerBlk*, USHORT, Relation*, str*, index_desc*, jrd_nod**);
#ifdef SCROLLABLE_CURSORS
static RecordSource* gen_nav_rsb(thread_db*, OptimizerBlk*, USHORT, Relation*, str*, index_desc*, RSE_GET_MODE);
#else
static RecordSource* gen_nav_rsb(thread_db*, OptimizerBlk*, USHORT, Relation*, str*, index_desc*);
#endif
static RecordSource* gen_outer(thread_db*, OptimizerBlk*, RecordSelExpr*, RiverStack&, jrd_nod**, jrd_nod**);
static RecordSource* gen_procedure(thread_db*, OptimizerBlk*, jrd_nod*);
static RecordSource* gen_residual_boolean(thread_db*, OptimizerBlk*, RecordSource*);
static RecordSource* gen_retrieval(thread_db*, OptimizerBlk*, SSHORT, jrd_nod**,
					jrd_nod**, bool, bool, jrd_nod**);
static RecordSource* gen_rsb(thread_db*, OptimizerBlk*, RecordSource*, jrd_nod*, SSHORT, Relation*, str*, jrd_nod*, float, double, double);
static RecordSource*	gen_skip (thread_db*, OptimizerBlk*, RecordSource*, jrd_nod*);
static RsbSort* gen_sort(thread_db*, OptimizerBlk*, const UCHAR*, const UCHAR*, RecordSource*, jrd_nod*, bool);
static bool gen_sort_merge(thread_db*, OptimizerBlk*, RiverStack&);
static RecordSource* gen_union(thread_db*, OptimizerBlk*, jrd_nod*, UCHAR *, USHORT, NodeStack*, UCHAR);
static void get_expression_streams(const jrd_nod*, firebird::SortedArray<int>&);
static void get_inactivities(const CompilerScratch*, ULONG*);
static jrd_nod* get_unmapped_node(thread_db*, jrd_nod*, jrd_nod*, UCHAR, bool);

static jrd_nod* genBaseResidualBoolean(thread_db*, OptimizerBlk*);
static RecordSource* genInnerBooleanRsb(thread_db*, OptimizerBlk*, RecordSource*, SSHORT);
static RecordSource* genOuterRsb(thread_db*, OptimizerBlk*, RecordSelExpr*, RiverStack&, jrd_nod**, jrd_nod**);
static RecordSource* genRetrievalRsb(thread_db*, OptimizerBlk*, SSHORT, jrd_nod**, jrd_nod**, 
									 bool, bool, SortedStreamList*);
static jrd_nod* genStreamBoolean(thread_db*, OptimizerBlk*, SSHORT,	bool, bool, bool);

static IndexedRelationship* indexed_relationship(thread_db*, OptimizerBlk*, USHORT);
static RecordSource* make_cross(thread_db*, OptimizerBlk*, RiverStack&);
static jrd_nod* make_index_node(thread_db*, Relation*, CompilerScratch*, const index_desc*);
static jrd_nod* make_inference_node(thread_db* tdbb, CompilerScratch*, jrd_nod*, jrd_nod*, jrd_nod*);
static jrd_nod* make_inversion(thread_db*, OptimizerBlk*, jrd_nod*, USHORT);
static jrd_nod* make_missing(thread_db*, OptimizerBlk*, Relation*, jrd_nod*, USHORT, index_desc*);
static jrd_nod* make_starts(thread_db*, OptimizerBlk*, Relation*, jrd_nod*, USHORT, index_desc*);
static bool map_equal(const jrd_nod*, const jrd_nod*, const jrd_nod*);
static void mark_indices(CompilerScratch::csb_repeat*, SSHORT);
static SSHORT match_index(thread_db*, OptimizerBlk*, SSHORT, jrd_nod*, const index_desc*);
static bool match_indices(thread_db*, OptimizerBlk*, SSHORT, jrd_nod*, const index_desc*);
static bool node_equality(const jrd_nod*, const jrd_nod*);
static jrd_nod* optimize_like(thread_db*, jrd_nod*);
#ifdef OPT_DEBUG
static void print_order(const OptimizerBlk*, USHORT, double, double);
#endif
static USHORT river_count(USHORT, jrd_nod**);
static bool river_reference(const River*, const jrd_nod*, bool* field_found = NULL);
static bool search_stack(const jrd_nod*, const NodeStack&);
static void set_active(OptimizerBlk*, const River*);
static void set_direction(const jrd_nod*, jrd_nod*);
static void set_inactive(OptimizerBlk*, const River*);
static void set_made_river(OptimizerBlk*, const River*);
static void set_position(const jrd_nod*, jrd_nod*, const jrd_nod*);
static void set_rse_inactive(CompilerScratch*, const RecordSelExpr*);
static void sort_indices_by_selectivity(thread_db* tdbb, CompilerScratch::csb_repeat*);
static SSHORT sort_indices_by_priority(thread_db* tdbb, CompilerScratch::csb_repeat*, index_desc**, UINT64*);


// macro definitions 

#ifdef OPT_DEBUG
#define DEBUG_PUNT		5
#define DEBUG_RELATIONSHIPS	4
#define DEBUG_ALL		3
#define DEBUG_CANDIDATE		2
#define DEBUG_BEST		1
#define DEBUG_NONE		0

IB_FILE *opt_debug_file = 0;
static int opt_debug_flag = DEBUG_NONE;
#endif

inline void SET_DEP_BIT(ULONG* array, const SLONG bit)
{
	array[bit / 32] |= (1L << (bit % 32));
}

inline void CLEAR_DEP_BIT(ULONG* array, const SLONG bit)
{
	array[bit / 32] &= ~(1L << (bit % 32));
}

inline bool TEST_DEP_BIT(const ULONG* array, const ULONG bit)
{
	return (array[bit / 32] & (1L << (bit % 32))) != 0;
}

inline bool TEST_DEP_ARRAYS(const ULONG* ar1, const ULONG* ar2)
{	return (ar1[0] & ar2[0]) || (ar1[1] & ar2[1]) ||
			(ar1[2] & ar2[2]) || (ar1[3] & ar2[3]) ||
			(ar1[4] & ar2[4]) || (ar1[5] & ar2[5]) ||
			(ar1[6] & ar2[6]) || (ar1[7] & ar2[7]);
}

// some arbitrary fudge factors for calculating costs, etc.--
// these could probably be better tuned someday

const double ESTIMATED_SELECTIVITY			= 0.01;
const int INVERSE_ESTIMATE					= 10;
const double INDEX_COST						= 30.0;
const int CACHE_PAGES_PER_STREAM			= 15;
const int SELECTIVITY_THRESHOLD_FACTOR		= 10;
const int OR_SELECTIVITY_THRESHOLD_FACTOR	= 2000;
const UINT64 LOWEST_PRIORITY_LEVEL		= 0;

// enumeration of sort datatypes 

static const UCHAR sort_dtypes[] = 
{
	0,							// dtype_unknown 
	SKD_text,					// dtype_text 
	SKD_cstring,				// dtype_cstring 
	SKD_varying,				// dtype_varying 
	0,
	0,
	0,							// dtype_packed 
	0,							// dtype_byte 
	SKD_short,					// dtype_short
	SKD_long,					// dtype_long 
	SKD_quad,					// dtype_quad 
	SKD_float,					// dtype_real 
	SKD_double,					// dtype_double
	SKD_double,					// dtype_d_float
	SKD_sql_date,				// dtype_sql_date 
	SKD_sql_time,				// dtype_sql_time 
	SKD_timestamp2,				// dtype_timestamp
	0,							// dtype_blob
	0,							// dtype_array 
	SKD_int64					// dtype_int64 
};

typedef UCHAR stream_array_t[MAX_STREAMS + 1];


bool OPT_access_path(thread_db* tdbb, 
					 Request* request,
					 UCHAR* buffer,
					 SSHORT buffer_length, 
					 USHORT* return_length)
{
/**************************************
 *
 *	O P T _ a c c e s s _ p a t h
 *
 **************************************
 *
 * Functional description
 *	Returns a formatted access path for all
 *	rse's in the specified request.
 *
 **************************************/
	DEV_BLKCHK(request, type_req);

#ifdef OBSOLETE
	const UCHAR* const begin = buffer;

	//loop through all RSEs in the request, 
	///and describe the rsb tree for that rsb 

	size_t i;

	for (i = 0; i < request->req_fors.getCount(); i++) 
		{
		RecordSource* rsb = request->req_fors[i];

		if (rsb && !dump_rsb(tdbb, request, rsb, &buffer, &buffer_length))
			break;
		}

	*return_length = buffer - begin;

	return (i >= request->req_fors.getCount());
#endif // OBSOLETE

	ExecutionPathInfoGen info(tdbb, buffer, buffer_length);
	bool result = info.putRequest(request);
	*return_length = info.length();
	return result;
}


RecordSource* OPT_compile(thread_db*		tdbb,
						  CompilerScratch*	csb, 
						  RecordSelExpr*	rse, 
						  NodeStack*		parent_stack,
						  int				compileFlags)
{
/**************************************
 *
 *	O P T _ c o m p i l e
 *
 **************************************
 *
 * Functional description
 *	Compile and optimize a record selection expression into a
 *	set of record source blocks (rsb's).
 *
 **************************************/
	stream_array_t streams, beds, local_streams, outer_streams, sub_streams,
		key_streams;
	
		
	//DEV_BLKCHK(csb, type_csb);
	//DEV_BLKCHK(rse, type_nod);
	//SET_TDBB(tdbb);
	
	Database* dbb = tdbb->tdbb_database;

#ifdef OPT_DEBUG
	if (opt_debug_flag != DEBUG_NONE && !opt_debug_file)
		opt_debug_file = fopen("opt_debug.out", "w");
#endif


	// If there is a boolean, there is some work to be done.  First,
	// decompose the boolean into conjunctions.  Then get descriptions
	// of all indices for all relations in the RecordSelExpr.  This will give
	// us the info necessary to allocate a optimizer block big
	// enough to hold this crud. 


	// Do not allocate the index_desc struct. Let BTR_all do the job. The allocated
	// memory will then be in csb->csb_rpt[stream].csb_idx_allocation, which
	// gets cleaned up before this function exits. 

	OptimizerBlk* opt = FB_NEW(*tdbb->tdbb_default) OptimizerBlk(tdbb->tdbb_default);
	opt->opt_streams.grow(csb->csb_n_stream);
	RecordSource* rsb = 0;

	try 
		{
		opt->opt_csb = csb;
		beds[0] = streams[0] = key_streams[0] = outer_streams[0] = sub_streams[0] = 0;
		NodeStack conjunct_stack;
		RiverStack rivers_stack;
		SLONG conjunct_count = 0;

		check_sorts(rse);
		jrd_nod* sort = rse->rse_sorted;
		jrd_nod* project = rse->rse_projection;
		jrd_nod* aggregate = rse->rse_aggregate;

		// put any additional booleans on the conjunct stack, and see if we 
		// can generate additional booleans by associativity--this will help 
		// to utilize indices that we might not have noticed
		
		if (rse->rse_boolean)
			conjunct_count = decompose(tdbb, rse->rse_boolean, conjunct_stack, csb);	

		conjunct_count += distribute_equalities(tdbb, conjunct_stack, csb, conjunct_count);

		// AB: If we have limit our retrieval with FIRST / SKIP syntax then
		// we may not deliver above conditions (from higher rse's) to this 
		// rse, because the results should be consistent.
		
		if (rse->rse_skip || rse->rse_first) 
			parent_stack = NULL;

		// clear the csb_active flag of all streams in the RecordSelExpr
		
		set_rse_inactive(csb, rse);
		UCHAR* p = streams + 1;

		// go through the record selection expression generating 
		// record source blocks for all streams

		// CVC: I defined this var here because it's assigned inside an if() shortly
		// below but it's used later in the loop always, so I assume the idea is that
		// iterations where nod_type != nod_rse are the ones that set up a new stream.
		// Hope this isn't some kind of logic error.
		
		SSHORT stream = -1;
		jrd_nod** ptr = rse->rse_relation;
		
		for (const jrd_nod* const* const end = ptr + rse->rse_count; ptr < end; ptr++)
			{
			jrd_nod* node = *ptr;

			// find the stream number and place it at the end of the beds array
			// (if this is really a stream and not another RecordSelExpr)

			if (node->nod_type != nod_rse)
				{
				stream = (USHORT)(long) node->nod_arg[STREAM_INDEX(node)];
				fb_assert(stream <= MAX_UCHAR);
				fb_assert(beds[0] < MAX_STREAMS && beds[0] < MAX_UCHAR);
				beds[++beds[0]] = (UCHAR) stream;
				}

			// for nodes which are not relations, generate an rsb to
			// represent that work has to be done to retrieve them;
			// find all the substreams involved and compile them as well

			rsb = NULL;
			local_streams[0] = 0;
			
			switch (node->nod_type)
				{
				case nod_union:
					{
					const SSHORT i = (SSHORT) key_streams[0];
					compute_dbkey_streams(csb, node, key_streams);

					NodeStack::const_iterator stack_end;
					
					if (parent_stack) 
						stack_end = conjunct_stack.merge(*parent_stack);

					rsb = gen_union(tdbb, opt, node, key_streams + i + 1,
									(key_streams[0] - i), &conjunct_stack, stream);
									
					if (parent_stack) 
						conjunct_stack.split(stack_end, *parent_stack);

					fb_assert(local_streams[0] < MAX_STREAMS && local_streams[0] < MAX_UCHAR);
					local_streams[++local_streams[0]] = (UCHAR)(long) node->nod_arg[e_uni_stream];
					break;
					}
					
				case nod_aggregate:
					{
					fb_assert((int) (long)node->nod_arg[e_agg_stream] <= MAX_STREAMS);
					fb_assert((int) (long)node->nod_arg[e_agg_stream] <= MAX_UCHAR);
					NodeStack::const_iterator stack_end;
					
					if (parent_stack) 
						stack_end = conjunct_stack.merge(*parent_stack);
						
					rsb = gen_aggregate(tdbb, opt, node, &conjunct_stack, stream);
					
					if (parent_stack) 
						conjunct_stack.split(stack_end, *parent_stack);

					fb_assert(local_streams[0] < MAX_STREAMS && local_streams[0] < MAX_UCHAR);
					local_streams[++local_streams[0]] = (UCHAR)(long) node->nod_arg[e_agg_stream];
					break;
					}
					
				case nod_procedure:
					rsb = gen_procedure(tdbb, opt, node);
					fb_assert(local_streams[0] < MAX_STREAMS && local_streams[0] < MAX_UCHAR);
					local_streams[++local_streams[0]] = (UCHAR)(long) node->nod_arg[e_prc_stream];
					break;
					
				case nod_rse:
					compute_rse_streams(csb, (RecordSelExpr*) node, beds);
					compute_rse_streams(csb, (RecordSelExpr*) node, local_streams);
					compute_dbkey_streams(csb, node, key_streams);
					
					// pass RecordSelExpr boolean only to inner substreams because join condition 
					// should never exclude records from outer substreams 
					
					if (rse->rse_jointype == blr_inner 
							|| (rse->rse_jointype == blr_left 
								&& (ptr - rse->rse_relation) == 1))
						{
						// AB: For an (X LEFT JOIN Y) mark the outer-streams (X) as 
						// active because the inner-streams (Y) are always "dependent" 
						// on the outer-streams. So that index retrieval nodes could be made.
						// For an INNER JOIN mark previous generated RecordSource's as active.
						
						if (rse->rse_jointype == blr_left) 
							for (SSHORT i = 1; i <= outer_streams[0]; i++) 
								csb->csb_rpt[outer_streams[i]].csb_flags |= csb_active;

						const NodeStack::iterator stackSavepoint(conjunct_stack);
						NodeStack::const_iterator stack_end;
						NodeStack deliverStack;

						if (rse->rse_jointype != blr_inner) 
							{
							// Make list of nodes that can be delivered to a outer-stream.
							// In fact this are all nodes except when a IS NULL (nod_missing) 
							// comparision is done.
							// Note! Don't forget that this can be burried inside a expression
							// such as "CASE WHEN (FieldX IS NULL) THEN 0 ELSE 1 END = 0"
							
							NodeStack::iterator stackItem;
							
							if (parent_stack)
								stackItem = *parent_stack;

							for (; stackItem.hasData(); ++stackItem) 
								{
								jrd_nod* deliverNode = stackItem.object();
								
								if (!expression_possible_unknown(deliverNode)) 
									deliverStack.push(deliverNode);
								}
								
							stack_end = conjunct_stack.merge(deliverStack);
							}
						else 
							if (parent_stack)
								stack_end = conjunct_stack.merge(*parent_stack);

						rsb = OPT_compile(tdbb, csb, (RecordSelExpr*) node, &conjunct_stack, 0);

						// Remove added parent conjuctions from the stack of inner joins

						if (rse->rse_jointype != blr_inner) 
							conjunct_stack.split(stack_end, deliverStack);
						else 
							if (parent_stack)
								conjunct_stack.split(stack_end, *parent_stack);

						if (rse->rse_jointype == blr_left) 
							for (SSHORT i = 1; i <= outer_streams[0]; i++) 
								csb->csb_rpt[outer_streams[i]].csb_flags &= ~csb_active;
						}
					else 
						{
						rsb = OPT_compile(tdbb, csb, (RecordSelExpr*) node, parent_stack, 0);
						}
						
					break;
				}

			// if an rsb has been generated, we have a non-relation;
			// so it forms a river of its own since it is separately 
			// optimized from the streams in this rsb

			if (rsb) 
				{
				const SSHORT i = local_streams[0];
				River* river = FB_NEW_RPT(*tdbb->tdbb_default, i) River();
				river->riv_count = (UCHAR) i;
				river->riv_rsb = rsb;
				MOVE_FAST(local_streams + 1, river->riv_streams, i);
				
				// AB: Save all inner-part streams				
				if (rse->rse_jointype == blr_inner  
						|| (rse->rse_jointype == blr_left && (ptr - rse->rse_relation) == 0))
					{
					find_used_streams(rsb, sub_streams);
					// Save also the outer streams						
					if (rse->rse_jointype == blr_left)
						find_used_streams(rsb, outer_streams);
					}

				set_made_river(opt, river);
				set_inactive(opt, river);
				rivers_stack.push(river);
				continue;
				}

			// we have found a base relation; record its stream 
			// number in the streams array as a candidate for 
			// merging into a river

			// TMN: Is the intention really to allow streams[0] to overflow?
			// I must assume that is indeed not the intention (not to mention
			// it would make code later on fail), so I added the following fb_assert.
			
			fb_assert(streams[0] < MAX_STREAMS && streams[0] < MAX_UCHAR);
			++streams[0];
			*p++ = (UCHAR) stream;

			if (rse->rse_jointype == blr_left) 
				{
				fb_assert(outer_streams[0] < MAX_STREAMS && outer_streams[0] < MAX_UCHAR);
				outer_streams[++outer_streams[0]] = stream;
				}

			// if we have seen any booleans or sort fields, we may be able to
			// use an index to optimize them; retrieve the current format of 
			// all indices at this time so we can determine if it's possible 
			// AB: if a parent_stack was available and conjunct_count was 0
			// then no indices where retrieved. Added also OR check on 
			// parent_stack below. SF BUG # [ 508594 ]

			if (conjunct_count || sort || project || aggregate || parent_stack)
				{
				Relation* relation = (Relation*) node->nod_arg[e_rel_relation];
				
				if (relation && !relation->rel_file)
					{
					csb->csb_rpt[stream].csb_indices =
						BTR_all(tdbb, relation, &csb->csb_rpt[stream].csb_idx);
					sort_indices_by_selectivity(tdbb, &csb->csb_rpt[stream]);
					mark_indices(&csb->csb_rpt[stream], relation->rel_id);
					}
				else
					csb->csb_rpt[stream].csb_indices = 0;

				const Format* format = CMP_format(tdbb, csb, stream);
				csb->csb_rpt[stream].csb_cardinality = 
					OPT_getRelationCardinality(tdbb, relation, format);
				}
			}

		// this is an attempt to make sure we have a large enough cache to 
		// efficiently retrieve this query; make sure the cache has a minimum
		// number of pages for each stream in the RecordSelExpr (the number is just a guess)
		
		if (streams[0] > 5) 
			CCH_EXPAND(tdbb, (ULONG) (streams[0] * CACHE_PAGES_PER_STREAM));

		// At this point we are ready to start optimizing.  
		// We will use the opt block to hold information of
		// a global nature, meaning that it needs to stick 
		// around for the rest of the optimization process.

		// Set base-point before the parent/distributed nodes begin.
		
		opt->opt_base_conjuncts = (SSHORT) conjunct_count;

		// AB: Add parent conjunctions to conjunct_stack, keep in mind
		// the outer-streams! For outer streams put missing (IS NULL)
		// conjunctions in the missingStack.
		//
		// opt_rpt[0..opt_base_conjuncts-1] = defined conjunctions to this stream
		// opt_rpt[0..opt_base_parent_conjuncts-1] = defined conjunctions to this 
		//   stream and allowed distributed conjunctions (with parent)
		// opt_rpt[0..opt_base_missing_conjuncts-1] = defined conjunctions to this 
		//   stream and allowed distributed conjunctions and allowed parent
		// opt_rpt[0..opt_conjuncts_count-1] = all conjunctions
		//
		// allowed = booleans that can never evaluate to NULL/Unknown or turn
		//   NULL/Unkown into a True or False.
		
		SLONG distributed_count = 0;
		NodeStack missingStack;
		
		if (parent_stack && parent_stack->getCount())
			{
			NodeStack::iterator iter(*parent_stack);
			
			for (; iter.hasData() && conjunct_count < MAX_CONJUNCTS; ++iter) 
				{
				jrd_nod* node = iter.object();
				
				if ((rse->rse_jointype != blr_inner) && expression_possible_unknown(node))
					{
					// parent missing conjunctions shouldn't be 
					// distributed to FULL OUTER JOIN streams at all
					
					if (rse->rse_jointype != blr_full) 
						missingStack.push(node);
					}
				else
					{
					conjunct_stack.push(node);
					conjunct_count++;
					}
				}	
				
			// We've now merged parent, try again to make more conjunctions. 
			
			distributed_count = distribute_equalities(tdbb, conjunct_stack, csb, conjunct_count);
			conjunct_count += distributed_count;
			}
			
		// The newly created conjunctions belong to the base conjunctions. 
		// After them are starting the parent conjunctions.
		
		opt->opt_base_parent_conjuncts = opt->opt_base_conjuncts + (SSHORT) distributed_count;

		// Set base-point before the parent IS NULL nodes begin
		
		opt->opt_base_missing_conjuncts = (SSHORT) conjunct_count;
			
		// Check if size of optimizer block exceeded.
		
		if (conjunct_count > MAX_CONJUNCTS) 
			ERR_post(isc_optimizer_blk_exc, 0); 
			// Msg442: size of optimizer block exceeded

		// Put conjunctions in opt structure. 
		// Note that it's a stack and we get the nodes in reversed order from the stack.
		
		opt->opt_conjuncts.grow(conjunct_count);
		SSHORT i;
		SSHORT j = 0;
		SSHORT nodeBase = 0;
		
		for (i = conjunct_count; i > 0; i--) 
			{
			jrd_nod* node = conjunct_stack.pop();

			// a conjunct is eitther a base conjunction or a parent or
			// a new conjunction created by "distribution" from the stack.
			// mark the new ones as belonging to their respective bases

			if (i == opt->opt_base_conjuncts)
				{
				j = 0;
				nodeBase = 0;
				}
			else if (i == conjunct_count)
				{
				j = 0;
				nodeBase = opt->opt_base_conjuncts;
				}
			else if (i == (conjunct_count - distributed_count))
				{
				j = 0;
				nodeBase = opt->opt_base_conjuncts + distributed_count;
				}

			opt->opt_conjuncts[nodeBase + j].opt_conjunct_node = node;
			compute_dependencies(node, opt->opt_conjuncts[nodeBase + j].opt_dependencies);
			j++;
			}

		// Put the parent missing nodes on the stack.
		
		while (missingStack.hasData() && (conjunct_count < MAX_CONJUNCTS))
			{
			opt->opt_conjuncts.grow(conjunct_count + 1);
			jrd_nod* node = missingStack.pop();
			opt->opt_conjuncts[conjunct_count].opt_conjunct_node = node;
			compute_dependencies(node,  opt->opt_conjuncts[conjunct_count].opt_dependencies);
			conjunct_count++;
			}

		// Deoptimize some conjuncts in advance

		for (i = 0; i < opt->opt_conjuncts.getCount(); i++)
			if (opt->opt_conjuncts[i].opt_conjunct_node->nod_flags & nod_deoptimize)
				opt->opt_conjuncts[i].opt_conjunct_flags |= opt_conjunct_matched;

		// attempt to optimize aggregates via an index, if possible

		if (aggregate && !sort && !project) 
			sort = aggregate;
		else 
			rse->rse_aggregate = aggregate = NULL;

		// AB: Mark the previous used streams (sub-RecordSelExpr's) as active

		for (i = 1; i <= sub_streams[0]; i++) 
			csb->csb_rpt[sub_streams[i]].csb_flags |= csb_active;			

		// outer joins require some extra processing

		if (rse->rse_jointype != blr_inner) 
			{
			if (dbb->dbb_ods_version >= ODS_VERSION11) 
				rsb = genOuterRsb(tdbb, opt, rse, rivers_stack, &sort, &project);
			else
				rsb = gen_outer(tdbb, opt, rse, rivers_stack, &sort, &project);
			}
		else 
			{
			bool sort_present = (sort);
			bool sort_can_be_used = true;
			jrd_nod* const saved_sort_node = sort;

			// AB: If previous rsb's are already on the stack we can't use
			// an navigational-retrieval for an ORDER BY cause the next
			// streams are JOINed to the previous ones
			
			if (rivers_stack.hasData()) 
				{
				sort = NULL;
				sort_can_be_used = false;
				
				// AB: We could already have multiple rivers at this
				// point so try to do some sort/merging now.
				
				while (rivers_stack.hasMore(1) && gen_sort_merge(tdbb, opt, rivers_stack))
					;
					
				// AB: Mark the previous used streams (sub-RecordSelExpr's) again
				// as active, because an SORT/MERGE could reset the flags
				
				for (i = 1; i <= sub_streams[0]; i++) 
					csb->csb_rpt[sub_streams[i]].csb_flags |= csb_active;			
				}

			fb_assert(streams[0] != 1 || csb->csb_rpt[streams[1]].csb_relation != 0);

			while (true)
				{
				// AB: Determine which streams have an index relationship
				// with the currently active rivers. This is needed so that
				// no merge is made between a new cross river and the
				// currently active rivers. Where in the new cross river 
				// a stream depends (index) on the active rivers.
				
				stream_array_t dependent_streams, free_streams;
				dependent_streams[0] = free_streams[0] = 0;
				find_index_relationship_streams(tdbb, opt, streams, dependent_streams, free_streams);

				// If we have dependent and free streams then we can't rely on 
				// the sort node to be used for index navigation.
				
				if (dependent_streams[0] && free_streams[0]) 
					{
					sort = NULL;
					sort_can_be_used = false;
					}

				if (dependent_streams[0]) 
					{
					// copy free streams
					
					for (i = 0; i <= free_streams[0]; i++)
						streams[i] = free_streams[i];

					// Make rivers from the dependent streams
					
					gen_join(tdbb, opt, dependent_streams, rivers_stack, &sort, &project, rse->rse_plan);

					// Generate 1 river which holds a cross join rsb between
					// all currently available rivers.

					// First get total count of streams.
					
					int count = 0;
					RiverStack::iterator stack1(rivers_stack);
					
					for (; stack1.hasData(); ++stack1) 
						count += stack1.object()->riv_count;

					// Create river and copy the streams.
					
					River* river = FB_NEW_RPT(*tdbb->tdbb_default, count) River();
					river->riv_count = (UCHAR) count;
					UCHAR* stream = river->riv_streams;
					RiverStack::iterator stack2(rivers_stack);
					
					for (; stack2.hasData(); ++stack2) 
						{
						River* subRiver = stack2.object();
						MOVE_FAST(subRiver->riv_streams, stream, subRiver->riv_count);
						stream += subRiver->riv_count;
						}
						
					river->riv_rsb = make_cross(tdbb, opt, rivers_stack);
					rivers_stack.push(river);

					// Mark the river as active.
					set_made_river(opt, river);
					set_active(opt, river);
					}
				else
					{
					if (free_streams[0]) 
						{
						// Deactivate streams from rivers on stack, because
						// the remaining streams don't have any indexed relationship with them.
						RiverStack::iterator stack1(rivers_stack);
						
						for (; stack1.hasData(); ++stack1) 
							set_inactive(opt, stack1.object());
						}

					break;
					}
				}

			// attempt to form joins in decreasing order of desirability 
			
			gen_join(tdbb, opt, streams, rivers_stack, &sort, &project, rse->rse_plan);

			// If there are multiple rivers, try some sort/merging
			
			while (rivers_stack.hasMore(1) && gen_sort_merge(tdbb, opt, rivers_stack))
				;

			rsb = make_cross(tdbb, opt, rivers_stack);

			// AB: When we have a merge then a previous made ordering with 
			// an index doesn't guarantee that the result will be in that
			// order. So we assigned the sort node back.
			// SF BUG # [ 221921 ] ORDER BY has no effect
			
			RecordSource* test_rsb = rsb;
			
			if ((rsb) && (rsb->rsb_type == rsb_boolean) && (rsb->rsb_next)) 
				test_rsb = rsb->rsb_next;
				
			if ((sort_present && !sort_can_be_used) ||
				((test_rsb) && (test_rsb->rsb_type == rsb_merge) && !sort && sort_present))
				sort = saved_sort_node;

			// Pick up any residual boolean that may have fallen thru the cracks

			rsb = gen_residual_boolean(tdbb, opt, rsb);
			}

		// if the aggregate was not optimized via an index, get rid of the 
		// sort and flag the fact to the calling routine
		
		if (aggregate && sort) 
			{
			rse->rse_aggregate = NULL;
			sort = NULL;
			}

		// check index usage in all the base streams to ensure
		// that any user-specified access plan is followed

		for (i = 1; i <= streams[0]; i++) 
			check_indices(tdbb, &csb->csb_rpt[streams[i]]);

		// Eliminate any duplicate dbkey streams in sort and 
		// project (aka distinct)

		if (project || sort) 
			{
			const UCHAR* const b_end = beds + beds[0];
			const UCHAR* const k_end = key_streams + key_streams[0];
			UCHAR* k = &key_streams[1];
			
			for (const UCHAR* p2 = k; p2 <= k_end; p2++) 
				{
				const UCHAR* q = &beds[1];
				
				while (q <= b_end && *q != *p2) 
					q++;

				if (q > b_end) 
					*k++ = *p2;
				}
				
			key_streams[0] = k - &key_streams[1];

			// Handle project clause, if present.
			
			if (project) 
				rsb = gen_sort(tdbb, opt, beds, key_streams, rsb, project, true);

			// Handle sort clause if present
			
			if (sort) 
				rsb = gen_sort(tdbb, opt, beds, key_streams, rsb, sort, false);
			}

		// Handle first and/or skip.  The skip MUST (if present)
		// appear in the rsb list AFTER the first.  Since the gen_first and gen_skip
		// functions add their nodes at the beginning of the rsb list we MUST call
		// gen_skip before gen_first.

		if (rse->rse_skip) 
			rsb = gen_skip(tdbb, opt, rsb, rse->rse_skip);

		if (rse->rse_first) 
			rsb = gen_first(tdbb, opt, rsb, rse->rse_first);

		// release memory allocated for index descriptions
		
		for (i = 0; i < streams[0]; i++) 
			{
			stream = streams[i + 1];
			delete csb->csb_rpt[stream].csb_idx;
			csb->csb_rpt[stream].csb_idx = 0;

			// CVC: The following line added because OPT_compile is recursive, both directly
			// and through gen_union(), too. Otherwise, we happen to step on deallocated memory
			// and this is the cause of the crashes with indices that have plagued IB since v4.

			csb->csb_rpt[stream].csb_indices = 0;
			}

		DEBUG
		
		// free up memory for optimizer structures
		
		delete opt;

	#ifdef OPT_DEBUG
		if (opt_debug_file) 
			{
			ib_fflush(opt_debug_file);
			//ib_fclose(opt_debug_file);
			//opt_debug_file = 0;
			}
	#endif
		}	// try

	catch (...) 
		{
		for (SSHORT i = 0; i < streams[0]; i++) 
			{
			const SSHORT stream = streams[i + 1];
			delete csb->csb_rpt[stream].csb_idx;
			csb->csb_rpt[stream].csb_idx = 0;
			csb->csb_rpt[stream].csb_indices = 0; // Probably needed to be safe
			}
			
		delete opt;
		throw;
		}

	if (rse->rse_writelock)
		//rsb->rsb_flags |= rsb_writelock;
		{
		StreamStack stack;
		rsb->findRsbs(&stack, NULL);
		
		while (stack.hasData())
			{
			int stream = stack.pop();
			Relation *relation = csb->csb_rpt[stream].csb_relation;
			
			if (relation && !relation->rel_view_rse && !relation->rel_file) 
				rsb = new (tdbb->tdbb_default) RsbWriteLock(csb, rsb, stream);
			}
		}
	
	// Assign pointer to list of dependent invariant values

	rsb->rsb_invariants = rse->rse_invariants;
	
	if (compileFlags & OPT_singular)
		rsb = new (tdbb->tdbb_default) RsbSingular(csb, rsb);
		
	if (compileFlags & OPT_count)
		rsb = new (tdbb->tdbb_default) RsbCount(csb, rsb);
		
	return rsb;
}


jrd_nod* OPT_make_dbkey(thread_db* tdbb, 
						OptimizerBlk* opt, 
						jrd_nod* boolean, 
						USHORT stream)
{
/**************************************
 *
 *	O P T _ m a k e _ d b k e y
 *
 **************************************
 *
 * Functional description
 *	If boolean is an equality comparison on the proper dbkey,
 *	make a "bit_dbkey" operator (makes bitmap out of dbkey
 *	expression.
 *
 *	This is a little hairy, since view dbkeys are expressed as
 *	concatenations of primitive dbkeys.
 *
 **************************************/
	DEV_BLKCHK(opt_, type_opt);
	DEV_BLKCHK(boolean, type_nod);

	// If this isn't an equality, it isn't even interesting

	if (boolean->nod_type != nod_eql)
		return NULL;

	// Find the side of the equality that is potentially a dbkey.  If
	// neither, make the obvious deduction 

	jrd_nod* dbkey = boolean->nod_arg[0];
	jrd_nod* value = boolean->nod_arg[1];
	SLONG n = 0;

	if (dbkey->nod_type != nod_dbkey && dbkey->nod_type != nod_concatenate)
		{
		if (value->nod_type != nod_dbkey 
				&& value->nod_type != nod_concatenate)
			return NULL;

		dbkey = value;
		value = boolean->nod_arg[0];
		}

	// If the value isn't computable, this has been a waste of time 

	CompilerScratch* csb = opt->opt_csb;

	if (!OPT_computable(csb, value, stream, false, false)) 
		return NULL;

	// If this is a concatenation, find an appropriate dbkey 

	if (dbkey->nod_type == nod_concatenate) 
		{
		dbkey = find_dbkey(dbkey, stream, &n);

		if (!dbkey)
			return NULL;
		}


	// Make sure we have the correct stream 

	if ((USHORT)(long) dbkey->nod_arg[0] != stream)
		return NULL;

	// If this is a dbkey for the appropriate stream, it's invertable 

	dbkey = PAR_make_node(tdbb, 2);
	dbkey->nod_count = 1;
	dbkey->nod_type = nod_bit_dbkey;
	dbkey->nod_arg[0] = value;
	dbkey->nod_arg[1] = (jrd_nod*) (long) n;
	dbkey->nod_impure = CMP_impure(csb, sizeof(struct impure_inversion));

	return dbkey;
}


jrd_nod* OPT_make_index(thread_db* tdbb, 
						OptimizerBlk* opt, 
						Relation* relation, 
						index_desc* idx)
{
/**************************************
 *
 *	O P T _ m a k e _ i n d e x
 *
 **************************************
 *
 * Functional description
 *	Build node for index scan.
 *
 **************************************/
 
	/***
	SET_TDBB(tdbb);
	DEV_BLKCHK(opt_, type_opt);
	DEV_BLKCHK(relation, type_rel);
	***/
	
	// Allocate both a index retrieval node and block. 

	jrd_nod* node = make_index_node(tdbb, relation, opt->opt_csb, idx);
	IndexRetrieval* retrieval = (IndexRetrieval*) node->nod_arg[e_idx_retrieval];
	retrieval->irb_relation = relation;

	// Pick up lower bound segment values

	jrd_nod** lower = retrieval->irb_value;
	jrd_nod** upper = retrieval->irb_value + idx->idx_count;
	const OptimizerBlk::opt_segment* const end = opt->opt_segments + idx->idx_count;
	const OptimizerBlk::opt_segment* tail;

	if (idx->idx_flags & idx_descending) 
		{
		for (tail = opt->opt_segments; tail->opt_lower && tail < end; tail++)
			*upper++ = tail->opt_lower;
			
		for (tail = opt->opt_segments; tail->opt_upper && tail < end; tail++)
			*lower++ = tail->opt_upper;
			
		retrieval->irb_generic |= irb_descending;
		}
	else 
		{
		for (tail = opt->opt_segments; tail->opt_lower && tail < end; tail++)
			*lower++ = tail->opt_lower;
			
		for (tail = opt->opt_segments; tail->opt_upper && tail < end; tail++)
			*upper++ = tail->opt_upper;
		}

	retrieval->irb_lower_count = lower - retrieval->irb_value;
	retrieval->irb_upper_count = (upper - retrieval->irb_value) - idx->idx_count;

	bool equiv = false;

	for (tail = opt->opt_segments; tail->opt_match && tail < end; tail++)
		if (tail->opt_match->nod_type == nod_equiv)
			{
			equiv = true;
			break;
			}

	// This index is never used for IS NULL, thus we can ignore NULLs
	// already at index scan. But this rule doesn't apply to nod_equiv
	// which requires NULLs to be found in the index.
	// A second exception is when this index is used for navigation.
	
	if (!equiv && !(idx->idx_runtime_flags & idx_navigate))
		retrieval->irb_generic |= irb_ignore_null_value_key;


	bool includeLower = true, includeUpper = true;
	for (tail = opt->opt_segments; (tail->opt_lower || tail->opt_upper) && 
									tail->opt_match && (tail < end); tail++)
		{
		switch (tail->opt_match->nod_type)
			{
			case nod_gtr: 
				if (retrieval->irb_generic & irb_descending)
					includeUpper = false;
				else
					includeLower = false;
				break;
			
			case nod_lss:
				if (retrieval->irb_generic & irb_descending)
					includeLower = false;
				else
					includeUpper = false;
				break;
			}
		}

	if (!includeLower)
		retrieval->irb_generic |= irb_exclude_lower;

	if (!includeUpper)
		retrieval->irb_generic |= irb_exclude_upper;

	// Check to see if this is really an equality retrieval 
	if (retrieval->irb_lower_count == retrieval->irb_upper_count) 
		{
		retrieval->irb_generic |= irb_equality;
		lower = retrieval->irb_value;
		upper = retrieval->irb_value + idx->idx_count;
		
		const jrd_nod* const* const end_node = lower + retrieval->irb_lower_count;

		while (lower < end_node)
			if (*upper++ != *lower++)
				{
				retrieval->irb_generic &= ~irb_equality;
				break;
				}
		}

	// If we are matching less than the full index, this is a partial match 

	if (idx->idx_flags & idx_descending) 
		{
		if (retrieval->irb_lower_count < idx->idx_count) 
			retrieval->irb_generic |= irb_partial;
		}
	else 
		if (retrieval->irb_upper_count < idx->idx_count) 
			retrieval->irb_generic |= irb_partial;

	// mark the index as utilized for the purposes of this compile 

	idx->idx_runtime_flags |= idx_used;

	return node;
}


int OPT_match_index(thread_db* tdbb, 
					OptimizerBlk* opt, 
					USHORT stream, 
					index_desc* idx)
{
/**************************************
 *
 *	O P T _ m a t c h _ i n d e x
 *
 **************************************
 *
 * Functional description
 *	Match any active (computable but not consumed) boolean
 *	conjunctions against a given index.  This is used by
 *	the external relation modules to do index optimization.
 *	Return the number of matching items.
 *
 **************************************/
	DEV_BLKCHK(opt, type_opt);

	// if there are not conjunctions, don't waste our time 

	if (!opt->opt_base_conjuncts)
		return 0;

	CompilerScratch* csb = opt->opt_csb;
	const OptimizerBlk::opt_conjunct* const opt_end =
		opt->opt_conjuncts.begin() + opt->opt_base_conjuncts;
	int n = 0;
	clear_bounds(opt, idx);

	for (OptimizerBlk::opt_conjunct* tail = opt->opt_conjuncts.begin();
			tail < opt_end; tail++)
		{
		jrd_nod* node = tail->opt_conjunct_node;

		if (!(tail->opt_conjunct_flags & opt_conjunct_used)
				&& OPT_computable(csb, node, -1, true, false))
			n += match_index(tdbb, opt, stream, node, idx);
		}

	return n; 
}


static bool augment_stack(jrd_nod* node, 
						  NodeStack& stack)
{
/**************************************
 *
 *	a u g m e n t _ s t a c k
 *
 **************************************
 *
 * Functional description
 *	Add node to stack unless node is already on stack.
 *
 **************************************/
	DEV_BLKCHK(node, type_nod);

	for (NodeStack::const_iterator temp(stack); temp.hasData(); ++temp) 
		if (node_equality(node, temp.object())) 
			return false;

	stack.push(node);

	return true;
}


static UINT64 calculate_priority_level(thread_db* tdbb, 
									   const OptimizerBlk* opt, 
									   const index_desc* idx)
{
/**************************************
 *
 *	c a l c u l a t e _ p r i o r i t y _ l e v e l
 *
 **************************************
 *
 * Functional description
 *	Return an calculated value based on
 *	how nodes where matched on the index.
 *	Before calling this function the
 *	match_index function must be called first!
 *         
 **************************************/
	if (opt->opt_segments[0].opt_lower || opt->opt_segments[0].opt_upper) 
		{
		// Count how many fields can be used in this index and
		// count the maximum equals that matches at the begin.

		USHORT idx_eql_count = 0;
		USHORT idx_field_count = 0;
		const OptimizerBlk::opt_segment* idx_tail = opt->opt_segments;
		const OptimizerBlk::opt_segment* const idx_end = idx_tail + idx->idx_count;

		for (; idx_tail < idx_end && (idx_tail->opt_lower || idx_tail->opt_upper); idx_tail++)
			{
			idx_field_count++;
			const jrd_nod* node = idx_tail->opt_match;

			if (node->nod_type == nod_eql) 
				idx_eql_count++;
			else 
				break;
			}
		
		// Note: dbb->dbb_max_idx = 1022 for the largest supported page of 16K and
		//						    62 for the smallest page of 1K

		const UINT64 max_idx = tdbb->tdbb_database->dbb_max_idx + 1;
		UINT64 unique_prefix = 0;

		if ((idx->idx_flags & idx_unique) && (idx_eql_count == idx->idx_count)) 
			unique_prefix = (max_idx - idx->idx_count) * max_idx * max_idx * max_idx;

		// Calculate our priority level.

		return unique_prefix + ((idx_eql_count * max_idx * max_idx) + 
			(idx_field_count * max_idx) + (max_idx - idx->idx_count));

		}
	else 
		return LOWEST_PRIORITY_LEVEL;

}


static void check_indices(thread_db* tdbb, 
						  const CompilerScratch::csb_repeat* csb_tail)
{
/**************************************
 *
 *	c h e c k _ i n d i c e s
 *
 **************************************
 *
 * Functional description
 *	Check to make sure that the user-specified
 *	indices were actually utilized by the optimizer.
 *         
 **************************************/
	const jrd_nod* plan = csb_tail->csb_plan;
	if (!plan) 
		return;

	if (plan->nod_type != nod_retrieve) 
		return;

	Relation* relation = csb_tail->csb_relation;

	// if there were no indices fetched at all but the
	// user specified some, error out using the first
	// index specified 

	const jrd_nod* access_type = 0;

	if (!csb_tail->csb_indices 
			&& (access_type = plan->nod_arg[e_retrieve_access_type]))		
		ERR_post(isc_index_unused, isc_arg_string, 
				access_type->nod_arg[2], 0); // index %s cannot be used in the specified plan 

	// check to make sure that all indices are either used or marked not to be used,
	// and that there are no unused navigational indices 

	SqlIdentifier index_name;

	const index_desc* idx = csb_tail->csb_idx->items;

	for (USHORT i = 0; i < csb_tail->csb_indices; i++) 
		{
		if (!(idx->idx_runtime_flags & (idx_plan_dont_use | idx_used)) 
				|| ((idx->idx_runtime_flags & idx_plan_navigate)
					&& !(idx->idx_runtime_flags & idx_navigate)))
			{
			if (!(idx->idx_runtime_flags & (idx_plan_missing | idx_plan_starts)))
				{	
				if (relation) 
					MET_lookup_index(tdbb, index_name, relation->rel_name,
									 (USHORT) (idx->idx_id + 1));
				else 
					index_name[0] = 0;

				ERR_post(isc_index_unused, isc_arg_string,
						 ERR_cstring(index_name), 0); // index %s cannot be used in the specified plan 

				}
			}
		++idx;
		}
}


static bool check_relationship(const OptimizerBlk* opt, 
							   USHORT position, 
							   USHORT stream)
{
/**************************************
 *
 *	c h e c k _ r e l a t i o n s h i p 
 *
 **************************************
 *
 * Functional description
 *	Check for a potential indexed relationship.
 *
 **************************************/
	DEV_BLKCHK(opt, type_opt);

	const OptimizerBlk::opt_stream* tail = opt->opt_streams.begin();
	const OptimizerBlk::opt_stream* const end = tail + position;

	for (; tail < end; tail++)
		{
		const USHORT n = tail->opt_stream_number;
		const IndexedRelationship* relationship = opt->opt_streams[n].opt_relationships;

		for (; relationship; relationship = relationship->irl_next)
			if (stream == relationship->irl_stream) 
				return true;
		}

	return false;
}


static void check_sorts(RecordSelExpr* rse)
{
/**************************************
 *
 *	c h e c k _ s o r t s
 *
 **************************************
 *
 * Functional description
 *	Try to optimize out unnecessary sorting. 
 *
 **************************************/
	DEV_BLKCHK(rse, type_nod);

	jrd_nod* sort = rse->rse_sorted;
	jrd_nod* project = rse->rse_projection;

	// check if a GROUP BY exists using the same fields as the project or sort:
	// if so, the projection can be eliminated; if no projection exists, then 
	// the sort can be eliminated.

	jrd_nod *group, *sub_rse;

	if ((project || sort) 
			&& (rse->rse_count == 1) 
			&& (sub_rse = rse->rse_relation[0])
			&& (sub_rse->nod_type == nod_aggregate) 
			&& (group = sub_rse->nod_arg[e_agg_group]))
		{
		// if all the fields of the project are the same as all the fields 
		// of the group by, get rid of the project.

		if (project && (project->nod_count == group->nod_count)) 
			{
			jrd_nod** project_ptr = project->nod_arg;
			const jrd_nod* const* const project_end = project_ptr + project->nod_count;

			for (; project_ptr < project_end; project_ptr++) 
				{
				const jrd_nod* const* group_ptr = group->nod_arg;
				const jrd_nod* const* const group_end = group_ptr + group->nod_count;

				for (; group_ptr < group_end; group_ptr++) 
					if (map_equal(*group_ptr, *project_ptr, sub_rse->nod_arg[e_agg_map])) 
						break;

				if (group_ptr == group_end) 
					break;
				}

			// we can now ignore the project, but in case the project is being done 
			// in descending order because of an order by, do the group by the same way.

			if (project_ptr == project_end) 
				{
				set_direction(project, group);
				project = rse->rse_projection = NULL;
				}
			}

		// if there is no projection, then we can make a similar optimization 
		// for sort, except that sort may have fewer fields than group by.

		if (!project && sort && (sort->nod_count <= group->nod_count)) 
			{
			const jrd_nod* const* sort_ptr = sort->nod_arg;
			const jrd_nod* const* const sort_end = sort_ptr + sort->nod_count;

			for (; sort_ptr < sort_end; sort_ptr++)
				{
				const jrd_nod* const* group_ptr = group->nod_arg;
				const jrd_nod* const* const group_end = group_ptr + sort->nod_count;

				for (; group_ptr < group_end; group_ptr++)
					if (map_equal(*group_ptr, *sort_ptr, sub_rse->nod_arg[e_agg_map])) 
						break;

				if (group_ptr == group_end) 
					break;
				}

			// if all the fields in the sort list match the first n fields in the 
			// project list, we can ignore the sort, but update the sort order 
			// (ascending/descending) to match that in the sort list

			if (sort_ptr == sort_end) 
				{
				set_direction(sort, group);
				set_position(sort, group, sub_rse->nod_arg[e_agg_map]);
				sort = rse->rse_sorted = NULL;
				}
			}

		}

	// examine the ORDER BY and DISTINCT clauses; if all the fields in the 
	// ORDER BY match the first n fields in the DISTINCT in any order, the 
	// ORDER BY can be removed, changing the fields in the DISTINCT to match 
	// the ordering of fields in the ORDER BY.

	if (sort && project && (sort->nod_count <= project->nod_count)) 
		{
		const jrd_nod* const* sort_ptr = sort->nod_arg;
		const jrd_nod* const* const sort_end = sort_ptr + sort->nod_count;

		for (; sort_ptr < sort_end; sort_ptr++) 
			{
			const jrd_nod* const* project_ptr = project->nod_arg;
			const jrd_nod* const* const project_end = project_ptr + sort->nod_count;

			for (; project_ptr < project_end; project_ptr++)
				{
				if ((*sort_ptr)->nod_type == nod_field
						&& (*project_ptr)->nod_type == nod_field
						&& (*sort_ptr)->nod_arg[e_fld_stream] == (*project_ptr)->nod_arg[e_fld_stream]
						&& (*sort_ptr)->nod_arg[e_fld_id] == (*project_ptr)->nod_arg[e_fld_id])
					break;
				}

			if (project_ptr == project_end)
				break;
			}


		// if all the fields in the sort list match the first n fields 
		// in the project list, we can ignore the sort, but update 
		// the project to match the sort.

		if (sort_ptr == sort_end) 
			{
			set_direction(sort, project);
			set_position(sort, project, NULL);
			sort = rse->rse_sorted = NULL;
			}
		}

	// RP: optimize sort with OUTER JOIN
	// if all the fields in the sort list are from one stream, check the stream is
	// the most outer stream, if true update rse and ignore the sort

	if (sort && !project) 
		{
		int sort_stream = 0;
		bool usableSort = true;
		const jrd_nod* const* sort_ptr = sort->nod_arg;
		const jrd_nod* const* const sort_end = sort_ptr + sort->nod_count;

		for (; sort_ptr < sort_end; sort_ptr++) 
			{
			if ((*sort_ptr)->nod_type == nod_field) 
				{
				// Get stream for this field at this position.

				const int current_stream = (int)(long)(*sort_ptr)->nod_arg[e_fld_stream];

				// If this is the first position node, save this stream.
				// If not and the current stream is different then the previous stream
				// then we can't use this sort for an indexed order retrieval.					

				if (sort_ptr == sort->nod_arg) 
			    	sort_stream = current_stream;
				else if (current_stream != sort_stream) 
					{
					usableSort = false;
					break;
					}
				}	
			else 
				{
				// If this is not the first position node, reject this sort.
				// Two expressions cannot be mapped to a single index.

				if (sort_ptr > sort->nod_arg) 
					{
					usableSort = false;
					break;
					}

				// This position doesn't use a simple field, thus we should
				// check the expression internals.

				firebird::SortedArray<int> streams;
				get_expression_streams(*sort_ptr, streams);

				// We can use this sort only if there's a single stream
				// referenced by the expression.

				if (streams.getCount() == 1) 
					sort_stream = streams[0];
				else 
					{
					usableSort = false;
					break;
					}
				}
			}

		if (usableSort) 
			{
			RecordSelExpr* new_rse = NULL;
			jrd_nod* node = (jrd_nod*) rse;

			while (node) 
				{
				if (node->nod_type == nod_rse) 
					{
					new_rse = (RecordSelExpr*) node;

					// AB: Don't distribute the sort when a FIRST/SKIP is supplied,
					// because that will affect the behaviour from the deeper RSE.

					if (new_rse != rse &&
						(new_rse->rse_first || new_rse->rse_skip)) 
						{
						node = NULL;
						break;
						}

					// Walk trough the relations of the RSE and see if a
					// matching stream can be found.

					if (new_rse->rse_jointype == blr_inner) 
						{
						if (new_rse->rse_count == 1) 
                            node = new_rse->rse_relation[0];
						else 
							{
							bool sortStreamFound = false;

							for (int i = 0; i < new_rse->rse_count; i++) 
								{
								jrd_nod* subNode = (jrd_nod*) new_rse->rse_relation[i];

								if (subNode->nod_type == nod_relation  
										&& ((int)(long)subNode->nod_arg[e_rel_stream]) == sort_stream 
										&& new_rse != rse) 
									{
									sortStreamFound = true;
									break;
									}
								}

							if (sortStreamFound)
								{
								new_rse->rse_sorted = sort;
								sort = rse->rse_sorted = NULL;
								}

							node = NULL;
							}
						}
					else if (new_rse->rse_jointype == blr_left) 
						node = new_rse->rse_relation[0];
					else 
						node = NULL;
					}
				else
					{
					if (node->nod_type == nod_relation  
							&& ((int)(long)node->nod_arg[e_rel_stream]) == sort_stream  
							&& new_rse && new_rse != rse) 
						{
						new_rse->rse_sorted = sort;
						sort = rse->rse_sorted = NULL;
						}

					node = NULL;
					}
				}
			}
		}
}


static void class_mask(USHORT count, 
					   jrd_nod** eq_class, 
					   ULONG* mask)
{
/**************************************
 *
 *	c l a s s _ m a s k
 *
 **************************************
 *
 * Functional description
 *	Given an sort/merge join equivalence class (vector of node pointers
 *	of representative values for rivers), return a bit mask of rivers
 *	with values.
 *
 **************************************/
#ifdef DEV_BUILD
/* 	The DEV_BLKCHK macro is defined as nothing.
	if (*eq_class) 
		DEV_BLKCHK(*eq_class, type_nod);
*/
#endif

	if (count > MAX_CONJUNCTS) 
		ERR_post(isc_optimizer_blk_exc, 0); // Msg442: size of optimizer block exceeded 

	SLONG i;

	for (i = 0; i < OPT_STREAM_BITS; i++) 
		mask[i] = 0;

	for (i = 0; i < count; i++, eq_class++) 
		{
		if (*eq_class) 
			{
			SET_DEP_BIT(mask, i);
			DEV_BLKCHK(*eq_class, type_nod);
			}
		}
}


static void clear_bounds(OptimizerBlk* opt, 
						 const index_desc* idx)
{
/**************************************
 *
 *	c l e a r _ b o u n d s
 *
 **************************************
 *
 * Functional description
 *	Clear upper and lower value slots before matching booleans to
 *	indices.
 *
 **************************************/
	DEV_BLKCHK(opt, type_opt);

	const OptimizerBlk::opt_segment* const opt_end = &opt->opt_segments[idx->idx_count];

	for (OptimizerBlk::opt_segment* tail = opt->opt_segments; tail < opt_end; tail++) 
		{
		tail->opt_lower = NULL;
		tail->opt_upper = NULL;
		tail->opt_match = NULL;
		}
}


static jrd_nod* compose(thread_db* tdbb, 
						jrd_nod** node1, 
						jrd_nod* node2, NOD_T node_type)
{
/**************************************
 *
 *	c o m p o s e
 *
 **************************************
 *
 * Functional description
 *	Build and AND out of two conjuncts.
 *
 **************************************/

	DEV_BLKCHK(*node1, type_nod);
	DEV_BLKCHK(node2, type_nod);

	if (!node2) 
		return *node1;

	if (!*node1)
		return (*node1 = node2);

	return *node1 = OPT_make_binary_node(tdbb, node_type, *node1, node2, false);
}


static void compute_dependencies(const jrd_nod* node, 
								 ULONG* dependencies)
{
/**************************************
 *
 *	c o m p u t e _ d e p e n d e n c i e s
 *
 **************************************
 *
 * Functional description
 *	Compute stream dependencies for evaluation of an expression.
 *
 **************************************/

	DEV_BLKCHK(node, type_nod);

	// Recurse thru interesting sub-nodes 

	const jrd_nod* const* ptr = node->nod_arg;

	if (node->nod_type == nod_procedure) 
		return;

	{ // scope for MSVC6
	const jrd_nod* const* const end = ptr + node->nod_count;

	for (; ptr < end; ptr++)
		compute_dependencies(*ptr, dependencies);
	} // scope

	const RecordSelExpr* rse;
	const jrd_nod* sub;
	const jrd_nod* value;

	switch (node->nod_type) 
		{
		case nod_field:
			{
			const SLONG n = (SLONG)(long) node->nod_arg[e_fld_stream];
			SET_DEP_BIT(dependencies, n);
			return;
			}

		case nod_rec_version:
		case nod_dbkey:
			{
			const SLONG n = (SLONG)(long) node->nod_arg[0];
			SET_DEP_BIT(dependencies, n);
			return;
			}

		case nod_min:
		case nod_max:
		case nod_average:
		case nod_total:
		case nod_count:
		case nod_from:
			if ((sub = node->nod_arg[e_stat_default])) 
				compute_dependencies(sub, dependencies);

			rse = (RecordSelExpr*) node->nod_arg[e_stat_rse];
			value = node->nod_arg[e_stat_value];
			break;

		case nod_rse:
			rse = (RecordSelExpr*) node;
			value = NULL;
			break;

		default:
			return;
		}

	// Node is a record selection expression.  Groan.  Ugh.  Yuck. 

	if ((sub = rse->rse_first))
		compute_dependencies(sub, dependencies);

	// Check sub-expressions 

	if ((sub = rse->rse_boolean))
		compute_dependencies(sub, dependencies);

	if ((sub = rse->rse_sorted)) 
		compute_dependencies(sub, dependencies);

	if ((sub = rse->rse_projection)) 
		compute_dependencies(sub, dependencies);

	// Check value expression, if any 

	if (value) 
		compute_dependencies(value, dependencies);

	// Reset streams inactive 

	ptr = rse->rse_relation;

	for (const jrd_nod* const* const end = ptr + rse->rse_count; ptr < end; ptr++)
		{
		if ((*ptr)->nod_type != nod_rse) 
			{
			const SLONG n = (SLONG)(long) (*ptr)->nod_arg[STREAM_INDEX((*ptr))];
			CLEAR_DEP_BIT(dependencies, n);
			}
		}
}


static void compute_dbkey_streams(const CompilerScratch* csb, 
								  const jrd_nod* node,
                                  UCHAR* streams)
{
/**************************************
 *
 *	c o m p u t e _ d b k e y _ s t r e a m s
 *
 **************************************
 *
 * Functional description
 *	Identify all of the streams for which a
 *	dbkey may need to be carried through a sort.
 *
 **************************************/
	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(node, type_nod);

	if (node->nod_type == nod_relation) 
		{
		fb_assert(streams[0] < MAX_STREAMS && streams[0] < MAX_UCHAR);
		streams[++streams[0]] = (UCHAR)(long) node->nod_arg[e_rel_stream];
		}
	else if (node->nod_type == nod_union) 
		{
		const jrd_nod* clauses = node->nod_arg[e_uni_clauses];

		if (clauses->nod_type != nod_procedure) 
			{
			const jrd_nod* const* ptr = clauses->nod_arg;
			for (const jrd_nod* const* const end = ptr + clauses->nod_count;
						ptr < end; ptr += 2)
				compute_dbkey_streams(csb, *ptr, streams);
			}
		}
	else if (node->nod_type == nod_rse) 
		{
		const RecordSelExpr* rse = (RecordSelExpr*) node;
		const jrd_nod* const* ptr = rse->rse_relation;

		for (const jrd_nod* const* const end = ptr + rse->rse_count;
				ptr < end; ptr++)
			compute_dbkey_streams(csb, *ptr, streams);
		}
}


static void compute_rse_streams(const CompilerScratch* csb, 
								const RecordSelExpr* rse,
								UCHAR* streams)
{
/**************************************
 *
 *	c o m p u t e _ r s e _ s t r e a m s
 *
 **************************************
 *
 * Functional description
 *	Identify the streams that make up an RecordSelExpr.
 *
 **************************************/
	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(rse, type_nod);

	const jrd_nod* const* ptr = rse->rse_relation;

	for (const jrd_nod* const* const end = ptr + rse->rse_count;
			ptr < end; ptr++)
		{
		const jrd_nod* node = *ptr;

		if (node->nod_type != nod_rse)
			{
			fb_assert(streams[0] < MAX_STREAMS && streams[0] < MAX_UCHAR);
			streams[++streams[0]] = (UCHAR)(long) node->nod_arg[STREAM_INDEX(node)];
			}
		else 
			compute_rse_streams(csb, (const RecordSelExpr*) node, streams);
		}
}


static bool check_for_nod_from(const jrd_nod* node)
{
/**************************************
 *
 *	c h e c k _ f o r _ n o d _ f r o m
 *
 **************************************
 *
 * Functional description
 *	Check for nod_from under >=0 nod_cast nodes.
 *
 **************************************/
	if (node->nod_type == nod_from)
		return true;
	if (node->nod_type == nod_cast)
		return check_for_nod_from(node->nod_arg[e_cast_source]);
	return false;
}


static SLONG decompose(thread_db* tdbb,
					   jrd_nod* boolean_node, 
					   NodeStack& stack, 
					   CompilerScratch* csb)
{
/**************************************
 *
 *	d e c o m p o s e
 *
 **************************************
 *
 * Functional description
 *	Decompose a boolean into a stack of conjuctions.
 *
 **************************************/
	DEV_BLKCHK(boolean_node, type_nod);
	DEV_BLKCHK(csb, type_csb);

	if (boolean_node->nod_type == nod_and) 
		return decompose(tdbb, boolean_node->nod_arg[0], stack, csb) +
			decompose(tdbb, boolean_node->nod_arg[1], stack, csb);

	// turn a between into (a greater than or equal) AND (a less than  or equal) 

	if (boolean_node->nod_type == nod_between) 
		{
		jrd_nod* arg = boolean_node->nod_arg[0];

		if (check_for_nod_from(arg)) 
			{
			// Without this ERR_punt(), server was crashing with sub queries 
			// under "between" predicate, Bug No. 73766 */
			ERR_post(isc_optimizer_between_err, 0);
			// Msg 493: Unsupported field type specified in BETWEEN predicate 
			}

		jrd_nod* node = OPT_make_binary_node(tdbb, nod_geq, arg, boolean_node->nod_arg[1], true);
		stack.push(node);
		arg = CMP_clone_node(tdbb, csb, arg);
		node = OPT_make_binary_node(tdbb, nod_leq, arg, boolean_node->nod_arg[2], true);
		stack.push(node);
		return 2;
		}

	// turn a LIKE into a LIKE and a STARTING WITH, if it starts
	// with anything other than a pattern-matching character 

	jrd_nod* arg;

	if ((boolean_node->nod_type == nod_like) 
			&& (arg = optimize_like(tdbb, boolean_node))) 
		{
		stack.push(OPT_make_binary_node(tdbb, nod_starts, 
			boolean_node->nod_arg[0], arg, false));
		stack.push(boolean_node);
		return 2;
		}


	if (boolean_node->nod_type == nod_or)
		{
		NodeStack or_stack;
		if (decompose(tdbb, boolean_node->nod_arg[0], or_stack, csb) >= 2)
			{
			boolean_node->nod_arg[0] = or_stack.pop();
			while (or_stack.hasMore(0))
				{
				boolean_node->nod_arg[0] =
					OPT_make_binary_node(tdbb, nod_and, boolean_node->nod_arg[0], or_stack.pop(), true);
				}
			}

		or_stack.clear();
		if (decompose(tdbb, boolean_node->nod_arg[1], or_stack, csb) >= 2)
			{
			boolean_node->nod_arg[1] = or_stack.pop();
			while (or_stack.hasMore(0))
				{
				boolean_node->nod_arg[1] =	
					OPT_make_binary_node(tdbb, nod_and, boolean_node->nod_arg[1], or_stack.pop(), true);				
				}
			}
		}

	stack.push(boolean_node);

	return 1;
}


static USHORT distribute_equalities(thread_db* tdbb, 
									NodeStack& org_stack, 
									CompilerScratch* csb, 
									USHORT base_count)
{
/**************************************
 *
 *	d i s t r i b u t e _ e q u a l i t i e s
 *
 **************************************
 *
 * Functional description
 *	Given a stack of conjunctions, generate some simple
 *	inferences.  In general, find classes of equalities,
 *	then find operations based on members of those classes.
 *	If we find any, generate additional conjunctions.  In
 *	SSHORT:
 *
 *		If (a == b) and (a $ c) --> (b $ c) for any
 *		operation '$'.
 *
 **************************************/
	firebird::ObjectsArray<NodeStack> classes;
	firebird::ObjectsArray<NodeStack>::iterator eq_class;

	DEV_BLKCHK(csb, type_csb);

	// Zip thru stack of booleans looking for field equalities 

	for (NodeStack::iterator stack1(org_stack); stack1.hasData(); ++stack1) 
		{
		jrd_nod* boolean = stack1.object();

		if (boolean->nod_flags & nod_deoptimize)
			continue;

		if (boolean->nod_type != nod_eql)
			continue;

		jrd_nod* node1 = boolean->nod_arg[0];

		if (node1->nod_type != nod_field)
			continue;

		jrd_nod* node2 = boolean->nod_arg[1];

		if (node2->nod_type != nod_field)
			continue;

		for (eq_class = classes.begin(); eq_class != classes.end(); ++eq_class)
			{
			if (search_stack(node1, *eq_class)) 
				{
				augment_stack(node2, *eq_class);
				break;
				}
			else if (search_stack(node2, *eq_class)) 
				{
				eq_class->push(node1);
				break;
				}
			}

		if (eq_class == classes.end()) 
			{
			NodeStack& s = classes.add();
			s.push(node1);
			s.push(node2);
			eq_class = classes.back();
			}
		}

	if (classes.getCount() == 0)
		return 0;

	// Make another pass looking for any equality relationships that may
	// have crept in between classes (this could result from the
	// sequence (A = B, C = D, B = C) 

	for (eq_class = classes.begin(); eq_class != classes.end(); ++eq_class)
		{
		for (NodeStack::const_iterator stack2(*eq_class); stack2.hasData(); ++stack2) 
			{
			for (firebird::ObjectsArray<NodeStack>::iterator eq_class2(eq_class); 
				++eq_class2 != classes.end();)
				{
				if (search_stack(stack2.object(), *eq_class2)) 
					{
					DEBUG;
					while (eq_class2->hasData()) 
						augment_stack(eq_class2->pop(), *eq_class);
					}
				}
			}
		}

	USHORT count = 0;

	// Start by making a pass distributing field equalities 

	for (eq_class = classes.begin(); eq_class != classes.end(); ++eq_class) 
		{
		if (eq_class->hasMore(3)) 
			{
			for (NodeStack::iterator outer(*eq_class); outer.hasData(); ++outer) 
				{
				for (NodeStack::iterator inner(outer); (++inner).hasData(); ) 
					{
					jrd_nod* boolean = OPT_make_binary_node(tdbb, nod_eql, 
												outer.object(), inner.object(), true);

					if ((base_count + count < MAX_CONJUNCTS) 
							&& augment_stack(boolean, org_stack))
						{
						DEBUG;
						count++;
						}
					else
						delete boolean;
					}
				}
			}
		}

	// Now make a second pass looking for non-field equalities 

	for (NodeStack::iterator stack3(org_stack); stack3.hasData(); ++stack3) 
		{
		jrd_nod* boolean = stack3.object();

		if (boolean->nod_type != nod_eql 
				&& boolean->nod_type != nod_gtr 
				&& boolean->nod_type != nod_geq
				&& boolean->nod_type != nod_leq 
				&& boolean->nod_type != nod_lss
				&& boolean->nod_type != nod_matches 
				&& boolean->nod_type != nod_contains 
				&& boolean->nod_type != nod_like)
			continue;

		const jrd_nod* node1 = boolean->nod_arg[0];
		const jrd_nod* node2 = boolean->nod_arg[1];
		bool reverse = false;

		if (node1->nod_type != nod_field) 
			{
			const jrd_nod* swap_node = node1;
			node1 = node2;
			node2 = swap_node;
			reverse = true;
			}

		if (node1->nod_type != nod_field) 
			continue;

		if (node2->nod_type != nod_literal 
				&& node2->nod_type != nod_variable
				&& node2->nod_type != nod_argument) 
			continue;

		for (eq_class = classes.begin(); eq_class != classes.end(); ++eq_class) 
			{
			if (search_stack(node1, *eq_class)) 
				{
				for (NodeStack::iterator temp(*eq_class); temp.hasData(); ++temp) 
					{
					if (!node_equality(node1, temp.object())) 
						{
						jrd_nod* arg1;
						jrd_nod* arg2;

						if (reverse) 
							{
							arg1 = boolean->nod_arg[0];
							arg2 = temp.object();
							}
						else 
							{
							arg1 = temp.object();
							arg2 = boolean->nod_arg[1];
							}

						// From the conjuncts X(A,B) and A=C, infer the
						// conjunct X(C,B)
	
						jrd_nod* new_node = make_inference_node(tdbb, csb, boolean, 
													arg1, arg2);

						if ((base_count + count < MAX_CONJUNCTS) 
								&& augment_stack(new_node, org_stack))
							count++;
						}
					}
				break;
				}
			}
		}

	return count;
}

#ifdef OBSOLETE
static bool dump_index(thread_db* tdbb, 
					   const jrd_nod* node,
					   UCHAR** buffer_ptr, 
					   SSHORT* buffer_length)
{
/**************************************
 *
 *	d u m p _ i n d e x
 *
 **************************************
 *
 * Functional description
 *	Dump an index inversion tree to 
 *	an info buffer.
 *
 **************************************/
	DEV_BLKCHK(node, type_nod);

	UCHAR* buffer = *buffer_ptr;

	if (--(*buffer_length) < 0) 
		return false;

	// spit out the node type

	switch (node->nod_type)
		{
		case nod_bit_and:
			*buffer++ = isc_info_rsb_and;
			break;
		case nod_bit_or:
		case nod_bit_in:
			*buffer++ = isc_info_rsb_or;
			break;
		case nod_bit_dbkey:
			*buffer++ = isc_info_rsb_dbkey;
			break;
		case nod_index:
			*buffer++ = isc_info_rsb_index;
			break;
		}

	SqlIdentifier index_name;

	// dump sub-nodes or the actual index info

	if ((node->nod_type == nod_bit_and)
			|| (node->nod_type == nod_bit_or) 
			|| (node->nod_type == nod_bit_in))
		{
		if (!dump_index(tdbb, node->nod_arg[0], &buffer, buffer_length)) 
			return false;

		if (!dump_index(tdbb, node->nod_arg[1], &buffer, buffer_length)) 
			return false;
		}
	else if (node->nod_type == nod_index) 
		{
		IndexRetrieval* retrieval = (IndexRetrieval*) node->nod_arg[e_idx_retrieval];
		MET_lookup_index(tdbb, index_name, retrieval->irb_relation->rel_name,
						 (USHORT) (retrieval->irb_index + 1));
		SSHORT length = strlen(index_name);

		*buffer_length -= 1 + length;

		if (*buffer_length < 0) 
			return false;

		*buffer++ = (SCHAR) length;
		const TEXT* i = index_name;
		while (length--) 
			*buffer++ = *i++;
		}

	*buffer_ptr = buffer;

	return true;
}

static bool dump_rsb(thread_db* tdbb, 
					 const jrd_req* request, c
					 const RecordSource* rsb, 
					 UCHAR** buffer_ptr, 
					 SSHORT* buffer_length)
{
/**************************************
 *
 *	d u m p _ r s b
 *
 **************************************
 *
 * Functional description
 *	Returns a formatted access path for
 *	a particular rsb.
 *
 **************************************/
	DEV_BLKCHK(rsb, type_rsb);
	UCHAR *buffer = *buffer_ptr;

	// leave room for the rsb begin, type, and end.

	*buffer_length -= 4;

	if (*buffer_length < 0)
		return false;

	*buffer++ = isc_info_rsb_begin;

	// dump out the alias or relation name if it exists.

	Relation* relation = rsb->rsb_relation;
	USHORT length = 0;
	const SCHAR* name = NULL;
	Procedure* procedure;

	const str* alias = rsb->rsb_alias;

	if (alias)
		{
		length = alias->str_length;
		name = (char*) alias->str_data;
		}
	else if (relation) 
		{
		length = strlen(relation->rel_name);
		name = relation->rel_name;
		}

	if (name) 
		{
		*buffer_length -= 2 + length;

		if (*buffer_length < 0)
			return false;

		*buffer++ = isc_info_rsb_relation;
		*buffer++ = (SCHAR) length;

		while (length--)
			*buffer++ = (UCHAR)*name++;
		}

	// print out the type followed immediately by any
	//   type-specific data 
	   
	USHORT return_length;
	*buffer++ = isc_info_rsb_type;
	
	switch (rsb->rsb_type) 
		{
		case rsb_indexed:
			*buffer++ = isc_info_rsb_indexed;
			if (!dump_index(tdbb, ((RsbIndexed*) rsb)->inversion, &buffer, buffer_length)) 
				return false;
			break;

		case rsb_navigate:
			*buffer++ = isc_info_rsb_navigate;
			
			//if (!dump_index(tdbb, (jrd_nod*) rsb->rsb_arg[RSB_NAV_index], &buffer, buffer_length)) 
			if (!dump_index(tdbb, ((RsbNavigate*) rsb)->retrievalInversion, &buffer, buffer_length)) 
				return false;

			// dimitr:	here we report indicies used to limit
			//			the navigational-based retrieval
			
			//if (rsb->rsb_arg[RSB_NAV_inversion]) 
			if (((RsbNavigate*) rsb)->inversion) 
				{
				*buffer_length -= 2;
				
				if (*buffer_length < 0) 
					return false;

				*buffer++ = isc_info_rsb_type;
				*buffer++ = isc_info_rsb_indexed;
				
				if (!dump_index(tdbb, ((RsbNavigate*) rsb)->inversion, &buffer, buffer_length))
					return false;
				}
			break;

		case rsb_sequential:
			*buffer++ = isc_info_rsb_sequential;
			break;

		case rsb_cross:
			*buffer++ = isc_info_rsb_cross;
			break;

		case rsb_sort:
			*buffer++ = isc_info_rsb_sort;
			break;

		case rsb_procedure:
			*buffer++ = isc_info_rsb_procedure;

			// don't try to print out plans of procedures called by procedures, since 
			// we could get into a recursive situation; if the customer wants to know 
			// the plan produced by the sub-procedure, they can invoke it directly.

			if (request->req_procedure) 
				break;
			

			procedure = ((RsbProcedure*) rsb)->procedure;

			if (!procedure || !procedure->hasRequest()) 
				return false;

			// CVC: This is becoming trickier. There are procedures that don't have a plan
			// because they don't access tables. In this case, the engine gives up and swallows
			// the whole plan. Not acceptable.

			if (!procedure->findRequest()->req_fors.getCount()) 
				{
				length = procedure->findName().length();
				*buffer_length -= (6 + length);

				if (*buffer_length < 0) 
					return false;
				
				*buffer++ = isc_info_rsb_begin;
				*buffer++ = isc_info_rsb_relation;
				*buffer++ = (SCHAR) length;
				strncpy ((char *)buffer, (const TEXT *)procedure->findName(), length);
				buffer += length;
				*buffer++ = isc_info_rsb_type;
				*buffer++ = isc_info_rsb_sequential;
				*buffer++ = isc_info_rsb_end;
				break;
				}

			if (!OPT_access_path(tdbb, procedure->findRequest(), buffer, 
								*buffer_length, &return_length))
				return false;

			*buffer_length -= return_length;

			if (*buffer_length < 0)
				return false;

			buffer += return_length;
			break;

		case rsb_first:
			*buffer++ = isc_info_rsb_first;
			break;

		case rsb_skip:
			*buffer++ = isc_info_rsb_skip;
			break;
	        
		case rsb_boolean:
			*buffer++ = isc_info_rsb_boolean;
			break;

		case rsb_union:
			*buffer++ = isc_info_rsb_union;
			break;

		case rsb_aggregate:
			*buffer++ = isc_info_rsb_aggregate;
			break;

		case rsb_merge:
			*buffer++ = isc_info_rsb_merge;
			break;

		case rsb_ext_sequential:
			*buffer++ = isc_info_rsb_ext_sequential;
			break;

		case rsb_ext_indexed:
			*buffer++ = isc_info_rsb_ext_indexed;
			break;

		case rsb_ext_dbkey:
			*buffer++ = isc_info_rsb_ext_dbkey;
			break;

		case rsb_left_cross:
			*buffer++ = isc_info_rsb_left_cross;
			break;

		default:
			*buffer++ = isc_info_rsb_unknown;
			break;
		}

	// dump out any sub-rsbs; for join-type rses like cross
	// and merge, dump out the count of streams first, then
	// loop through the substreams and dump them out.

	if (--(*buffer_length) < 0) 
		return false;

	//const RecordSource* const* ptr;
	//const RecordSource* const* end;

	switch (rsb->rsb_type) 
		{
		case rsb_cross:
			*buffer++ = (UCHAR) rsb->rsb_count;
			/***
			ptr = rsb->rsb_arg;
			for (end = ptr + rsb->rsb_count; ptr < end; ptr++) 
				if (!dump_rsb(tdbb, request, *ptr, &buffer, buffer_length)) 
					return false;
			***/
			for (int n = 0; n < rsb->rsb_count; ++n)
				if (!dump_rsb(tdbb, request, ((RsbCross*) rsb)->rsbs[n], &buffer, buffer_length))
					return false;
			break;

		case rsb_union:
			//*buffer++ = rsb->rsb_count / 2;
			*buffer++ = (UCHAR) rsb->rsb_count;
			/***
			ptr = rsb->rsb_arg;
			for (end = ptr + rsb->rsb_count; ptr < end; ptr++) 
				{
				if (!dump_rsb(tdbb, request, *ptr, &buffer, buffer_length)) 
					return false;

				ptr++;
				}
			***/
			for (int n = 0; n < rsb->rsb_count; ++n)
				if (!dump_rsb(tdbb, request, ((RsbUnion*) rsb)->rsbs[n], &buffer, buffer_length))
					return false;
			break;

		case rsb_merge:
			*buffer++ = (UCHAR) rsb->rsb_count;
			/***
			ptr = rsb->rsb_arg;
			for (end = ptr + rsb->rsb_count * 2; ptr < end; ptr += 2)
				if (!dump_rsb(tdbb, request, *ptr, &buffer, buffer_length)) 
					return false;
			***/
			for (int n = 0; n < rsb->rsb_count; ++n)
				if (!dump_rsb(tdbb, request, ((RsbMerge*) rsb)->sortRsbs[n], &buffer, buffer_length))
			break;

		case rsb_left_cross:
			*buffer++ = 2;
			
			if (!dump_rsb(tdbb, request, ((RsbLeftCross*) rsb)->outerRsb, &buffer, buffer_length))
				return false;

			if (!dump_rsb(tdbb, request, ((RsbLeftCross*) rsb)->innerRsb, &buffer, buffer_length)) 
				return false;

			break;
	  
		default:    // Shut up compiler warnings.
			break;
		}

	// dump out the next rsb.

	if (rsb->rsb_next) 
		if (!dump_rsb(tdbb, request, rsb->rsb_next, &buffer, buffer_length)) 
			return false;

	*buffer++ = isc_info_rsb_end;
	*buffer_ptr = buffer;

	return true;
}
#endif // OBSOLETE

static bool estimate_cost(thread_db* tdbb,
						  OptimizerBlk* opt,
						  USHORT stream,
						  double *cost, 
						  double *resulting_cardinality)
{
/**************************************
 *
 *	e s t i m a t e _ c o s t
 *
 **************************************
 *
 * Functional description
 *	Make an estimate of the cost to fetch a stream.  The cost
 *	is a function of estimated cardinality of the relation, index
 *	selectivity, and total boolean selectivity.  Since none of
 *	this information is available, the estimates are likely to
 *	be a bit weak.  Return true if the relation is index 
 *	retrievable.
 *
 **************************************/
	DEV_BLKCHK(opt, type_opt);
	SET_TDBB(tdbb);

	CompilerScratch* csb = opt->opt_csb;
	CompilerScratch::csb_repeat* csb_tail = &csb->csb_rpt[stream];
	csb_tail->csb_flags |= csb_active;
	double cardinality = MAX(csb_tail->csb_cardinality, 10);
	double index_selectivity = 1.0;
	USHORT indexes = 0, equalities = 0, inequalities = 0, index_hits = 0;
	bool unique = false;
	ULONG inactivities[OPT_STREAM_BITS];
	get_inactivities(csb, inactivities);

	// Compute index selectivity.  This involves finding the indices
	// to be utilized and making a crude guess of selectivities.

	if (opt->opt_conjuncts.getCount()) 
		{
		const index_desc* idx = csb_tail->csb_idx->items;
		for (USHORT i = 0; i < csb_tail->csb_indices; i++) 
			{
			SSHORT n = 0;
			clear_bounds(opt, idx);
			const OptimizerBlk::opt_conjunct* const opt_end = opt->opt_conjuncts.end();
			const OptimizerBlk::opt_conjunct* tail = opt->opt_conjuncts.begin();

			for (; tail < opt_end; tail++)
				{
				jrd_nod* node = tail->opt_conjunct_node;

				if (!(tail->opt_conjunct_flags & opt_conjunct_used)
						&& !(TEST_DEP_ARRAYS(tail->opt_dependencies, inactivities)))
					n += match_index(tdbb, opt, stream, node, idx);
				}

			OptimizerBlk::opt_segment* segment = opt->opt_segments;

			if (segment->opt_lower || segment->opt_upper) 
				{
				indexes++;
				USHORT count;

				for (count = 0; count < idx->idx_count; count++, segment++) 
					if (!segment->opt_lower || segment->opt_lower != segment->opt_upper) 
						break;

				double s = idx->idx_selectivity;

				if (s <= 0 || s >= 1)
					s = ESTIMATED_SELECTIVITY;

				if (count == idx->idx_count)
					{
					if (idx->idx_flags & idx_unique) 
						{
						unique = true;
						s = 1 / cardinality;
						}
					}
				else 
					s *= INVERSE_ESTIMATE;

				index_selectivity *= s;
				index_hits += MAX(count, n);
					}
			++idx;
			}
		}

	// We now known the relation cardinality, the combined index selectivity,
	// and the number of index lookups required.  From this we can compute the
	// cost of executing the record selection expression (cost of index lookups
	// plus the number of records fetched).

	if (indexes) 
		*cost = cardinality * index_selectivity + indexes * INDEX_COST;
	else 
		*cost = cardinality;

	// Next, we need to estimate the number of records coming out of the
	// record stream.  This is based on conjunctions without regard to whether
	// or not they were the result of index operations.

	const OptimizerBlk::opt_conjunct* const opt_end =
		opt->opt_conjuncts.end();
	OptimizerBlk::opt_conjunct* tail = opt->opt_conjuncts.begin();

	for (; tail < opt_end; tail++)
		{
		jrd_nod* node = tail->opt_conjunct_node;
		if (!(tail->opt_conjunct_flags & opt_conjunct_used) 
				&& !(TEST_DEP_ARRAYS(tail->opt_dependencies, inactivities))) 
			{
			if (node->nod_type == nod_eql)
				++equalities;
			else 
				++inequalities;

			tail->opt_conjunct_flags |= opt_conjunct_used;
			}
		}

	double selectivity;
	const SSHORT n = inequalities + 3 * (equalities - index_hits);

	if (n > 0) 
		{
		selectivity = 0.3 / n;
		if (selectivity > index_selectivity) 
			selectivity = index_selectivity;
		}
	else 
		selectivity = index_selectivity;

	cardinality *= selectivity;

	if (unique)
		*resulting_cardinality = cardinality;
	else
		*resulting_cardinality = MAX(cardinality, 1.0);

	csb_tail->csb_flags |= csb_active;
	
	// AB: Nice that we return a boolean, but do we ever need it?

	return (indexes != 0);	
}


static bool expression_possible_unknown(const jrd_nod* node)
{
/**************************************
 *
 *      e x p r e s s i o n _ p o s s i b l e _ u n k n o w n
 *
 **************************************
 *
 * Functional description
 *  Check if expression could return NULL
 *  or expression can turn NULL into 
 *  a True/False.
 *
 **************************************/
	DEV_BLKCHK(node, type_nod);

	if (!node) 
		return false;

	switch (node->nod_type) 
		{
		case nod_cast:
			return expression_possible_unknown(node->nod_arg[e_cast_source]);

		case nod_extract:
			return expression_possible_unknown(node->nod_arg[e_extract_value]);

		case nod_field:
		case nod_rec_version:
		case nod_dbkey:
		case nod_argument:
		case nod_current_date:
		case nod_current_role:
		case nod_current_time:
		case nod_current_timestamp:
		case nod_gen_id:
		case nod_gen_id2:
		case nod_internal_info:
		case nod_literal:
		case nod_null:
		case nod_user_name:
		case nod_variable:
			return false;

		case nod_or:
		case nod_and:

		case nod_add:
		case nod_add2:
		case nod_concatenate:
		case nod_divide:
		case nod_divide2:
		case nod_multiply:
		case nod_multiply2:
		case nod_negate:
		case nod_subtract:
		case nod_subtract2:

		case nod_upcase:
		case nod_substr:

		case nod_like:
		case nod_between:
		case nod_contains:
		case nod_starts:
		case nod_eql:
		case nod_neq:
		case nod_geq:
		case nod_gtr:
		case nod_lss:
		case nod_leq:
			{
			// Check all sub-nodes of this node.

			const jrd_nod* const* ptr = node->nod_arg;
			const jrd_nod* const* const end = ptr + node->nod_count;

			for (; ptr < end; ptr++)
				if (expression_possible_unknown(*ptr)) 
					return true;

			return false;
			}

		default :
			return true;
		}
}


static bool expression_contains_stream(CompilerScratch* csb,
									   const jrd_nod* node, 
									   UCHAR stream, 
									   bool* otherActiveStreamFound)
{
/**************************************
 *
 *      e x p r e s s i o n _ c o n t a i n s _ s t r e a m
 *
 **************************************
 *
 * Functional description
 *  Search if somewhere in the expression the given stream 
 *  is used. If a unknown node is found it will return true.
 *
 **************************************/
	DEV_BLKCHK(node, type_nod);

	if (!node) 
		return false;

	RecordSelExpr* rse = NULL;

	USHORT n;
	switch (node->nod_type) 
		{
		case nod_field:
			n = (USHORT)(long) node->nod_arg[e_fld_stream];

			if (otherActiveStreamFound && (n != stream)) 
				if (csb->csb_rpt[n].csb_flags & csb_active) 
					*otherActiveStreamFound = true;

			return ((USHORT)(long) node->nod_arg[e_fld_stream] == stream);

		case nod_rec_version:
		case nod_dbkey:
			n = (USHORT)(long) node->nod_arg[0];

			if (otherActiveStreamFound && (n != stream)) 
				if (csb->csb_rpt[n].csb_flags & csb_active)
					*otherActiveStreamFound = true;

			return ((USHORT)(long) node->nod_arg[0] == stream);

		case nod_cast:
			return expression_contains_stream(csb, 
				node->nod_arg[e_cast_source], stream, otherActiveStreamFound);

		case nod_extract:
			return expression_contains_stream(csb,
				node->nod_arg[e_extract_value], stream, otherActiveStreamFound);

		case nod_function:
			return expression_contains_stream(csb,
				node->nod_arg[e_fun_args], stream, otherActiveStreamFound);

		case nod_procedure:
			return expression_contains_stream(csb, node->nod_arg[e_prc_inputs], 
				stream, otherActiveStreamFound);

		case nod_any:
		case nod_unique:
		case nod_ansi_any:
		case nod_ansi_all:
		case nod_exists:
			return expression_contains_stream(csb, node->nod_arg[e_any_rse], 
				stream, otherActiveStreamFound);

		case nod_argument:
		case nod_current_date:
		case nod_current_role:
		case nod_current_time:
		case nod_current_timestamp:
		case nod_gen_id:
		case nod_gen_id2:
		case nod_internal_info:
		case nod_literal:
		case nod_null:
		case nod_user_name:
		case nod_variable:
			return false;

		case nod_rse:
			rse = (RecordSelExpr*) node;
			break;

		case nod_average:
		case nod_count:
		case nod_count2:
		case nod_from:
		case nod_max:
		case nod_min:
		case nod_total:
			{
			const jrd_nod* nodeDefault = node->nod_arg[e_stat_rse];
			bool result = false;

			if (nodeDefault  
					&& expression_contains_stream(csb, nodeDefault, 
						stream, otherActiveStreamFound)) 
				{
				result = true;
				if (!otherActiveStreamFound) 
					return result;
				}

			rse = (RecordSelExpr*) node->nod_arg[e_stat_rse];
			const jrd_nod* value = node->nod_arg[e_stat_value];

			if (value && expression_contains_stream(csb, value, 
									stream, otherActiveStreamFound)) 
				{
				result = true;

				if (!otherActiveStreamFound)
					return result;
				}

			return result;
			}
			break;

		// go into the node arguments

		case nod_add:
		case nod_add2:
		case nod_agg_average:
		case nod_agg_average2:
		case nod_agg_average_distinct:
		case nod_agg_average_distinct2:
		case nod_agg_max:
		case nod_agg_min:
		case nod_agg_total:
		case nod_agg_total2:
		case nod_agg_total_distinct:
		case nod_agg_total_distinct2:
		case nod_concatenate:
		case nod_divide:
		case nod_divide2:
		case nod_multiply:
		case nod_multiply2:
		case nod_negate:
		case nod_subtract:
		case nod_subtract2:

		case nod_upcase:
		case nod_substr:

		case nod_like:
		case nod_between:
		case nod_sleuth:
		case nod_missing:
		case nod_value_if:
		case nod_matches:
		case nod_contains:
		case nod_starts:
		case nod_equiv:
		case nod_eql:
		case nod_neq:
		case nod_geq:
		case nod_gtr:
		case nod_lss:
		case nod_leq:
			{
			// Check all sub-nodes of this node.
			const jrd_nod* const* ptr = node->nod_arg;
			bool result = false;
			const jrd_nod* const* const end = ptr + node->nod_count;

			for (; ptr < end; ptr++)
				{
				if (expression_contains_stream(csb, *ptr, 
					stream, otherActiveStreamFound)) 
					{
					result = true;
					if (!otherActiveStreamFound)
						return result;
					}
				}

			return result;
			}

		default :
			return true;
		}

	if (rse) 
		{
		jrd_nod* sub;

		if ((sub = rse->rse_first) &&  expression_contains_stream(csb, sub, 
											stream, otherActiveStreamFound)) 
			return true;

		if ((sub = rse->rse_skip) && expression_contains_stream(csb, sub, stream, 
											otherActiveStreamFound)) 
			return true;

		if ((sub = rse->rse_boolean) && expression_contains_stream(csb, sub, 
											stream, otherActiveStreamFound)) 
			return true;

		if ((sub = rse->rse_sorted) && expression_contains_stream(csb, sub, stream, 
											otherActiveStreamFound)) 
			return true;

		if ((sub = rse->rse_projection) && expression_contains_stream(csb, sub, 
											stream, otherActiveStreamFound)) 
			return true;
		}

	return false;
}


static void find_best(thread_db* tdbb,
					  OptimizerBlk* opt,
					  USHORT stream,
					  USHORT position,
					  const UCHAR* streams,
					  const jrd_nod* plan_node,
					  double cost,
					  double cardinality)
{
/**************************************
 *
 *	f i n d _ b e s t
 *
 **************************************
 *
 * Functional description
 *	Find the best join from the passed "stream" to 
 *	the remaining "streams" in the rse.  This routine
 *	uses recursion to successively consider all 
 *	possible join orders which use indexed 
 *	relationships to form joins.
 *
 **************************************/
	SET_TDBB(tdbb);
	DEV_BLKCHK(opt, type_opt);
	DEV_BLKCHK(plan_node, type_nod);
#ifdef OPT_DEBUG
	// this is used only in development so is not in the message file.
	if (opt_debug_flag >= DEBUG_PUNT) {
		ERR_post(isc_random, isc_arg_string, "punt", 0);
	}
#endif
	// if a plan was specified, check that this order matches the order
	// that the user provided; this may seem like an ass-backwards way to 
	// enforce ordering, but I think it is important to follow the same
	// code path for SET PLAN as for a normal optimization--it reduces 
	// chances for bugs to be introduced, and forces the person maintaining
	// the optimizer to think about SET PLAN when new features are added --deej

	if (plan_node && (streams[position + 1] != stream))
		return;

	// do some initializations.

	CompilerScratch* csb = opt->opt_csb;
	csb->csb_rpt[stream].csb_flags |= csb_active;
	const UCHAR* stream_end = &streams[1] + streams[0];
	opt->opt_streams[position].opt_stream_number = stream;
	++position;
	const OptimizerBlk::opt_stream* order_end = opt->opt_streams.begin() + position;
	OptimizerBlk::opt_stream* stream_data = opt->opt_streams.begin() + stream;

	// Save the various flag bits from the optimizer block to reset its
	// state after each test.

	firebird::HalfStaticArray<UCHAR, OPT_STATIC_ITEMS> 
		stream_flags(*tdbb->tdbb_default), conjunct_flags(*tdbb->tdbb_default);
	stream_flags.grow(csb->csb_n_stream);
	conjunct_flags.grow(opt->opt_conjuncts.getCount());
	size_t i;

	for (i = 0; i < stream_flags.getCount(); i++)
		stream_flags[i] = opt->opt_streams[i].opt_stream_flags & opt_stream_used;

	for (i = 0; i < conjunct_flags.getCount(); i++)
		conjunct_flags[i] = opt->opt_conjuncts[i].opt_conjunct_flags & opt_conjunct_used;

	// Compute delta and total estimate cost to fetch this stream.

	double position_cost, position_cardinality, new_cost = 0, new_cardinality = 0;

	if (!plan_node) 
		{
		estimate_cost(tdbb, opt, stream, &position_cost,
					  &position_cardinality);
		new_cost = cost + cardinality * position_cost;
		new_cardinality = position_cardinality * cardinality;
		}

	++opt->opt_combinations;

	// If the partial order is either longer than any previous partial order,
	// or the same length and cheap, save order as "best".

	if (position > opt->opt_best_count 
			|| (position == opt->opt_best_count && new_cost < opt->opt_best_cost)) 
		{
		opt->opt_best_count = position;
		opt->opt_best_cost = new_cost;
		OptimizerBlk::opt_stream* tail = opt->opt_streams.begin();

		for (; tail < order_end; tail++) 
			tail->opt_best_stream = tail->opt_stream_number;

#ifdef OPT_DEBUG
		if (opt_debug_flag >= DEBUG_CANDIDATE)
			print_order(opt, position, new_cardinality, new_cost);

		}
	else 
		{
		if (opt_debug_flag >= DEBUG_ALL) 
			print_order(opt, position, new_cardinality, new_cost);

#endif
		}
	// mark this stream as "used" in the sense that it is already included 
	// in this particular proposed stream ordering.

	stream_data->opt_stream_flags |= opt_stream_used;
	bool done = false;

	// if we've used up all the streams there's no reason to go any further.

	if (position == streams[0]) 
		done = true;

	// We need to prune the combinations to avoid spending all of our time 
	// recursing through find_best().  Based on experimentation, the cost of 
	// recursion becomes significant at about a 7 table join.  Therefore, 
	// make a simplifying assumption that if we have already seen a join 
	// ordering that is lower cost than this one, give up.

	if (!done && position > 4) 
		{
		OptimizerBlk::opt_stream* tail = &opt->opt_streams[position];
		
		// If we are the new low-cost join ordering, record that 
		//   fact.  Otherwise, give up. 

		if (tail->opt_best_stream_cost == 0 
				|| new_cost < tail->opt_best_stream_cost)
			tail->opt_best_stream_cost = new_cost;
		else 
			if (!plan_node) 
				done = true;
		}

	// First, handle any streams that have direct unique indexed 
	// relationships to this stream.  If there are any, we 
	// won't consider (now) indirect relationships.

	if (!done) 
		{
		IndexedRelationship* relationship = stream_data->opt_relationships;

		for (; relationship; relationship = relationship->irl_next) 
			{
			if (relationship->irl_unique 
					&& (!(opt->opt_streams[relationship->irl_stream].opt_stream_flags & opt_stream_used))) 
				{
				for (const UCHAR* ptr = streams + 1; ptr < stream_end; ptr++) 
					{
					if (*ptr == relationship->irl_stream) 
						{
						if (!plan_node)
							done = true;

						find_best(tdbb, opt, relationship->irl_stream,
							position, streams, plan_node, new_cost, new_cardinality);
						break;
						}
					}
				}
			}
		}

	// Next, handle any streams that have direct indexed relationships to this
	// stream.  If there are any, we won't consider (now) indirect relationships

	if (!done) 
		{
		IndexedRelationship* relationship = stream_data->opt_relationships;
		for (; relationship; relationship = relationship->irl_next)
			{
			if (!(opt->opt_streams[relationship->irl_stream].opt_stream_flags & opt_stream_used)) 
				{
				for (const UCHAR* ptr = streams + 1; ptr < stream_end; ptr++) 
					{
					if (*ptr == relationship->irl_stream) 
						{
						if (!plan_node) 
							done = true;

						find_best(tdbb, opt, relationship->irl_stream,
								  position, streams, plan_node, new_cost,
								  new_cardinality);
						break;
						}
					}
				}
			}
		}

	// If there were no direct relationships, look for indirect relationships

	if (!done) 
		{
		for (const UCHAR* ptr = streams + 1; ptr < stream_end; ptr++) 
			{
			if (!(opt->opt_streams[*ptr].opt_stream_flags & opt_stream_used) 
					&& check_relationship(opt, position, *ptr))
				{
				find_best(tdbb, opt, *ptr, position,
						  streams, plan_node, new_cost, new_cardinality);
				}
			}
		}

	// Clean up from any changes made for compute the cost for this stream
	csb->csb_rpt[stream].csb_flags &= ~csb_active;

	for (i = 0; i < stream_flags.getCount(); i++)
		opt->opt_streams[i].opt_stream_flags &= stream_flags[i];

	for (i = 0; i < conjunct_flags.getCount(); i++)
		opt->opt_conjuncts[i].opt_conjunct_flags &= conjunct_flags[i];
}


static void find_index_relationship_streams(thread_db* tdbb, 
											OptimizerBlk* opt, 
											const UCHAR* streams,
											UCHAR* dependent_streams, 
											UCHAR* free_streams)
{
/**************************************
 *
 *	f i n d _ i n d e x _r e l a t i o n s h i p _ s t r e a m s 
 *
 **************************************
 *
 * Functional description
 *	Find the streams that can use a index 
 *	with the currently active streams.
 *
 **************************************/

	DEV_BLKCHK(opt, type_opt);
	SET_TDBB(tdbb);
	Database* dbb = tdbb->tdbb_database;

	CompilerScratch* csb = opt->opt_csb;
	const UCHAR* end_stream = streams + 1 + streams[0];
	const UCHAR* stream = streams + 1;

	for (; stream < end_stream; stream++) 
		{
		CompilerScratch::csb_repeat* csb_tail = &csb->csb_rpt[*stream];

		// Set temporary active flag for this stream

		csb_tail->csb_flags |= csb_active;

		bool indexed_relationship = false;

		if (opt->opt_conjuncts.getCount()) 
			{
			if (dbb->dbb_ods_version >= ODS_VERSION11) 
				{
				// Calculate the inversion for this stream. 
				// The returning candidate contains the streams that will be used for 
				// index retrieval. This meant that if some stream is used this stream
				// depends on already active streams and can not be used in a separate
				// SORT/MERGE.

				InversionCandidate* candidate = NULL;
				OptimizerRetrieval* optimizerRetrieval = FB_NEW(*tdbb->tdbb_default) 
					OptimizerRetrieval(tdbb, *tdbb->tdbb_default, opt, *stream, false, false, NULL);
				candidate = optimizerRetrieval->getCost();

				if (candidate->dependentFromStreams.getCount() >= 1) 
					indexed_relationship = true;

				delete candidate;
				delete optimizerRetrieval;
				}
			else 
				{
				const index_desc* idx = csb_tail->csb_idx->items;

				// Walk through all indexes from this relation

				for (USHORT i = 0; i < csb_tail->csb_indices; i++, idx++) 
					{
					// Ignore index when not specified in explicit PLAN

					if (idx->idx_runtime_flags & idx_plan_dont_use) 
						continue;

					clear_bounds(opt, idx);

					const OptimizerBlk::opt_conjunct* const opt_end =
						opt->opt_conjuncts.end();
					const OptimizerBlk::opt_conjunct* tail = opt->opt_conjuncts.begin();

					// Walk through all conjunctions

					for (; tail < opt_end; tail++)
						{
						jrd_nod* node = tail->opt_conjunct_node;

						// Try to match conjunction against index

						bool activeStreamFound = false;

						if (!(tail->opt_conjunct_flags & opt_conjunct_used) 
								&& expression_contains_stream(csb, node, *stream, 
													&activeStreamFound))
							{
							if (activeStreamFound) 
								match_index(tdbb, opt, *stream, node, idx);
							}
						}

					// If first segment could be matched we're able to use a 
					// index that is dependent on the already active streams.

					OptimizerBlk::opt_segment* segment = opt->opt_segments;

					if (segment->opt_lower || segment->opt_upper) 
						{
						indexed_relationship = true;
						break;
						}
					}
				}
			}

		if (indexed_relationship) 
			dependent_streams[++dependent_streams[0]] = *stream;
		else 
			free_streams[++free_streams[0]] = *stream;

		// Reset active flag

		csb_tail->csb_flags &= ~csb_active;
		}
}


static jrd_nod* find_dbkey(jrd_nod* dbkey, 
						   USHORT stream, 
						   SLONG* position)
{
/**************************************
 *
 *	f i n d _ d b k e y
 *
 **************************************
 *
 * Functional description
 *	Search a dbkey (possibly a concatenated one) for
 *	a dbkey for specified stream.
 *
 **************************************/
	DEV_BLKCHK(dbkey, type_nod);

	if (dbkey->nod_type == nod_dbkey) 
		{
		if ((USHORT)(long) dbkey->nod_arg[0] == stream)
			return dbkey;
		else 
			{
			*position = *position + 1;
			return NULL;
			}
		}
	else if (dbkey->nod_type == nod_concatenate) 
		{
        jrd_nod** ptr = dbkey->nod_arg;
		const jrd_nod* const* const end = ptr + dbkey->nod_count;

		for (; ptr < end; ptr++)
			{
			jrd_nod* dbkey_temp = find_dbkey(*ptr, stream, position);
			if (dbkey_temp)
				return dbkey_temp;
			}
		}

	return NULL;
}


static USHORT find_order(thread_db* tdbb,
						 OptimizerBlk* opt,
						 const UCHAR* streams, 
						 const jrd_nod* plan_node)
{
/**************************************
 *
 *	f i n d _ o r d e r
 *
 **************************************
 *
 * Functional description
 *	Given a set of streams, select the "best order" to join them.
 *	The "best order" is defined as longest, cheapest join order
 *	(length, of course, takes precedence over cost).  The best
 *	order is developed and returned in the optimization block.
 *
 **************************************/
	SET_TDBB(tdbb);
	DEV_BLKCHK(opt, type_opt);
	DEV_BLKCHK(plan_node, type_nod);
	opt->opt_best_count = 0;

	// if a plan was specified, the order is already 
	// present in the streams vector, so we only want
	// to try one order.

	const UCHAR* stream_end;

	if (plan_node)
		stream_end = &streams[1] + 1;
	else
		stream_end = &streams[1] + streams[0];

	// Consider each stream as the leftmost stream in the join order;
	// for each stream, the best order from that stream is considered,
	// and the one which is best is placed into the opt block.  Thus
	// at the end of this loop the opt block holds the best order.

	const UCHAR* stream = streams + 1;

	for (; stream < stream_end; stream++)
		find_best(tdbb, opt, *stream, 0, streams, plan_node, 
				(double) 0, (double) 1);

#ifdef OPT_DEBUG
	if (opt_debug_flag >= DEBUG_BEST)
		{
		const OptimizerBlk::opt_stream* const order_end =
			opt->opt_streams.begin() + opt->opt_best_count;
		fprintf(opt_debug_file,
				   "find_order()  -- best_count: %2.2d, best_streams: ",
				   opt->opt_best_count); 
		const OptimizerBlk::opt_stream* tail = opt->opt_streams.begin();

		for (; tail < order_end; tail++)
			fprintf(opt_debug_file, "%2.2d ", tail->opt_best_stream);

		fprintf(opt_debug_file,
				   "\n\t\t\tbest_cost: %g\tcombinations: %ld\n",
				   opt->opt_best_cost, opt->opt_combinations);
		}
#endif

	return opt->opt_best_count;
}


static void find_used_streams(const RecordSource* rsb, 
							  UCHAR* streams)
{
/**************************************
 *
 *	f i n d _ u s e d _ s t r e a m s
 *
 **************************************
 *
 * Functional description
 *	Find all streams through the given rsb
 *	and add them to the stream list.
 *
 **************************************/
 
	if (!(rsb)) 
		return;

	//const RecordSource* const* ptr;
	//const RecordSource* const* end;
	USHORT stream;
	bool found = false;

	switch (rsb->rsb_type) 
		{
		case rsb_aggregate:
		case rsb_ext_indexed:
		case rsb_ext_sequential:
		case rsb_indexed:
		case rsb_navigate:
		case rsb_procedure:
		case rsb_sequential:
		case rsb_union:
			stream = rsb->rsb_stream;
			found = true;
			break;

		case rsb_cross:
			{
			for (RecordSource **ptr = ((RsbCross*) rsb)->rsbs, **end = ptr + rsb->rsb_count; ptr < end; ptr++) 
				find_used_streams(*ptr, streams);
			}
			break;

		case rsb_merge:
			{
			//for (ptr = rsb->rsb_arg, end = ptr + rsb->rsb_count * 2; ptr < end;	ptr += 2) 
			for (RsbSort **ptr = ((RsbMerge*) rsb)->sortRsbs, **end = ptr + rsb->rsb_count; ptr < end; ptr++) 
				find_used_streams(*ptr, streams);
			}
			break;

		case rsb_left_cross:
			find_used_streams(((RsbLeftCross*) rsb)->innerRsb, streams);
			find_used_streams(((RsbLeftCross*) rsb)->outerRsb, streams);
			break;

        default:	// Shut up compiler warnings.
			break;
		}

	if (rsb->rsb_next)
		find_used_streams(rsb->rsb_next, streams);

	if (found) 
		{
		found = false;
		
		for (USHORT i = 1; i <= streams[0]; i++) 
			if (stream == streams[i])
				{
				found = true;
				break;
				}
			
		if (!found) 
			streams[++streams[0]] = stream;
		}
}


static void form_rivers(thread_db* tdbb,
						OptimizerBlk* opt,
						const UCHAR* streams,
						RiverStack& river_stack,
						jrd_nod** sort_clause,
						jrd_nod** project_clause,
						jrd_nod* plan_clause)
{
/**************************************
 *
 *	f o r m _ r i v e r s
 *
 **************************************
 *
 * Functional description
 *	Form streams into rivers according
 *	to the user-specified plan.
 *
 **************************************/
	SET_TDBB(tdbb);
	/* the DEV_BLKCHK macro generates compiler warnings
	DEV_BLKCHK(opt, type_opt);

	if (sort_clause) 
		DEV_BLKCHK(*sort_clause, type_nod);

	if (project_clause) 
		DEV_BLKCHK(*project_clause, type_nod);

	DEV_BLKCHK(plan_clause, type_nod);
	*/

	stream_array_t temp;
	temp[0] = 0;
	USHORT count = plan_clause->nod_count;

	// this must be a join or a merge node, so go through
	// the substreams and place them into the temp vector
	// for formation into a river.

	jrd_nod* plan_node = 0;
	jrd_nod** ptr = plan_clause->nod_arg;
	const jrd_nod* const* const end = ptr + count;

	for (; ptr < end; ptr++) 
		{
		plan_node = *ptr;

		if (plan_node->nod_type == nod_merge || plan_node->nod_type == nod_join) 
			{
			form_rivers(tdbb, opt, streams, river_stack, sort_clause,
				project_clause, plan_node);
			continue;
			}

		// at this point we must have a retrieval node, so put
		// the stream into the river.

		fb_assert(plan_node->nod_type == nod_retrieve);
		const jrd_nod* relation_node = plan_node->nod_arg[e_retrieve_relation];
		const UCHAR stream = (UCHAR)(long) relation_node->nod_arg[e_rel_stream];
		// dimitr:	the plan may contain more retrievals than the "streams"
		//			array (some streams could already be joined to the active
		//			rivers), so we populate the "temp" array only with the
		//			streams that appear in both the plan and the "streams"
		//			array.
		const UCHAR* ptr_stream = streams + 1;
		const UCHAR* const end_stream = ptr_stream + streams[0];
		while (ptr_stream < end_stream) 
			{
			if (*ptr_stream++ == stream) 
				{
				temp[0]++;
				temp[temp[0]] = stream;
				break;
				}
			}
		}

	// just because the user specified a join does not mean that 
	// we are able to form a river;  thus form as many rivers out
	// of the join are as necessary to exhaust the streams.
	// AB: Only form rivers when any retrieval node is seen, for
	// example a MERGE on two JOINs will come with no retrievals
	// at this point.
	// CVC: Notice "plan_node" is pointing to the last element in the loop above.
	// If the loop didn't execute, we had garbage in "plan_node".

	if (temp[0] != 0) 
		{
		OptimizerInnerJoin* innerJoin = NULL;

		Database* dbb = tdbb->tdbb_database;
		if (dbb->dbb_ods_version >= ODS_VERSION11) 
			{
			// For ODS11 and higher databases we can use new calculations
			innerJoin = FB_NEW(*tdbb->tdbb_default) 
				OptimizerInnerJoin(tdbb, *tdbb->tdbb_default, opt, temp, river_stack, 
					sort_clause, project_clause, plan_clause);
			}

		do 
			{
			count = innerJoin ?
				innerJoin->findJoinOrder() :
				find_order(tdbb, opt, temp, plan_node);
			} 
		while 
			(form_river(tdbb, opt, count, streams, temp, river_stack, 
							sort_clause, project_clause, 0));

		delete innerJoin;
		}
}


static bool form_river(thread_db* tdbb,
					   OptimizerBlk* opt,
					   USHORT count,
					   const UCHAR* streams,
					   UCHAR* temp,
					   RiverStack& river_stack,
					   jrd_nod** sort_clause,
					   jrd_nod** project_clause, 
					   jrd_nod* plan_clause)
{
/**************************************
 *
 *	f o r m _ r i v e r
 *
 **************************************
 *
 * Functional description
 *	Form streams into rivers (combinations of streams). 
 *
 **************************************/
	CompilerScratch* csb = opt->opt_csb;

	// Allocate a river block and move the best order into it.
	
	River* river = FB_NEW_RPT(*tdbb->tdbb_default, count) River();
	river_stack.push(river);
	river->riv_count = (UCHAR) count;

	RsbCross *rsb;
	//RecordSource** ptr;

	if (count == 1) 
		rsb = NULL;
		//ptr = &river->riv_rsb;
	else 
		{
		//river->riv_rsb = rsb = FB_NEW_RPT(*tdbb->tdbb_default, count) RecordSource(csb);
		rsb = new (tdbb->tdbb_default) RsbCross(csb, count);
		river->riv_rsb = rsb;
		//rsb->rsb_type = rsb_cross;
		//rsb->rsb_count = count;
		rsb->rsb_impure = CMP_impure(csb, sizeof(struct irsb));
		//ptr = rsb->rsb_arg;
		}

	UCHAR* stream = river->riv_streams;
	const OptimizerBlk::opt_stream* const opt_end = opt->opt_streams.begin() + count;
	
	if (count != streams[0]) 
		sort_clause = project_clause = NULL;

	Database* dbb = tdbb->tdbb_database;
	const bool ods11orHigher = (dbb->dbb_ods_version >= ODS_VERSION11);
	OptimizerBlk::opt_stream* tail;
	int n = 0;
	
	for (tail = opt->opt_streams.begin(); tail < opt_end; tail++, stream++, n++) 
		{
		*stream = (UCHAR) tail->opt_best_stream;
		RecordSource* subRsb = NULL;

		if (ods11orHigher)
			{
			subRsb = genRetrievalRsb(tdbb, opt, *stream, sort_clause, 
				project_clause, false, false, NULL);

			subRsb = genInnerBooleanRsb(tdbb, opt, subRsb, *stream);
			}
		else
			subRsb = gen_retrieval(tdbb, opt, *stream, sort_clause, 
				project_clause, false, false, NULL);

		if (rsb)
			rsb->rsbs[n] = subRsb;
		else
			river->riv_rsb = subRsb;
			
		sort_clause = project_clause = NULL;
		}

	// determine whether the rsb we just made should be marked as a projection.
	
	if (rsb && rsb->rsbs[0] && (rsb->rsbs[0]->rsb_flags & rsb_project))
		rsb->rsb_flags |= rsb_project;

	set_made_river(opt, river);
	set_inactive(opt, river);

	// Reform "temp" from streams not consumed.
	
	stream = temp + 1;
	const UCHAR* const end_stream = stream + temp[0];
	
	if (!(temp[0] -= count)) 
		return false;

	for (UCHAR* t2 = stream; t2 < end_stream; t2++) 
		{
		for (tail = opt->opt_streams.begin(); tail < opt_end; tail++) 
			if (*t2 == tail->opt_best_stream)
				goto used;
				
		*stream++ = *t2;
		used:;
		}

	return true;
}



static RecordSource* gen_aggregate(thread_db* tdbb, OptimizerBlk* opt, jrd_nod* node, 
									NodeStack* parent_stack, UCHAR shellStream)
{
/**************************************
 *
 *	g e n _ a g g r e g a t e
 *
 **************************************
 *
 * Functional description
 *	Generate an RecordSource* (Record Source Block) for each aggregate operation.
 *	Generate an AggregateSort* (Aggregate Sort Block) for each DISTINCT aggregate.
 *      
 **************************************/

	CompilerScratch* csb = opt->opt_csb;
	RecordSelExpr* rse = (RecordSelExpr*) node->nod_arg[e_agg_rse];
	rse->rse_sorted = node->nod_arg[e_agg_group];
	jrd_nod* map = node->nod_arg[e_agg_map];

	// AB: Try to distribute items from the HAVING CLAUSE to the WHERE CLAUSE.
	// Zip thru stack of booleans looking for fields that belong to shellStream.
	// Those fields are mappings. Mappings that hold a plain field may be used 
	// to distribute. Handle the simple cases only.

	NodeStack deliverStack;
	gen_deliver_unmapped(tdbb, &deliverStack, map, parent_stack, shellStream);

	// try to optimize MAX and MIN to use an index; for now, optimize
	// only the simplest case, although it is probably possible
	// to use an index in more complex situations
	
	jrd_nod** ptr;
	jrd_nod* agg_operator;
	
	if ((map->nod_count == 1) 
			&& (ptr = map->nod_arg) 
			&& (agg_operator = (*ptr)->nod_arg[e_asgn_from]) 
			&& (agg_operator->nod_type == nod_agg_min 
				|| agg_operator->nod_type == nod_agg_max))
		{
		// generate a sort block which the optimizer will try to map to an index 

		jrd_nod* aggregate = PAR_make_node(tdbb, 3);
		aggregate->nod_type = nod_sort;
		aggregate->nod_count = 1;
		aggregate->nod_arg[0] = agg_operator->nod_arg[e_asgn_from];
		
		// in the max case, flag the sort as descending
		
		if (agg_operator->nod_type == nod_agg_max) 
			aggregate->nod_arg[1] = (jrd_nod*) TRUE;

		//  10-Aug-2004. Nickolay Samofatov
		//   Unneeded nulls seem to be skipped somehow. 
		   
		aggregate->nod_arg[2] = (jrd_nod*) rse_nulls_default;
		rse->rse_aggregate = aggregate;
		}

	// allocate and optimize the record source block

	RecordSource *next = OPT_compile(tdbb, csb, rse, &deliverStack, 0);
	//RecordSource* rsb = FB_NEW_RPT(*tdbb->tdbb_default, 1) RecordSource(csb);
	RsbAggregate *rsb = new (tdbb->tdbb_default) RsbAggregate(csb, next, node);
	//rsb->rsb_type = rsb_aggregate;
	fb_assert((int) (long)node->nod_arg[e_agg_stream] <= MAX_STREAMS);
	fb_assert((int) (long)node->nod_arg[e_agg_stream] <= MAX_UCHAR);
	rsb->rsb_stream = (UCHAR) (long) node->nod_arg[e_agg_stream];
	rsb->rsb_format = csb->csb_rpt[rsb->rsb_stream].csb_format;
	//rsb->rsb_next = OPT_compile(tdbb, csb, rse, &deliverStack);
	//rsb->rsb_arg[0] = (RecordSource*) node;
	//rsb->rsb_impure = CMP_impure(csb, sizeof(struct irsb));

	if (rse->rse_aggregate)
		{
		// The rse_aggregate is still set. That means the optimizer
		// was able to match the field to an index, so flag that fact
		// so that it can be handled in EVL_group

		if (agg_operator->nod_type == nod_agg_min)
			agg_operator->nod_type = nod_agg_min_indexed;
		else if (agg_operator->nod_type == nod_agg_max) 
			agg_operator->nod_type = nod_agg_max_indexed;
		}

	// Now generate a separate AggregateSort (Aggregate Sort Block) for each
	// distinct operation;
	// note that this should be optimized to use indices if possible

	DSC      descriptor;
	DSC*     desc = &descriptor;
	ptr = map->nod_arg;
	const jrd_nod* const* const end = ptr + map->nod_count;

	for (; ptr < end; ptr++)
		{
		jrd_nod* from = (*ptr)->nod_arg[e_asgn_from];

		if ((from->nod_type == nod_agg_count_distinct)    
				|| (from->nod_type == nod_agg_total_distinct)    
				|| (from->nod_type == nod_agg_total_distinct2)   
				|| (from->nod_type == nod_agg_average_distinct2) 
				|| (from->nod_type == nod_agg_average_distinct))
			{
			const USHORT count = asb_delta + 1 
					+ (sizeof(SortKeyDef) + sizeof(jrd_nod**) - 1) / sizeof(jrd_nod**);
			AggregateSort* asb = (AggregateSort*) PAR_make_node(tdbb, count);
			asb->nod_type = nod_asb;
			asb->nod_count = 0;

			// build the sort key definition. Turn varying text and
			// cstrings into text 

			CMP_get_desc(tdbb, csb, from->nod_arg[0], desc);

			if (desc->dsc_dtype == dtype_varying) 
				{
				desc->dsc_dtype = dtype_text;
				desc->dsc_length -= sizeof(USHORT);
				}
			else if (desc->dsc_dtype == dtype_cstring) 
				{
				desc->dsc_dtype = dtype_text;
				desc->dsc_length--;
				}

			SortKeyDef* sort_key = asb->asb_key_desc = (SortKeyDef*) asb->asb_key_data;
			sort_key->skd_offset = 0;

			// UCHAR desc->dsc_dtype is always >=0
			// fb_assert(desc->dsc_dtype >= 0)

			fb_assert(desc->dsc_dtype < FB_NELEM(sort_dtypes));
			sort_key->skd_dtype = sort_dtypes[desc->dsc_dtype];

			//	as it is legal to have skd_dtype = 0
			//   I have removed these asserts, to avoid
			//   server restarts in debug mode.
			//   FSG 18.Dec.2000

			// fb_assert (sort_key->skd_dtype != 0); 

			sort_key->skd_length = desc->dsc_length;
			sort_key->skd_flags = SKD_ascending;
			asb->nod_impure = CMP_impure(csb, sizeof(impure_agg_sort));
			asb->asb_desc = *desc;
			from->nod_arg[1] = (jrd_nod*) asb;
			from->nod_count = 2;
			}	
		}

	return rsb;
}


static RecordSource* gen_boolean(thread_db* tdbb, 
								 OptimizerBlk* opt,
								 RecordSource* prior_rsb, 
								 jrd_nod* node)
{
/**************************************
 *
 *	g e n _ b o o l e a n
 *
 **************************************
 *
 * Functional description
 *	Compile and optimize a record selection expression into a
 *	set of record source blocks (rsb's).
 *
 **************************************/
	CompilerScratch* csb = opt->opt_csb;

	RecordSource* rsb = new (tdbb->tdbb_default) RsbBoolean(csb, prior_rsb, node);

	rsb->rsb_impure = CMP_impure(csb, sizeof(struct irsb));
	rsb->rsb_cardinality = prior_rsb->rsb_cardinality;
	
	return rsb;
}


static void gen_deliver_unmapped(thread_db* tdbb, 
								 NodeStack* deliverStack, 
								 jrd_nod* map, 
								 NodeStack* parentStack, 
								 UCHAR shellStream)
{
/**************************************
 *
 *	g e n _ d e l i v e r _ u n m a p p e d
 *
 **************************************
 *
 * Functional description
 *	Make new boolean nodes from nodes that
 *	contain a field from the given shellStream.
 *  Those fields are references (mappings) to 
 *	other nodes and are used by aggregates and
 *	union rse's.
 *
 **************************************/
	DEV_BLKCHK(map, type_nod);
	SET_TDBB(tdbb);

	for (NodeStack::iterator stack1(*parentStack); stack1.hasData(); ++stack1) 
		{
		jrd_nod* boolean = stack1.object();

		// Reduce to simple comparisons

		if (!((boolean->nod_type == nod_eql)
				|| (boolean->nod_type == nod_gtr)
				|| (boolean->nod_type == nod_geq)
				|| (boolean->nod_type == nod_leq) 
				|| (boolean->nod_type == nod_lss)
				|| (boolean->nod_type == nod_starts) 
				|| (boolean->nod_type == nod_missing))) 
			continue;


		// At least 1 mapping should be used in the arguments

		int indexArg;
		bool mappingFound = false;

		for (indexArg = 0; (indexArg < boolean->nod_count) && !mappingFound; indexArg++) 
			{
			jrd_nod* booleanNode = boolean->nod_arg[indexArg];

			if ((booleanNode->nod_type == nod_field)  
					&& ((USHORT)(long) booleanNode->nod_arg[e_fld_stream] == shellStream)) 
				mappingFound = true;
			}

		if (!mappingFound)
			continue;

		// Create new node and assign the correct existing arguments

		jrd_nod* deliverNode = PAR_make_node(tdbb, boolean->nod_count);
		deliverNode->nod_count = boolean->nod_count;
		deliverNode->nod_type = boolean->nod_type;
		deliverNode->nod_flags = boolean->nod_flags;
		bool wrongNode = false;

		for (indexArg = 0; (indexArg < boolean->nod_count) && (!wrongNode); indexArg++)
			{
			jrd_nod* booleanNode = get_unmapped_node(tdbb, boolean->nod_arg[indexArg],
													map, shellStream, true);

			wrongNode = (booleanNode == NULL);

			if (!wrongNode) 
				deliverNode->nod_arg[indexArg] = booleanNode;
			}

		if (wrongNode) 
			delete deliverNode;
		else			
			deliverStack->push(deliverNode);
		}
}


static RecordSource* gen_first(thread_db* tdbb, 
							   OptimizerBlk* opt,
							   RecordSource* prior_rsb, 
							   jrd_nod* node)
{
/**************************************
 *
 *	g e n _ f i r s t
 *
 **************************************
 *
 * Functional description
 *	Compile and optimize a record selection expression into a
 *	set of record source blocks (rsb's).
 *
 *
 *      NOTE: The rsb_first node MUST appear in the rsb list before the
 *          rsb_skip node.  The calling code MUST call gen_first after
 *          gen_skip.
 *
 **************************************/

	CompilerScratch* csb = opt->opt_csb;

	//RecordSource* rsb = FB_NEW_RPT(*tdbb->tdbb_default, 1) RecordSource(opt->opt_csb);

	RsbFirst *rsb = new (tdbb->tdbb_default) RsbFirst(csb, prior_rsb, node);

	//rsb->rsb_count = 1;
	//rsb->rsb_type = rsb_first;
	//rsb->rsb_next = prior_rsb;
	//rsb->rsb_arg[0] = (RecordSource*) node;
	//rsb->rsb_impure = CMP_impure(csb, sizeof(struct irsb_first_n));

	return rsb;
}


static void gen_join(thread_db* tdbb,
					 OptimizerBlk* opt,
					 UCHAR* streams,
					 RiverStack& river_stack,
					 jrd_nod** sort_clause,
					 jrd_nod** project_clause,
					 jrd_nod* plan_clause)
{
/**************************************
 *
 *	g e n _ j o i n
 *
 **************************************
 *
 * Functional description
 *	Find all indexed relationships between streams, 
 *	then form streams into rivers (combinations of 
 * 	streams).    
 *
 **************************************/
	DEV_BLKCHK(opt, type_opt);
	DEV_BLKCHK(*sort_clause, type_nod);
	DEV_BLKCHK(*project_clause, type_nod);
	DEV_BLKCHK(plan_clause, type_nod);

	Database* dbb = tdbb->tdbb_database;
	CompilerScratch* csb = opt->opt_csb;

	if (!streams[0])
		return;

	// For ODS11 and higher databases we can use new calculations
	if (dbb->dbb_ods_version >= ODS_VERSION11) 
		{
		// a plan claus and  more than one stream expects a join/merge
		if (plan_clause && streams[0] > 1) 
			{
			form_rivers(tdbb, opt, streams, river_stack, sort_clause, project_clause, plan_clause);
			return;
			}
			
		OptimizerInnerJoin* innerJoin = FB_NEW(*tdbb->tdbb_default) 
			OptimizerInnerJoin(tdbb, *tdbb->tdbb_default, opt, streams, river_stack, 
			sort_clause, project_clause, plan_clause);

		stream_array_t temp;
		MOVE_FAST(streams, temp, streams[0] + 1);

		USHORT count;
		
		do 
			count = innerJoin->findJoinOrder();
		while (form_river(tdbb, opt, count, streams, temp, river_stack,  sort_clause, project_clause, 0));

		delete innerJoin;
		return;
		}

	// If there is only a single stream, don't bother with a join.
	
	if (streams[0] == 1) 
		{
		// if a nod_cardinality references this stream,
		// compute the cardinality even though we don't
		// need it to optimize retrieval.

		CompilerScratch::csb_repeat* csb_tail = &csb->csb_rpt[streams[1]];
		fb_assert(csb_tail);
		
		if (csb_tail->csb_flags & csb_compute) 
			{
			Relation* relation = csb_tail->csb_relation;
			fb_assert(relation);
			const Format* format = CMP_format(tdbb, csb, streams[1]);
			fb_assert(format);
			csb_tail->csb_cardinality = OPT_getRelationCardinality(tdbb, relation, format);
			}

		River* river = FB_NEW_RPT(*tdbb->tdbb_default, 1) River();
		river->riv_count = 1;

		fb_assert(csb->csb_rpt[streams[1]].csb_relation);

		if (dbb->dbb_ods_version >= ODS_VERSION11) 
			{
			river->riv_rsb =
				genRetrievalRsb(tdbb, opt, streams[1], sort_clause, project_clause,
							false, false, NULL);

			river->riv_rsb = 
				genInnerBooleanRsb(tdbb, opt, river->riv_rsb, streams[1]);
			}
		else
			river->riv_rsb =
				gen_retrieval(tdbb, opt, streams[1], sort_clause, project_clause,
							false, false, NULL);

		river->riv_streams[0] = streams[1];
		river_stack.push(river);
		return;
		}

	// Compute cardinality and indexed relationships for all streams.
	
	const UCHAR* const end_stream = streams + 1 + streams[0];
	
	for (UCHAR* stream = streams + 1; stream < end_stream; stream++) 
		{
		CompilerScratch::csb_repeat* csb_tail = &csb->csb_rpt[*stream];
		fb_assert(csb_tail);
		Relation* relation = csb_tail->csb_relation;
		fb_assert(relation);
		const Format* format = CMP_format(tdbb, csb, *stream);
		
		// if this is an external file, set an arbitrary cardinality; 
		// if a plan was specified, don't bother computing cardinality;
		// otherwise give a rough estimate based on the number of data
		// pages times the estimated number of records per page -- note
		// this is an upper limit since all pages are probably not full
		// and many of the records on page may be back versions.

		if (plan_clause) 
			csb_tail->csb_cardinality = (float) 0;
		else 
			csb_tail->csb_cardinality = OPT_getRelationCardinality(tdbb, relation, format);

		// find indexed relationships from this stream to every other stream
		
		OptimizerBlk::opt_stream* tail = opt->opt_streams.begin() + *stream;
		csb_tail->csb_flags |= csb_active;
		
		for (const UCHAR* t2 = streams + 1; t2 < end_stream; t2++)
			{
			if (*t2 != *stream) 
				{
				CompilerScratch::csb_repeat* csb_tail2 = &csb->csb_rpt[*t2];
				csb_tail2->csb_flags |= csb_active;
				IndexedRelationship* relationship = indexed_relationship(tdbb, opt, *t2);
				
				if (relationship) 
					{
					relationship->irl_next = tail->opt_relationships;
					tail->opt_relationships = relationship;
					relationship->irl_stream = *t2;
					}
					
				csb_tail2->csb_flags &= ~csb_active;
				}
			}
			
		csb_tail->csb_flags &= ~csb_active;

#ifdef OPT_DEBUG
		if (opt_debug_flag >= DEBUG_RELATIONSHIPS) 
			{
			fprintf(opt_debug_file,
					   "gen_join () -- relationships from stream %2.2d: ",
					   *stream);

			for (IndexedRelationship* relationship = tail->opt_relationships;
			      relationship; relationship = relationship->irl_next)
				fprintf(opt_debug_file, "%2.2d %s ",
							relationship->irl_stream,
							(relationship->irl_unique) ? "(unique)" : "");

			fprintf(opt_debug_file, "\n");
			}
#endif
		}

	// if the user specified a plan, force a join order;
	// otherwise try to find one
	
	if (plan_clause) 
		form_rivers(tdbb, opt, streams, river_stack, sort_clause, project_clause, plan_clause);
	else 
		{
		// copy the streams vector to a temporary space to be used
		// to form rivers out of streams
		stream_array_t temp;
		MOVE_FAST(streams, temp, streams[0] + 1);

        USHORT count;
		do 
			count = find_order(tdbb, opt, temp, 0);
		while (form_river (tdbb, opt, count, streams, temp, river_stack,  sort_clause, project_clause, 0));
		}
}


static RecordSource* gen_navigation(thread_db* tdbb,
									OptimizerBlk* opt,
									USHORT stream, 
									Relation* relation, 
									str* alias, 
									index_desc* idx,
									jrd_nod** sort_ptr)
{
/**************************************
 *
 *	g e n _ n a v i g a t i o n
 *
 **************************************
 *
 * Functional description
 *	See if a navigational walk of an index is in order.  If so,
 *	generate the appropriate RecordSource* and zap the sort pointer.  If
 *	not, return NULL.   Prior to ODS7, missing values sorted in
 *	the wrong place for ascending indices, so don't use them.
 *
 **************************************/
	SET_TDBB(tdbb);
	DEV_BLKCHK(opt, type_opt);
	DEV_BLKCHK(relation, type_rel);
	DEV_BLKCHK(alias, type_str);
	DEV_BLKCHK(*sort_ptr, type_nod);

	Database* dbb = tdbb->tdbb_database;

	// Check sort order against index.  If they don't match, give up and
	// go home.  Also don't bother if we have a non-unique index.
	// This is because null values aren't placed in a "good" spot in
	// the index in versions prior to V3.2.

	jrd_nod* sort = *sort_ptr;

	// if the number of fields in the sort is greater than the number of 
	// fields in the index, the index will not be used to optimize the    
	// sort--note that in the case where the first field is unique, this 
	// could be optimized, since the sort will be performed correctly by 
	// navigating on a unique index on the first field--deej

	if (sort->nod_count > idx->idx_count) 
		return NULL;


	// not sure the significance of this magic number; if it's meant to 
	// signify that we shouldn't navigate on a system table, our catalog 
	// has grown beyond 16 tables--it doesn't seem like a problem 
	// to allow navigating through system tables, so I won't bump the 
	// number up, but I'll leave it at 16 for safety's sake--deej

	if (relation->rel_id <= 16) 
		return NULL;

	// if the user-specified access plan for this request didn't
	// mention this index, forget it

	if ((idx->idx_runtime_flags & idx_plan_dont_use) 
			&& !(idx->idx_runtime_flags & idx_plan_navigate)) 	
		return NULL;

	if (idx->idx_flags & idx_expressn)
		if (sort->nod_count != 1)
			return NULL;

	// check to see if the fields in the sort match the fields in the index 
	// in the exact same order--we used to check for ascending/descending prior 
	// to SCROLLABLE_CURSORS, but now descending sorts can use ascending indices 
	// and vice versa.

#ifdef SCROLLABLE_CURSORS
	RSE_GET_MODE mode;
	RSE_GET_MODE last_mode = RSE_get_next;
#endif

	index_desc::idx_repeat* idx_tail = idx->idx_rpt;
	jrd_nod** ptr = sort->nod_arg;
	const jrd_nod* const* const end = ptr + sort->nod_count;

	for (; ptr < end; ptr++, idx_tail++)
		{
		jrd_nod* node = *ptr;

		if (idx->idx_flags & idx_expressn)
			if (!OPT_expression_equal(tdbb, opt, idx, node, stream))
				return NULL;	
		else if (node->nod_type != nod_field
				|| (USHORT)(long) node->nod_arg[e_fld_stream] != stream
				|| (USHORT)(long) node->nod_arg[e_fld_id] != idx_tail->idx_field )

			return NULL;

		// Do we need to do something special about null placement?
		// - for ODS11 default nulls placement always may be matched to index
		// - for ODS10 and earlier indices always placed nulls at the end of dataset
		// The variable temp exists to reduce the number of reinterpret_casts

		IPTR temp = reinterpret_cast<IPTR>(ptr[2 * sort->nod_count]);

		if (dbb->dbb_ods_version >= ODS_VERSION11  
				&& ((temp == rse_nulls_first && ptr[sort->nod_count])
			    || (temp == rse_nulls_last && !ptr[sort->nod_count]))
				|| (dbb->dbb_ods_version < ODS_VERSION11 && temp  == rse_nulls_first)
#ifdef SCROLLABLE_CURSORS
				)
#else
				|| (ptr[sort->nod_count] && !(idx->idx_flags & idx_descending))
				|| (!ptr[sort->nod_count] && (idx->idx_flags & idx_descending)))
#endif
			return NULL;

#ifdef SCROLLABLE_CURSORS
		// determine whether we ought to navigate backwards or forwards through 
		// the index--we can't allow navigating one index in two different directions 
		// on two different fields at the same time!

		mode = ((ptr[sort->nod_count] && !(idx->idx_flags & idx_descending))
					|| (!ptr[sort->nod_count] && (idx->idx_flags & idx_descending))) ?
						RSE_get_backward : RSE_get_forward;

		if (last_mode == RSE_get_next) 
			last_mode = mode;
		else if (last_mode != mode) 
			return NULL;
#endif
		}

	// Looks like we can do a navigational walk.  Flag that
	// we have used this index for navigation, and allocate
	// a navigational rsb for it.

	*sort_ptr = NULL;
	idx->idx_runtime_flags |= idx_navigate;
	return gen_nav_rsb(tdbb, opt, stream, relation, alias, idx
#ifdef SCROLLABLE_CURSORS
					   , mode
#endif
		);
}


static RecordSource* gen_nav_rsb(thread_db* tdbb,  
								 OptimizerBlk* opt, 
								 USHORT stream,  
								 Relation* relation, 
								 str* alias, 
								 index_desc* idx
#ifdef SCROLLABLE_CURSORS
								 , RSE_GET_MODE mode
#endif
	)
{
/**************************************
 *
 *	g e n _ n a v _ r s b
 *
 **************************************
 *
 * Functional description
 *	Generate a navigational rsb, either
 *	for a compile or for a set index.
 *
 **************************************/
	DEV_BLKCHK(opt, type_opt);
	DEV_BLKCHK(relation, type_rel);
	DEV_BLKCHK(alias, type_str);
	SET_TDBB(tdbb);

	USHORT key_length = ROUNDUP(BTR_key_length(tdbb, relation, idx), sizeof(SLONG));

	RsbNavigate *rsb = new (tdbb->tdbb_default) RsbNavigate(opt->opt_csb, stream, relation, alias,
															  OPT_make_index(tdbb, opt, relation, idx),
															  key_length);

#ifdef SCROLLABLE_CURSORS
	// indicate that the index needs to be navigated in a mirror-image   
	// fashion; that when the user wants to go backwards we actually go 
	// forwards and vice versa
	
	if (mode == RSE_get_backward) 
		rsb->rsb_flags |= rsb_descending;
#endif
	return rsb;
}



static RecordSource* gen_outer(thread_db* tdbb,
							   OptimizerBlk* opt,
							   RecordSelExpr* rse,
							   RiverStack& river_stack, 
							   jrd_nod** sort_clause, 
							   jrd_nod** project_clause)
{
/**************************************
 *
 *	g e n _ o u t e r
 *
 **************************************
 *
 * Functional description
 *	Generate a top level outer join.  The "outer" and "inner"
 *	sub-streams must be handled differently from each other.
 *	The inner is like other streams.  The outer stream isn't
 *	because conjuncts may not eliminate records from the
 *	stream.  They only determine if a join with an inner
 *	stream record is to be attempted.
 *
 **************************************/
	struct 
		{
		RecordSource* stream_rsb;
		USHORT stream_num;
		} stream_o, stream_i, *stream_ptr[2];
	
	// Determine which stream should be outer and which is inner.
	// In the case of a left join, the syntactically left stream is the 
	// outer, and the right stream is the inner.  For all others, swap 
	// the sense of inner and outer, though for a full join it doesn't 
	// matter and we should probably try both orders to see which is 
	// more efficient.
	
	if (rse->rse_jointype != blr_left) 
		{
		stream_ptr[1] = &stream_o;
		stream_ptr[0] = &stream_i;
		}
	else 
		{
		stream_ptr[0] = &stream_o;
		stream_ptr[1] = &stream_i;
		}

	// Loop through the outer join sub-streams in
	// reverse order because rivers may have been PUSHed
	
	River* river;
	SSHORT i;
	jrd_nod* node;
	
	for (i = 1; i >= 0; i--) 
		{
		node = rse->rse_relation[i];
		
		if (node->nod_type == nod_union
				|| node->nod_type == nod_aggregate
			 	|| node->nod_type == nod_procedure 
			 	|| node->nod_type == nod_rse)
			{
			river = river_stack.pop();
			stream_ptr[i]->stream_rsb = river->riv_rsb;
			}
		else 
			{
			stream_ptr[i]->stream_rsb = NULL;
			stream_ptr[i]->stream_num = (USHORT)(long) node->nod_arg[STREAM_INDEX(node)];
			}
		}

	// Generate rsbs for the sub-streams.  For the left sub-stream
	// we also will get a boolean back
	
	jrd_nod* boolean = NULL;
	jrd_nod* inner_boolean = NULL;
	
	if (!stream_o.stream_rsb) 
		stream_o.stream_rsb = gen_retrieval(tdbb, opt, stream_o.stream_num, sort_clause,
											 project_clause, true, false, &boolean);

	// in the case of a full join, we must make sure we don't exclude record from 
	// the inner stream; otherwise just retrieve it as we would for an inner join
	
	if (!stream_i.stream_rsb)
		{
		const bool bFullJoin = rse->rse_jointype == blr_full;
		const bool bOuter	= bFullJoin;
		jrd_nod**   ppNod	= bFullJoin ? &inner_boolean : 0;

		// AB: the sort clause for the inner stream of an
		// OUTER JOIN is never usefull for index retrieval.
		// dimitr: the same for DISTINCT via navigational index

		stream_i.stream_rsb = gen_retrieval(tdbb, opt, stream_i.stream_num,
			              NULL, NULL, bOuter, true, ppNod);
		}

	// generate a parent boolean rsb for any remaining booleans that 
	// were not satisfied via an index lookup
	
	stream_i.stream_rsb = gen_residual_boolean(tdbb, opt, stream_i.stream_rsb);

	// Allocate and fill in the rsb
	
	//RecordSource* rsb = FB_NEW_RPT(*tdbb->tdbb_default, RSB_LEFT_count) RecordSource(opt->opt_csb);

	RsbLeftCross *rsb = new (tdbb->tdbb_default) RsbLeftCross(opt->opt_csb, inner_boolean, stream_i.stream_rsb,
															  boolean, stream_o.stream_rsb);
	//rsb->rsb_type = rsb_left_cross;
	//rsb->rsb_count = 2;

	rsb->rsb_impure = CMP_impure(opt->opt_csb, sizeof(struct irsb));

	//rsb->rsb_arg[RSB_LEFT_outer] = stream_o.stream_rsb;
	//rsb->rsb_arg[RSB_LEFT_inner] = stream_i.stream_rsb;
	//rsb->rsb_arg[RSB_LEFT_boolean] = (RecordSource*) boolean;
	//rsb->rsb_arg[RSB_LEFT_inner_boolean] = (RecordSource*) inner_boolean;

	rsb->rsb_left_streams = FB_NEW(*tdbb->tdbb_default) StreamStack(*tdbb->tdbb_default);
	rsb->rsb_left_inner_streams = FB_NEW(*tdbb->tdbb_default) StreamStack(*tdbb->tdbb_default);
	rsb->rsb_left_rsbs = FB_NEW(*tdbb->tdbb_default) RsbStack(*tdbb->tdbb_default);
		
	// find all the outer and inner substreams and push them on a stack.
	
	//find_rsbs(stream_i.stream_rsb, rsb->rsb_left_streams,  rsb->rsb_left_rsbs);

	stream_i.stream_rsb->findRsbs(rsb->rsb_left_streams,  rsb->rsb_left_rsbs);
	
	if (rse->rse_jointype == blr_full) 
		//find_rsbs(stream_o.stream_rsb, rsb->rsb_left_inner_streams, NULL);
		stream_o.stream_rsb->findRsbs(rsb->rsb_left_inner_streams, rsb->rsb_left_rsbs);

	return rsb;
}


static RecordSource* genOuterRsb(thread_db* tdbb, OptimizerBlk* opt,
							   RecordSelExpr* rse, RiverStack& river_stack, 
							   jrd_nod** sort_clause, jrd_nod** project_clause)
{
/**************************************
 *
 *	g e n O u t e r R s b
 *
 **************************************
 *
 * Functional description
 *	Generate a top level outer join.  The "outer" and "inner"
 *	sub-streams must be handled differently from each other.
 *	The inner is like other streams.  The outer stream isn't
 *	because conjuncts may not eliminate records from the
 *	stream.  They only determine if a join with an inner
 *	stream record is to be attempted.
 *
 **************************************/
	struct 
		{
		RecordSource* stream_rsb;
		USHORT stream_num;
		} stream_o, stream_i, *stream_ptr[2];
	
	// Determine which stream should be outer and which is inner.
	// In the case of a left join, the syntactically left stream is the 
	// outer, and the right stream is the inner.  For all others, swap 
	// the sense of inner and outer, though for a full join it doesn't 
	// matter and we should probably try both orders to see which is 
	// more efficient.
	
	if (rse->rse_jointype != blr_left) 
		{
		stream_ptr[1] = &stream_o;
		stream_ptr[0] = &stream_i;
		}
	else 
		{
		stream_ptr[0] = &stream_o;
		stream_ptr[1] = &stream_i;
		}

	// Loop through the outer join sub-streams in
	// reverse order because rivers may have been PUSHed
	
	RecordSource* rsb = NULL;
	River* outerRiver = NULL;
	River* innerRiver = NULL;
	SSHORT i;
	jrd_nod* node;
	
	for (i = 1; i >= 0; i--) 
		{
		node = rse->rse_relation[i];
		
		if (node->nod_type == nod_union
				|| node->nod_type == nod_aggregate
			 	|| node->nod_type == nod_procedure 
			 	|| node->nod_type == nod_rse)
			{
			River* river = river_stack.pop();
			stream_ptr[i]->stream_rsb = river->riv_rsb;

			if (stream_ptr[i] == &stream_i)
				innerRiver = river;
			else
				outerRiver = river;
			}
		else 
			{
			stream_ptr[i]->stream_rsb = NULL;
			stream_ptr[i]->stream_num = (USHORT)(long) node->nod_arg[STREAM_INDEX(node)];
			}
		}

	// Generate rsbs for the sub-streams. 	
	jrd_nod* outerBoolean = NULL;
	jrd_nod* innerBoolean = NULL;
	
	if (!stream_o.stream_rsb) 
		{
		stream_o.stream_rsb = 
			genRetrievalRsb(tdbb, opt, stream_o.stream_num, 
				sort_clause, project_clause, true, false, NULL);

		outerBoolean = genStreamBoolean(tdbb, opt, stream_o.stream_num, true, false, false);

		if (outerBoolean)
			stream_o.stream_rsb = gen_boolean(tdbb, opt, stream_o.stream_rsb, outerBoolean);

		outerRiver = FB_NEW_RPT(*tdbb->tdbb_default, 1) River();
		outerRiver->riv_count = 1;
		outerRiver->riv_streams[0] = stream_o.stream_num;
		}

	// in the case of a full join, we must make sure we don't exclude record from 
	// the inner stream; otherwise just retrieve it as we would for an inner join
	const bool bFullJoin = (rse->rse_jointype == blr_full);
	bool indexedInnerStream = false;
	if (!stream_i.stream_rsb)
		{
		SortedStreamList dependentStreams;

		// AB: the sort clause for the inner stream of an
		// OUTER JOIN is never usefull for index retrieval.
		// dimitr: the same for DISTINCT via navigational index
		stream_i.stream_rsb = 
			genRetrievalRsb(tdbb, opt, stream_i.stream_num, 
				NULL, NULL, bFullJoin, true, &dependentStreams);

		// Walk through the index dependent streams and check
		// if at least one outer stream is one of them.
		int depCount = dependentStreams.getCount();
		if (depCount >= 1)
			{
			StreamStack outerStreams;
			RsbStack outerRsbs;
			stream_o.stream_rsb->findRsbs(&outerStreams, &outerRsbs);

			while (outerStreams.hasData() && !indexedInnerStream)
				{
				int outerStream = outerStreams.pop();

				int depIndex = 0;
				for (; depIndex < depCount; depIndex++)
						if (outerStream == dependentStreams[depIndex])
							{
							indexedInnerStream = true;
							break;
							}
				}
			}

		// Pick up usable conjunctions early as possible to avoid unneeded fetches.
		if (!bFullJoin)
			{
			if (!indexedInnerStream)
				set_inactive(opt, outerRiver);

			innerBoolean = genStreamBoolean(tdbb, opt, stream_i.stream_num, false, true, true);

			if (!indexedInnerStream)
				set_active(opt, outerRiver);

			if (innerBoolean)
				stream_i.stream_rsb = gen_boolean(tdbb, opt, stream_i.stream_rsb, innerBoolean);
			}

		innerRiver = FB_NEW_RPT(*tdbb->tdbb_default, 1) River();
		innerRiver->riv_count = 1;
		innerRiver->riv_streams[0] = stream_i.stream_num;
		}

	if (!indexedInnerStream)
		{
		// We didn't had a index retrieval for the inner stream with
		// a reference to the outer stream.
		// Try to create a SORT/MERGE.
		NodeStack innerNodes;
		NodeStack outerNodes;

		OptimizerBlk::opt_conjunct* tail = opt->opt_conjuncts.begin();
		const OptimizerBlk::opt_conjunct* opt_end = tail 
			+ (bFullJoin ? opt->opt_base_missing_conjuncts : opt->opt_conjuncts.getCount());

		// Find equality nodes that match both inner and outer river
		for (; tail < opt_end; tail++)
			{
			jrd_nod* node = tail->opt_conjunct_node;
			if (node->nod_type == nod_eql)
				{
				jrd_nod* leftNode = node->nod_arg[0];
				jrd_nod* rightNode = node->nod_arg[1];
				if (!river_reference(innerRiver, leftNode))
					if (river_reference(innerRiver, rightNode))
						{
						jrd_nod* tempNode = leftNode;
						leftNode = rightNode;
						rightNode = tempNode;
						}
					else
						continue;

				if (!river_reference(outerRiver, rightNode))
					continue;

				if (tail->opt_conjunct_flags & opt_conjunct_used)
					{
					innerNodes.clear();
					outerNodes.clear();
					break;
					}
					
				innerNodes.push(leftNode);
				outerNodes.push(rightNode);	
				}
			}

		// If we've found a match, generate the MERGE/SORT
		if (innerNodes.getCount() && outerNodes.getCount())
			{
			innerBoolean = genBaseResidualBoolean(tdbb, opt);

			RsbOuterMerge *mergeRsb = new (tdbb->tdbb_default) 
				RsbOuterMerge(opt->opt_csb, 2, !bFullJoin, innerBoolean);

			mergeRsb->rsb_impure = CMP_impure(opt->opt_csb, (USHORT) (sizeof(struct irsb_mrg) 
				+ 2 * sizeof(irsb_mrg::irsb_mrg_repeat)));

			jrd_nod* sort = FB_NEW_RPT(*tdbb->tdbb_default, innerNodes.getCount() * 3) jrd_nod();
			sort->nod_type = nod_sort;
			sort->nod_count = innerNodes.getCount();

			jrd_nod** ptr = sort->nod_arg;
			for (; !innerNodes.isEmpty(); ptr++) 
				{
				ptr[sort->nod_count] = (jrd_nod*) FALSE; // Ascending sort
				ptr[sort->nod_count * 2] = (jrd_nod*) rse_nulls_default; // Default nulls placement
				*ptr = (jrd_nod*)innerNodes.pop();
				}

			RsbSort* innerRsbSort = gen_sort(tdbb, opt, &innerRiver->riv_count, NULL, stream_i.stream_rsb, sort, false);

			mergeRsb->sortRsbs[1] = innerRsbSort;
			mergeRsb->sortNodes[1] = sort;

			sort = FB_NEW_RPT(*tdbb->tdbb_default, outerNodes.getCount() * 3) jrd_nod();
			sort->nod_type = nod_sort;
			sort->nod_count = outerNodes.getCount();

			ptr = sort->nod_arg;
			for (; !outerNodes.isEmpty(); ptr++) 
				{
				ptr[sort->nod_count] = (jrd_nod*) FALSE; // Ascending sort
				ptr[sort->nod_count * 2] = (jrd_nod*) rse_nulls_default; // Default nulls placement
				*ptr = (jrd_nod*)outerNodes.pop();
				}

			RsbSort* outerRsbSort = gen_sort(tdbb, opt, &outerRiver->riv_count, NULL, stream_o.stream_rsb, sort, false);

			mergeRsb->sortRsbs[0] = outerRsbSort;
			mergeRsb->sortNodes[0] = sort;

			rsb = mergeRsb;
			}
		}

	if (!rsb)
		{
		if (bFullJoin)
			innerBoolean = genBaseResidualBoolean(tdbb, opt);

		// generate a parent boolean rsb for any remaining booleans that 
		// were not satisfied via an index lookup	
		stream_i.stream_rsb = gen_residual_boolean(tdbb, opt, stream_i.stream_rsb);

		// Allocate and fill in the rsb
		RsbLeftCross* leftCrossRsb = new (tdbb->tdbb_default) 
			RsbLeftCross(opt->opt_csb, innerBoolean, stream_i.stream_rsb,
				outerBoolean, stream_o.stream_rsb);

		leftCrossRsb->rsb_impure = CMP_impure(opt->opt_csb, sizeof(struct irsb));
		leftCrossRsb->rsb_left_streams = FB_NEW(*tdbb->tdbb_default) StreamStack(*tdbb->tdbb_default);
		leftCrossRsb->rsb_left_inner_streams = FB_NEW(*tdbb->tdbb_default) StreamStack(*tdbb->tdbb_default);
		leftCrossRsb->rsb_left_rsbs = FB_NEW(*tdbb->tdbb_default) RsbStack(*tdbb->tdbb_default);
			
		// find all the outer and inner substreams and push them on a stack.
		stream_i.stream_rsb->findRsbs(leftCrossRsb->rsb_left_streams,  leftCrossRsb->rsb_left_rsbs);
		
		if (bFullJoin) 
			stream_o.stream_rsb->findRsbs(leftCrossRsb->rsb_left_inner_streams, leftCrossRsb->rsb_left_rsbs);

		rsb = leftCrossRsb;
		}

	return rsb;
}


static RecordSource* gen_procedure(thread_db* tdbb, 
								   OptimizerBlk* opt, 
								   jrd_nod* node)
{
/**************************************
 *
 *	g e n _ p r o c e d u r e
 *
 **************************************
 *
 * Functional description
 *	Compile and optimize a record selection expression into a
 *	set of record source blocks (rsb's).
 *
 **************************************/
	DEV_BLKCHK(opt, type_opt);
	DEV_BLKCHK(node, type_nod);
	//SET_TDBB(tdbb);

	CompilerScratch* csb = opt->opt_csb;	
	Procedure * procedure = tdbb->tdbb_database->findProcedure(tdbb,
														(int)(long)node->nod_arg[e_prc_procedure]);

	//RecordSource* rsb = FB_NEW_RPT(*tdbb->tdbb_default, RSB_PRC_count) RecordSource();

	RsbProcedure *rsb = new (tdbb->tdbb_default) RsbProcedure(opt->opt_csb, 
															procedure, node->nod_arg[e_prc_inputs], 
															node->nod_arg[e_prc_in_msg]);
	//rsb->rsb_type = rsb_procedure;

	rsb->rsb_stream = (UCHAR)(long) node->nod_arg[e_prc_stream];

	//rsb->procedure = procedure;

	rsb->rsb_format = procedure->findFormat();
	rsb->rsb_impure = CMP_impure(csb, sizeof(struct irsb_procedure));

	//rsb->rsb_arg[RSB_PRC_inputs] = (RecordSource*) node->nod_arg[e_prc_inputs];
	//rsb->rsb_arg[RSB_PRC_in_msg] = (RecordSource*) node->nod_arg[e_prc_in_msg];
	
	return rsb;
}


static RecordSource* gen_residual_boolean(thread_db* tdbb, 
										  OptimizerBlk* opt,
										  RecordSource* prior_rsb)
{
/**************************************
 *
 *	g e n _ r e s i d u a l _ b o o l e a n
 *
 **************************************
 *
 * Functional description
 *	Pick up any residual boolean remaining,
 *	meaning those that have not been used 
 *	as part of some join.  These booleans
 *	must still be applied to the result stream.
 *
 **************************************/

	jrd_nod* boolean = genBaseResidualBoolean(tdbb, opt);

	if (!boolean)
		return prior_rsb;

	return gen_boolean(tdbb, opt, prior_rsb, boolean);
}


static jrd_nod* genBaseResidualBoolean(thread_db* tdbb, OptimizerBlk* opt)
{
/**************************************
 *
 *	g e n B a s e B o o l e a n
 *
 **************************************
 *
 * Functional description
 *	Walk through all our conjunctions
 *	directly given to the current rse.
 *	Such as an ON clause for a JOIN.
 *	Pick up any residual boolean remaining.
 *
 **************************************/
	OptimizerBlk::opt_conjunct* tail = opt->opt_conjuncts.begin();
	const OptimizerBlk::opt_conjunct* const opt_end =
		opt->opt_conjuncts.begin() + opt->opt_base_conjuncts;

	jrd_nod* boolean = NULL;

	for (; tail < opt_end; tail++) 
		{
		jrd_nod* node = tail->opt_conjunct_node;
		
		if (!(tail->opt_conjunct_flags & opt_conjunct_used))
			{
			compose(tdbb, &boolean, node, nod_and);
			tail->opt_conjunct_flags |= opt_conjunct_used;
			}
		}

	return boolean;
}


static RecordSource* genInnerBooleanRsb(thread_db* tdbb, OptimizerBlk* opt,
                                        RecordSource* retrievalRsb, SSHORT stream)
/**************************************
 *
 *	g e n I n n e r B o o l e a n R s b
 *
 **************************************
 *
 * Functional description
 *	Generate RsbBoolean if possible for
 *  a inner joined stream.
 *
 **************************************/
{
	bool indexed = 
		(retrievalRsb->rsb_type == rsb_indexed) ||
		((retrievalRsb->rsb_type == rsb_navigate) && 
			(((RsbNavigate*) retrievalRsb)->inversion));

	jrd_nod* nodBoolean = genStreamBoolean(tdbb, opt, stream, false, false, indexed);

	if (nodBoolean)
		return gen_boolean(tdbb, opt, retrievalRsb, nodBoolean);
	else
		return retrievalRsb;
}


static RecordSource* genRetrievalRsb(thread_db* tdbb, OptimizerBlk* opt,
								   SSHORT stream,
								   jrd_nod** sort_ptr, jrd_nod** project_ptr,
								   bool ignoreBase, bool ignoreParentMissings,
								   SortedStreamList* dependentStreams)
{
/**************************************
 *
 *	g e n R e t r i e v a l R s b
 *
 **************************************
 *
 * Functional description
 *	Only for ODS11!!
 *	Compile and optimize a record selection expression into a
 *	set of record source blocks (rsb's).
 *
 **************************************/
	CompilerScratch* csb = opt->opt_csb;
	CompilerScratch::csb_repeat* csb_tail = &csb->csb_rpt[stream];
	Relation* relation = csb_tail->csb_relation;

	fb_assert(relation);

	csb_tail->csb_flags |= csb_active;
	
	if (sort_ptr && *sort_ptr && project_ptr && *project_ptr) 
		sort_ptr = NULL;

	jrd_nod* inversion = NULL;
	RecordSource* rsb = NULL;
	Database* dbb = tdbb->tdbb_database;
	double estimatedSelectivity = 1;
	double estimatedCost = 0;
	
	if (relation->rel_file) 
		{
		rsb = EXT_optimize(tdbb, opt, stream, sort_ptr ? sort_ptr : project_ptr);
		//estimatedCost = ????;
		}
	else
		{
		OptimizerRetrieval* optimizerRetrieval = FB_NEW(*tdbb->tdbb_default) 
			OptimizerRetrieval(tdbb, *tdbb->tdbb_default, opt, stream, 
				ignoreBase, ignoreParentMissings, sort_ptr);
		InversionCandidate* candidate = optimizerRetrieval->getInversion(&rsb);
		
		if (candidate)
			{
			estimatedSelectivity = candidate->selectivity;
			estimatedCost = candidate->cost;

			if (candidate->inversion)
				inversion = candidate->inversion;			

			if (dependentStreams)
				dependentStreams->join(candidate->dependentFromStreams);

			}

		delete candidate;
		delete optimizerRetrieval;
		}

	if (rsb) 
		{
		if (rsb->rsb_type == rsb_navigate && inversion) 
			((RsbNavigate*) rsb)->inversion = inversion;
		}
	else 
		{
		SSHORT size;
		str* alias = OPT_make_alias(tdbb, csb, csb_tail);
		
		if (inversion) 
			{
			rsb = new (tdbb->tdbb_default) RsbIndexed(opt->opt_csb, stream, relation, alias, inversion);
			size = sizeof(struct irsb_index);
			}
		else 
			{
			rsb = new (tdbb->tdbb_default) RsbSequential(opt->opt_csb, stream, relation, alias);		
			}
		}

	rsb->rsb_cardinality = (ULONG) csb_tail->csb_cardinality;
	rsb->estimatedSelectivity = estimatedSelectivity;
	rsb->estimatedCost = estimatedCost;
	return rsb;
}


static jrd_nod* genStreamBoolean(thread_db* tdbb, OptimizerBlk* opt, 
								 SSHORT stream,	bool ignoreBase, 
								 bool ignoreParentMissings, bool indexedStream)
{
/**************************************
 *
 *	g e n S t r e a m B o o l e a n
 *
 **************************************
 *
 * Functional description
 *	Walk through the needed conjunctions and test 
 *  if those are computable against the stream. 
 *	For a stream that has no indexed retrieval 
 *	pick up booleans that contain only stream-references
 *	to itself, so others could be used for MERGE.
 *
 **************************************/
	CompilerScratch* csb = opt->opt_csb;
	CompilerScratch::csb_repeat* csb_tail = &csb->csb_rpt[stream];
	Relation* relation = csb_tail->csb_relation;
	OptimizerBlk::opt_conjunct* tail = opt->opt_conjuncts.begin();
	const OptimizerBlk::opt_conjunct* const opt_end = 
		tail + (ignoreParentMissings ? opt->opt_base_missing_conjuncts : opt->opt_conjuncts.getCount());

	// Now make another pass thru the conjuncts finding unused, computable
	// booleans.  When one is found, roll it into a final boolean and mark
	// it used. If a computable boolean didn't match against an index then
	// mark the stream to denote unmatched booleans.
		
	if (ignoreBase) 
		tail += opt->opt_base_parent_conjuncts;

	jrd_nod* boolean = NULL;

	for (; tail < opt_end; tail++)
		{
		jrd_nod* node = tail->opt_conjunct_node;
		
		if (!relation->rel_file) 
			compose(tdbb, &boolean, OPT_make_dbkey(tdbb, opt, node, stream), nod_bit_and);

		if (!(tail->opt_conjunct_flags & opt_conjunct_used)
			// ANY/ALL boolean should always be left as a residual one
			&& !(node->nod_flags & nod_deoptimize)
			&& OPT_computable(csb, node, -1, false, false))
			{
			// If no index is used then leave other nodes alone, because they 
			// could be used for building a SORT/MERGE.			
			if ((indexedStream && expression_contains_stream(csb, node, stream, NULL)) ||
				(!indexedStream && OPT_computable(csb, node, stream, false, true)))
				{
				compose(tdbb, &boolean, node, nod_and);
				tail->opt_conjunct_flags |= opt_conjunct_used;

				if (!ignoreBase && !(tail->opt_conjunct_flags & opt_conjunct_matched))
					csb_tail->csb_flags |= csb_unmatched;
				}
			}
		}

	return boolean;
}


static RecordSource* gen_retrieval(thread_db* tdbb,
								   OptimizerBlk* opt,
								   SSHORT stream,
								   jrd_nod** sort_ptr, 
								   jrd_nod** project_ptr,
								   bool outer_flag,
								   bool inner_flag,
								   jrd_nod** return_boolean)
{
/**************************************
 *
 *	g e n _ r e t r i e v a l
 *
 **************************************
 *
 * Functional description
 *	Compile and optimize a record selection expression into a
 *	set of record source blocks (rsb's).
 *
 **************************************/
	OptimizerBlk::opt_conjunct* tail;
	bool full = false;


	// since a full outer join is a special case for us, as we have 2 outer
	// streams, recoginze this condition and set the full flag, also reset the
	// inner flag. This condition is only statisfied for the second stream in
	// the full join. This condition is only set from the call in gen_outer() in
	// case of a full join. 

	// if inner and outer are true set the inner flag back to false and set the 
	// full flag 

	if ((inner_flag == true) && (outer_flag == true)) 
		{
		inner_flag = false;
		full = true;
		}

	CompilerScratch* csb = opt->opt_csb;
	CompilerScratch::csb_repeat* csb_tail = &csb->csb_rpt[stream];
	Relation* relation = csb_tail->csb_relation;

	fb_assert(relation);

	str* alias = OPT_make_alias(tdbb, csb, csb_tail);
	csb_tail->csb_flags |= csb_active;
	
	// bug #8180 reported by Bill Karwin: when a DISTINCT and an ORDER BY 
	// are done on different fields, and the ORDER BY can be mapped to an 
	// index, then the records are returned in the wrong order because the 
	// DISTINCT sort is performed after the navigational walk of the index; 
	// for that reason, we need to de-optimize this case so that the ORDER 
	// BY does not use an index; if desired, we could re-optimize by doing 
	// the DISTINCT first, using a sparse bit map to store the DISTINCT 
	// records, then perform the navigational walk for the ORDER BY and 
	// filter the records out with the sparse bitmap.  However, that is a 
	// task for another day.  --deej 
	   
	// Bug #8958: comment out anything having to do with mapping a DISTINCT 
	// to an index, for now.  The fix for this bug was going to be so involved 
	// that it made more sense to deoptimize this case for the time being until 
	// we can determine whether it really makes sense to optimize a DISTINCT, 
	// or for that matter, and ORDER BY, via an index--there is a case 
	// to be made that it is a deoptimization and more testing needs to be done 
	//to determine that; see more comments in the bug description
	
	if (sort_ptr && *sort_ptr && project_ptr && *project_ptr) 
		sort_ptr = NULL;

	// Time to find inversions.  For each index on the relation
	// match all unused booleans against the index looking for upper
	// and lower bounds that can be computed by the index.  When
	// all unused conjunctions are exhausted, see if there is enough
	// information for an index retrieval.  If so, build up an
	// inversion component of the boolean. 


	jrd_nod* inversion = NULL;
	// It's recalculated later.

	const OptimizerBlk::opt_conjunct* opt_end = opt->opt_conjuncts.begin() 
									+ (inner_flag ? 
										opt->opt_base_missing_conjuncts : opt->opt_conjuncts.getCount());
	RecordSource* rsb = NULL;
	bool index_used = false;
	bool full_unique_match = false;

	Database* dbb = tdbb->tdbb_database;
	const bool ods11orHigher = (dbb->dbb_ods_version >= ODS_VERSION11);
	double estimatedSelectivity = 1;
	double estimatedCost = 0;
	
/*
	// For ODS11 and higher databases we can use new calculations
	if (ods11orHigher && !relation->rel_file) 
		{
		OptimizerRetrieval* optimizerRetrieval = FB_NEW(*tdbb->tdbb_default) 
			OptimizerRetrieval(tdbb, *tdbb->tdbb_default, opt, stream, outer_flag, inner_flag, sort_ptr);
		InversionCandidate* candidate = optimizerRetrieval->getInversion(&rsb);
		
		if (candidate)
			{
			estimatedSelectivity = candidate->selectivity;
			estimatedCost = candidate->cost;

			if (candidate->inversion)
				inversion = candidate->inversion;			
			}

		delete candidate;
		delete optimizerRetrieval;
		}


	// External files are handled differently too
	else 
*/
	if (relation->rel_file) 
		rsb = EXT_optimize(tdbb, opt, stream, sort_ptr ? sort_ptr : project_ptr);

	// we want to start with indices which have more index segments, attempting to match 
	// all the conjuncts possible to these indices, on the theory that one index matched 
	// to n booleans is more selective and uses less resources than using n indices;
	// therefore find out which index has the most segments and proceed backwards from there; 
	// NOTE: it's possible that a boolean might be matched to an index and then later it could 
	// have been paired with another boolean to match another index such that both booleans 
	// could be calculated via the index; currently we won't detect that case

	else if (opt->opt_conjuncts.getCount() || (sort_ptr && *sort_ptr) /*|| (project_ptr && *project_ptr)*/ )
		{
		firebird::HalfStaticArray<index_desc*, OPT_STATIC_ITEMS> idx_walk_vector(*tdbb->tdbb_default);
		idx_walk_vector.grow(csb_tail->csb_indices);
		index_desc** idx_walk = idx_walk_vector.begin();
		firebird::HalfStaticArray<UINT64, OPT_STATIC_ITEMS> idx_priority_level_vector(*tdbb->tdbb_default);
		idx_priority_level_vector.grow(csb_tail->csb_indices);
		UINT64* idx_priority_level = idx_priority_level_vector.begin();

		SSHORT i = 0;
		
		for (index_desc* idx = csb_tail->csb_idx->items; i < csb_tail->csb_indices; ++i, ++idx)
			{
			idx_walk[i] = idx;
			idx_priority_level[i] = LOWEST_PRIORITY_LEVEL;
			
			// skip this part if the index wasn't specified for indexed 
			// retrieval (still need to look for navigational retrieval)
			
			if ((idx->idx_runtime_flags & idx_plan_dont_use) 
					&& !(idx->idx_runtime_flags & idx_plan_navigate))
				continue;

			// go through all the unused conjuncts and see if 
			// any of them are computable using this index
			
			clear_bounds(opt, idx);
			tail = opt->opt_conjuncts.begin();
			
			if (outer_flag) 
				tail += opt->opt_base_parent_conjuncts;

			for (; tail < opt_end; tail++)
				{
				if (tail->opt_conjunct_flags & opt_conjunct_matched) 
					continue;

				jrd_nod* node = tail->opt_conjunct_node;
				
				if (!(tail->opt_conjunct_flags & opt_conjunct_used) 
						&& OPT_computable(csb, node, -1, (inner_flag || outer_flag), false))
					match_index(tdbb, opt, stream, node, idx);

				if (node->nod_type == nod_starts)
					compose(tdbb, &inversion,
							make_starts(tdbb, opt, relation, node, stream, idx), nod_bit_and);
							
				if (node->nod_type == nod_missing)
					compose(tdbb, &inversion, make_missing(tdbb, opt, relation, node,
										 stream, idx), nod_bit_and);
				}

			// look for a navigational retrieval (unless one was already found or
			// there is no sort block); if no navigational retrieval on this index,
			// add an indexed retrieval to the inversion tree
			
			if (!rsb)
				{
				if (sort_ptr && *sort_ptr)
					{
					rsb = gen_navigation(tdbb, opt, stream, relation, alias, idx, sort_ptr);
					if (rsb) 
						continue;
					}

				// for now, make sure that we only map a DISTINCT to an index if they contain 
				// the same number of fields; it should be possible to map a DISTINCT to an 
				// index which has extra fields to the right, but we need to add some code 
				// in NAV_get_record() to check when the relevant fields change, rather than 
				// the whole index key

				//if (project_ptr && *project_ptr)
	     
				//	if ((idx->idx_count == (*project_ptr)->nod_count) &&
				//        (rsb = gen_navigation (tdbb, opt, stream, relation, alias, idx, project_ptr)))
				//    {
				//	    rsb->rsb_flags |= rsb_project;
				//		continue;
				//    }
				}

			// Calculate the priority level for this index, if necessary

			if (opt->opt_segments[0].opt_lower || opt->opt_segments[0].opt_upper) 
				idx_priority_level[i] = calculate_priority_level(tdbb, opt, idx);
			}

		// Sort indices based on the priority level into idx_walk.
		
		SSHORT idx_walk_count = sort_indices_by_priority(tdbb, csb_tail, idx_walk, 
															idx_priority_level);

		// Walk through the indicies based on earlier calculated count and
		// when necessary build the index

		firebird::HalfStaticArray<SSHORT, OPT_STATIC_ITEMS>
			conjunct_position_vector(*tdbb->tdbb_default);

		firebird::HalfStaticArray<OptimizerBlk::opt_conjunct*, OPT_STATIC_ITEMS> 
			matching_nodes_vector(*tdbb->tdbb_default);

		for (i = 0; i < idx_walk_count; i++)
			{
			index_desc* idx = idx_walk[i];
			
			if (idx->idx_runtime_flags & idx_plan_dont_use)
				continue;

			conjunct_position_vector.shrink(0);
			matching_nodes_vector.shrink(0);
			clear_bounds(opt, idx);
			tail = opt->opt_conjuncts.begin();
			
			if (outer_flag) 
				tail += opt->opt_base_parent_conjuncts;

			for (; tail < opt_end; tail++) 
				{
				// Test if this conjunction is available for this index.
				
				if (!(tail->opt_conjunct_flags & opt_conjunct_matched)) 
					{
					// Setting opt_lower and/or opt_upper values

					jrd_nod* node = tail->opt_conjunct_node;
					
					if (!(tail->opt_conjunct_flags & opt_conjunct_used)  
							&& OPT_computable(csb, node, -1, (inner_flag || outer_flag), false))
						{
						if (match_index(tdbb, opt, stream, node, idx)) 
							{
							SSHORT position = 0;
							const OptimizerBlk::opt_segment* idx_tail = opt->opt_segments;
							const OptimizerBlk::opt_segment* const idx_end =  idx_tail + idx->idx_count;
							
							for (; idx_tail < idx_end; idx_tail++, position++) 
								if (idx_tail->opt_match == node)
									break;

							// if we're at the end and we have a resulting count
							// from match_index, still a node could not
							// be assigned, because equal nodes are
							// preferred against other ones.
							// Flag this node as used, so that no other
							// index is used with this bad one.
							// example: WHERE (ID = 100) and (ID >= 1)

							if (idx_tail >= idx_end && !csb_tail->csb_plan) 
								tail->opt_conjunct_flags |= opt_conjunct_matched;
							else 
								{
								matching_nodes_vector.add(tail);
								conjunct_position_vector.add(position);
								}
							}
						}
					}
				}

			if (opt->opt_segments[0].opt_lower || opt->opt_segments[0].opt_upper) 
				{
				// Use a different marking if a PLAN was specified, this is
				// for backwards compatibility.  Juck...
				// if there is a plan mark only used conjuncts in this index as used
				
				if (csb_tail->csb_plan) 
					{
					const OptimizerBlk::opt_segment* idx_tail = opt->opt_segments;
					const OptimizerBlk::opt_segment* const idx_end =  idx_tail + idx->idx_count;
					
					for (; idx_tail < idx_end &&  (idx_tail->opt_lower || idx_tail->opt_upper); idx_tail++)
						for (tail = opt->opt_conjuncts.begin(); tail < opt_end; tail++)
							if (idx_tail->opt_match == tail->opt_conjunct_node) 
								tail->opt_conjunct_flags |= opt_conjunct_matched;
					}

				// If there is no plan
				// Mark all conjuncts that could be calculated against the 
				// index as used. For example if you have :
				// (node1 >= constant) and (node1 <= constant) be sure both
				// conjuncts will be marked as opt_conjunct_matched

				else 
					{
					SSHORT position = 0;
					const OptimizerBlk::opt_segment* idx_tail = opt->opt_segments;
					const OptimizerBlk::opt_segment* const idx_end = idx_tail + idx->idx_count;
					
					for (; idx_tail < idx_end && (idx_tail->opt_lower || idx_tail->opt_upper);
					     idx_tail++, position++)
						for (size_t j = 0; j < conjunct_position_vector.getCount(); j++) 
							if (conjunct_position_vector[j] == position) 
								matching_nodes_vector[j]->opt_conjunct_flags |= opt_conjunct_matched;
					}

				jrd_nod* idx_node = OPT_make_index(tdbb, opt, relation, idx);
				IndexRetrieval*	retrieval = (IndexRetrieval*) idx_node->nod_arg[e_idx_retrieval];
				compose(tdbb, &inversion, idx_node, nod_bit_and);
				idx->idx_runtime_flags |= idx_used_with_and;
				index_used = true;

				// When we composed a UNIQUE index stop composing, because
				// this is the best we can get, but only when full used.
				
				if ((idx->idx_flags & idx_unique) && !(csb_tail->csb_plan) 
						&& !(retrieval->irb_generic & irb_partial)) 
					{
					full_unique_match = true;
					break; // Go out of idx_walk loop
					}
				}
			}
		}

	// Now make another pass thru the outer conjuncts only, finding unused,
	// computable booleans.  When one is found, roll it into a final
	// boolean and mark it used.

	if (outer_flag) 
		{
		*return_boolean = NULL;
		opt_end = opt->opt_conjuncts.begin() + opt->opt_base_conjuncts;
		
		for (tail = opt->opt_conjuncts.begin(); tail < opt_end; tail++) 
			{
			jrd_nod* node = tail->opt_conjunct_node;
			
			if (!(tail->opt_conjunct_flags & opt_conjunct_used)
					&& !(node->nod_flags & nod_deoptimize)
					&& OPT_computable(csb, node, -1, false, false))
				{
				compose(tdbb, return_boolean, node, nod_and);
				tail->opt_conjunct_flags |= opt_conjunct_used;
				}
			}
		}

	// Now make another pass thru the conjuncts finding unused, computable
	// booleans.  When one is found, roll it into a final boolean and mark
	// it used. If a computable boolean didn't match against an index then
	// mark the stream to denote unmatched booleans.
	
	jrd_nod* opt_boolean = NULL;
	opt_end = opt->opt_conjuncts.begin() + (inner_flag ? opt->opt_base_missing_conjuncts : opt->opt_conjuncts.getCount());
	tail = opt->opt_conjuncts.begin();
	
	if (outer_flag) 
		tail += opt->opt_base_parent_conjuncts;

	for (; tail < opt_end; tail++)
		{
		jrd_nod* node = tail->opt_conjunct_node;
		
		if (!relation->rel_file) 
			compose(tdbb, &inversion, OPT_make_dbkey(tdbb, opt, node, stream), nod_bit_and);

		if (!(tail->opt_conjunct_flags & opt_conjunct_used)
			&& OPT_computable(csb, node, -1, false, false))
			{
			// Don't waste time trying to match OR to available indices
			// if we already have an excellent match
			
			if (!ods11orHigher && (node->nod_type == nod_or) && !full_unique_match) 
				compose(tdbb, &inversion, make_inversion(tdbb, opt, node, stream), nod_bit_and); 

			// If no index is used then leave other nodes alone, because they 
			// could be used for building a SORT/MERGE.
			
			if ((inversion && expression_contains_stream(csb, node, stream, NULL)) ||
				(!inversion && OPT_computable(csb, node, stream, false, true))) 
				{
				// ANY/ALL boolean should always be left as a residual one
				if (!(node->nod_flags & nod_deoptimize))
					{
					compose(tdbb, &opt_boolean, node, nod_and);
					tail->opt_conjunct_flags |= opt_conjunct_used;
				
					if (!outer_flag &&
						!(tail->opt_conjunct_flags & opt_conjunct_matched))
						{
						csb_tail->csb_flags |= csb_unmatched;
						}
					}				
				}
			}
		}

	if (full)
		{
		return gen_rsb(tdbb, opt, rsb, inversion, stream, relation, alias,
					   *return_boolean, csb_tail->csb_cardinality, 
					   estimatedSelectivity, estimatedCost);
		}

	return gen_rsb(tdbb, opt, rsb, inversion, stream, relation, alias,
					opt_boolean, csb_tail->csb_cardinality,
                    estimatedSelectivity, estimatedCost);
}


static RecordSource* gen_rsb(thread_db* tdbb,
							 OptimizerBlk* opt,
							 RecordSource* rsb,
							 jrd_nod* inversion,
							 SSHORT stream,
							 Relation* relation, 
							 str* alias, 
							 jrd_nod* boolean, 
							 float cardinality,
							 double estimatedSelectivity, 
							 double estimatedCost)
{
/**************************************
 *
 *	g e n _ r s b
 *
 **************************************
 *
 * Functional description
 *	Generate a record source block to handle either a sort or a project.
 *
 **************************************/
	/***
	DEV_BLKCHK(opt, type_opt);
	DEV_BLKCHK(rsb, type_rsb);
	DEV_BLKCHK(inversion, type_nod);
	DEV_BLKCHK(relation, type_rel);
	DEV_BLKCHK(alias, type_str);
	DEV_BLKCHK(boolean, type_nod);
	SET_TDBB(tdbb);
	***/
	
	if (rsb) 
		{
		if (rsb->rsb_type == rsb_navigate && inversion) 
			//rsb->rsb_arg[RSB_NAV_inversion] = (RecordSource*) inversion;
			((RsbNavigate*) rsb)->inversion = inversion;
		}
	else 
		{
		SSHORT size;
		
		if (inversion) 
			{
			//rsb = FB_NEW_RPT(*tdbb->tdbb_default, 1) RecordSource(opt->opt_csb);

			rsb = new (tdbb->tdbb_default) RsbIndexed(opt->opt_csb, stream, relation, alias, inversion);

			//rsb->rsb_type = rsb_indexed;
			//rsb->rsb_count = 1;

			size = sizeof(struct irsb_index);

			//rsb->rsb_impure = CMP_impure(opt->opt_csb, size);
			//rsb->rsb_arg[0] = (RecordSource*) inversion;
			//rsb->rsb_stream = (UCHAR) stream;
			//rsb->rsb_relation = relation;
			//rsb->rsb_alias = alias;
			}
		else 
			{
			//rsb = FB_NEW_RPT(*tdbb->tdbb_default, 0) RecordSource(opt->opt_csb);

			rsb = new (tdbb->tdbb_default) RsbSequential(opt->opt_csb, stream, relation, alias);

			//rsb->rsb_type = rsb_sequential;
			//size = sizeof(struct irsb);
			
			if (boolean)
				opt->opt_csb->csb_rpt[stream].csb_flags |= csb_unmatched;
			}

		
		// if this is a blr_stream, we need to leave room
		// in the impure area for a navigational-type rsb;
		// even if this is not currently a navigational rsb, 
		// OPT_set_index() could be used to convert it to one.
		
		/***
		if (opt->opt_g_flags & opt_g_stream) 
			{
			fb_assert(false);		// this hasn't been converted yet
			size = sizeof(impure_inversion);
			size = nav_rsb_size(rsb, MAX_KEY, size);
			}
		***/
		}

	rsb->rsb_cardinality = (ULONG) cardinality;
	rsb->estimatedSelectivity = estimatedSelectivity;
	rsb->estimatedCost = estimatedCost;

	if (boolean) 
		{
		rsb = gen_boolean(tdbb, opt, rsb, boolean);

		// retain the cardinality for use at runtime by blr_cardinality
	
		rsb->rsb_cardinality = (ULONG) cardinality;
		}
	
	return rsb;
}


static RecordSource* gen_skip(thread_db* tdbb, 
							  OptimizerBlk* opt,
							  RecordSource* prior_rsb, 
							  jrd_nod* node)
{
/**************************************
 *
 *	g e n _ s k i p
 *
 **************************************
 *
 * Functional description
 *	Compile and optimize a record selection expression into a
 *	set of record source blocks (rsb's).
 *
 *      NOTE: The rsb_skip node MUST appear in the rsb list after the
 *          rsb_first node.  The calling code MUST call gen_skip before
 *          gen_first.
 *
 **************************************/
    
    CompilerScratch* csb = opt->opt_csb;

	// was : rsb = (RecordSource*) ALLOCDV (type_rsb, 1);
    // RecordSource* rsb = FB_NEW_RPT(*tdbb->tdbb_default, 0) RecordSource(opt->opt_csb);

    RsbSkip *rsb = new (tdbb->tdbb_default) RsbSkip(csb, prior_rsb, node);

    //rsb->rsb_count = 1;
    //rsb->rsb_type = rsb_skip;
    //rsb->rsb_next = prior_rsb;
    //rsb->rsb_arg [0] = (RecordSource*) node;
    //rsb->rsb_impure = CMP_impure (csb, sizeof (struct irsb_skip_n));
    
    return rsb;
}


static RsbSort* gen_sort(thread_db* tdbb,
						 OptimizerBlk* opt,
						 const UCHAR* streams,
						 const UCHAR* dbkey_streams,
						 RecordSource* prior_rsb, 
						 jrd_nod* sort, 
						 bool project_flag)
{
/**************************************
 *
 *	g e n _ s o r t
 *
 **************************************
 *
 * Functional description
 *	Generate a record source block to handle either a sort or a project.
 *	The two case are virtual identical -- the only difference is that
 *	project eliminates duplicates.  However, since duplicates are
 *	recognized and handled by sort, the JRD processing is identical.
 *
 **************************************/
	DEV_BLKCHK(opt, type_opt);
	DEV_BLKCHK(prior_rsb, type_rsb);
	DEV_BLKCHK(sort, type_nod);
	//SET_TDBB(tdbb);
	
	// We already know the number of keys, but we also need to compute the
	// total number of fields, keys and non-keys, to be pumped thru sort.  Starting
	// with the number of keys, count the other field referenced.  Since a field
	// is often a key, check for overlap to keep the length of the sort record
	// down. 
	   
	// Along with the record number, the transaction id of the
	// record will also be stored in the sort file.  This will
	// be used to detect update conflict in read committed 
	// transactions.

	const UCHAR* ptr;
	dsc descriptor;

	CompilerScratch* csb = opt->opt_csb;
	USHORT items = sort->nod_count + (streams[0] * 2) + (dbkey_streams ? dbkey_streams[0] : 0);
	const UCHAR* const end_ptr = streams + streams[0];
	const jrd_nod* const* const end_node = sort->nod_arg + sort->nod_count;
	firebird::Stack<SLONG> id_stack;
	StreamStack stream_stack;
	
	for (ptr = &streams[1]; ptr <= end_ptr; ptr++) 
		{
		UInt32Bitmap::Accessor accessor(csb->csb_rpt[*ptr].csb_fields);

		if (accessor.getFirst()) 
			do
				{
				const ULONG id = accessor.current();
				items++;
				id_stack.push(id);
				stream_stack.push(*ptr);
				
				for (jrd_nod** node_ptr = sort->nod_arg; node_ptr < end_node; node_ptr++)
					{
					jrd_nod* node = *node_ptr;
					
					if (node->nod_type == nod_field
							&& (USHORT)(long) node->nod_arg[e_fld_stream] == *ptr
							&& (USHORT)(long) node->nod_arg[e_fld_id] == id)
						{
						dsc* desc = &descriptor;
						CMP_get_desc(tdbb, csb, node, desc);
						
						// International type text has a computed key 
						
						if (IS_INTL_DATA(desc))
							break;
							
						--items;
						id_stack.pop();
						stream_stack.pop();
						break;
						}
					}
				} while (accessor.getNext());
		}

	// Now that we know the number of items, allocate a sort map block.  Allocate
	// it sufficiently large that there is room for a sort key descriptor on the
	// end. 

	const USHORT count = items +
		(sizeof(SortKeyDef) * 2 * sort->nod_count 
		+ sizeof(smb_repeat) - 1) / sizeof(smb_repeat);
		 
	SortMap* map = FB_NEW_RPT(*tdbb->tdbb_default, count) SortMap();
	map->smb_keys = sort->nod_count * 2;
	map->smb_count = items;
	
	if (project_flag) 
		map->smb_flags |= SMB_project;

    ULONG map_length = 0;

	// Loop thru sort keys building sort keys.  Actually, to handle null values
	// correctly, two sort keys are made for each field, one for the null flag
	// and one for field itself.
	
	smb_repeat* map_item = map->smb_rpt;
	SortKeyDef* sort_key = (SortKeyDef*) & map->smb_rpt[items];
	map->smb_key_desc = sort_key;
	
	for (jrd_nod** node_ptr = sort->nod_arg; node_ptr < end_node; node_ptr++, map_item++)
		{
		// Pick up sort key expression. 

		jrd_nod* node = *node_ptr;
		dsc* desc = &descriptor;
		CMP_get_desc(tdbb, csb, node, desc);
		
		// Allow for "key" forms of International text to grow 
		
		if (IS_INTL_DATA(desc)) 
			{
			// Turn varying text and cstrings into text. 

			if (desc->dsc_dtype == dtype_varying) 
				{
				desc->dsc_dtype = dtype_text;
				desc->dsc_length -= sizeof(USHORT);
				}
			else if (desc->dsc_dtype == dtype_cstring) 
				{
				desc->dsc_dtype = dtype_text;
				desc->dsc_length--;
				}
			desc->dsc_length = INTL_key_length(tdbb, INTL_INDEX_TYPE(desc), desc->dsc_length);
			}

		// Make key for null flag 

#ifndef WORDS_BIGENDIAN
		map_length = ROUNDUP(map_length, sizeof(SLONG));
#endif
		sort_key->skd_offset = map_item->smb_flag_offset = (USHORT) map_length++;
		sort_key->skd_dtype = SKD_text;
		sort_key->skd_length = 1;

		// Handle nulls placement

		sort_key->skd_flags = SKD_ascending;
		
		// Put nulls at the tail for ODS10 and earlier
		// but have SQL-compliant nulls ordering for ODS11+

		if (tdbb->tdbb_database->dbb_ods_version < ODS_VERSION11)
			{
			if ((long)*(node_ptr + sort->nod_count*2) == rse_nulls_first)
				sort_key->skd_flags |= SKD_descending;
			}
		else 
			{
			if (((long)*(node_ptr + sort->nod_count*2) == rse_nulls_default && !*(node_ptr + sort->nod_count)) ||
				(long)*(node_ptr + sort->nod_count*2) == rse_nulls_first)
				sort_key->skd_flags |= SKD_descending;
			}
			
		++sort_key;
		
		// Make key for sort key proper
		
#ifndef WORDS_BIGENDIAN
		map_length = ROUNDUP(map_length, sizeof(SLONG));
#else
		if (desc->dsc_dtype >= dtype_aligned)
			map_length = FB_ALIGN(map_length, type_alignments[desc->dsc_dtype]);
#endif
		sort_key->skd_offset = (USHORT) map_length;
		sort_key->skd_flags = SKD_ascending;
		
		if (*(node_ptr + sort->nod_count))
			sort_key->skd_flags |= SKD_descending;
			
		// UCHAR desc->dsc_dtype is always >=0
		//			fb_assert(desc->dsc_dtype >= 0);
		
		fb_assert(desc->dsc_dtype < FB_NELEM(sort_dtypes));
		sort_key->skd_dtype = sort_dtypes[desc->dsc_dtype];
		// fb_assert (sort_key->skd_dtype != 0); 
		
		if (sort_key->skd_dtype == SKD_varying || sort_key->skd_dtype == SKD_cstring)
			if (desc->dsc_ttype == ttype_binary)
				sort_key->skd_flags |= SKD_binary;

		sort_key->skd_length = desc->dsc_length;
		++sort_key;
		map_item->smb_node = node;
		map_item->smb_desc = *desc;
		map_item->smb_desc.dsc_address = (UCHAR *) (long) map_length;
		map_length += desc->dsc_length;
		
		if (node->nod_type == nod_field) 
			{
			map_item->smb_stream = (USHORT)(long) node->nod_arg[e_fld_stream];
			map_item->smb_field_id = (USHORT)(long) node->nod_arg[e_fld_id];
			}
		}

	map_length = ROUNDUP(map_length, sizeof(SLONG));
	map->smb_key_length = (USHORT) map_length >> SHIFTLONG;
	USHORT flag_offset = (USHORT) map_length;
	map_length += items - sort->nod_count;
	
	// Now go back and process all to fields involved with the sort.  If the
	// field has already been mentioned as a sort key, don't bother to repeat
	// it. 
   
	while (stream_stack.hasData()) 
		{
		const SLONG id = id_stack.pop();
		// AP: why USHORT - we pushed UCHAR
		const USHORT stream = stream_stack.pop();
		const Format* format = CMP_format(tdbb, csb, stream);
		const dsc* desc = &format->fmt_desc[id];
		
		if (id >= format->fmt_count || desc->dsc_length == 0)
			IBERROR(157);		// msg 157 cannot sort on a field that does not exist
			
		if (desc->dsc_dtype >= dtype_aligned)
			map_length = FB_ALIGN(map_length, type_alignments[desc->dsc_dtype]);
			
		map_item->smb_field_id = (SSHORT) id;
		map_item->smb_stream = stream;
		map_item->smb_flag_offset = flag_offset++;
		map_item->smb_desc = *desc;
		map_item->smb_desc.dsc_address = (UCHAR *) (long)map_length;
		map_length += desc->dsc_length;
		map_item++;
		}

	// Make fields for record numbers record for all streams 

	map_length = ROUNDUP(map_length, sizeof(SLONG));
	
	for (ptr = &streams[1]; ptr <= end_ptr; ptr++, map_item++) 
		{
		map_item->smb_field_id = SMB_DBKEY;
		map_item->smb_stream = *ptr;
		dsc* desc = &map_item->smb_desc;
		desc->dsc_dtype = dtype_int64;
		desc->dsc_length = sizeof(SINT64);
		desc->dsc_address = (UCHAR *)(long)map_length;
		map_length += desc->dsc_length;
		}

	// Make fields for transaction id of record for all streams 

	for (ptr = &streams[1]; ptr <= end_ptr; ptr++, map_item++) 
		{
		map_item->smb_field_id = SMB_TRANS_ID;
		map_item->smb_stream = *ptr;
		dsc* desc = &map_item->smb_desc;
		desc->dsc_dtype = dtype_long;
		desc->dsc_length = sizeof(SLONG);
		desc->dsc_address = (UCHAR *) (long)map_length;
		map_length += desc->dsc_length;
		}

	if (dbkey_streams) 
		{
        ptr = &dbkey_streams[1];
		for (const UCHAR* const end_ptrL = dbkey_streams + dbkey_streams[0]; ptr <= end_ptrL; ptr++, map_item++)
			{
			map_item->smb_field_id = SMB_DBKEY;
			map_item->smb_stream = *ptr;
			dsc* desc = &map_item->smb_desc;
			desc->dsc_dtype = dtype_int64;
			desc->dsc_length = sizeof(SINT64);
			desc->dsc_address = (UCHAR *)(long)map_length;
			map_length += desc->dsc_length;
			}
		}

	// Make fields to store varying and cstring length. 

	const SortKeyDef* const end_key = sort_key;
	
	for (sort_key = map->smb_key_desc; sort_key < end_key; sort_key++) 
		if (sort_key->skd_dtype == SKD_varying || sort_key->skd_dtype == SKD_cstring)
			{
			sort_key->skd_vary_offset = (USHORT) map_length;
			map_length += sizeof(USHORT);
			}

	if (map_length > MAX_SORT_RECORD)
		ERR_post(isc_sort_rec_size_err, isc_arg_number, map_length, 0);
		// Msg438: sort record size of %ld bytes is too big 
		
	map->smb_length = (USHORT) map_length;
	
	// That was most unpleasant.  Never the less, it's done (except for
	// the debugging).  All that remains is to build the record source
	// block for the sort. 
   
	//RecordSource* rsb = FB_NEW_RPT(*tdbb->tdbb_default, 1) RecordSource();

	RsbSort *rsb = new (tdbb->tdbb_default) RsbSort(opt->opt_csb, prior_rsb, map);

	//rsb->rsb_type = rsb_sort;
	//rsb->rsb_next = prior_rsb;
	//rsb->rsb_arg[0] = (RecordSource*) map;

	rsb->rsb_impure = CMP_impure(csb, sizeof(struct irsb_sort));
	
	return rsb;
}


static bool gen_sort_merge(thread_db* tdbb, 
						   OptimizerBlk* opt, 
						   RiverStack& org_rivers)
{
/**************************************
 *
 *	g e n _ s o r t _ m e r g e
 *
 **************************************
 *
 * Functional description
 *	We've got a set of rivers that may or may not be amenable to
 *	a sort/merge join, and it's time to find out.  If there are,
 *	build a sort/merge RecordSource*, push it on the rsb stack, and update
 *	rivers accordingly.  If two or more rivers were successfully
 *	joined, return true.  If the whole things is a moby no-op,
 *	return false.
 *
 **************************************/
	USHORT i;
	ULONG selected_rivers[OPT_STREAM_BITS], selected_rivers2[OPT_STREAM_BITS];
	DEV_BLKCHK(opt, type_opt);
	//SET_TDBB(tdbb);
	Database* dbb = tdbb->tdbb_database;

	// Count the number of "rivers" involved in the operation, then allocate
	// a scratch block large enough to hold values to compute equality
	// classes.
	
	USHORT cnt = 0;
	
	for (RiverStack::iterator stack1(org_rivers); stack1.hasData(); ++stack1) 
		stack1.object()->riv_number = cnt++;

	firebird::HalfStaticArray<jrd_nod*, OPT_STATIC_ITEMS> scratch(*tdbb->tdbb_default);
	scratch.grow(opt->opt_base_conjuncts * cnt);
	jrd_nod** classes = scratch.begin();

	// Compute equivalence classes among streams.  This involves finding groups
	// of streams joined by field equalities.
	
	jrd_nod** last_class = classes;
	OptimizerBlk::opt_conjunct* tail = opt->opt_conjuncts.begin();
	const OptimizerBlk::opt_conjunct* const end = tail + opt->opt_base_conjuncts;
	jrd_nod **eq_class, **ptr;
	
	for (; tail < end; tail++)
		{
		if (tail->opt_conjunct_flags & opt_conjunct_used) 
			continue;

		jrd_nod* node = tail->opt_conjunct_node;
		
		if (node->nod_type != nod_eql) 
			continue;

		jrd_nod* node1 = node->nod_arg[0];
		jrd_nod* node2 = node->nod_arg[1];
		
		for (RiverStack::iterator stack0(org_rivers); stack0.hasData(); ++stack0) 
			{
			River* river1 = stack0.object();
			
			if (!river_reference(river1, node1)) 
				{
				if (river_reference(river1, node2)) 
					{
					node = node1;
					node1 = node2;
					node2 = node;
					}
				else 
					continue;
				}
				
			for (RiverStack::iterator stack2(stack0); (++stack2).hasData();)
				{
				River* river2 = stack2.object();
				
				if (river_reference(river2, node2)) 
					{
					for (eq_class = classes; eq_class < last_class; eq_class += cnt)
						if (node_equality(node1, classes[river1->riv_number])
								|| node_equality(node2, classes[river2->riv_number]))
							break;
						
					eq_class[river1->riv_number] = node1;
					eq_class[river2->riv_number] = node2;
					
					if (eq_class == last_class) 
						last_class += cnt;
					}
				}
			}
		}

	// Pick both a set of classes and a set of rivers on which to join with
	// sort merge.  Obviously, if the set of classes is empty, return false
	// to indicate that nothing could be done. 

	USHORT river_cnt = 0, stream_cnt = 0;
	firebird::HalfStaticArray<jrd_nod**, OPT_STATIC_ITEMS> selected_classes(*tdbb->tdbb_default, cnt);
	
	for (eq_class = classes; eq_class < last_class; eq_class += cnt) 
		{
		i = river_count(cnt, eq_class);
		
		if (i > river_cnt) 
			{
			river_cnt = i;
			selected_classes.shrink(0);
			selected_classes.add(eq_class);
			class_mask(cnt, eq_class, selected_rivers);
			}
		else 
			{
			class_mask(cnt, eq_class, selected_rivers2);
			
			for (i = 0; i < OPT_STREAM_BITS; i++) 
				if ((selected_rivers[i] & selected_rivers2[i]) != selected_rivers[i]) 
					break;
					
			if (i == OPT_STREAM_BITS)
				selected_classes.add(eq_class);
			}
		}

	if (!river_cnt)
		return false;

	// Build a sort stream.
	
	//RecordSource* merge_rsb = FB_NEW_RPT(*tdbb->tdbb_default, river_cnt * 2) RecordSource(opt->opt_csb);

	RsbMerge *merge_rsb = new (tdbb->tdbb_default) RsbMerge(opt->opt_csb, river_cnt);

	//merge_rsb->rsb_count = river_cnt;
	//merge_rsb->rsb_type = rsb_merge;

	merge_rsb->rsb_impure = CMP_impure(opt->opt_csb, 
										(USHORT) (sizeof(struct irsb_mrg) 
											+ river_cnt * sizeof(irsb_mrg::irsb_mrg_repeat)));

	//RecordSource** rsb_tail = merge_rsb->rsb_arg;

	stream_cnt = 0;
	
	// AB: Get the lowest river position from the rivers that are merged.
	// Note that we're walking the rivers in backwards direction.
	
	USHORT lowestRiverPosition = 0;
	int n = 0;
	
	for (RiverStack::iterator stack3(org_rivers); stack3.hasData(); ++stack3) 
		{
		River* river1 = stack3.object();
		
		if (!(TEST_DEP_BIT(selected_rivers, river1->riv_number))) 
			continue;

		if (river1->riv_number > lowestRiverPosition) 
			lowestRiverPosition = river1->riv_number;

		stream_cnt += river1->riv_count;
		jrd_nod* sort = FB_NEW_RPT(*tdbb->tdbb_default, selected_classes.getCount() * 3) jrd_nod();
		sort->nod_type = nod_sort;
		sort->nod_count = selected_classes.getCount();
		jrd_nod*** selected_class;
		
		for (selected_class = selected_classes.begin(), ptr = sort->nod_arg;
				selected_class < selected_classes.end(); selected_class++) 
			{
			ptr[sort->nod_count] = (jrd_nod*) FALSE; // Ascending sort
			ptr[sort->nod_count * 2] = (jrd_nod*) rse_nulls_default; // Default nulls placement
			*ptr++ = (*selected_class)[river1->riv_number];
			}

		RsbSort* rsb = gen_sort(tdbb, opt, &river1->riv_count, NULL, river1->riv_rsb, sort, false);

		//*rsb_tail++ = rsb;
		//*rsb_tail++ = (RecordSource*) sort;

		merge_rsb->sortRsbs[n] = rsb;
		merge_rsb->sortNodes[n] = sort;
		n++;
		}

	// Finally, merge selected rivers into a single river, and rebuild 
	// original river stack.
	// AB: Be sure that the rivers 'order' will be kept.
	
	River* river1 = FB_NEW_RPT(*tdbb->tdbb_default, stream_cnt) River();
	river1->riv_count = (UCHAR) stream_cnt;
	river1->riv_rsb = merge_rsb;
	UCHAR* stream = river1->riv_streams;
	RiverStack newRivers(org_rivers.getPool());
	
	while (org_rivers.hasData()) 
		{
		River* river2 = org_rivers.pop();
		
		if (TEST_DEP_BIT(selected_rivers, river2->riv_number)) 
			{
			MOVE_FAST(river2->riv_streams, stream, river2->riv_count);
			stream += river2->riv_count;
			
			// If this is the lowest position put in the new river.
			
			if (river2->riv_number == lowestRiverPosition) 
				newRivers.push(river1);
			}
		else 
			newRivers.push(river2);
		}

	// AB: Put new rivers list back in the original list.
	// Note that the rivers in the new stack are reversed.
	
	while (newRivers.hasData()) 
		org_rivers.push(newRivers.pop());

	// Pick up any boolean that may apply.

	USHORT flag_vector[MAX_STREAMS + 1], *fv;
	UCHAR stream_nr;
	
	// AB: Inactivate currently all streams from every river, because we
	// need to know which nodes are computable between the rivers used
	// for the merge.
	
	for (stream_nr = 0, fv = flag_vector; stream_nr < opt->opt_csb->csb_n_stream; stream_nr++) 
		{
		*fv++ = opt->opt_csb->csb_rpt[stream_nr].csb_flags & csb_active;
		opt->opt_csb->csb_rpt[stream_nr].csb_flags &= ~csb_active;
		}

	set_active(opt, river1);
	jrd_nod* node = NULL;
	
	for (tail = opt->opt_conjuncts.begin(); tail < end; tail++)
		{
		jrd_nod* node1 = tail->opt_conjunct_node;
		
		if (!(tail->opt_conjunct_flags & opt_conjunct_used)
				&& !(node1->nod_flags & nod_deoptimize)
				&& OPT_computable(opt->opt_csb, node1, -1, false, false))
			{
			compose(tdbb, &node, node1, nod_and);
			tail->opt_conjunct_flags |= opt_conjunct_used;
			}
		}

	if (node) 
		river1->riv_rsb = gen_boolean(tdbb, opt, river1->riv_rsb, node);

	set_inactive(opt, river1);

	for (stream_nr = 0, fv = flag_vector;  stream_nr < opt->opt_csb->csb_n_stream; stream_nr++) 
		opt->opt_csb->csb_rpt[stream_nr].csb_flags |= *fv++;

	return true;
}


static RecordSource* gen_union(thread_db* tdbb,OptimizerBlk* opt,
							   jrd_nod*union_node,
							   UCHAR *streams,
							   USHORT nstreams,
							   NodeStack* parent_stack,
							   UCHAR shellStream)
{
/**************************************
 *
 *	g e n _ u n i o n
 *
 **************************************
 *
 * Functional description
 *	Generate a union complex.
 *
 **************************************/
 
	/***
	DEV_BLKCHK(opt, type_opt);
	DEV_BLKCHK(union_node, type_nod);
	SET_TDBB(tdbb);
	***/
	
	jrd_nod* clauses = union_node->nod_arg[e_uni_clauses];
	const USHORT count = clauses->nod_count;
	CompilerScratch* csb = opt->opt_csb;

	//RecordSource* rsb = FB_NEW_RPT(*tdbb->tdbb_default, count + nstreams + 1) RecordSource(opt->opt_csb);

	RsbUnion *rsb = new (tdbb->tdbb_default) RsbUnion(csb, count / 2, nstreams);

	//rsb->rsb_type = rsb_union;
	//rsb->rsb_count = count;

	rsb->rsb_stream = (UCHAR)(long) union_node->nod_arg[e_uni_stream];
	rsb->rsb_format = csb->csb_rpt[rsb->rsb_stream].csb_format;

	//rsb->rsb_impure = CMP_impure(csb, sizeof(struct irsb));
	//RecordSource** rsb_ptr = rsb->rsb_arg;

	jrd_nod** ptr = clauses->nod_arg;
	int n = 0;
	
	for (const jrd_nod* const* const end = ptr + count; ptr < end; ++n) 
		{
		RecordSelExpr* rse = (RecordSelExpr*) *ptr++;
		jrd_nod* map = *ptr++;

		// AB: Try to distribute booleans from the top rse for a UNION to 
		// the WHERE clause of every single rse.
		
		NodeStack deliverStack;
		gen_deliver_unmapped(tdbb, &deliverStack, map, parent_stack, shellStream);

		//*rsb_ptr++ = OPT_compile(tdbb, csb, rse, &deliverStack);
		//*rsb_ptr++ = (RecordSource*) map;

		rsb->rsbs[n] = OPT_compile(tdbb, csb, rse, &deliverStack, 0);
		rsb->maps[n] = map;
		}

	// Save the count and numbers of the streams that make up the union 

	/***
	*rsb_ptr++ = (RecordSource*)(long) nstreams;
	while (nstreams--) 
		*rsb_ptr++ = (RecordSource*)(long) *streams++;
	***/
	
	for (n = 0; n < nstreams; ++n)
		rsb->streams [n] = *streams++;
	

	return rsb;
}


static void get_expression_streams(const jrd_nod* node,
								   firebird::SortedArray<int>& streams)
{
/**************************************
 *
 *  g e t _ e x p r e s s i o n _ s t r e a m s
 *
 **************************************
 *
 * Functional description
 *  Return all streams referenced by the expression.
 *
 **************************************/
	DEV_BLKCHK(node, type_nod);

	if (!node) 
		return;

	RecordSelExpr* rse = NULL;

	int n;
	int pos;

	switch (node->nod_type) 
		{
		case nod_field:
			n = (int)(long) node->nod_arg[e_fld_stream];

			if (!streams.find(n, pos))
				streams.add(n);

			break;

		case nod_rec_version:
		case nod_dbkey:
			n = (int)(long) node->nod_arg[0];

			if (!streams.find(n, pos))
				streams.add(n);

			break;

		case nod_cast:
			get_expression_streams(node->nod_arg[e_cast_source], streams);
			break;

		case nod_extract:
			get_expression_streams(node->nod_arg[e_extract_value], streams);
			break;

/* FB2.0 INTL
		case nod_strlen:
			get_expression_streams(node->nod_arg[e_strlen_value], streams);
			break;
*/

		case nod_function:
			get_expression_streams(node->nod_arg[e_fun_args], streams);
			break;

		case nod_procedure:
			get_expression_streams(node->nod_arg[e_prc_inputs], streams);
			break;

		case nod_any:
		case nod_unique:
		case nod_ansi_any:
		case nod_ansi_all:
		case nod_exists:
			get_expression_streams(node->nod_arg[e_any_rse], streams);
			break;

		case nod_argument:
		case nod_current_date:
		case nod_current_role:
		case nod_current_time:
		case nod_current_timestamp:
		case nod_gen_id:
		case nod_gen_id2:
		case nod_internal_info:
		case nod_literal:
		case nod_null:
		case nod_user_name:
		case nod_variable:
			break;

		case nod_rse:
			rse = (RecordSelExpr*) node;
			break;

		case nod_average:
		case nod_count:
		case nod_count2:
		case nod_from:
		case nod_max:
		case nod_min:
		case nod_total:
			get_expression_streams(node->nod_arg[e_stat_rse], streams);
			get_expression_streams(node->nod_arg[e_stat_value], streams);
			break;

		// go into the node arguments

		case nod_add:
		case nod_add2:
		case nod_agg_average:
		case nod_agg_average2:
		case nod_agg_average_distinct:
		case nod_agg_average_distinct2:
		case nod_agg_max:
		case nod_agg_min:
		case nod_agg_total:
		case nod_agg_total2:
		case nod_agg_total_distinct:
		case nod_agg_total_distinct2:
		case nod_concatenate:
		case nod_divide:
		case nod_divide2:
		case nod_multiply:
		case nod_multiply2:
		case nod_negate:
		case nod_subtract:
		case nod_subtract2:

		case nod_upcase:
/* FB2.0 INTL
		case nod_lowcase:
		case nod_trim:
*/
		case nod_substr:

		case nod_like:
		case nod_between:
		case nod_sleuth:
		case nod_missing:
		case nod_value_if:
		case nod_matches:
		case nod_contains:
		case nod_starts:
		case nod_equiv:
		case nod_eql:
		case nod_neq:
		case nod_geq:
		case nod_gtr:
		case nod_lss:
		case nod_leq:
			{
			// Check all sub-nodes of this node.

			const jrd_nod* const* ptr = node->nod_arg;
			const jrd_nod* const* const end = ptr + node->nod_count;

			for (; ptr < end; ptr++)
				get_expression_streams(*ptr, streams);

			break;
			}

		default:
			break;
		}

	if (rse) 
		{
		get_expression_streams(rse->rse_first, streams);
		get_expression_streams(rse->rse_skip, streams);
		get_expression_streams(rse->rse_boolean, streams);
		get_expression_streams(rse->rse_sorted, streams);
		get_expression_streams(rse->rse_projection, streams);
		}
}


static void get_inactivities(const CompilerScratch* csb,
							 ULONG* dependencies)
{
/**************************************
 *
 *	g e t _ i n a c t i v i t i e s
 *
 **************************************
 *
 * Functional description
 *	Find any streams not explicitily active.
 *
 **************************************/
	SLONG n;
	DEV_BLKCHK(csb, type_csb);

	for (n = 0; n < OPT_STREAM_BITS; n++)
		dependencies[n] = (ULONG) - 1;

	n = 0;
	CompilerScratch::rpt_const_itr tail = csb->csb_rpt.begin();
	const CompilerScratch::rpt_const_itr end = tail + csb->csb_n_stream;

	for (; tail < end; n++, tail++)
		if (tail->csb_flags & csb_active)
			CLEAR_DEP_BIT(dependencies, n);
}


static jrd_nod* get_unmapped_node(thread_db* tdbb, 
								  jrd_nod* node,
								  jrd_nod* map, 
								  UCHAR shellStream, 
								  bool rootNode)
{
/**************************************
 *
 *	g e t _ u n m a p p e d _ n o d e
 *
 **************************************
 *
 *	Return correct node if this node is
 *  allowed in a unmapped boolean.
 *
 **************************************/
	DEV_BLKCHK(map, type_nod);
	SET_TDBB(tdbb);

	// Check if node is a mapping and if so unmap it, but
	// only for root nodes (not contained in another node).
	// This can be expanded by checking complete expression
	// (Then don't forget to leave aggregate-functions alone 
	//  in case of aggregate rse)
	// Because this is only to help using an index we keep 
	// it simple.

	if ((node->nod_type == nod_field)  
			&& ((USHORT)(long) node->nod_arg[e_fld_stream] == shellStream)) 
		{
		USHORT fieldId = (USHORT)(long) node->nod_arg[e_fld_id];

		if (!rootNode || (fieldId >= map->nod_count)) 
			return NULL;

		// Check also the expression inside the map, because aggregate
		// functions aren't allowed to be delivered to the WHERE clause.

		return get_unmapped_node(tdbb, map->nod_arg[fieldId]->nod_arg[e_asgn_from], 
			map, shellStream, false);
	}

	jrd_nod* returnNode = NULL;

	switch (node->nod_type) 
		{
		case nod_cast:
			if (get_unmapped_node(tdbb, node->nod_arg[e_cast_source], 
									map, shellStream, false) != NULL)
				returnNode = node;

			break;

		case nod_extract:
			if (get_unmapped_node(tdbb, node->nod_arg[e_extract_value], 
									map, shellStream, false) != NULL)
				returnNode = node;

			break;

		case nod_argument:
		case nod_current_date:
		case nod_current_role:
		case nod_current_time:
		case nod_current_timestamp:
		case nod_field:
		case nod_gen_id:
		case nod_gen_id2:
		case nod_internal_info:
		case nod_literal:
		case nod_null:
		case nod_user_name:
		case nod_variable:
			returnNode = node;
			break;

		case nod_add:
		case nod_add2:
		case nod_concatenate:
		case nod_divide:
		case nod_divide2:
		case nod_multiply:
		case nod_multiply2:
		case nod_negate:
		case nod_subtract:
		case nod_subtract2:
		case nod_upcase:
		case nod_substr:
			{
			// Check all sub-nodes of this node.

			jrd_nod** ptr = node->nod_arg;
			const jrd_nod* const* const end = ptr + node->nod_count;
			for (; ptr < end; ptr++)
				if (get_unmapped_node(tdbb, *ptr, map, shellStream, 
										false) == NULL)
					return NULL;
			return node;
			}

		default:
			return NULL;
		}

	return returnNode;
}


static IndexedRelationship* indexed_relationship(thread_db* tdbb, 
												 OptimizerBlk* opt,
												 USHORT stream)
{
/**************************************
 *
 *	i n d e x e d _ r e l a t i o n s h i p
 *
 **************************************
 *
 * Functional description
 *	See if two streams are related by an index.
 *	An indexed relationship is a means of joining two 
 *	streams via an index, which is possible when a field from
 *	each of the streams is compared with a field from the other,
 *	and there is an index on one stream to retrieve the value
 *	of the other field.
 *
 **************************************/

	DEV_BLKCHK(opt, type_opt);
	SET_TDBB(tdbb);

	if (!opt->opt_base_conjuncts) 
		return NULL;

	CompilerScratch* csb = opt->opt_csb;
	CompilerScratch::csb_repeat* csb_tail  = &csb->csb_rpt[stream];
	OptimizerBlk::opt_conjunct* opt_end = 
		opt->opt_conjuncts.begin() + opt->opt_base_conjuncts;
	IndexedRelationship* relationship = NULL;

	//  Loop thru indexes looking for a match 

	const index_desc* idx = csb_tail->csb_idx->items;

	for (USHORT i = 0; i < csb_tail->csb_indices; ++i, ++idx)
		{
		// skip this part if the index wasn't specified for indexed retrieval 
		if (idx->idx_runtime_flags & idx_plan_dont_use) 
			continue;

		clear_bounds(opt, idx);
		OptimizerBlk::opt_conjunct* tail;

		for (tail = opt->opt_conjuncts.begin(); tail < opt_end; tail++)
			{
			jrd_nod* node = tail->opt_conjunct_node;
			if (!(tail->opt_conjunct_flags & opt_conjunct_used)
					&& OPT_computable(csb, node, -1, false, false))
				{
				// AB: Why only check for AND structures ? 
				// Added match_indices for support of "OR" with INNER JOINs 

				// match_index(tdbb, opt, stream, node, idx);

				match_indices(tdbb, opt, stream, node, idx);

				// AB: Why should we look further?

				if (opt->opt_segments[0].opt_lower || opt->opt_segments[0].opt_upper) 
					break;
				}
			}

		if (opt->opt_segments[0].opt_lower || opt->opt_segments[0].opt_upper) 
			{
			if (!relationship) 
				relationship = FB_NEW(*tdbb->tdbb_default) IndexedRelationship;

			if (idx->idx_flags & idx_unique) 
				{
				relationship->irl_unique = TRUE;
				break;
				}
			}
		}

	return relationship;
}


static RecordSource* make_cross(thread_db* tdbb, OptimizerBlk* opt, RiverStack& stack)
{
/**************************************
 *
 *	m a k e _ c r o s s
 *
 **************************************
 *
 * Functional description
 *	Generate a cross block.
 *
 **************************************/
	DEV_BLKCHK(opt, type_opt);
	//SET_TDBB(tdbb);
	
	const int count = stack.getCount();
	
	if (count == 1) 
		return stack.pop()->riv_rsb;

	CompilerScratch* csb = opt->opt_csb;

	//RecordSource* rsb = FB_NEW_RPT(*tdbb->tdbb_default, count) RecordSource(opt->opt_csb);

	RsbCross *rsb = new (tdbb->tdbb_default) RsbCross(csb, count);

	//rsb->rsb_type = rsb_cross;
	//rsb->rsb_count = count;

	rsb->rsb_impure = CMP_impure(csb, sizeof(struct irsb));
	
	/***
	RecordSource** ptr = rsb->rsb_arg + count;
	while (stack.hasData()) 
		*--ptr = stack.pop()->riv_rsb;
	***/
	
	for (int n = count - 1; n >= 0; --n)
		rsb->rsbs[n] = stack.pop()->riv_rsb;

	return rsb;
}


static JRD_NOD make_index_node(thread_db* tdbb, 
							   Relation* relation,
							   CompilerScratch* csb, 
							   const index_desc* idx)
{
/**************************************
 *
 *	m a k e _ i n d e x _ n o d e
 *
 **************************************
 *
 * Functional description
 *	Make an index node and an index retrieval block.
 *
 **************************************/
	//DEV_BLKCHK(relation, type_rel);
	//DEV_BLKCHK(csb, type_csb);
	//SET_TDBB(tdbb);
	
	// check whether this is during a compile or during
	// a SET INDEX operation 
	  
	if (csb)
		//CMP_post_resource(tdbb, &csb->csb_resources, reinterpret_cast < BLK > (relation),  Resource::rsc_index, idx->idx_id);
		csb->postResource(new Resource(relation));
	else
		//CMP_post_resource(tdbb, &tdbb->tdbb_request->req_resources,reinterpret_cast < BLK > (relation), Resource::rsc_index, idx->idx_id);
		tdbb->tdbb_request->postResource(new Resource(relation));
		
	jrd_nod* node = PAR_make_node(tdbb, e_idx_length);
	node->nod_type = nod_index;
	node->nod_count = 0;
	IndexRetrieval* retrieval = FB_NEW_RPT(*tdbb->tdbb_default, idx->idx_count * 2) IndexRetrieval();
	node->nod_arg[e_idx_retrieval] = (jrd_nod*) retrieval;
	retrieval->irb_index = idx->idx_id;
	MOVE_FAST(idx, &retrieval->irb_desc, sizeof(retrieval->irb_desc));
	
	if (csb)
		node->nod_impure = CMP_impure(csb, sizeof(impure_inversion));
		
	return node;
}


static JRD_NOD make_inference_node(thread_db* tdbb, 
								   CompilerScratch* csb, 
								   jrd_nod* boolean, 
								   jrd_nod* arg1, 
								   jrd_nod* arg2)
{
/**************************************
 *
 *	m a k e _ i n f e r e n c e _ n o d e
 *
 **************************************
 *
 * Defined
 *	1996-Jan-15 David Schnepper 
 *
 * Functional description
 *	From the predicate, boolean, and infer a new
 *	predicate using arg1 & arg2 as the first two
 *	parameters to the predicate.
 *
 *	This is used when the engine knows A<B and A=C, and
 *	creates a new node to represent the infered knowledge C<B.
 *
 *	Note that this may be sometimes incorrect with 3-value
 *	logic (per Chris Date's Object & Relations seminar).
 *	Later stages of query evaluation evaluate exactly
 *	the originally specified query, so 3-value issues are
 *	caught there.  Making this inference might cause us to
 *	examine more records than needed, but would not result
 *	in incorrect results.
 *
 *	Note that some nodes, specifically nod_like, have
 *	more than two parameters for a boolean operation.
 *	(nod_like has an optional 3rd parameter for the ESCAPE character
 *	 option of SQL)
 *	Nod_sleuth also has an optional 3rd parameter (for the GDML
 *	matching ESCAPE character language).  But nod_sleuth is 
 *	(apparently) not considered during optimization.
 *
 *
 **************************************/
	DEV_BLKCHK(csb, type_csb);
	DEV_BLKCHK(boolean, type_nod);
	DEV_BLKCHK(arg1, type_nod);
	DEV_BLKCHK(arg2, type_nod);
	fb_assert(boolean->nod_count >= 2);	// must be a conjunction boolean 

	// Clone the input predicate 

	jrd_nod* node = PAR_make_node(tdbb, boolean->nod_count);
	node->nod_type = boolean->nod_type;

	// We may safely copy invariantness flag because
	// (1) we only distribute field equalities
	// (2) invariantness of second argument of STARTING WITH or LIKE is solely
	//    determined by its dependency on any of the fields
	// If provisions above change the line below will have to be modified

	node->nod_flags = boolean->nod_flags;

	// But substitute new values for some of the predicate arguments 

	node->nod_arg[0] = CMP_clone_node(tdbb, csb, arg1);
	node->nod_arg[1] = CMP_clone_node(tdbb, csb, arg2);

	// Arguments after the first two are just cloned (eg: LIKE ESCAPE clause) 

	for (USHORT n = 2; n < boolean->nod_count; n++)
		node->nod_arg[n] = CMP_clone_node(tdbb, csb, boolean->nod_arg[n]);

	// Share impure area for cached invariant value used to hold pre-compiled
	// pattern for new LIKE and CONTAINING algorithms.
	// Proper cloning of impure area for this node would require careful accounting
	// of new invariant dependencies - we avoid such hassles via using single 
	// cached pattern value for all node clones. This is faster too.

	if (node->nod_flags & nod_invariant)
		node->nod_impure = boolean->nod_impure;

	return node;
}


static JRD_NOD make_inversion(thread_db* tdbb, 
							  OptimizerBlk* opt,
							  jrd_nod* boolean, 
							  USHORT stream)
{
/**************************************
 *
 *	m a k e _ i n v e r s i o n
 *
 **************************************
 *
 * Functional description
 *	Build an inversion for a boolean, if possible.  Otherwise,
 *	return NULL.  Make inversion is call initially from 
 *	gen_retrieval to handle "or" nodes, but may be called
 *	recursively for almost anything.
 *
 **************************************/
	SET_TDBB(tdbb);
	DEV_BLKCHK(opt, type_opt);
	DEV_BLKCHK(boolean, type_nod);

	CompilerScratch::csb_repeat* csb_tail = &opt->opt_csb->csb_rpt[stream];
	Relation* relation = csb_tail->csb_relation;

	if ((!relation) || (relation->rel_file)) 
		return NULL;

	// Handle the "OR" case up front

	jrd_nod* inversion;

	if (boolean->nod_type == nod_or)
		{
		inversion = make_inversion(tdbb, opt, boolean->nod_arg[0], stream);

		if (!inversion)
			return NULL;

		jrd_nod* inversion2 = make_inversion(tdbb, opt, boolean->nod_arg[1], stream);

		if (inversion2)
			{
			USHORT invIdx1 = reinterpret_cast<IndexRetrieval*>(inversion->nod_arg[e_idx_retrieval])->irb_index;
			USHORT invIdx2 = reinterpret_cast<IndexRetrieval*>(inversion2->nod_arg[e_idx_retrieval])->irb_index;
			USHORT bitInIdx1 = reinterpret_cast<IndexRetrieval*>(inversion->nod_arg[1]->nod_arg[e_idx_retrieval])->irb_index;

			if ((inversion->nod_type == nod_index) 
					&& (inversion2->nod_type == nod_index) 
					&& (invIdx1 == invIdx2))
				return compose(tdbb, &inversion, inversion2, nod_bit_in);
		    else if ((inversion->nod_type == nod_bit_in) 
					&& (inversion2->nod_type == nod_index) 
					&& (bitInIdx1 == invIdx2))
    			return compose(tdbb, &inversion, inversion2, nod_bit_in);
			else
				return compose(tdbb, &inversion, inversion2, nod_bit_or);
			}

		if (inversion->nod_type == nod_index)
			delete inversion->nod_arg[e_idx_retrieval];

		delete inversion;
		return NULL;
		}

	// Time to find inversions.  For each index on the relation
	// match all unused booleans against the index looking for upper
	// and lower bounds that can be computed by the index.  When
	// all unused conjunctions are exhausted, see if there is enough
	// information for an index retrieval.  If so, build up and
	// inversion component of the boolean.

	// AB: If the boolean is a part of an earlier created index 
	// retrieval check with the compound_selectivity if it's 
	// really interesting to use.

	inversion = NULL;
	bool accept_starts = true;
	bool accept_missing = true;
	bool used_in_compound = false;
	float compound_selectivity = 1; // Real maximum selectivity possible is 1.

	firebird::HalfStaticArray<index_desc*, OPT_STATIC_ITEMS> 
		idx_walk_vector(*tdbb->tdbb_default);
	idx_walk_vector.grow(csb_tail->csb_indices);
	index_desc** idx_walk = idx_walk_vector.begin();
	firebird::HalfStaticArray<UINT64, OPT_STATIC_ITEMS> 
		idx_priority_level_vector(*tdbb->tdbb_default);
	idx_priority_level_vector.grow(csb_tail->csb_indices);
	UINT64* idx_priority_level = idx_priority_level_vector.begin();

	index_desc* idx = csb_tail->csb_idx->items;

	if (opt->opt_base_conjuncts) 
		{
		for (SSHORT i = 0; i < csb_tail->csb_indices; i++) 
			{
			idx_walk[i] = idx;
			idx_priority_level[i] = LOWEST_PRIORITY_LEVEL;

			clear_bounds(opt, idx);

			// If we've got a match, calculate the priority level of this index.

			if (match_index(tdbb, opt, stream, boolean, idx) 
					&& !(idx->idx_runtime_flags & idx_plan_dont_use)) 
				idx_priority_level[i] = calculate_priority_level(tdbb, opt, idx);

			// If the index was already used in an AND node check
			// if this node is also present in this index

			if (idx->idx_runtime_flags & idx_used_with_and) 
				{
				if ((match_index(tdbb, opt, stream, boolean, idx)) 
						&& (idx->idx_selectivity < compound_selectivity)) 
					{
					compound_selectivity = idx->idx_selectivity;
					used_in_compound = true;
					}
				}

			// Because indices are already sort based on their selectivity
			// it's not needed to more then 1 index for a node

			if ((boolean->nod_type == nod_starts) && accept_starts) 
				{
				jrd_nod* node = make_starts(tdbb, opt, relation, boolean, stream, idx);

				if (node) 
					{
					compose(tdbb, &inversion, node, nod_bit_and);
					accept_starts = false;
					}
				}

			if ((boolean->nod_type == nod_missing) && accept_missing) 
				{
				jrd_nod* node = make_missing(tdbb, opt, relation, boolean, stream, idx);

				if (node) 
					{
					compose(tdbb, &inversion, node, nod_bit_and);
					accept_missing = false;
					}
				}

			++idx;
			}
		}

	// Sort indices based on the priority level into idx_walk

	const SSHORT idx_walk_count = sort_indices_by_priority(tdbb, csb_tail, 
											idx_walk, idx_priority_level);

	bool accept = true;
	idx = csb_tail->csb_idx->items;

	if (opt->opt_base_conjuncts) 
		{
		for (SSHORT i = 0; i < idx_walk_count; i++) 
			{
			idx = idx_walk[i];

			if (idx->idx_runtime_flags & idx_plan_dont_use) 
				continue;

			clear_bounds(opt, idx);

			if (((accept || used_in_compound)  
					&& (idx->idx_selectivity < compound_selectivity * OR_SELECTIVITY_THRESHOLD_FACTOR))
					|| (csb_tail->csb_plan)) 
				{
				match_index(tdbb, opt, stream, boolean, idx);

				if (opt->opt_segments[0].opt_lower || opt->opt_segments[0].opt_upper) 
					{
					compose(tdbb, &inversion, OPT_make_index(tdbb, opt, relation, idx),
																nod_bit_and);
					accept = false;
					}				
				}
			}
		}

	if (!inversion)
		inversion = OPT_make_dbkey(tdbb, opt, boolean, stream);

	return inversion;
}


static JRD_NOD make_missing(thread_db* tdbb,
						OptimizerBlk* opt,
						Relation* relation,
						jrd_nod* boolean, 
						USHORT stream, 
						index_desc* idx)
{
/**************************************
 *
 *	m a k e _ m i s s i n g
 *
 **************************************
 *
 * Functional description
 *	If the a given boolean is an index optimizable, build and 
 *	return a inversion type node.  Indexes built before minor
 *	version 3 (V3.2) have unreliable representations for missing
 *	character string fields, so they won't be used.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->tdbb_database;

	DEV_BLKCHK(opt, type_opt);
	DEV_BLKCHK(relation, type_rel);
	DEV_BLKCHK(boolean, type_nod);

	jrd_nod* field = boolean->nod_arg[0];

	if (idx->idx_flags & idx_expressn)
		{
		fb_assert(idx->idx_expression != NULL);

		if (!OPT_expression_equal(tdbb, opt, idx, field, stream))
			return NULL;
		}
	else
		{
		if (field->nod_type != nod_field)
			return NULL;

		if ((USHORT)(long) field->nod_arg[e_fld_stream] != stream ||
				(USHORT)(long) field->nod_arg[e_fld_id] != idx->idx_rpt[0].idx_field)	
			return NULL;
		}
		
	jrd_nod* node = make_index_node(tdbb, relation, opt->opt_csb, idx);
	IndexRetrieval* retrieval = (IndexRetrieval*) node->nod_arg[e_idx_retrieval];
	retrieval->irb_relation = relation;

	// irb_starting? Why? Because we don't know exact length for the field.
	// For ascending NULLs in ODS 11 and higher this doesn't matter, because
	// a NULL is always stored as a key with length 0 (zero).

	if ((dbb->dbb_ods_version < ODS_VERSION11) || (idx->idx_flags & idx_descending)) 
		retrieval->irb_generic = irb_starting;

	retrieval->irb_lower_count = retrieval->irb_upper_count = 1;

	// If we are matching less than the full index, this is a partial match

	if (retrieval->irb_upper_count < idx->idx_count)
		retrieval->irb_generic |= irb_partial;

	// Set descending flag on retrieval if index is descending

	if (idx->idx_flags & idx_descending) 
		retrieval->irb_generic |= irb_descending;

	jrd_nod* value = PAR_make_node(tdbb, 0);
	retrieval->irb_value[0] = retrieval->irb_value[idx->idx_count] = value;
	value->nod_type = nod_null;
	idx->idx_runtime_flags |= idx_plan_missing;

	return node;
}


static JRD_NOD make_starts(thread_db* tdbb,
						   OptimizerBlk* opt,
						   Relation* relation, 
						   jrd_nod* boolean, 
						   USHORT stream, 
						   index_desc* idx)
{
/**************************************
 *
 *	m a k e _ s t a r t s
 *
 **************************************
 *
 * Functional description
 *	If the given boolean is an index optimizable, build and 
 *	return a inversion type node.
 *
 **************************************/
	SET_TDBB(tdbb);

	DEV_BLKCHK(opt, type_opt);
	DEV_BLKCHK(relation, type_rel);
	DEV_BLKCHK(boolean, type_nod);

	if (boolean->nod_type != nod_starts)
		return NULL;

	jrd_nod* field = boolean->nod_arg[0];
	jrd_nod* value = boolean->nod_arg[1];

	if (idx->idx_flags & idx_expressn)
		{
		fb_assert(idx->idx_expression != NULL);

		if (!(OPT_expression_equal(tdbb, opt, idx, field, stream) 
				&& OPT_computable(opt->opt_csb, value, stream, true, false)))
			{
			if (OPT_expression_equal(tdbb, opt, idx, value, stream) 
					&& OPT_computable(opt->opt_csb, field, stream, true, false))
				{
				field = value; 
				value = boolean->nod_arg[0]; 
				}
			else
				return NULL;
			}
		}
	else
		{
		if (field->nod_type != nod_field)
			{
			// dimitr:	any idea how we can use an index in this case?
			//			The code below produced wrong results.
			return NULL;
			/*
			if (value->nod_type != nod_field)
				return NULL;
			field = value;
			value = boolean->nod_arg[0];
			*/
			}

		// Every string starts with an empty string so
		// don't bother using an index in that case.

		if (value->nod_type == nod_literal)
			{
			const dsc* literal_desc = &((Literal*) value)->lit_desc;

			if ((literal_desc->dsc_dtype == dtype_text 
					&& literal_desc->dsc_length == 0) 
					|| (literal_desc->dsc_dtype == dtype_varying 
					&& literal_desc->dsc_length == sizeof(USHORT)))
				return NULL;

			}

		if ((USHORT)(long) field->nod_arg[e_fld_stream] != stream
				|| 	(USHORT)(long) field->nod_arg[e_fld_id] != idx->idx_rpt[0].idx_field
				|| !(idx->idx_rpt[0].idx_itype == idx_string
				|| idx->idx_rpt[0].idx_itype == idx_byte_array
				|| idx->idx_rpt[0].idx_itype == idx_metadata
				|| idx->idx_rpt[0].idx_itype >= idx_first_intl_string)
				|| !OPT_computable(opt->opt_csb, value, stream, false, false))
			return NULL;
		}

	jrd_nod* node = make_index_node(tdbb, relation, opt->opt_csb, idx);
	IndexRetrieval* retrieval = (IndexRetrieval*) node->nod_arg[e_idx_retrieval];
	retrieval->irb_relation = relation;
	retrieval->irb_generic = irb_starting;

	// STARTING WITH can never include NULL values, thus ignore 
	// them already at index scan

	retrieval->irb_generic |= irb_ignore_null_value_key;

	retrieval->irb_lower_count = retrieval->irb_upper_count = 1;

	// If we are matching less than the full index, this is a partial match

	if (retrieval->irb_upper_count < idx->idx_count) 
		retrieval->irb_generic |= irb_partial;

	// Set descending flag on retrieval if index is descending
	if (idx->idx_flags & idx_descending) 
		retrieval->irb_generic |= irb_descending;

	retrieval->irb_value[0] = retrieval->irb_value[idx->idx_count] = value;
	idx->idx_runtime_flags |= idx_plan_starts;

	return node;
}


static bool map_equal(const jrd_nod* field1, 
					  const jrd_nod* field2, 
					  const jrd_nod* map)
{
/**************************************
 *
 *	m a p _ e q u a l
 *
 **************************************
 *
 * Functional description
 *	Test to see if two fields are equal, where the fields 
 *	are in two different streams possibly mapped to each other.
 *	Order of the input fields is important.
 *
 **************************************/
	DEV_BLKCHK(field1, type_nod);
	DEV_BLKCHK(field2, type_nod);
	DEV_BLKCHK(map, type_nod);

	if (field1->nod_type != nod_field) 
		return false;

	if (field2->nod_type != nod_field) 
		return false;


	// look through the mapping and see if we can find an equivalence.

	const jrd_nod* const* map_ptr = map->nod_arg;
	const jrd_nod* const* const map_end = map_ptr + map->nod_count;

	for (; map_ptr < map_end; map_ptr++) 
		{
		const jrd_nod* map_from = (*map_ptr)->nod_arg[e_asgn_from];
		const jrd_nod* map_to = (*map_ptr)->nod_arg[e_asgn_to];

		if (map_from->nod_type != nod_field || map_to->nod_type != nod_field)
			continue;

		if (field1->nod_arg[e_fld_stream] != map_from->nod_arg[e_fld_stream]
				|| field1->nod_arg[e_fld_id] != map_from->nod_arg[e_fld_id])
			continue;

		if (field2->nod_arg[e_fld_stream] != map_to->nod_arg[e_fld_stream]
				|| field2->nod_arg[e_fld_id] != map_to->nod_arg[e_fld_id])
			continue;

		return true;
		}

	return false;
}



static void mark_indices(CompilerScratch::csb_repeat* csb_tail, 
						 SSHORT relation_id)
{
/**************************************
 *
 *	m a r k _ i n d i c e s
 *
 **************************************
 *
 * Functional description
 *	Mark indices that were not included 
 *	in the user-specified access plan.
 *
 **************************************/
	const jrd_nod* plan = csb_tail->csb_plan;

	if (!plan)
		return;

	if (plan->nod_type != nod_retrieve)
		return;

	// find out how many indices were specified; if
	// there were none, this is a sequential retrieval 

	USHORT plan_count = 0;
	const jrd_nod* access_type = plan->nod_arg[e_retrieve_access_type];

	if (access_type)
		plan_count = access_type->nod_count;

	// go through each of the indices and mark it unusable
	// for indexed retrieval unless it was specifically mentioned 
	// in the plan; also mark indices for navigational access 

	index_desc* idx = csb_tail->csb_idx->items;

	for (USHORT i = 0; i < csb_tail->csb_indices; i++) 
		{
		if (access_type) 
			{
			const jrd_nod* const* arg = access_type->nod_arg;
			const jrd_nod* const* const end = arg + plan_count;

			for (; arg < end; arg += e_access_type_length) 
				{
				if (relation_id != (SSHORT)(long) arg[e_access_type_relation])
					{
					// index %s cannot be used in the specified plan 
					const char* iname =
						reinterpret_cast<const char*>(arg[e_access_type_index_name]);
					ERR_post(isc_index_unused, isc_arg_string, ERR_cstring(iname), 0);
					}

				if (idx->idx_id == (USHORT)(long) arg[e_access_type_index])
					{
					// dimitr:	navigational access can use only one index,
					//			hence the extra check 
					if (access_type->nod_type == nod_navigational 
							&& arg == access_type->nod_arg)
						idx->idx_runtime_flags |= idx_plan_navigate;
					else  // nod_indices
						break;
					}
				}

			if (arg == end)
				idx->idx_runtime_flags |= idx_plan_dont_use;
			}
		else 
			idx->idx_runtime_flags |= idx_plan_dont_use;

		++idx;
		}
}


static SSHORT match_index(thread_db* tdbb,
						  OptimizerBlk* opt,
						  SSHORT stream, jrd_nod* boolean, 
						  const index_desc* idx)
{
/**************************************
 *
 *	m a t c h _ i n d e x
 *
 **************************************
 *
 * Functional description
 *	Match a boolean against an index location lower and upper
 *	bounds.  Return the number of relational nodes that were
 *	matched.  In ODS versions prior to 7, descending indexes
 *	were not reliable and will not be used.
 *
 **************************************/
	DEV_BLKCHK(opt, type_opt);
	DEV_BLKCHK(boolean, type_nod);
	SET_TDBB(tdbb);

	if (boolean->nod_type == nod_and)
		return match_index(tdbb, opt, stream, boolean->nod_arg[0], idx) 
				+ match_index(tdbb, opt, stream, boolean->nod_arg[1], idx);


	bool forward = true;

	jrd_nod* match = boolean->nod_arg[0];
	jrd_nod* value = (boolean->nod_count < 2) ? NULL : boolean->nod_arg[1];

	// If this is an expression see if one side or the other is matchable 
	// to the index expression

	if (idx->idx_flags & idx_expressn)
		{
	    fb_assert(idx->idx_expression != NULL);

		if (!OPT_expression_equal(tdbb, opt, idx, match, stream) 
				|| (value && !OPT_computable(opt->opt_csb, value, stream, true, false)))
			{
			if (value && OPT_expression_equal(tdbb, opt, idx, value, stream) 
					&& OPT_computable(opt->opt_csb, match, stream, true, false))
				{
				match = boolean->nod_arg[1];
				value = boolean->nod_arg[0];
				}
			else
				return 0;
			}
		}

	// If left side is not a field, swap sides. 
	// If left side is still not a field, give up

	else if (match->nod_type != nod_field 
				|| (USHORT)(long) match->nod_arg[e_fld_stream] != stream 
				|| (value && !OPT_computable(opt->opt_csb, value, stream, true, false)))
		{
		match = value;
		value = boolean->nod_arg[0];

		if (!match || match->nod_type != nod_field
					|| (USHORT)(long) match->nod_arg[e_fld_stream] != stream 
					|| !OPT_computable(opt->opt_csb, value, stream, true, false))
			return 0;

		forward = false;
		}

	// match the field to an index, if possible, and save the value to be matched 
	// as either the lower or upper bound for retrieval, or both 

	SSHORT count = 0;
	SSHORT i = 0;
	OptimizerBlk::opt_segment* ptr = opt->opt_segments;

	for (; i < idx->idx_count; i++, ptr++)
		{
		if ((idx->idx_flags & idx_expressn) 
				|| (USHORT)(long) match->nod_arg[e_fld_id] == idx->idx_rpt[i].idx_field)
			{
			++count;

			// AB: If we have already an exact match don't 
			// override it with worser matches, but increment the 
			// count so that the node will be marked as matched! 

			if (ptr->opt_match && ptr->opt_match->nod_type == nod_eql) 
				break;

			switch (boolean->nod_type) 
				{
				case nod_between:
					if (!forward ||!OPT_computable(opt->opt_csb, boolean->nod_arg[2], 
													stream, true, false)) 
						return 0;

					ptr->opt_lower = value;
					ptr->opt_upper = boolean->nod_arg[2];
					ptr->opt_match = boolean;
					break;

				case nod_equiv:
				case nod_eql:
					ptr->opt_lower = ptr->opt_upper = value;
					ptr->opt_match = boolean;
					break;

				case nod_gtr:
				case nod_geq:
					if (forward) 
						ptr->opt_lower = value;
					else 
						ptr->opt_upper = value;

					ptr->opt_match = boolean;
					break;

				case nod_lss:
				case nod_leq:
					if (forward)
						ptr->opt_upper = value;
					else
						ptr->opt_lower = value;

					ptr->opt_match = boolean;
					break;

				default:    // Shut up compiler warnings 
					break;
				}
			}
		}

	return count;
}


static bool match_indices(thread_db* tdbb,
						  OptimizerBlk* opt,
						  SSHORT stream, jrd_nod* boolean, 
						  const index_desc* idx)
{
/**************************************
 *
 *	m a t c h _ i n d i c e s
 *
 **************************************
 *
 * Functional description
 *	Match a boolean against an index location lower and upper
 *	bounds.  Return the number of relational nodes that were
 *	matched.  In ODS versions prior to 7, descending indexes
 *	were not reliable and will not be used.
 *
 **************************************/
	DEV_BLKCHK(opt, type_opt);
	DEV_BLKCHK(boolean, type_nod);
	SET_TDBB(tdbb);

	if (boolean->nod_count < 2) 
		return false;

	if (boolean->nod_type == nod_or) 
		{
		if (match_indices(tdbb, opt, stream, boolean->nod_arg[0], idx) &&
			match_indices(tdbb, opt, stream, boolean->nod_arg[1], idx)) 
			{
			opt->opt_segments[0].opt_match = NULL;
			return true;
			}
		}
	else 
		{
		if (match_index(tdbb, opt, stream, boolean, idx)) 
			{
			opt->opt_segments[0].opt_match = NULL;
			return true;
			}
		}

	opt->opt_segments[0].opt_match = NULL;
	opt->opt_segments[0].opt_upper = NULL;
	opt->opt_segments[0].opt_lower = NULL;
	return false;
}


static bool node_equality(const jrd_nod* node1, const jrd_nod* node2)
{
/**************************************
 *
 *	n o d e _ e q u a l i t y
 *
 **************************************
 *
 * Functional description
 *	Test two field node pointers for symbolic equality.
 *
 **************************************/
	DEV_BLKCHK(node1, type_nod);
	DEV_BLKCHK(node2, type_nod);

	if (!node1 || !node2) 
		return false;

	if (node1->nod_type != node2->nod_type) 
		return false;

	if (node1 == node2)
		return true;

	switch (node1->nod_type) 
		{
		case nod_field:
			return (node1->nod_arg[e_fld_stream] == node2->nod_arg[e_fld_stream]
					&& node1->nod_arg[e_fld_id] == node2->nod_arg[e_fld_id]);

		case nod_equiv:
		case nod_eql:
			if (node_equality(node1->nod_arg[0], node2->nod_arg[0])
					&& node_equality(node1->nod_arg[1], node2->nod_arg[1])) 
				return true;

			if (node_equality(node1->nod_arg[0], node2->nod_arg[1])
					&& node_equality(node1->nod_arg[1], node2->nod_arg[0]))
				return true;

			return false;

		case nod_gtr:
		case nod_geq:
		case nod_leq:
		case nod_lss:
		case nod_matches:
		case nod_contains:
		case nod_like:
		default:
			break;
		}

	return false;
}


static jrd_nod* optimize_like(thread_db* tdbb, jrd_nod* like_node)
{
/**************************************
 *
 *	o p t i m i z e _ l i k e
 *
 **************************************
 *
 * Functional description
 *	Optimize a LIKE expression, if possible,
 *	into a "starting with" AND a "like".  This
 *	will allow us to use the index for the
 *	starting with, and the LIKE can just tag
 *	aSLONG for the ride.
 *	But on the ride it does useful work, consider
 *	match LIKE "ab%c".  This is optimized by adding
 *	AND starting_with "ab", but the LIKE clause is
 *	still needed.
 *
 **************************************/
	UCHAR tmp_buffer[32];		// large enough to hold 1 ch of escape string
	SET_TDBB(tdbb);
	DEV_BLKCHK(like_node, type_nod);
	
	jrd_nod* search_node = like_node->nod_arg[1];
	jrd_nod* escape_node = (like_node->nod_count > 2) ? like_node->nod_arg[2] : NULL;

	// if the matching string or the escape string can't 
	// be evaluated at compile time, forget it 

	if ((search_node->nod_type != nod_literal) 
			|| (escape_node && escape_node->nod_type != nod_literal))
		return NULL;

	dsc* search_desc = &((Literal*) search_node)->lit_desc;
	dsc* escape_desc = 0;

	if (escape_node)
		escape_desc = &((Literal*) escape_node)->lit_desc;

	//  if either is not a character expression, forget it 

	if ((search_desc->dsc_dtype > dtype_any_text) 
			|| (escape_node && escape_desc->dsc_dtype > dtype_any_text))
		return NULL;

	//  Get the escape character, if any 
	TextType text_obj = NULL;
	USHORT escape_ch = 0;

	// Ensure escape string is same character set as search string 
	// Error report on Overflow is OK, as sql only allows a single
	// character escape string 

	if (escape_node)
		{
		const char* p2;
		USHORT p_count =
			MOV_make_string(escape_desc, INTL_TTYPE(search_desc), &p2,
							reinterpret_cast<vary*>(tmp_buffer),
							sizeof(tmp_buffer));
		UCHAR* p = reinterpret_cast<UCHAR*>(const_cast<char*>(p2));

		// Now get first character from escape string 

		escape_ch = INTL_getch(tdbb, &text_obj, INTL_TTYPE(search_desc), 
								&p, &p_count);
		}

	// If the first character is a wildcard char, forget it.
	// CVC: Beware, if INTL doesn't understand the next character, p_count is zero.
	// No check is done. We must handle this failure.

	UCHAR* p = search_desc->dsc_address;
	USHORT p_count = search_desc->dsc_length;
	USHORT ch = INTL_getch(tdbb, &text_obj, INTL_TTYPE(search_desc), 
							&p, &p_count);

	if ((!escape_node || ch != escape_ch)
			&& (ch == SQL_MATCH_1_CHAR || ch == SQL_MATCH_ANY_CHARS))
		return NULL;

	// allocate a literal node to store the starting with string;
	// assume it will be shorter than the search string
	// CVC: This assumption may not be true if we use "value like field".

	const SSHORT count = lit_delta + 
		(search_desc->dsc_length + sizeof(jrd_nod*) - 1) / sizeof(jrd_nod*);
	jrd_nod* node = PAR_make_node(tdbb, count);
	node->nod_type = nod_literal;
	node->nod_count = 0;
	Literal* literal = (Literal*) node;
	literal->lit_desc = *search_desc;
	UCHAR* q = reinterpret_cast<UCHAR*>(literal->lit_data);
	literal->lit_desc.dsc_address = q;

	// copy the string into the starting with literal, 
	// up to the first wildcard character 

	p_count = search_desc->dsc_length;
	p = search_desc->dsc_address;

	// if there are escape characters, skip past them and
	// don't treat the next char as a wildcard
	// CVC: Beware, if INTL doesn't understand the next character, 
	// p_count is zero. No check is done.
	// Same than above, but worse, since it will enter an infinite loop. 
	// Example: Spanish � against unicode.
	// Reported also on Portuguese characters that aren't ASCII < 128.
	// TBC: Added tests for infinite loop.  These may be replaced/updated
	//      when updated INTL support comes to Vulcan.

	for (const UCHAR* const end = p + search_desc->dsc_length; p < end;)
		{
		const UCHAR* p_start = p;
		ch = INTL_getch(tdbb, &text_obj, INTL_TTYPE(search_desc), 
						&p, &p_count);

		/*
		 * Check for transcoding error that would result in a hang.
		 */
		if (p == p_start)
			{
			delete node;
			ERR_post(isc_arith_except, isc_arg_gds, isc_transliteration_failed, 0 );
			}

		// Check for Escape character at end of string 

		if (escape_node && (ch == escape_ch))
			{
			if (!(p < end))
				break;

			p_start = p;
			ch = INTL_getch(tdbb, &text_obj, INTL_TTYPE(search_desc), 
							&p, &p_count);
			if (p == p_start)
				{ /* Transcode error check again */
				delete node;
				ERR_post(isc_arith_except, isc_arg_gds, isc_transliteration_failed, 0 );
				}
			}
		else if (ch == SQL_MATCH_1_CHAR || ch == SQL_MATCH_ANY_CHARS)
			break;

		// copy the bytes of the character 

		while (p_start < p) 
			*q++ = *p_start++;
		}

	literal->lit_desc.dsc_length = q - literal->lit_desc.dsc_address;
	return node;
}



#ifdef OPT_DEBUG
static void print_order(const OptimizerBlk* opt,
						USHORT position, 
						double cardinality, 
						double cost)
{
/**************************************
 *
 *	p r i n t _ o r d e r
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	DEV_BLKCHK(opt, type_opt);
	ib_fprintf(opt_debug_file, "print_order() -- position %2.2d: ", position);
	const OptimizerBlk::opt_stream* tail = opt->opt_streams.begin();
	const OptimizerBlk::opt_stream* const order_end = opt->opt_streams.begin() + position;

	for (; tail < order_end; tail++)
		ib_fprintf(opt_debug_file, "stream %2.2d, ", tail->opt_stream_number);

	ib_fprintf(opt_debug_file, "\n\t\t\tcardinality: %g\tcost: %g\n",
			   cardinality, cost);
}
#endif


static USHORT river_count(USHORT count, 
						  jrd_nod** eq_class)
{
/**************************************
 *
 *	r i v e r _ c o u n t
 *
 **************************************
 *
 * Functional description
 *	Given an sort/merge join equivalence class (vector of node pointers
 *	of representative values for rivers), return the count of rivers
 *	with values.
 *
 **************************************/
#ifdef DEV_BUILD
/* DEV_BLKCHK is null and causes a compiler warning
	if (*eq_class)
		DEV_BLKCHK(*eq_class, type_nod);
*/
#endif
	USHORT cnt = 0;

	for (USHORT i = 0; i < count; i++, eq_class++)
		if (*eq_class) 
			{
			cnt++;
			DEV_BLKCHK(*eq_class, type_nod);
			}

	return cnt;
}


static bool river_reference(const River* river, 
							const jrd_nod* node, 
							bool* field_found)
{
/**************************************
 *
 *	r i v e r _ r e f e r e n c e
 *
 **************************************
 *
 * Functional description
 *	See if a value node is a reference to a given river.
 *  AB: Handle also expressions (F1 + F2 * 3, etc..)
 *  The expression is checked if all fields that are
 *  buried inside are pointing to the the given river.
 *  If a passed field isn't referenced by the river then
 *  we have an expression with 2 fields pointing to
 *  different rivers and then the result is always false.
 *  NOTE! The first time this function is called 
 *  field_found should be NULL.
 *
 **************************************/
	DEV_BLKCHK(river, type_riv);
	DEV_BLKCHK(node, type_nod);

	bool lfield_found = false;
	bool root_caller = false;

	// If no boolean parameter is given then this is the first call 
	// to this function and we use the local boolean to pass to 
	// itselfs. The boolean is used to see if any field has passed
	// that references to the river.

	if (!field_found) 
		{
		root_caller = true;
		field_found = &lfield_found;
		}

	switch (node->nod_type) 
		{
		case nod_field :
			{
			// Check if field references to the river.

			const UCHAR* streams = river->riv_streams;
			const UCHAR* const end = streams + river->riv_count;

			for (; streams < end; streams++) 
				{
				if ((USHORT)(long) node->nod_arg[e_fld_stream] == *streams) 
					{
					*field_found = true;
					return true;
					}
				}

			return false;
			}

		default :
			{
			// Check all sub-nodes of this node.

			const jrd_nod* const* ptr = node->nod_arg;
			const jrd_nod* const* const end = ptr + node->nod_count;

			for (; ptr < end; ptr++)
				if (!river_reference(river, *ptr, field_found)) 
					return false;

			// if this was the first call then field_found tells 
			// us if any field (referenced by river) was found.

			if (root_caller) 
				return *field_found;
			else 
				return true;
			}
		}
/*  
	// AB: Original code FB1.0 , just left as reference for a while
	UCHAR *streams, *end;
	DEV_BLKCHK(river, type_riv);
	DEV_BLKCHK(node, type_nod);
	if (node->nod_type != nod_field)
		return false;
	for (streams = river->riv_streams, end =
		 streams + river->riv_count; streams < end; streams++)
		if ((USHORT) node->nod_arg[e_fld_stream] == *streams)
			return true;
	return false; 
*/
	return false;
}


static bool search_stack(const jrd_nod* node, 
						 const NodeStack& stack)
{
/**************************************
 *
 *	s e a r c h _ s t a c k
 *
 **************************************
 *
 * Functional description
 *	Search a stack for the presence of a particular value.
 *
 **************************************/
	DEV_BLKCHK(node, type_nod);

	for (NodeStack::const_iterator iter(stack); iter.hasData(); ++iter) 
		if (node_equality(node, iter.object())) 
			return true;

	return false;
}


static void set_active(OptimizerBlk* opt, 
					   const River* river)
{
/**************************************
 *
 *	s e t _ a c t i v e
 *
 **************************************
 *
 * Functional description
 *	Set a group of streams active.
 *
 **************************************/
	DEV_BLKCHK(opt, type_opt);
	DEV_BLKCHK(river, type_riv);
	CompilerScratch* csb = opt->opt_csb;
	const UCHAR* streams = river->riv_streams;
	const UCHAR* const end = streams + river->riv_count;

	for (; streams < end; streams++)
		csb->csb_rpt[*streams].csb_flags |= csb_active;
}


static void set_direction(const jrd_nod* from_clause, jrd_nod* to_clause)
{
/**************************************
 *
 *	s e t _ d i r e c t i o n
 *
 **************************************
 *
 * Functional description
 *	Update the direction of a GROUP BY, DISTINCT, or ORDER BY
 *	clause to the same direction as another clause. Do the same
 *  for the nulls placement flag.
 *
 **************************************/
	DEV_BLKCHK(from_clause, type_nod);
	DEV_BLKCHK(to_clause, type_nod);

	// Both clauses are allocated with thrice the number of arguments to
	// leave room at the end for an ascending/descending and nulls placement flags,
	// one for each field.

	jrd_nod* const* from_ptr = from_clause->nod_arg;
	jrd_nod** to_ptr = to_clause->nod_arg;
	const jrd_nod* const* const end = from_ptr + from_clause->nod_count;

	for (;from_ptr < end; from_ptr++, to_ptr++)
	{
		*(to_ptr + to_clause->nod_count) = *(from_ptr + from_clause->nod_count);
		*(to_ptr + to_clause->nod_count * 2) = *(from_ptr + from_clause->nod_count * 2);
	}
}



static void set_inactive(OptimizerBlk* opt, 
						 const River* river)
{
/**************************************
 *
 *	s e t _ i n a c t i v e
 *
 **************************************
 *
 * Functional description
 *	Set a group of streams inactive.
 *
 **************************************/
	DEV_BLKCHK(opt, type_opt);
	DEV_BLKCHK(river, type_riv);
	CompilerScratch* csb = opt->opt_csb;
	const UCHAR* streams = river->riv_streams;
	const UCHAR* const end = streams + river->riv_count;

	for (;  streams < end; streams++)
		csb->csb_rpt[*streams].csb_flags &= ~csb_active;
}


static void set_made_river(OptimizerBlk* opt, const River* river)
{
/**************************************
 *
 *	s e t _ m a d e _ r i v e r
 *
 **************************************
 *
 * Functional description
 *      Mark all the streams in a river with the csb_made_river flag.
 * 
 *      A stream with this flag set, incicates that this stream has
 *      already been made into a river. Currently, this flag is used
 *      in computable() to decide if we can use the an index to
 *      optimise retrieving the streams involved in the conjunct.
 *
 *      We can use an index in retrieving the streams involved in a
 *      conjunct if both of the streams are currently active or have
 *      been processed (and made into rivers) before.
 *
 **************************************/
	DEV_BLKCHK(opt, type_opt);
	DEV_BLKCHK(river, type_riv);
	CompilerScratch* csb = opt->opt_csb;
	const UCHAR* streams = river->riv_streams;
	const UCHAR* const end = streams + river->riv_count;

	for (; streams < end; streams++)
		csb->csb_rpt[*streams].csb_flags |= csb_made_river;
}


static void set_position(const jrd_nod* from_clause, 
						 jrd_nod* to_clause, 
						 const jrd_nod* map)
{
/**************************************
 *
 *	s e t _ p o s i t i o n
 *
 **************************************
 *
 * Functional description
 *	Update the fields in a GROUP BY, DISTINCT, or ORDER BY 
 *	clause to the same position as another clause, possibly 
 *	using a mapping between the streams.
 *
 **************************************/
	DEV_BLKCHK(from_clause, type_nod);
	DEV_BLKCHK(to_clause, type_nod);
	DEV_BLKCHK(map, type_nod);

	// Track the position in the from list with "to_swap", and find the corresponding 
	// field in the from list with "to_ptr", then swap the two fields.  By the time 
	// we get to the end of the from list, all fields in the to list will be reordered. 

	jrd_nod** to_swap = to_clause->nod_arg;
	const jrd_nod* const* from_ptr = from_clause->nod_arg;
	const jrd_nod* const* const from_end = from_ptr + from_clause->nod_count;

	for (; from_ptr < from_end; from_ptr++)
		{
		jrd_nod** to_ptr = to_clause->nod_arg;
		const jrd_nod* const* const to_end = to_ptr + from_clause->nod_count;

		for (; to_ptr < to_end; to_ptr++)
			{
			if ((map && map_equal(*to_ptr, *from_ptr, map))
					|| (!map 
						&& (*from_ptr)->nod_arg[e_fld_stream] == (*to_ptr)->nod_arg[e_fld_stream]
						&& (*from_ptr)->nod_arg[e_fld_id] == (*to_ptr)->nod_arg[e_fld_id]))
				{
				jrd_nod* swap = *to_swap;
				*to_swap = *to_ptr;
				*to_ptr = swap;
				}
			}

		to_swap++;
		}

}


static void set_rse_inactive(CompilerScratch* csb, 
							 const RecordSelExpr* rse)
{
/***************************************************
 *
 *  s e t _ r s e _ i n a c t i v e
 *
 ***************************************************
 *
 * Functional Description:
 *    Set all the streams involved in an rse as inactive. Do it recursively.
 *
 ***************************************************/
	const jrd_nod* const* ptr = rse->rse_relation;
	const jrd_nod* const* const end = ptr + rse->rse_count;

	for (; ptr < end; ptr++)
		{
		const jrd_nod* node = *ptr;

		if (node->nod_type != nod_rse) 
			{
			const SSHORT stream = (USHORT)(long) node->nod_arg[STREAM_INDEX(node)];
			csb->csb_rpt[stream].csb_flags &= ~csb_active;
			}
		else
			set_rse_inactive(csb, (const RecordSelExpr*) node);
		}
}


static void sort_indices_by_selectivity(thread_db* tdbb, 
										CompilerScratch::csb_repeat* csb_tail)
{
/***************************************************
 *
 *  s o r t _ i n d i c e s _ b y _ s e l e c t i v i t y
 *
 ***************************************************
 *
 * Functional Description:
 *    Sort indices based on there selectivity.
 *    Lowest selectivy as first, highest as last.
 *
 ***************************************************/

	if (csb_tail->csb_plan) 
		return;

	index_desc* selected_idx = NULL;
	USHORT i, j;
	firebird::Array<index_desc> idx_sort(tdbb->tdbb_default, csb_tail->csb_indices);
	bool same_selectivity = false;

	// Walk through the indices and sort them into into idx_sort
	// where idx_sort[0] contains the lowest selectivity (best) and
	// idx_sort[csb_tail->csb_indices - 1] the highest (worst)

	// Maximum selectivity is 1 (when all keys are the same)
	// Prefer ASC indices in the case of almost the same selectivities

	if (csb_tail->csb_idx && (csb_tail->csb_indices > 1)) 
		{
		for (j = 0; j < csb_tail->csb_indices; j++) 
			{
			float selectivity = 1; 
			index_desc* idx = csb_tail->csb_idx->items;

			for (i = 0; i < csb_tail->csb_indices; i++) 
				{
				if (selectivity > idx->idx_selectivity)
					same_selectivity = ((selectivity - idx->idx_selectivity) <= 0.00001);
				else 
					same_selectivity = ((idx->idx_selectivity - selectivity) <= 0.00001);

				if (!(idx->idx_runtime_flags & idx_marker)  
						&& (idx->idx_selectivity <= selectivity) 
						&& !((idx->idx_flags & idx_descending) && same_selectivity)) 
					{
					selectivity = idx->idx_selectivity;
					selected_idx = idx;
					}

				++idx;
				}

			// If no index was found than pick the first one available out of the list

			if ((!selected_idx) || (selected_idx->idx_runtime_flags & idx_marker)) 
				{
				idx = csb_tail->csb_idx->items;
				for (i = 0; i < csb_tail->csb_indices; i++) 
					{
					if (!(idx->idx_runtime_flags & idx_marker)) 
						{
						selected_idx = idx;
						break;
						}	
					++idx;
					}
				}

			selected_idx->idx_runtime_flags |= idx_marker;
			idx_sort.add(*selected_idx);
			}

		// Finally store the right order in cbs_tail->csb_idx

		index_desc* idx = csb_tail->csb_idx->items;

		for (j = 0; j < csb_tail->csb_indices; j++) 
			{
			idx->idx_runtime_flags &= ~idx_marker;
			MOVE_FAST(&idx_sort[j], idx, sizeof(index_desc));
			++idx;
			}
		}
}


static SSHORT sort_indices_by_priority(thread_db* tdbb, 
									   CompilerScratch::csb_repeat* csb_tail,
									   index_desc** idx_walk,
									   UINT64* idx_priority_level)
{
/***************************************************
 *
 *  s o r t _ i n d i c e s _ b y _ p r i o r i t y
 *
 ***************************************************
 *
 * Functional Description:
 *    Sort indices based on the priority level.
 *	  Real maximum selectivity possible is 1 
 *
 ***************************************************/
	firebird::HalfStaticArray<index_desc*, OPT_STATIC_ITEMS> idx_csb(tdbb->tdbb_default);
	idx_csb.grow(csb_tail->csb_indices);
	memcpy(idx_csb.begin(), idx_walk, csb_tail->csb_indices * sizeof(index_desc*));

	SSHORT idx_walk_count = 0;
	float selectivity = 1; 

	for (SSHORT i = 0; i < csb_tail->csb_indices; i++)
		{
		SSHORT last_idx = -1;
		UINT64 last_priority_level = 0;

		for (SSHORT j = csb_tail->csb_indices - 1; j >= 0; j--)
			{
			if (!(idx_priority_level[j] == 0) && 
				(idx_priority_level[j] >= last_priority_level))
				{
				last_priority_level = idx_priority_level[j];
				last_idx = j;
				}
			}

		// dimitr: it's better to use less indices with very good selectivity
		// than using all available ones. Here we're deciding how many indices we
		// should use. Since all indices are already ordered by their selectivity,
		// it becomes a trivial task. But note that indices with zero (unknown)
		// selectivity are always used, because we don't have a clue how useful
		// they are in fact, so we should be optimistic in this case. Unique
		// indices are also always used, because they are good by definition,
		// regardless of their (probably old) selectivity values. 

		if (last_idx >= 0) 
			{
			index_desc* idx = idx_csb[last_idx];
			bool should_be_used = true;

			if (idx->idx_selectivity && !(csb_tail->csb_plan)) 
				{
				if (!(idx->idx_flags & idx_unique) 
						&& (selectivity * SELECTIVITY_THRESHOLD_FACTOR < idx->idx_selectivity)) 
					should_be_used = false;

				selectivity = idx->idx_selectivity;
				}

			// Mark as used by setting priority_level to 0 

			idx_priority_level[last_idx] = 0; 

			if (should_be_used) 
				{
				idx_walk[idx_walk_count] = idx_csb[last_idx];
				idx_walk_count++;
				}
			}
		}

	return idx_walk_count;
}

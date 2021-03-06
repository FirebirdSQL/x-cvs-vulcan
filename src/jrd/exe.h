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

#ifndef JRD_EXE_H
#define JRD_EXE_H

#include "../jrd/jrd_blks.h"
#include "../common/classes/array.h"
#include "SVector.h"
#include "gen/iberror.h"

#define NODE(type, name, keyword) type,

typedef enum nod_t {
#include "../jrd/nod.h"
	nod_MAX
#undef NODE
} NOD_T;

#include "../jrd/dsc.h"
#include "../jrd/rse.h"

//#include "../jrd/err_proto.h"

// This macro enables DSQL tracing code
//#define CMP_DEBUG

#ifdef CMP_DEBUG
DEFINE_TRACE_ROUTINE(cmp_trace);
#define CMP_TRACE(args) cmp_trace args
#else
#define CMP_TRACE(args) /* nothing */
#endif

// NOTE: The definition of structures RecordSelExpr and lit must be defined in
//       exactly the same way as structure jrd_nod through item nod_count.
//       Now, inheritance takes care of those common data members.
class jrd_node_base : public pool_alloc_rpt<jrd_nod*, type_nod>
{
public:
	jrd_nod*	nod_parent;
	SLONG	nod_impure;			/* Inpure offset from request block */
	NOD_T	nod_type;				/* Type of node */
	UCHAR	nod_flags;
	SCHAR	nod_scale;			/* Target scale factor */
	USHORT	nod_count;			/* Number of arguments */
};


class jrd_nod : public jrd_node_base
{
public:
	jrd_nod*	nod_arg[1];
};

typedef jrd_nod* JRD_NOD;

const int nod_comparison = 1;
const int nod_id		= 1;		/* marks a field node as a blr_fid guy */
const int nod_quad		= 2;		/* compute in quad (default is long) */
const int nod_double	= 4;
const int nod_date		= 8;
const int nod_value		= 16;		/* full value area required in impure space */
const int nod_deoptimize	= 32;	/* boolean which requires deoptimization */
const int nod_agg_dbkey	= 64;		/* dbkey of an aggregate */
const int nod_invariant	= 128;		/* node is recognized as being invariant */


/* Special RecordSelExpr node */

class RecordSelExpr : public jrd_node_base
{
public:
	USHORT		rse_count;
	USHORT		rse_jointype;		/* inner, left, full */
	bool		rse_writelock;
	RecordSource*	rse_rsb;
	jrd_nod*	rse_first;
    jrd_nod*	rse_skip;
	jrd_nod*	rse_boolean;
	jrd_nod*	rse_sorted;
	jrd_nod*	rse_projection;
	jrd_nod*	rse_aggregate;	/* singleton aggregate for optimizing to index */
	jrd_nod*	rse_plan;		/* user-specified access plan */
	VarInvariantArray *rse_invariants; /* Invariant nodes bound to top-level RSE */
#ifdef SCROLLABLE_CURSORS
	jrd_nod*	rse_async_message;	/* asynchronous message to send for scrolling */
#endif
	firebird::Array<jrd_nod*> *rse_variables; // Variables and arguments this RSE depends on
	jrd_nod*	rse_relation[1];
};

//const int rse_stream	= 1;	/* flags RecordSelExpr-type node as a blr_stream type */
const int rse_singular	= 2;	/* flags RecordSelExpr-type node as from a singleton select */
const int rse_variant	= 4;	/* flags RecordSelExpr as variant (not invariant?) */

// Number of nodes may fit into nod_arg of normal node to get to rse_relation
const int rse_delta = (sizeof(RecordSelExpr) - sizeof(jrd_nod)) / sizeof(jrd_nod::blk_repeat_type);
//#define rse_delta	(sizeof(struct RecordSelExpr)-sizeof(struct jrd_nod))/sizeof(((jrd_nod*) NULL)->nod_arg[0])

// Types of nulls placement for each column in sort order
const int rse_nulls_default	= 0;
const int rse_nulls_first	= 1;
const int rse_nulls_last	= 2;


/* Literal value */

class Literal : public jrd_node_base
{
public:
	dsc		lit_desc;
	SINT64	lit_data[1]; // Defined this way to prevent SIGBUS error in 64-bit ports
};

const int lit_delta	= ((sizeof(Literal) - sizeof(jrd_nod) - sizeof(SINT64)) / sizeof(jrd_nod**));


/* Aggregate Sort Block (for DISTINCT aggregates) */

class AggregateSort : public pool_alloc<type_asb>
{
public:
	jrd_nod*	nod_parent;
	SLONG	nod_impure;			/* Impure offset from request block */
	NOD_T	nod_type;				/* Type of node */
	UCHAR	nod_flags;
	SCHAR	nod_scale;
	USHORT	nod_count;
	dsc		asb_desc;
	struct SortKeyDef* asb_key_desc;	/* for the aggregate   */
	UCHAR	asb_key_data[1];
};

const int asb_delta	= ((sizeof(AggregateSort) - sizeof(jrd_nod)) / sizeof (jrd_nod**));


/* Various structures in the impure area */

struct impure_state {
	SSHORT sta_state;
};

struct impure_value {
	struct dsc vlu_desc;
	USHORT vlu_flags; // Computed/invariant flags
	struct str *vlu_string;
	union {
		SSHORT vlu_short;
		SLONG vlu_long;
		SINT64 vlu_int64;
		SQUAD vlu_quad;
		SLONG vlu_dbkey[2];
		float vlu_float;
		double vlu_double;
		GDS_TIMESTAMP vlu_timestamp;
		GDS_TIME vlu_sql_time;
		GDS_DATE vlu_sql_date;
	} vlu_misc;
};

struct impure_value_ex : public impure_value {
	SLONG vlux_count;
};


const int VLU_computed	= 1;		/* An invariant sub-query has been computed */
const int VLU_null		= 2;		/* An invariant sub-query computed to null */


/* Inversion (i.e. nod_index) impure area */

struct impure_inversion {
	RecordBitmap* inv_bitmap;
};


/* ASB impure area */

struct impure_agg_sort {
	SLONG *iasb_sort_handle;
};


/* Various field positions */

const int e_for_re			= 0;
const int e_for_statement	= 1;
const int e_for_stall		= 2;
const int e_for_rsb			= 3;
const int e_for_length		= 4;

const int e_arg_flag		= 0;
const int e_arg_indicator	= 1;
const int e_arg_message		= 2;
const int e_arg_number		= 3;
const int e_arg_length		= 4;

const int e_msg_number		= 0;
const int e_msg_format		= 1;
const int e_msg_invariants	= 2; // TODO: AB
const int e_msg_next		= 3;
const int e_msg_length		= 4;

const int e_fld_stream		= 0;
const int e_fld_id			= 1;
const int e_fld_default_value	= 2;	/* hold column default value info if any,
								   (Literal*) */
const int e_fld_length		= 3;

const int e_sto_statement	= 0;
const int e_sto_statement2	= 1;
const int e_sto_sub_store	= 2;
const int e_sto_validate	= 3;
const int e_sto_relation	= 4;
const int e_sto_stream		= 5; // TODO: AB
const int e_sto_length		= 6;

const int e_erase_statement	= 0;
const int e_erase_sub_erase	= 1;
const int e_erase_stream	= 2;
const int e_erase_rsb		= 3;
const int e_erase_length	= 4;

const int e_sav_operation	= 0;
const int e_sav_name		= 1;
const int e_sav_length		= 2;

const int e_mod_statement	= 0;
const int e_mod_sub_mod		= 1;
const int e_mod_validate	= 2;
const int e_mod_map_view	= 3;
const int e_mod_org_stream	= 4;
const int e_mod_new_stream	= 5;
const int e_mod_rsb			= 6;
const int e_mod_length		= 7;

const int e_send_statement	= 0;
const int e_send_message	= 1;
const int e_send_length		= 2;

const int e_asgn_from		= 0;
const int e_asgn_to			= 1;
const int e_asgn_missing	= 2;	/* Value for comparison for missing */
const int e_asgn_missing2	= 3;	/* Value for substitute for missing */
const int e_asgn_length		= 4;

const int e_rel_stream		= 0;
const int e_rel_relation	= 1;
const int e_rel_view		= 2;	/* parent view for posting access */
const int e_rel_alias		= 3;	/* SQL alias for the relation */
const int e_rel_context		= 4;	/* user-specified context number for the relation reference */
const int e_rel_length		= 5;

const int e_idx_retrieval	= 0;
const int e_idx_length		= 1;

const int e_lbl_statement	= 0;
const int e_lbl_label		= 1;
const int e_lbl_length		= 2;

const int e_any_rse			= 0;
const int e_any_rsb			= 1;
const int e_any_length		= 2;

const int e_if_boolean		= 0;
const int e_if_true			= 1;
const int e_if_false		= 2;
const int e_if_length		= 3;

const int e_hnd_statement	= 0;
const int e_hnd_length		= 1;

const int e_val_boolean		= 0;
const int e_val_value		= 1;
const int e_val_length		= 2;

const int e_uni_stream		= 0;	/* Stream for union */
const int e_uni_clauses		= 1;	/* RecordSelExpr's for union */
const int e_uni_length		= 2;

const int e_agg_stream		= 0;
const int e_agg_rse			= 1;
const int e_agg_group		= 2;
const int e_agg_map			= 3;
const int e_agg_length		= 4;

/* Statistical expressions */

const int e_stat_rse		= 0;
const int e_stat_value		= 1;
const int e_stat_default	= 2;
const int e_stat_rsb		= 3;
const int e_stat_length		= 4;

/* Execute stored procedure */

const int e_esp_inputs		= 0;
const int e_esp_in_msg		= 1;
const int e_esp_outputs		= 2;
const int e_esp_out_msg		= 3;
const int e_esp_procedure	= 4;
const int e_esp_length		= 5;

/* Stored procedure view */

const int e_prc_inputs		= 0;
const int e_prc_in_msg		= 1;
const int e_prc_stream		= 2;
const int e_prc_procedure	= 3;
const int e_prc_length		= 4;

/* Function expression */

const int e_fun_args		= 0;
const int e_fun_function	= 1;
const int e_fun_length		= 2;

/* Generate id */

const int e_gen_value		= 0;
const int e_gen_relation	= 1;
const int e_gen_id			= 1;	/* Generator id (replaces e_gen_relation) */
const int e_gen_length		= 2;

/* Protection mask */

const int e_pro_class		= 0;
const int e_pro_relation	= 1;
const int e_pro_length		= 2;

/* Exception */

const int e_xcp_desc		= 0;
const int e_xcp_msg			= 1;
const int e_xcp_length		= 2;

/* Variable declaration */

const int e_var_id			= 0;
const int e_var_variable	= 1;
const int e_var_length		= 2;

const int e_dcl_id			= 0;
const int e_dcl_invariants	= 1;
const int e_dcl_desc		= 2;
const int e_dcl_length		= (2 + sizeof (DSC)/sizeof (jrd_nod*));	/* Room for descriptor */

const int e_dep_object		= 0;	/* node for registering dependencies */
const int e_dep_object_type	= 1;
const int e_dep_field		= 2;
const int e_dep_length		= 3;

const int e_scl_field		= 0;		/* Scalar expression (blr_index) */
const int e_scl_subscripts	= 1;
const int e_scl_length		= 2;

const int e_blk_action		= 0;
const int e_blk_handlers	= 1;
const int e_blk_length		= 2;

const int e_err_action		= 0;
const int e_err_conditions	= 1;
const int e_err_length		= 2;

/* Datatype cast operator */

const int e_cast_source		= 0;
const int e_cast_fmt		= 1;
const int e_cast_length		= 2;

/* IDAPI semantics nodes */

const int e_index_index		= 0;	/* set current index (blr_set_index) */
const int e_index_stream	= 1;
const int e_index_rsb		= 2;
const int e_index_length	= 3;

const int e_seek_offset		= 0;	/* for seeking through a stream */
const int e_seek_direction	= 1;
const int e_seek_rse		= 2;
const int e_seek_length		= 3;

const int e_find_args		= 0;		/* for finding a key value in a stream */
const int e_find_operator	= 1;
const int e_find_direction	= 2;
const int e_find_stream		= 3;
const int e_find_rsb		= 4;
const int e_find_length		= 5;

const int e_bookmark_id		= 0;	/* nod_bookmark */
const int e_bookmark_length	= 1;

const int e_setmark_id		= 0;	/* nod_set_bookmark */
const int e_setmark_stream	= 1;
const int e_setmark_rsb		= 2;
const int e_setmark_length	= 3;

const int e_getmark_stream	= 0;	/* nod_get_bookmark */
const int e_getmark_rsb		= 1;
const int e_getmark_length	= 2;

const int e_relmark_id		= 0;	/* nod_release_bookmark */
const int e_relmark_length	= 1;

const int e_lockrel_relation= 0;	/* nod_lock_relation */
const int e_lockrel_level	= 1;
const int e_lockrel_length	= 2;

const int e_lockrec_level	= 0;	/* nod_lock_record */
const int e_lockrec_stream	= 1;
const int e_lockrec_rsb		= 2;
const int e_lockrec_length	= 3;

const int e_brange_number	= 0;	/* nod_begin_range */
const int e_brange_length	= 1;

const int e_erange_number	= 0;	/* nod_end_range */
const int e_erange_length	= 1;

const int e_drange_number	= 0;	/* nod_delete_range */
const int e_drange_length	= 1;

const int e_rellock_lock	= 0;	/* nod_release_lock */
const int e_rellock_length	= 1;

const int e_find_dbkey_dbkey	= 0;	/* double duty for nod_find_dbkey and nod_find_dbkey_version */
const int e_find_dbkey_version	= 1;
const int e_find_dbkey_stream	= 2;
const int e_find_dbkey_rsb		= 3;
const int e_find_dbkey_length	= 4;

const int e_range_relation_number	= 0;	/* nod_range_relation */
const int e_range_relation_relation	= 1;
const int e_range_relation_length	= 2;

// This is for the plan node
const int e_retrieve_relation		= 0;
const int e_retrieve_access_type	= 1;
const int e_retrieve_length			= 2;

// This is for the plan's access_type subnode
const int e_access_type_relation	= 0;
const int e_access_type_index		= 1;
const int e_access_type_index_name	= 2;
const int e_access_type_length		= 3;

const int e_reset_from_stream	= 0;
const int e_reset_to_stream		= 1;
const int e_reset_from_rsb		= 2;
const int e_reset_length		= 3;

const int e_card_stream		= 0;
const int e_card_rsb		= 1;
const int e_card_length		= 2;

/* SQL Date supporting nodes */
const int e_extract_value	= 0;	/* Node */
const int e_extract_part	= 1;	/* Integer */
const int e_extract_count	= 1;	/* Number of nodes */
const int e_extract_length	= 2;	/* Number of entries in nod_args */

const int e_current_date_length		= 1;
const int e_current_time_length		= 1;
const int e_current_timestamp_length= 1;

const int e_dcl_cursor_number	= 0;
const int e_dcl_cursor_rse		= 1;
const int e_dcl_cursor_rsb		= 2;
const int e_dcl_cursor_length	= 3;

const int e_cursor_stmt_op		= 0;
const int e_cursor_stmt_number	= 1;
const int e_cursor_stmt_seek	= 2;
const int e_cursor_stmt_into	= 3;
const int e_cursor_stmt_length	= 4;


/* Exception condition list */

struct xcp_repeat {
	SSHORT xcp_type;
	SLONG xcp_code;
};

class PsqlException : public pool_alloc_rpt<xcp_repeat, type_xcp>
{
    public:
	SLONG xcp_count;
    xcp_repeat xcp_rpt[1];
};

const int xcp_sql_code	= 1;
const int xcp_gds_code	= 2;
const int xcp_xcp_code	= 3;
const int xcp_default	= 4;

class StatusXcp {
	ISC_STATUS_ARRAY status;

public:
	StatusXcp();

	void clear();
	void init(const ISC_STATUS*);
	void copyTo(ISC_STATUS*) const;
	bool success() const;
	SLONG as_gdscode() const;
	SLONG as_sqlcode() const;
};

// must correspond to the size of RDB$EXCEPTIONS.RDB$MESSAGE 
// minus size of vary::vary_length (USHORT) since RDB$MESSAGE
// declared as varchar
const int XCP_MESSAGE_LENGTH	= 1023 - sizeof(USHORT);

#endif /* JRD_EXE_H */


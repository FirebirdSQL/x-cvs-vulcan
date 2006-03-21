/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		node.h
 *	DESCRIPTION:	Definitions needed for accessing a parse tree
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
 * 2001.6.12 Claudio Valderrama: add break_* constants.
 * 2001.6.30 Claudio valderrama: Jim Starkey suggested to hold information
 * about source line in each node that's created.
 * 2001.07.28 John Bellardo: Added e_rse_limit to nod_rse and nod_limit.
 * 2001.08.03 John Bellardo: Reordered args to no_sel for new LIMIT syntax
 * 2002.07.30 Arno Brinkman:
 * 2002.07.30	Added nod_searched_case, nod_simple_case, nod_coalesce
 * 2002.07.30	and constants for arguments
 * 2002.08.04 Dmitry Yemanov: ALTER VIEW
 * 2002.10.21 Nickolay Samofatov: Added support for explicit pessimistic locks
 * 2002.10.29 Nickolay Samofatov: Added support for savepoints
 * 2004.01.16 Vlad Horsun: added support for default parameters and 
 *   EXECUTE BLOCK statement
 */

#ifndef DSQL_NODE_H
#define DSQL_NODE_H

#include "dsql_nodes.h"

/* definition of a syntax node created both
   in parsing and in context recognition */

class dsql_nod : public pool_alloc_rpt<class dsql_nod*, dsql_type_nod>
{
public:
	NOD_TYPE nod_type;			/* Type of node */
	DSC			nod_desc;				/* Descriptor */
	USHORT		nod_line;			/* Source line of the statement. */
	USHORT		nod_column;			/* Source column of the statement. */
	USHORT		nod_count;			/* Number of arguments */
	USHORT		nod_flags;
	dsql_nod	*nod_next;
	dsql_nod*	nod_arg[1];
	
	dsql_nod() : nod_type(nod_unknown_type), nod_count(0), nod_flags(0) {}
};
typedef dsql_nod* DSQL_NOD;

/* values of flags */
enum nod_flags_vals {
	NOD_AGG_DISTINCT		= 1,

	NOD_UNION_ALL			= 1,

	NOD_SINGLETON_SELECT	= 1,

	NOD_READ_ONLY			= 1,
	NOD_READ_WRITE			= 2,

	NOD_WAIT				= 1,
	NOD_NO_WAIT				= 2,

	NOD_VERSION				= 1,
	NOD_NO_VERSION			= 2,

	NOD_CONCURRENCY			= 1,
	NOD_CONSISTENCY			= 2,
	NOD_READ_COMMITTED		= 4,

	NOD_SHARED				= 1,
	NOD_PROTECTED			= 2,
	NOD_READ				= 4,
	NOD_WRITE				= 8,

	NOD_NULLS_FIRST			= 1,
	NOD_NULLS_LAST			= 2,

	REF_ACTION_CASCADE		= 1,
	REF_ACTION_SET_DEFAULT	= 2,
	REF_ACTION_SET_NULL		= 4,
	REF_ACTION_NONE			= 8,
	// Node flag indicates that this node has a different type or result
	// depending on the SQL dialect.
	NOD_COMP_DIALECT		= 16,

	NOD_SELECT_EXPR_SINGLETON	= 1,
	NOD_SELECT_EXPR_VALUE		= 2,

	NOD_CURSOR_EXPLICIT		= 1,
	NOD_CURSOR_FOR			= 2,
	NOD_CURSOR_ALL			= USHORT(-1U),

	NOD_DT_IGNORE_COLUMN_CHECK = 1,
	NOD_DT_ALLOW_OUTER_REFERENCE = 2
};


/* enumerations of the arguments to a node, offsets
   within the variable tail nod_arg */

/* Note Bene:
 *	e_<nodename>_count	== count of arguments in nod_arg
 *	This is often used as the count of sub-nodes, but there
 *	are cases when non-DSQL_NOD arguments are stuffed into nod_arg
 *	entries.  These include nod_udf, nod_gen_id, nod_cast,
 *	& nod_collate.
 */
enum node_args {
	e_select_expr = 0,		// nod_select
	e_select_update,
	e_select_lock,
	e_select_count,

	e_fpd_list = 0,			// nod_for_update
	e_fpd_count,

	e_sav_name = 0,			// nod_user_savepoint, nod_undo_savepoint
	e_sav_count,

	e_fld_context = 0,		// nod_field
	e_fld_field,
	e_fld_indices,
	e_fld_count,

	e_ary_array = 0,		// nod_array
	e_ary_indices,
	e_ary_count,

	e_xcp_name = 0,			// nod_exception
	e_xcp_text,
	e_xcp_count,

	e_blk_action = 0,		// nod_block
	e_blk_errs,
	e_blk_count,

	e_err_errs = 0,			// nod_on_error
	e_err_action,
	e_err_count,

	e_var_variable = 0,		// nod_variable
	e_var_count,

	e_pst_event = 0,		// nod_post
	e_pst_argument,
	e_pst_count,

	e_exec_sql_stmnt = 0,	// nod_exec_sql
	e_exec_sql_count,

	e_exec_into_stmnt = 0,	// nod_exec_into
	e_exec_into_block,
	e_exec_into_list,
	e_exec_into_label,
	e_exec_into_count,

	e_internal_info = 0,	// nod_internal_info
	e_internal_info_count,

	e_xcps_name = 0,		// nod_exception_stmt
	e_xcps_msg,
	e_xcps_count,

	e_rtn_procedure = 0,	// nod_procedure
	e_rtn_count,

	e_vrn_name = 0,			// nod_variable_name
	e_vrn_count,

	e_fln_context = 0,		// nod_field_name
	e_fln_name,
	e_fln_count,

	e_rel_context = 0,		// nod_relation
	e_rel_count,

	e_agg_context = 0,		// nod_aggregate
	e_agg_group,
	e_agg_rse,
	e_agg_count,

	e_rse_streams = 0,		// nod_rse
	e_rse_boolean,
	e_rse_sort,
	e_rse_reduced,
	e_rse_items,
	e_rse_first,
	e_rse_plan,
	e_rse_skip,
	e_rse_lock,
	e_rse_count,

	e_limit_skip = 0,		// nod_limit
	e_limit_length,
	e_limit_count,

	e_rows_skip = 0,		// nod_rows
	e_rows_length,
	e_rows_count,

	e_par_parameter = 0,	// nod_parameter
	e_par_count,

	e_flp_select = 0,		// nod_for_select
	e_flp_into,
	e_flp_cursor,
	e_flp_action,
	e_flp_label,
	e_flp_count,

	e_cur_name = 0,			// nod_cursor
	e_cur_rse,
	e_cur_number,
	e_cur_count,

	e_prc_name = 0,			// nod_procedure
	e_prc_inputs,
	e_prc_outputs,
	e_prc_dcls,
	e_prc_body,
	e_prc_source,
	e_prc_count,

	e_exe_procedure = 0,	// nod_exec_procedure
	e_exe_inputs,
	e_exe_outputs,
	e_exe_count,

	e_exe_blk = 0,			// nod_exec_block 
	e_exe_blk_inputs = 0,
	e_exe_blk_outputs,
	e_exe_blk_dcls,
	e_exe_blk_body,	
	e_exe_blk_count,	

	e_prm_val_fld = 0,
	e_prm_val_val,
	e_prm_val_count,

	e_msg_number = 0,		// nod_message
	e_msg_text,
	e_msg_count,

	e_sel_query_spec = 0,	// nod_select_expr
	e_sel_order,
	e_sel_rows,
	e_sel_count,

	e_qry_limit = 0,		// nod_query_spec
	e_qry_distinct,
	e_qry_list,
	e_qry_from,
	e_qry_where,
	e_qry_group,
	e_qry_having,
	e_qry_plan,
	e_qry_count,

	e_ins_relation = 0,		// nod_insert
	e_ins_fields,
	e_ins_values,
	e_ins_select,
	e_ins_count,

	e_sto_relation = 0,		// nod_store
	e_sto_statement,
	e_sto_rse,
	e_sto_count,

	e_del_relation = 0,		// nod_delete
	e_del_boolean,
	e_del_plan,
	e_del_sort,
	e_del_rows,
	e_del_cursor,
	e_del_count,

	e_era_relation = 0,		// nod_erase
	e_era_rse,
	e_era_count,

	e_erc_context = 0,		// nod_erase_current
	e_erc_count,

	e_mdc_context = 0,		// nod_modify_current
	e_mdc_update,
	e_mdc_statement,
	e_mdc_count,

	e_upd_relation = 0,		// nod_update
	e_upd_statement,
	e_upd_boolean,
	e_upd_plan,
	e_upd_sort,
	e_upd_rows,
	e_upd_cursor,
	e_upd_count,

	e_mod_source = 0,		// nod_modify
	e_mod_update,
	e_mod_statement,
	e_mod_rse,
	e_mod_count,

	e_map_context = 0,		// nod_map
	e_map_map,
	e_map_count,

	e_blb_field = 0,		// nod_get_segment & nod_put_segment
	e_blb_relation,
	e_blb_filter,
	e_blb_max_seg,
	e_blb_count,

	e_idx_unique = 0,		// nod_def_index
	e_idx_asc_dsc,
	e_idx_name,
	e_idx_table,
	e_idx_fields,
	e_idx_count,

	e_rln_name = 0,			// nod_relation_name
	e_rln_alias,
	e_rln_count,

	e_rpn_name = 0,			// nod_rel_proc_name
	e_rpn_alias,
	e_rpn_inputs,
	e_rpn_count,

	e_join_left_rel = 0,	// nod_join
	e_join_type,
	e_join_rght_rel,
	e_join_boolean,
	e_join_count,

	e_via_rse = 0, 			//
	e_via_value_1,
	e_via_value_2,
	e_via_count,

	e_if_condition = 0,		//
	e_if_true,
	e_if_false,
	e_if_count,

	e_while_cond = 0,		//
	e_while_action,
	e_while_label,
	e_while_count,

	e_drl_name = 0,			// nod_def_relation
	e_drl_elements,
	e_drl_ext_file,			// external file
	e_drl_count,

	e_dft_default = 0,		// nod_def_default
	e_dft_default_source,
	e_dft_count,

	e_dom_name = 0,			// nod_def_domain
	e_dom_default,
	e_dom_default_source,
	e_dom_constraint,
	e_dom_collate,
	e_dom_count,

	e_dfl_field = 0,		// nod_def_field
	e_dfl_default,
	e_dfl_default_source,
	e_dfl_constraint,
	e_dfl_collate,
	e_dfl_domain,
	e_dfl_computed,
	e_dfl_count,

	e_view_name = 0,		// nod_def_view
	e_view_fields,
	e_view_select,
	e_view_check,
	e_view_source,
	e_view_count,

	e_alt_name = 0,			// nod_mod_relation
	e_alt_ops,
	e_alt_count,

	e_grant_privs = 0,		// nod_grant
	e_grant_table,
	e_grant_users,
	e_grant_grant,
	e_grant_count,

	e_alias_value = 0,		// nod_alias
	e_alias_alias,
	e_alias_count,

	e_rct_name = 0,			// nod_rel_constraint
	e_rct_type,

	e_pri_columns = 0,		// nod_primary
	e_pri_index,
	e_pri_count,

	e_for_columns = 0,		// nod_foreign 
	e_for_reftable,
	e_for_refcolumns,
	e_for_action,
	e_for_index,
	e_for_count,

	e_ref_upd = 0,			// nod_ref_upd_del_action
	e_ref_del,
	e_ref_upd_del_count,

	e_ref_trig_action_count = 0,	//

	e_cnstr_name = 0,		// nod_def_constraint
	e_cnstr_table,			// NOTE: IF ADDING AN ARG IT MUST BE
	e_cnstr_type,			// NULLED OUT WHEN THE NODE IS
	e_cnstr_position,		// DEFINED IN parse.y
	e_cnstr_condition,
	e_cnstr_actions,
	e_cnstr_source,
	e_cnstr_message,
	e_cnstr_else,
	e_cnstr_count,

	e_trg_name = 0,			// nod_mod_trigger and nod_def trigger
	e_trg_table,
	e_trg_active,
	e_trg_type,
	e_trg_position,
	e_trg_actions,
	e_trg_source,
	e_trg_messages,
	e_trg_count,

	e_trg_act_dcls = 0,
	e_trg_act_body,
	e_trg_act_count,

	e_abrt_number = 0,		// nod_abort
	e_abrt_count,

	e_cast_target = 0,		// Not a DSQL_NOD   nod_cast
	e_cast_source,
	e_cast_count,

	e_coll_target = 0,		// Not a DSQL_NOD   nod_collate
	e_coll_source,
	e_coll_count,

	e_order_field = 0,		// nod_order
	e_order_flag,
	e_order_nulls,
	e_order_count,

	e_lock_tables = 0,		//
	e_lock_mode,

	e_database_name = 0,	//
	e_database_initial_desc,
	e_database_rem_desc,
	e_cdb_count,

	e_commit_retain = 0,	//

	e_adb_all = 0,			//
	e_adb_count,

	e_gen_name = 0,			//
	e_gen_count,

	e_filter_name = 0,		//
	e_filter_in_type,
	e_filter_out_type,
	e_filter_entry_pt,
	e_filter_module,
	e_filter_count,

	e_gen_id_name = 0,		// Not a DSQL_NOD   nod_gen_id
	e_gen_id_value,
	e_gen_id_count,


	e_udf_name = 0,			//
	e_udf_entry_pt,
	e_udf_module,
	e_udf_args,
	e_udf_return_value,
	e_udf_count,

	// computed field

	e_cmp_expr = 0,
	e_cmp_text,

	// create shadow

	e_shadow_number = 0,
	e_shadow_man_auto,
	e_shadow_conditional,
	e_shadow_name,
	e_shadow_length,
	e_shadow_sec_files,
	e_shadow_count,

	// alter index

	e_alt_index = 0,
	e_mod_idx_count,

	e_alt_idx_name = 0,
	e_alt_idx_name_count,

	// set statistics

	e_stat_name = 0,
	e_stat_count,

	// SQL extract() function

	e_extract_part = 0,				// constant representing part to extract
	e_extract_value,				// Must be a time or date value
	e_extract_count,

	// SQL CURRENT_TIME, CURRENT_DATE, CURRENT_TIMESTAMP

	e_current_time_count = 0,
	e_current_date_count = 0,
	e_current_timestamp_count = 0,

	e_alt_dom_name = 0,				// nod_modify_domain
	e_alt_dom_ops,
	e_alt_dom_count,

	e_mod_dom_new_dom_type = 0, 	// nod_mod_domain_type
	e_mod_dom_count,

	e_mod_fld_name_orig_name = 0,	// nod_mod_field_name
	e_mod_fld_name_new_name,
	e_mod_fld_name_count,

	e_mod_fld_type = 0,				// nod_mod_field_type
	e_mod_fld_type_dom_name = 2,
	e_mod_fld_type_count = 2,

	e_mod_fld_pos_orig_name = 0,	// nod_mod_field_position
	e_mod_fld_pos_new_position,
	e_mod_fld_pos_count,

	// CVC: blr_leave used to emulate break
	e_breakleave_label = 0,			// nod_breakleave
	e_breakleave_count,

	// SQL substring() function

	e_substr_value = 0,	// Anything that can be treated as a string
	e_substr_start,		// Where the slice starts
	e_substr_length,	// The length of the slice
	e_substr_count,

	e_udf_param_field = 0,
	e_udf_param_type,		// Basically, by_reference or by_descriptor
	e_udf_param_count,

	// CASE <case_operand> {WHEN <when_operand> THEN <when_result>}.. [ELSE <else_result>] END 
	// Node-constants for after pass1

	e_simple_case_case_operand = 0,	// 1 value
	e_simple_case_when_operands,	// list
	e_simple_case_results,			// list including else_result

	// CASE {WHEN <search_condition> THEN <when_result>}.. [ELSE <else_result>] END 
	// Node-constants for after pass1

	e_searched_case_search_conditions = 0,	// list boolean expressions
	e_searched_case_results,				// list including else_result

	e_label_name = 0,
	e_label_number,
	e_label_count,

	e_derived_table_rse = 0,		// Contains select_expr
	e_derived_table_alias,			// Alias name for derived table
	e_derived_table_column_alias,	// List with alias names from derived table columns
	e_derived_table_count,

	e_derived_field_value = 0,		// Contains the source expression
	e_derived_field_name,			// Name for derived table field
	e_derived_field_scope,			// Scope-level
	e_derived_field_count = 4,

	e_cur_stmt_id = 0,
	e_cur_stmt_seek,
	e_cur_stmt_into,
	e_cur_stmt_count,
	
	e_usr_name = 0,					// create/alter/drop/upgrade user
	e_usr_options,

	e_agg_function_expression = 0,
	e_agg_function_scope_level,
	e_agg_function_count
};

#endif // DSQL_NODE_H


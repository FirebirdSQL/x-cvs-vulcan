#ifndef _DSQLNODES_H_
#define _DSQLNODES_H_

/* Parameters to MAKE_constant */

enum dsql_constant_type {
	CONSTANT_STRING		= 0, // stored as a string
	CONSTANT_SLONG		= 1, // stored as a SLONG
	CONSTANT_DOUBLE		= 2, // stored as a string
	CONSTANT_DATE		= 3, // stored as a SLONG
	CONSTANT_TIME		= 4, // stored as a ULONG
	CONSTANT_TIMESTAMP	= 5, // stored as a QUAD
	CONSTANT_SINT64		= 6 // stored as a SINT64
};

/* an enumeration of the possible node types in a syntax tree */

typedef enum nod_t 
{
	nod_unknown_type = 0,
	nod_commit = 1,	/* Commands, not executed. */
	nod_rollback,
	nod_trans,
	nod_def_default,
	nod_del_default,
	nod_def_database,
	nod_def_domain,	
	nod_mod_domain,
	nod_del_domain,
	nod_mod_database, /* 10 */
	nod_def_relation,
	nod_mod_relation,
	nod_del_relation,
	nod_def_field,
	nod_mod_field,
	nod_del_field,
	nod_def_index,
	nod_del_index,
	nod_def_view,
	nod_def_constraint, /* 20 */
	nod_def_trigger,
	nod_mod_trigger,
	nod_del_trigger,
	nod_def_trigger_msg,
	nod_mod_trigger_msg,
	nod_del_trigger_msg,
	nod_def_procedure,	
	nod_mod_procedure,
	nod_del_procedure,
	nod_def_exception,  /* 30 */
	nod_mod_exception,
	nod_del_exception,
	nod_def_generator,
	nod_del_generator,
	nod_def_filter,
	nod_del_filter,
	nod_def_shadow,	
	nod_del_shadow,
	nod_def_udf,
	nod_del_udf,    /* 40 */
	nod_grant,
	nod_revoke,
	nod_rel_constraint,
	nod_delete_rel_constraint,
	nod_primary,
	nod_foreign,
	nod_abort,	
	nod_references,
	nod_proc_obj,
	nod_trig_obj, /* 50 */
	nod_view_obj,
	nod_list,	/* SQL statements, mapped into GDML statements */
	nod_select,
	nod_insert,
	nod_delete,
	nod_update,	
	nod_close,
	nod_open,
	nod_all,	/* ALL privileges */
	nod_execute, /* 60 */		/* EXECUTE privilege */
	nod_store,
	nod_modify,
	nod_erase,
	nod_assign,	
	nod_exec_procedure,
	nod_return,	/* Procedure statements */
	nod_exit,
	nod_while,
	nod_if,
	nod_for_select, /* 70 */
	nod_erase_current,
	nod_modify_current,
	nod_post,
	nod_block,	
	nod_on_error,
	nod_sqlcode,
	nod_gdscode,
	nod_exception,
	nod_exception_stmt,
	nod_default, /* 80 */
	nod_start_savepoint,
	nod_end_savepoint,
	nod_cursor,	/* used to create record streams */
	nod_relation,				
	nod_relation_name,
	nod_procedure_name,
	nod_rel_proc_name,
	nod_rse,
	nod_select_expr,            
	nod_union,                  /* 90 */
	nod_aggregate,
	nod_order,
	nod_flag,	
	nod_join,
/* NOTE: when adding an expression node, be sure to
   test various combinations; in particular, think 
   about parameterization using a '?' - there is code
   in PASS1_node which must be updated to find the
   data type of the parameter based on the arguments 
   to an expression node */
	nod_eql,
	nod_neq,
	nod_gtr,
	nod_geq,
	nod_leq,
	nod_lss,         /* 100 */
	nod_between,
	nod_like,
	nod_missing,
	nod_and,
	nod_or,
	nod_any,
	nod_not,
	nod_unique,
	nod_containing,
	nod_starting,
	nod_via,
	nod_field,	/* values */
	nod_dom_value,
	nod_field_name,
	nod_parameter,
	nod_constant,
	nod_map,
	nod_alias,
	nod_user_name,
	nod_user_group,
	nod_variable,
	nod_var_name,
	nod_array,
	nod_add,	/* functions */
	nod_subtract,
	nod_multiply,
	nod_divide,
	nod_negate,
	nod_concatenate,
	nod_substr,
	nod_null,
	nod_dbkey,
	nod_udf,
	nod_cast,
	nod_upcase,
	nod_collate,
	nod_gen_id,
	nod_add2,	/* functions different for dialect >= V6_TRANSITION */
	nod_subtract2,
	nod_multiply2,
	nod_divide2,
	nod_gen_id2,
	nod_average,	/* aggregates */
	nod_from,
	nod_max,
	nod_min,
	nod_total,
	nod_count,
	nod_exists,
	nod_singular,
	nod_agg_average,
	nod_agg_max,
	nod_agg_min,
	nod_agg_total,
	nod_agg_count,
	nod_agg_average2,
	nod_agg_total2,
	nod_get_segment,	/* blobs */
	nod_put_segment,
	nod_join_inner,	/* join types */
	nod_join_left,
	nod_join_right,
	nod_join_full,
	/* sql transaction support */
	nod_access,
	nod_wait,
	nod_isolation,
	nod_version,
	nod_table_lock,
	nod_lock_mode,
	nod_reserve,
	nod_commit_retain,
	/* sql database stmts support */
	nod_page_size,
	nod_file_length,
	nod_file_desc,
	nod_log_file_desc,
	nod_cache_file_desc,
	nod_group_commit_wait,
	nod_check_point_len,
	nod_num_log_buffers,
	nod_log_buffer_size,
	nod_drop_log,
	nod_drop_cache,
	nod_dfl_charset,
	/* sql connect options support (used for create database) */
	nod_password,
	nod_lc_ctype,	/* SET NAMES */
	/* Misc nodes */
	nod_udf_return_value,
	/* computed field */
	nod_def_computed,
	/* access plan stuff */
	nod_plan_expr,
	nod_plan_item,
	nod_merge,
	nod_natural,
	nod_index,
	nod_index_order,
	nod_set_generator,
	nod_set_generator2,	/* SINT64 value for dialect > V6_TRANSITION */
	/* alter index */
	nod_mod_index,
	nod_idx_active,
	nod_idx_inactive,
		/* drop behaviour */
	nod_restrict,
	nod_cascade,
	/* set statistics */
	nod_set_statistics,
	/* record version */
	nod_rec_version,
	/* ANY keyword used */
	nod_ansi_any,
	nod_eql_any,
	nod_neq_any,
	nod_gtr_any,
	nod_geq_any,
	nod_leq_any,
	nod_lss_any,
	/* ALL keyword used */
	nod_ansi_all,
	nod_eql_all,
	nod_neq_all,
	nod_gtr_all,
	nod_geq_all,
	nod_leq_all,
	nod_lss_all,
	/*referential integrity actions */
	nod_ref_upd_del,
	nod_ref_trig_action,
	/* SQL role support */
	nod_def_role,
	nod_role_name,
	nod_grant_admin,
	nod_del_role,
	/* SQL time & date support */
	nod_current_time,
	nod_current_date,
	nod_current_timestamp,
	nod_extract,
	/* ALTER column/domain support */
	nod_mod_domain_type,
	nod_mod_field_name,
	nod_mod_field_type,
	nod_mod_field_pos,

	/* CVC: SQL requires that DROP VIEW and DROP table are independent. */
	nod_del_view,
	nod_current_role, /* nod_role_name is already taken but only for DDL. */
	nod_breakleave,
	nod_redef_relation, /* allows silent creation/overwriting of a relation. */
	nod_udf_param, /* there should be a way to signal a param by descriptor! */
	nod_limit, /* limit support */
	nod_redef_procedure, /* allows silent creation/overwriting of a procedure. */
	nod_exec_sql, /* EXECUTE STATEMENT */
	nod_internal_info, /* internal engine info */
	nod_searched_case, /* searched CASE function */
	nod_simple_case, /* simple CASE function */
	nod_coalesce, /* COALESCE function */
	nod_mod_view, /* ALTER VIEW */
	nod_replace_procedure, /* CREATE OR ALTER PROCEDURE */
	nod_replace_trigger, /* CREATE OR ALTER TRIGGER */
	nod_replace_view, /* CREATE OR ALTER VIEW */
	nod_redef_view, /* allows silent creation/overwriting of a view */
	nod_for_update, /* FOR UPDATE clause */
	nod_user_savepoint, /* savepoints support */
	nod_release_savepoint,
	nod_undo_savepoint,
	nod_label, /* label support */
	nod_exec_into, /* EXECUTE STATEMENT INTO */
	nod_difference_file,
	nod_drop_difference,
	nod_begin_backup,
	nod_end_backup,
	nod_derived_table, // Derived table support
	nod_derived_field,  // Derived table support
	nod_cursor_open,
	nod_cursor_fetch,
	nod_cursor_close,
	nod_fetch_seek,
	nod_exec_block,		// EXECUTE BLOCK support
	nod_param_val,		// default value for SP parameters support
	nod_rows,	// ROWS support
	nod_query_spec,
	nod_equiv,
	nod_def_user,
	nod_del_user,
	nod_mod_user,
	nod_upg_user
} NOD_TYPE;

#endif


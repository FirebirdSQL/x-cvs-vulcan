/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		dsqlv.cpp
 *	DESCRIPTION:	Hold over code from the old dsql.cpp
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
 * 2001.07.06 Sean Leyne - Code Cleanup, removed "#ifdef READONLY_DATABASE"
 *                         conditionals, as the engine now fully supports
 *                         readonly databases.
 * December 2001 Mike Nordell: Major overhaul to (try to) make it C++
 * 2001.6.3 Claudio Valderrama: fixed a bad behaved loop in get_plan_info()
 * and get_rsb_item() that caused a crash when plan info was requested.
 * 2001.6.9 Claudio Valderrama: Added nod_del_view, nod_current_role and nod_breakleave.
 * 2002.10.29 Nickolay Samofatov: Added support for savepoints
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */
/**************************************************************
***************************************************************/

#include "firebird.h"
#include "fb_exception.h"
#include "../jrd/ib_stdio.h"
#include <stdlib.h>
#include <string.h>
#include "../dsql/dsql.h"
#include "../jrd/ibase.h"
#include "../jrd/thd.h"
#include "../jrd/align.h"
#include "../jrd/intl.h"
#include "../jrd/iberr.h"
#include "../dsql/sqlda.h"
#include "../dsql/alld_proto.h"
#include "../dsql/ddl_proto.h"
#include "../dsql/dsql_proto.h"
#include "../dsql/errd_proto.h"
#include "../dsql/gen_proto.h"
#include "../dsql/hsh_proto.h"
#include "../dsql/make_proto.h"
#include "../dsql/movd_proto.h"
#include "../dsql/parse_proto.h"
#include "../dsql/pass1_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/sch_proto.h"
#include "../jrd/thd_proto.h"
//#include "../jrd/why_proto.h"
//#include "../jrd/y_handle.h"
#include "../jrd/Procedure.h"

#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#ifdef VMS
#include <descrip.h>
#endif

/***
static void		cleanup(void*);
static void		cleanup_database(FRBRD**, SLONG);
static void		cleanup_transaction(FRBRD*, SLONG);
static void		close_cursor(dsql_req*);
static USHORT	convert(SLONG, UCHAR*);
static ISC_STATUS	error();
static void		execute_blob(dsql_req*, USHORT, const UCHAR*, USHORT, UCHAR*,
						 USHORT, UCHAR*, USHORT, UCHAR*);
static ISC_STATUS	execute_request(dsql_req*, FRBRD**, USHORT, const UCHAR*,
	USHORT, UCHAR*, USHORT, UCHAR*, USHORT, UCHAR*, bool);
static SSHORT	filter_sub_type(dsql_req*, const dsql_nod*);
static bool		get_indices(SSHORT*, const SCHAR**, SSHORT*, SCHAR**);
static USHORT	get_plan_info(dsql_req*, SSHORT, SCHAR**);
static USHORT	get_request_info(dsql_req*, SSHORT, SCHAR*);
static bool		get_rsb_item(SSHORT*, const SCHAR**, SSHORT*, SCHAR**, USHORT*,
							USHORT*);
static DBB		init(FRBRD**);
static void		map_in_out(dsql_req*, dsql_msg*, USHORT, const UCHAR*, USHORT, UCHAR*);
static USHORT	name_length(const TEXT*);
static USHORT	parse_blr(USHORT, const UCHAR*, const USHORT, par*);
static dsql_req*		prepare(dsql_req*, USHORT, const TEXT*, USHORT, USHORT);
static void		punt(void);
static UCHAR*	put_item(UCHAR, USHORT, const UCHAR*, UCHAR*, const UCHAR* const);
static void		release_request(dsql_req*, bool);
static ISC_STATUS	return_success(void);
static UCHAR*	var_info(dsql_msg*, const UCHAR*, const UCHAR* const, UCHAR*,
	const UCHAR* const, USHORT);

extern dsql_nod* DSQL_parse;

#ifdef DSQL_DEBUG
	unsigned DSQL_debug;
#endif

static bool init_flag = false;
static DBB databases;
static OPN open_cursors;

static const SCHAR db_hdr_info_items[] = {
	isc_info_db_sql_dialect,
	isc_info_ods_version,
	isc_info_base_level,
	isc_info_db_read_only,
	frb_info_att_charset,
	isc_info_end
};

static const SCHAR explain_info[] = {
	isc_info_access_path
};

static const SCHAR record_info[] = {
	isc_info_req_update_count, isc_info_req_delete_count,
	isc_info_req_select_count, isc_info_req_insert_count
};

static const UCHAR sql_records_info[] = {
	isc_info_sql_records
};

***/
#ifdef	ANY_THREADING
static MUTX_T databases_mutex;
static MUTX_T cursors_mutex;
static bool mutex_inited = false;
#endif


#ifdef DSQL_DEBUG
IMPLEMENT_TRACE_ROUTINE(dsql_trace, "DSQL");
#endif

#ifdef DSQL_DEBUG
	unsigned DSQL_debug;
#endif

#ifdef DSQL_DEBUG

static void trace_line(const char* message, ...) {
	char buffer[1024];
	va_list params;
	va_start(params, message);
	
#ifdef HAVE_VSNPRINTF
	vsnprintf(buffer, sizeof(buffer), message, params);
#else
	vsprintf(buffer, message, params);
#endif

	va_end(params);
	buffer[sizeof(buffer) - 1] = 0;
	gds__trace_raw(buffer);
}

/**
  
 	DSQL_pretty
  
    @brief	Pretty print a node tree.


    @param node
    @param column

 **/


void DSQL_pretty(const dsql_nod* node, int column)
{
	const dsql_str* string;
	TEXT buffer[1024];
	TEXT s[64];
	TEXT* p = buffer;
	p += sprintf(p, "%.7X ", (long) node);

	if (column) {
		USHORT l = column * 3;
		do {
			*p++ = ' ';
		} while (--l);
	}

	*p = 0;

	if (!node) {
		trace_line("%s *** null ***\n", buffer);
		return;
	}

	switch (MemoryPool::blk_type(node)) {
	case (TEXT) dsql_type_str:
		trace_line("%sSTRING: \"%s\"\n", buffer, ((dsql_str*) node)->str_data);
		return;

	/***
	case (TEXT) dsql_type_fld:
		trace_line("%sFIELD: %s\n", buffer, (const char*) ((dsql_fld*) node)->fld_name);
		return;
	***/
	
	case (TEXT) dsql_type_sym:
		trace_line("%sSYMBOL: %s\n", buffer, ((dsql_sym*) node)->sym_string);
		return;

	case (TEXT) dsql_type_nod:
		break;

	default:
		trace_line("%sUNKNOWN BLOCK TYPE\n", buffer);
		return;
	}

	const TEXT* verb;
	const dsql_nod* const* ptr = node->nod_arg;
	const dsql_nod* const* const end = ptr + node->nod_count;

	switch (node->nod_type) {
	case nod_abort:
		verb = "abort";
		break;
	case nod_agg_average:
		verb = "agg_average";
		break;
	case nod_agg_count:
		verb = "agg_count";
		break;
/* count2
    case nod_agg_distinct: verb = "agg_distinct";	break;
*/
	case nod_agg_max:
		verb = "agg_max";
		break;
	case nod_agg_min:
		verb = "agg_min";
		break;
	case nod_agg_total:
		verb = "agg_total";
		break;
	case nod_add:
		verb = "add";
		break;
	case nod_alias:
		verb = "alias";
		break;
	case nod_ansi_all:
	case nod_all:
		verb = "all";
		break;
	case nod_and:
		verb = "and";
		break;
	case nod_ansi_any:
	case nod_any:
		verb = "any";
		break;
	case nod_array:
		verb = "array element";
		break;
	case nod_assign:
		verb = "assign";
		break;
	case nod_average:
		verb = "average";
		break;
	case nod_between:
		verb = "between";
		break;
	case nod_cast:
		verb = "cast";
		break;
	case nod_collate:
		verb = "collate";
		break;
	case nod_concatenate:
		verb = "concatenate";
		break;
	case nod_containing:
		verb = "containing";
		break;
	case nod_count:
		verb = "count";
		break;
	case nod_current_date:
		verb = "current_date";
		break;
	case nod_current_time:
		verb = "current_time";
		break;
	case nod_current_timestamp:
		verb = "current_timestamp";
		break;
	case nod_cursor:
		verb = "cursor";
		break;
	case nod_dbkey:
		verb = "dbkey";
		break;
	case nod_rec_version:
		verb = "record_version";
		break;
	case nod_def_database:
		verb = "define database";
		break;
	case nod_def_field:
		verb = "define field";
		break;
	case nod_def_generator:
		verb = "define generator";
		break;
	case nod_def_filter:
		verb = "define filter";
		break;
	case nod_def_index:
		verb = "define index";
		break;
	case nod_def_relation:
		verb = "define relation";
		break;
   // CVC: New node redef_relation. 
    case nod_redef_relation:    
        verb = "redefine relation";    
        break;
	case nod_def_view:
		verb = "define view";
		break;
	case nod_redef_view:
		verb = "redefine view";
		break;
	case nod_mod_view:
		verb = "modify view";
		break;
	case nod_replace_view:
		verb = "replace view";
		break;
	case nod_delete:
		verb = "delete";
		break;
	case nod_del_field:
		verb = "delete field";
		break;
	case nod_del_filter:
		verb = "delete filter";
		break;
	case nod_del_generator:
		verb = "delete generator";
		break;
	case nod_del_index:
		verb = "delete index";
		break;
	case nod_del_relation:
		verb = "delete relation";
		break;
    // CVC: New node del_view. 
    case nod_del_view:     
        verb = "delete view";       
        break;
	case nod_def_procedure:
		verb = "define procedure";
		break;
	case nod_mod_procedure:
		verb = "modify procedure";
		break;
	case nod_replace_procedure:
		verb = "replace procedure";
		break;
    // CVC: New node redef_procedure. 
    case nod_redef_procedure:  
        verb = "redefine procedure"; 
        break;
	case nod_del_procedure:
		verb = "delete procedure";
		break;
	case nod_def_trigger:
		verb = "define trigger";
		break;
	case nod_mod_trigger:
		verb = "modify trigger";
		break;
	case nod_replace_trigger:
		verb = "replace trigger";
		break;
	case nod_del_trigger:
		verb = "delete trigger";
		break;
	case nod_divide:
		verb = "divide";
		break;
	case nod_eql_all:
	case nod_eql_any:
	case nod_eql:
		verb = "eql";
		break;
	case nod_erase:
		verb = "erase";
		break;
	case nod_execute:
		verb = "execute";
		break;
	case nod_exec_procedure:
		verb = "execute procedure";
		break;
	case nod_exists:
		verb = "exists";
		break;
	case nod_extract:
		verb = "extract";
		break;
	case nod_flag:
		verb = "flag";
		break;
	case nod_foreign:
		verb = "foreign key";
		break;
	case nod_gen_id:
		verb = "gen_id";
		break;
	case nod_geq_all:
	case nod_geq_any:
	case nod_geq:
		verb = "geq";
		break;
	case nod_get_segment:
		verb = "get segment";
		break;
	case nod_grant:
		verb = "grant";
		break;
	case nod_gtr_all:
	case nod_gtr_any:
	case nod_gtr:
		verb = "gtr";
		break;
	case nod_insert:
		verb = "insert";
		break;
	case nod_internal_info:
		verb = "internal info";
		break;
	case nod_join:
		verb = "join";
		break;
	case nod_join_full:
		verb = "join_full";
		break;
	case nod_join_left:
		verb = "join_left";
		break;
	case nod_join_right:
		verb = "join_right";
		break;
	case nod_leq_all:
	case nod_leq_any:
	case nod_leq:
		verb = "leq";
		break;
	case nod_like:
		verb = "like";
		break;
	case nod_list:
		verb = "list";
		break;
	case nod_lss_all:
	case nod_lss_any:
	case nod_lss:
		verb = "lss";
		break;
	case nod_max:
		verb = "max";
		break;
	case nod_min:
		verb = "min";
		break;
	case nod_missing:
		verb = "missing";
		break;
	case nod_modify:
		verb = "modify";
		break;
	case nod_mod_database:
		verb = "modify database";
		break;
	case nod_mod_field:
		verb = "modify field";
		break;
	case nod_mod_relation:
		verb = "modify relation";
		break;
	case nod_multiply:
		verb = "multiply";
		break;
	case nod_negate:
		verb = "negate";
		break;
	case nod_neq_all:
	case nod_neq_any:
	case nod_neq:
		verb = "neq";
		break;
	case nod_not:
		verb = "not";
		break;
	case nod_null:
		verb = "null";
		break;
	case nod_or:
		verb = "or";
		break;
	case nod_order:
		verb = "order";
		break;
	case nod_primary:
		verb = "primary key";
		break;
	case nod_procedure_name:
		verb = "procedure name";
		break;
	case nod_put_segment:
		verb = "put segment";
		break;
	case nod_relation_name:
		verb = "relation name";
		break;
	case nod_rel_proc_name:
		verb = "rel/proc name";
		break;
	case nod_return:
		verb = "return";
		break;
	case nod_revoke:
		verb = "revoke";
		break;
	case nod_rse:
		verb = "rse";
		break;
	case nod_select:
		verb = "select";
		break;
	case nod_select_expr:
		verb = "select expr";
		break;
	case nod_starting:
		verb = "starting";
		break;
	case nod_store:
		verb = "store";
		break;
	case nod_substr:
		verb = "substr";
		break;
	case nod_subtract:
		verb = "subtract";
		break;
	case nod_total:
		verb = "total";
		break;
	case nod_update:
		verb = "update";
		break;
	case nod_union:
		verb = "union";
		break;
	case nod_unique:
		verb = "unique";
		break;
	case nod_upcase:
		verb = "upcase";
		break;
	case nod_singular:
		verb = "singular";
		break;
	case nod_user_name:
		verb = "user_name";
		break;
        // CVC: New node current_role. 
    case nod_current_role: 
        verb = "current_role";  
        break;
	case nod_via:
		verb = "via";
		break;

	case nod_coalesce:
		verb = "coalesce";
		break;

	case nod_simple_case:
		verb = "simple_case";
		break;

	case nod_searched_case:
		verb = "searched_case";
		break;

	case nod_add2:
		verb = "add2";
		break;
	case nod_agg_total2:
		verb = "agg_total2";
		break;
	case nod_divide2:
		verb = "divide2";
		break;
	case nod_gen_id2:
		verb = "gen_id2";
		break;
	case nod_multiply2:
		verb = "multiply2";
		break;
	case nod_subtract2:
		verb = "subtract2";
		break;
	case nod_limit:
		verb = "limit";
		break;
	case nod_rows:
		verb = "rows";
		break;
	/* IOL:	missing	node types */
	case nod_on_error:
		verb = "on error";
		break;
	case nod_block:
		verb = "block";
		break;
	case nod_default:
		verb = "default";
		break;
	case nod_plan_expr:
		verb = "plan";
		break;
	case nod_index:
		verb = "index";
		break;
	case nod_index_order:
		verb = "order";
		break;
	case nod_plan_item:
		verb = "item";
		break;
	case nod_natural:
		verb = "natural";
		break;
	case nod_join_inner:
		verb = "join_inner";
		break;
	// SKIDDER: some more missing node types 
	case nod_commit:
		verb = "commit";
		break;
	case nod_rollback:
		verb = "rollback";
		break;
	case nod_trans:
		verb = "trans";
		break;
	case nod_def_default:
		verb = "def_default";
		break;
	case nod_del_default:
		verb = "del_default";
		break;
	case nod_def_domain:
		verb = "def_domain";
		break;
	case nod_mod_domain:
		verb = "mod_domain";
		break;
	case nod_del_domain:
		verb = "del_domain";
		break;
	case nod_def_constraint:
		verb = "def_constraint";
		break;
	case nod_def_trigger_msg:
		verb = "def_trigger_msg";
		break;
	case nod_mod_trigger_msg:
		verb = "mod_trigger_msg";
		break;
	case nod_del_trigger_msg:
		verb = "del_trigger_msg";
		break;
	case nod_def_exception:
		verb = "def_exception";
		break;
	case nod_mod_exception:
		verb = "mod_exception";
		break;
	case nod_del_exception:
		verb = "del_exception";
		break;
	case nod_def_shadow:
		verb = "def_shadow";
		break;
	case nod_del_shadow:
		verb = "del_shadow";
		break;
	case nod_def_udf:
		verb = "def_udf";
		break;
	case nod_del_udf:
		verb = "del_udf";
		break;
	case nod_rel_constraint:
		verb = "rel_constraint";
		break;
	case nod_delete_rel_constraint:
		verb = "delete_rel_constraint";
		break;
	case nod_references:
		verb = "references";
		break;
	case nod_proc_obj:
		verb = "proc_obj";
		break;
	case nod_trig_obj:
		verb = "trig_obj";
		break;
	case nod_view_obj:
		verb = "view_obj";
		break;
	case nod_exit:
		verb = "exit";
		break;
	case nod_if:
		verb = "if";
		break;
	case nod_erase_current:
		verb = "erase_current";
		break;
	case nod_modify_current:
		verb = "modify_current";
		break;
	case nod_post:
		verb = "post";
		break;
	case nod_sqlcode:
		verb = "sqlcode";
		break;
	case nod_gdscode:
		verb = "gdscode";
		break;
	case nod_exception:
		verb = "exception";
		break;
	case nod_exception_stmt:
		verb = "exception_stmt";
		break;
	case nod_start_savepoint:
		verb = "start_savepoint";
		break;
	case nod_end_savepoint:
		verb = "end_savepoint";
		break;
	case nod_dom_value:
		verb = "dom_value";
		break;
	case nod_user_group:
		verb = "user_group";
		break;
	case nod_from:
		verb = "from";
		break;
	case nod_agg_average2:
		verb = "agg_average2";
		break;
	case nod_access:
		verb = "access";
		break;
	case nod_wait:
		verb = "wait";
		break;
	case nod_isolation:
		verb = "isolation";
		break;
	case nod_version:
		verb = "version";
		break;
	case nod_table_lock:
		verb = "table_lock";
		break;
	case nod_lock_mode:
		verb = "lock_mode";
		break;
	case nod_reserve:
		verb = "reserve";
		break;
	case nod_commit_retain:
		verb = "commit_retain";
		break;
	case nod_page_size:
		verb = "page_size";
		break;
	case nod_file_length:
		verb = "file_length";
		break;
	case nod_file_desc:
		verb = "file_desc";
		break;
	case nod_log_file_desc:
		verb = "log_file_desc";
		break;
	case nod_cache_file_desc:
		verb = "cache_file_desc";
		break;
	case nod_group_commit_wait:
		verb = "group_commit_wait";
		break;
	case nod_check_point_len:
		verb = "check_point_len";
		break;
	case nod_num_log_buffers:
		verb = "num_log_buffers";
		break;
	case nod_log_buffer_size:
		verb = "log_buffer_size";
		break;
	case nod_drop_log:
		verb = "drop_log";
		break;
	case nod_drop_cache:
		verb = "drop_cache";
		break;
	case nod_dfl_charset:
		verb = "dfl_charset";
		break;
	case nod_password:
		verb = "password";
		break;
	case nod_lc_ctype:
		verb = "lc_ctype";
		break;
	case nod_udf_return_value:
		verb = "udf_return_value";
		break;
	case nod_def_computed:
		verb = "def_computed";
		break;
	case nod_merge:
		verb = "merge";
		break;
	case nod_set_generator:
		verb = "set_generator";
		break;
	case nod_set_generator2:
		verb = "set_generator2";
		break;
	case nod_mod_index:
		verb = "mod_index";
		break;
	case nod_idx_active:
		verb = "idx_active";
		break;
	case nod_idx_inactive:
		verb = "idx_inactive";
		break;
	case nod_restrict:
		verb = "restrict";
		break;
	case nod_cascade:
		verb = "cascade";
		break;
	case nod_set_statistics:
		verb = "set_statistics";
		break;
	case nod_ref_upd_del:
		verb = "ref_upd_del";
		break;
	case nod_ref_trig_action:
		verb = "ref_trig_action";
		break;
	case nod_def_role:
		verb = "def_role";
		break;
	case nod_role_name:
		verb = "role_name";
		break;
	case nod_grant_admin:
		verb = "grant_admin";
		break;
	case nod_del_role:
		verb = "del_role";
		break;
	case nod_mod_domain_type:
		verb = "mod_domain_type";
		break;
	case nod_mod_field_name:
		verb = "mod_field_name";
		break;
	case nod_mod_field_type:
		verb = "mod_field_type";
		break;
	case nod_mod_field_pos:
		verb = "mod_field_pos";
		break;
	case nod_udf_param:
		verb = "udf_param";
		break;
	case nod_exec_sql:
		verb = "exec_sql";
		break;
	case nod_for_update:
		verb = "for_update";
		break;
	case nod_user_savepoint:
		verb = "user_savepoint";
		break;
	case nod_release_savepoint:
		verb = "release_savepoint";
		break;
	case nod_undo_savepoint:
		verb = "undo_savepoint";
		break;
	case nod_difference_file:
		verb = "difference_file";
		break;
	case nod_drop_difference:
		verb = "drop_difference";
		break;
	case nod_begin_backup:
		verb = "begin_backup";
		break;
	case nod_end_backup:
		verb = "end_backup";
		break;
	case nod_derived_table:
		verb = "derived_table";
		break;

	case nod_exec_into:
		verb = "exec_into";
		break;

	case nod_breakleave:
		verb = "breakleave";
		break;

	case nod_for_select:
		verb = "for_select";
		break;

	case nod_while:
		verb = "while";
		break;

	case nod_label:
		verb = "label";
		DSQL_pretty(node->nod_arg[e_label_name], column + 1);
		trace_line("%s   number %d\n", buffer,
			(int)(IPTR)node->nod_arg[e_label_number]);
		return;

	case nod_derived_field:
		verb = "derived_field";
		trace_line("%s%s\n", buffer, verb);
		DSQL_pretty(node->nod_arg[e_derived_field_value], column + 1);
		DSQL_pretty(node->nod_arg[e_derived_field_name], column + 1);
		trace_line("%s   scope %d\n", buffer,
			(USHORT)(U_IPTR)node->nod_arg[e_derived_field_scope]);
		return;

	case nod_aggregate:
		{
		verb = "aggregate";
		trace_line("%s%s\n", buffer, verb);
		const dsql_ctx* context = (dsql_ctx*) node->nod_arg[e_agg_context];
		trace_line("%s   context %d\n", buffer, context->ctx_context);
		dsql_map* map = context->ctx_map;
		if (map != NULL)
			trace_line("%s   map\n", buffer);
		while (map) {
			trace_line("%s      position %d\n", buffer, map->map_position);
			DSQL_pretty(map->map_node, column + 2);
			map = map->map_next;
		}
		DSQL_pretty(node->nod_arg[e_agg_group], column + 1);
		DSQL_pretty(node->nod_arg[e_agg_rse], column + 1);
		return;
		}

	case nod_constant:
		verb = "constant";
		if (node->nod_desc.dsc_address) {
			if (node->nod_desc.dsc_dtype == dtype_text)
				sprintf(s, "constant \"%s\"", node->nod_desc.dsc_address);
			else
				sprintf(s, "constant %ld",
						*(SLONG *) (node->nod_desc.dsc_address));
			verb = s;
		}
		break;

	case nod_field:
		{
		dsql_ctx* context = (dsql_ctx*) node->nod_arg[e_fld_context];
		dsql_rel* relation = context->ctx_relation;
		Procedure* procedure = context->ctx_procedure;
		dsql_fld* field = (dsql_fld*) node->nod_arg[e_fld_field];
		const char *name = "unknown_db_object";
		if (relation)
			name = relation->rel_name;
		else if (procedure)
			name = (const TEXT *)procedure->findName();
		trace_line("%sfield %s.%s, context %d\n", buffer, name,
			/***
 			(const char*) (relation != NULL ? 
 				(const char*) relation->rel_name : 
 				(procedure != NULL ? 
 					procedure->prc_name : 
 					"unknown_db_object")), 
			***/
 			(const char*) field->fld_name, context->ctx_context);
		return;
		}
	
	case nod_field_name:
		trace_line("%sfield name: \"", buffer);
		string = (dsql_str*) node->nod_arg[e_fln_context];
		if (string)
			trace_line("%s.", string->str_data);
		string = (dsql_str*) node->nod_arg[e_fln_name];
        if (string != 0) {
            trace_line("%s\"\n", string->str_data);
        }
        else {
            trace_line("%s\"\n", "*");
        }
		return;

	case nod_map:
		{
		verb = "map";
		trace_line("%s%s\n", buffer, verb);
		const dsql_ctx* context = (dsql_ctx*) node->nod_arg[e_map_context];
		trace_line("%s   context %d\n", buffer, context->ctx_context);
		for (const dsql_map* map = (dsql_map*) node->nod_arg[e_map_map]; map;
			map = map->map_next)
		{
			trace_line("%s   position %d\n", buffer, map->map_position);
			DSQL_pretty(map->map_node, column + 1);
		}
		return;
		}

	case nod_relation:
		{
		dsql_ctx* context = (dsql_ctx*) node->nod_arg[e_rel_context];
		dsql_rel* relation = context->ctx_relation;
		Procedure* procedure = context->ctx_procedure;
 		if( relation != NULL ) {
 			trace_line("%srelation %s, context %d\n",
 				buffer, 
				(const char*) relation->rel_name, 
				context->ctx_context);
 		}
		else if ( procedure != NULL ) {
 			trace_line("%sprocedure %s, context %d\n",
 				buffer, (const TEXT *)procedure->findName(), context->ctx_context);
 		}
		else {
 			trace_line("%sUNKNOWN DB OBJECT, context %d\n",
 				buffer, context->ctx_context);
 		}
		return;
		}

	case nod_variable:
		{
		const var* variable = (var*) node->nod_arg[e_var_variable];
        // Adding variable->var_variable_number to display, obviously something
        // is missing from the printf, and Im assuming this was it.
        // (anyway can't be worse than it was MOD 05-July-2002.
		trace_line("%svariable %s %d\n", buffer, variable->var_name, variable->var_variable_number);
		return;
		}

	case nod_var_name:
		trace_line("%svariable name: \"", buffer);
		string = (dsql_str*) node->nod_arg[e_vrn_name];
		trace_line("%s\"\n", string->str_data);
		return;

	case nod_parameter:
		if (node->nod_column) {
			trace_line("%sparameter: %d\n",	buffer,
				(USHORT)(long)node->nod_arg[e_par_parameter]);
		}
		else {
			const par* param = (par*) node->nod_arg[e_par_parameter];
			trace_line("%sparameter: %d\n",	buffer, param->par_index);
		}
		return;


    case nod_udf:
        trace_line ("%sfunction: \"", buffer);  
        
		/* nmcc: how are we supposed to tell which type of nod_udf this is ?? */
		/* CVC: The answer is that nod_arg[0] can be either the udf name or the
		pointer to udf struct returned by METD_get_function, so we should resort
		to the block type. The replacement happens in pass1_udf(). */
			//        switch (node->nod_arg[e_udf_name]->nod_header.blk_type) {
        
        switch (MemoryPool::blk_type(node->nod_arg[e_udf_name])) 
			{
			case dsql_type_udf:
				trace_line ("%s\"\n", ((dsql_udfx*) node->nod_arg[e_udf_name])->udf_name);
				break;
			case dsql_type_str:  
				string = (dsql_str*) node->nod_arg[e_udf_name];
				trace_line ("%s\"\n", string->str_data);
				break;
			default:
				trace_line ("%s\"\n", "<ERROR>");
				break;
			}
			
        ptr++;

        if (node->nod_count == 2) 
            DSQL_pretty (*ptr, column + 1);

        return;

	default:
		sprintf(s, "unknown type %d", node->nod_type);
		verb = s;
	}

	if (node->nod_desc.dsc_dtype) {
		trace_line("%s%s (%d,%d,%p)\n",
			   buffer, verb,
			   node->nod_desc.dsc_dtype,
			   node->nod_desc.dsc_length, node->nod_desc.dsc_address);
	} else {
		trace_line("%s%s\n", buffer, verb);
	}
	++column;

	while (ptr < end) {
		DSQL_pretty(*ptr++, column);
	}

	return;
}
#endif

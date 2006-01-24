%{
/* 
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		parse.y
 *	DESCRIPTION:	Dynamic SQL parser
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
 * 2002-02-24 Sean Leyne - Code Cleanup of old Win 3.1 port (WINDOWS_ONLY)
 * 2001.05.20 Neil McCalden: Allow a udf to be used in a 'group by' clause.
 * 2001.05.30 Claudio Valderrama: DROP TABLE and DROP VIEW lead now to two
 *   different node types so DDL can tell which is which.
 * 2001.06.13 Claudio Valderrama: SUBSTRING is being surfaced.
 * 2001.06.30 Claudio valderrama: Feed (line,column) for each node. See node.h.
 * 2001.07.10 Claudio Valderrama: Better (line,column) report and "--" for comments.
 * 2001.07.28 John Bellardo: Changes to support parsing LIMIT and FIRST
 * 2001.08.03 John Bellardo: Finalized syntax for LIMIT, change LIMIT to SKIP
 * 2001.08.05 Claudio Valderrama: closed Bug #448062 and other spaces that appear
 *   in rdb$*_source fields when altering domains plus one unexpected null pointer.
 * 2001.08.12 Claudio Valderrama: adjust SUBSTRING's starting pos argument here
 *   and not in gen.c; this closes Bug #450301.
 * 2001.10.01 Claudio Valderrama: enable explicit GRANT...to ROLE role_name.
 * 2001.10.06 Claudio Valderrama: Honor explicit USER keyword in GRANTs and REVOKEs.
 * 2002.07.05 Mark O'Donohue: change keyword DEBUG to KW_DEBUG to avoid
 *			clashes with normal DEBUG macro.
 * 2002.07.30 Arno Brinkman:  
 * 2002.07.30 	Let IN predicate handle value_expressions
 * 2002.07.30 	tokens CASE, NULLIF, COALESCE added
 * 2002.07.30 	See block < CASE expression > what is added to value as case_expression
 * 2002.07.30 	function is split up into aggregate_function, numeric_value_function, string_value_function, generate_value_function
 * 2002.07.30 	new group_by_function and added to grp_column_elem
 * 2002.07.30 	cast removed from function and added as cast_specification to value
 * 2002.08.04 Claudio Valderrama: allow declaring and defining variables at the same time
 * 2002.08.04 Dmitry Yemanov: ALTER VIEW
 * 2002.08.06 Arno Brinkman: ordinal added to grp_column_elem for using positions in group by
 * 2002.08.07 Dmitry Yemanov: INT64/LARGEINT are replaced with BIGINT and available in dialect 3 only
 * 2002.08.31 Dmitry Yemanov: allowed user-defined index names for PK/FK/UK constraints
 * 2002.09.01 Dmitry Yemanov: RECREATE VIEW
 * 2002.09.28 Dmitry Yemanov: Reworked internal_info stuff, enhanced
 *							exception handling in SPs/triggers,
 *							implemented ROWS_AFFECTED system variable
 * 2002.10.21 Nickolay Samofatov: Added support for explicit pessimistic locks
 * 2002.10.29 Nickolay Samofatov: Added support for savepoints
 * 2002.12.03 Dmitry Yemanov: Implemented ORDER BY clause in subqueries.
 * 2002.12.18 Dmitry Yemanov: Added support for SQL-compliant labels and LEAVE statement
 * 2002.12.28 Dmitry Yemanov: Added support for parametrized events.
 * 2003.01.14 Dmitry Yemanov: Fixed bug with cursors in triggers.
 * 2003.01.15 Dmitry Yemanov: Added support for runtime trigger action checks.
 * 2003.02.10 Mike Nordell  : Undefined Microsoft introduced macros to get a clean compile.
 * 2003.05.24 Nickolay Samofatov: Make SKIP and FIRST non-reserved keywords
 * 2003.06.13 Nickolay Samofatov: Make INSERTING/UPDATING/DELETING non-reserved keywords
 * 2003.07.01 Blas Rodriguez Somoza: Change DEBUG and IN to avoid conflicts in win32 build/bison
 * 2003.08.11 Arno Brinkman: Changed GROUP BY to support all expressions and added "AS" support
 *						   with table alias. Also removed group_by_function and ordinal.
 * 2003.08.14 Arno Brinkman: Added support for derived tables.
 * 2003.10.05 Dmitry Yemanov: Added support for explicit cursors in PSQL.
 * 2004.01.16 Vlad Horsun: added support for default parameters and 
 *   EXECUTE BLOCK statement
 */

/* AB:Sync FB 1.173 */

#if defined _AIX || defined MVS
#define SAVE_PAGE_SIZE PAGE_SIZE
#undef PAGE_SIZE
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "fbdev.h"
#include "../jrd/common.h"

#if defined _AIX || defined MVS
#undef PAGE_SIZE
#define PAGE_SIZE SAVE_PAGE_SIZE
#endif

#include "gen/iberror.h"
#include "../dsql/dsql.h"
#include "../jrd/ibase.h"
#include "../jrd/flags.h"
#include "../dsql/alld_proto.h"
#include "../dsql/errd_proto.h"
#include "../dsql/hsh_proto.h"
#include "../dsql/make_proto.h"
#include "../dsql/parse_proto.h"
#include "../dsql/keywords.h"
#include "../dsql/misc_func.h"
#include "../jrd/gds_proto.h"
#include "../jrd/thd_proto.h"
//#include "../jrd/err_proto.h"
#include "../dsql/SQLParse.h"
#include "../dsql/SQLSyntaxError.h"
#include "OSRIException.h"
#include "Procedure.h"
#include "val.h"
static void	yyerror(const TEXT*);

/* since UNIX isn't standard, we have to define
   stuff which is in <limits.h> (which isn't available
   on all UNIXes... */

const long SHRT_POS_MAX			= 32767;
const long SHRT_UNSIGNED_MAX	= 65535;
const long SHRT_NEG_MAX			= 32768;
const long LONG_POS_MAX			= 2147483647;
const int POSITIVE	= 0;
const int NEGATIVE	= 1;
const int UNSIGNED	= 2;

const int MIN_CACHE_BUFFERS	= 250;
const int DEF_CACHE_BUFFERS	= 1000;

/* Fix 69th procedure problem - solution from Oleg Loa */
#define YYSTACKSIZE	2048
#define YYMAXDEPTH	2048

#define YYSTYPE		DSQL_NOD
#if defined(DEBUG) || defined(DEV_BUILD)
#define YYDEBUG		1
#endif

static const char NULL_STRING[] = "";

inline SLONG trigger_type_suffix(const int slot1, const int slot2, const int slot3)
{
	return ((slot1 << 1) | (slot2 << 3) | (slot3 << 5));
}

#define YYPARSE_PARAM_TYPE
#define YYPARSE_PARAM sqlParse
#define YYLEX_PARAM sqlParse

#define yyparse				dsql_yyparse
#define MAKE_NODE			((SQLParse*) sqlParse)->makeNode
#define MAKE_FLAG_NODE		((SQLParse*) sqlParse)->makeFlagNode
#define MAKE_PARAMETER		((SQLParse*) sqlParse)->makeParameter
#define MAKE_LIST			((SQLParse*) sqlParse)->makeList
#define MAKE_FILE			((SQLParse*) sqlParse)->makeFile
#define MAKE_FIELD			((SQLParse*) sqlParse)->makeField
#define MAKE_CONSTANT		((SQLParse*) sqlParse)->makeConstant
#define MAKE_INTEGER(v)		((SQLParse*) sqlParse)->makeConstant(v)
#define MAKE_TRIGGER_TYPE	((SQLParse*) sqlParse)->makeTriggerType
#define MAKE_STRING			((SQLParse*) sqlParse)->makeString
#define MAKE_STRING_CONSTANT ((SQLParse*) sqlParse)->makeStringConstant

#include "../dsql/chars.h"

static void	prepare_console_debug (int, int  *);
static void	yyabandon (SSHORT, ISC_STATUS);


int yylex (YYSTYPE *lvalp, void* sqlParse)
{
	return ((SQLParse*) sqlParse)->getToken(lvalp);
}


%}

%pure_parser

/* token declarations */

/* Tokens are organized chronologically by date added.
   See dsql/keywords.cpp for a list organized alphabetically */

/* Tokens in v4.0 -- not separated into v3 and v4 tokens */

%token ACTIVE
%token ADD
%token AFTER
%token ALL
%token ALTER
%token AND
%token ANY
%token AS
%token ASC
%token AT
%token AVG
%token AUTO
//%token BASENAME
%token BEFORE
%token BEGIN
%token BETWEEN
%token BLOB
%token BY
//%token CACHE
%token CAST
%token CHARACTER
%token CHECK
//%token CHECK_POINT_LEN
%token COLLATE
%token COMMA
%token COMMIT
%token COMMITTED
%token COMPUTED
%token CONCATENATE
%token CONDITIONAL
%token CONSTRAINT
%token CONTAINING
%token COUNT
%token CREATE
%token CSTRING
%token CURRENT
%token CURSOR
%token DATABASE
%token DATE
%token DB_KEY
%token KW_DEBUG
%token DECIMAL
%token DECLARE
%token DEFAULT
%token KW_DELETE
%token DESC
%token DISTINCT
%token DO
%token DOMAIN
%token DROP
%token ELSE
%token END
%token ENTRY_POINT
%token EQL
%token ESCAPE
%token EXCEPTION
%token EXECUTE
%token EXISTS
%token EXIT
%token EXTERNAL
%token FILTER
%token FOR
%token FOREIGN
%token FROM
%token FULL
%token FUNCTION
%token GDSCODE
%token GEQ
%token GENERATOR
%token GEN_ID
%token GRANT
%token GROUP
//%token GROUP_COMMIT_WAIT
%token GTR
%token HAVING
%token IF
%token KW_IN
%token INACTIVE
%token INNER
%token INPUT_TYPE
%token INDEX
%token INSERT
%token INTEGER
%token INTO
%token IS
%token ISOLATION
%token JOIN
%token KEY
%token KW_CHAR
%token KW_DEC
%token KW_DOUBLE
%token KW_FILE
%token KW_FLOAT
%token KW_INT
%token KW_LONG
%token KW_NULL
%token KW_NUMERIC
%token KW_UPPER
%token KW_VALUE
%token LENGTH
//%token LOGFILE
%token LPAREN
%token LEFT
%token LEQ
%token LEVEL
%token LIKE
//%token LOG_BUF_SIZE
%token LSS
%token MANUAL
%token MAXIMUM
%token MAX_SEGMENT
%token MERGE
%token MESSAGE
%token MINIMUM
%token MODULE_NAME
%token NAMES
%token NATIONAL
%token NATURAL
%token NCHAR
%token NEQ
%token NO
%token NOT
%token NOT_GTR
%token NOT_LSS
//%token NUM_LOG_BUFS
%token OF
%token ON
%token ONLY
%token OPTION
%token OR
%token ORDER
%token OUTER
%token OUTPUT_TYPE
%token OVERFLOW
%token PAGE
%token PAGES
%token KW_PAGE_SIZE
%token PARAMETER
%token PASSWORD
%token PLAN
%token POSITION
%token POST_EVENT
%token PRECISION
%token PRIMARY
%token PRIVILEGES
%token PROCEDURE
%token PROTECTED
//%token RAW_PARTITIONS
%token READ
%token REAL
%token REFERENCES
%token RESERVING
%token RETAIN
%token RETURNING_VALUES
%token RETURNS
%token REVOKE
%token RIGHT
%token RPAREN
%token ROLLBACK
%token SEGMENT
%token SELECT
%token SET
%token SHADOW
%token KW_SHARED
%token SINGULAR
%token KW_SIZE
%token SMALLINT
%token SNAPSHOT
%token SOME
%token SORT
%token SQLCODE
%token STABILITY
%token STARTING
%token STATISTICS
%token SUB_TYPE
%token SUSPEND
%token SUM
%token TABLE
%token THEN
%token TO
%token TRANSACTION
%token TRIGGER
%token UNCOMMITTED
%token UNION
%token UNIQUE
%token UPDATE
%token USER
%token VALUES
%token VARCHAR
%token VARIABLE
%token VARYING
%token VERSION
%token VIEW
%token WAIT
%token WHEN
%token WHERE
%token WHILE
%token WITH
%token WORK
%token WRITE

%token FLOAT_NUMBER NUMBER NUMERIC SYMBOL STRING INTRODUCER 

/* New tokens added v5.0 */

%token ACTION
%token ADMIN
%token CASCADE
%token FREE_IT			/* ISC SQL extension */
%token RESTRICT
%token ROLE

/* New tokens added v6.0 */

%token COLUMN
%token TYPE
%token EXTRACT
%token YEAR
%token MONTH
%token DAY
%token HOUR
%token MINUTE
%token SECOND
%token WEEKDAY			/* ISC SQL extension */
%token YEARDAY			/* ISC SQL extension */
%token TIME
%token TIMESTAMP
%token CURRENT_DATE
%token CURRENT_TIME
%token CURRENT_TIMESTAMP

/* special aggregate token types returned by lex in v6.0 */

%token NUMBER64BIT SCALEDINT

/* CVC: Special Firebird additions. */

%token CURRENT_USER
%token CURRENT_ROLE
%token KW_BREAK
%token SUBSTRING
%token RECREATE
%token KW_DESCRIPTOR
%token FIRST
%token SKIP

/* tokens added for Firebird 1.5 */

%token CURRENT_CONNECTION
%token CURRENT_TRANSACTION
%token BIGINT
%token CASE
%token NULLIF
%token COALESCE
%token USING
%token NULLS
%token LAST
%token ROW_COUNT
%token LOCK
%token SAVEPOINT
%token RELEASE
%token STATEMENT
%token LEAVE
%token INSERTING
%token UPDATING
%token DELETING
/* Special pseudo-tokens introduced to handle case 
	when our grammar is not LARL(1) */
%token KW_INSERTING
%token KW_UPDATING
%token KW_DELETING

/* tokens added for Firebird 2.0 */

%token BACKUP
%token KW_DIFFERENCE
%token OPEN
%token CLOSE
%token FETCH
%token ROWS
%token BLOCK
%token IIF
%token SCALAR_ARRAY
%token CROSS
%token NEXT
%token SEQUENCE
%token RESTART

/* tokens added for Vulcan */

%token UPGRADE


/* precedence declarations for expression evaluation */

%left	OR
%left	AND
%left	NOT
%left	'=' '<' '>' LIKE EQL NEQ GTR LSS GEQ LEQ NOT_GTR NOT_LSS
%left	'+' '-'
%left	'*' '/'
%left	CONCATENATE
%left	COLLATE

/* Fix the dangling IF-THEN-ELSE problem */
%nonassoc THEN
%nonassoc ELSE

/* The same issue exists with ALTER COLUMN now that keywords can be used
   in order to change their names.  The syntax which shows the issue is:
	 ALTER COLUMN where column is part of the alter statement
	   or
	 ALTER COLUMN where column is the name of the column in the relation
*/
%nonassoc ALTER
%nonassoc COLUMN

%%

/* list of possible statements */

top		: statement
			{ ((SQLParse*) sqlParse)->parseTree = $1; }
		| statement ';'
			{ ((SQLParse*) sqlParse)->parseTree = $1; }
		;

statement	: alter
		| blob_io
		| commit
		| create
		| declare
		| delete
		| drop
		| grant
		| insert
		| exec_procedure
		| exec_block
		| recreate
		| replace
		| revoke
		| rollback
		| savepoint
		| select
		| set
		| update
		| upgrade
		| KW_DEBUG signed_short_integer
			{ prepare_console_debug ((int)(long) $2, &((SQLParse*) sqlParse)->yydebug);
			  $$ = MAKE_NODE (nod_null, (int) 0, NULL); }
		;


/* GRANT statement */

grant	: GRANT privileges ON table_noise simple_table_name
			TO non_role_grantee_list grant_option
			{ $$ = MAKE_NODE (nod_grant, (int) e_grant_count, 
					$2, $5, MAKE_LIST($7), $8); }
		| GRANT proc_privileges ON PROCEDURE simple_proc_name
			TO non_role_grantee_list grant_option
			{ $$ = MAKE_NODE (nod_grant, (int) e_grant_count, 
					$2, $5, MAKE_LIST($7), $8); }
		| GRANT role_name_list TO role_grantee_list role_admin_option
			{ $$ = MAKE_NODE (nod_grant, (int) e_grant_count, 
					MAKE_LIST($2), MAKE_LIST($4), NULL, $5); }
		;

table_noise	: TABLE
		|
		;

privileges	: ALL
			{ $$ = MAKE_NODE (nod_all, (int) 0, NULL); }
		| ALL PRIVILEGES
			{ $$ = MAKE_NODE (nod_all, (int) 0, NULL); }
		| privilege_list
			{ $$ = MAKE_LIST ($1); }
		;

privilege_list	: privilege
		| privilege_list ',' privilege
			{ $$ = MAKE_NODE (nod_list, (int) 2, $1, $3); }
		;

proc_privileges	: EXECUTE
			{ $$ = MAKE_LIST (MAKE_NODE (nod_execute, (int) 0, NULL)); }
		;

privilege	: SELECT
			{ $$ = MAKE_NODE (nod_select, (int) 0, NULL); }
		| INSERT
			{ $$ = MAKE_NODE (nod_insert, (int) 0, NULL); }
		| KW_DELETE
			{ $$ = MAKE_NODE (nod_delete, (int) 0, NULL); }
		| UPDATE column_parens_opt
			{ $$ = MAKE_NODE (nod_update, (int) 1, $2); }
		| REFERENCES column_parens_opt
			{ $$ = MAKE_NODE (nod_references, (int) 1, $2); }
		;

grant_option	: WITH GRANT OPTION
			{ $$ = MAKE_NODE (nod_grant, (int) 0, NULL); }
		|
			{ $$ = 0; }
		;

role_admin_option   : WITH ADMIN OPTION
			{ $$ = MAKE_NODE (nod_grant_admin, (int) 0, NULL); }
		|
			{ $$ = 0; }
		;

simple_proc_name: symbol_procedure_name
			{ $$ = MAKE_NODE (nod_procedure_name, (int) 1, $1); }
		;


/* REVOKE statement */

revoke	: REVOKE rev_grant_option privileges ON table_noise simple_table_name
			FROM non_role_grantee_list
			{ $$ = MAKE_NODE (nod_revoke, (int) e_grant_count,
					$3, $6, MAKE_LIST($8), $2); }
		| REVOKE rev_grant_option proc_privileges ON PROCEDURE simple_proc_name
			FROM non_role_grantee_list
			{ $$ = MAKE_NODE (nod_revoke, (int) e_grant_count,
					$3, $6, MAKE_LIST($8), $2); }
		| REVOKE rev_admin_option role_name_list FROM role_grantee_list
			{ $$ = MAKE_NODE (nod_revoke, (int) e_grant_count,
					MAKE_LIST($3), MAKE_LIST($5), NULL, $2); }
		; 

rev_grant_option : GRANT OPTION FOR
			{ $$ = MAKE_NODE (nod_grant, (int) 0, NULL); }
		|
			{ $$ = NULL; }
		;

rev_admin_option : ADMIN OPTION FOR
			{ $$ = MAKE_NODE (nod_grant_admin, (int) 0, NULL); }
		|
			{ $$ = NULL; }
		;

non_role_grantee_list	: grantee_list
		| user_grantee_list
		;

grantee_list	: grantee
		| grantee_list ',' grantee
			{ $$ = MAKE_NODE (nod_list, (int) 2, $1, $3); }
		| grantee_list ',' user_grantee
			{ $$ = MAKE_NODE (nod_list, (int) 2, $1, $3); }
		| user_grantee_list ',' grantee
			{ $$ = MAKE_NODE (nod_list, (int) 2, $1, $3); }
		;

grantee : PROCEDURE symbol_procedure_name
		{ $$ = MAKE_NODE (nod_proc_obj, (int) 1, $2); }
	| TRIGGER symbol_trigger_name
		{ $$ = MAKE_NODE (nod_trig_obj, (int) 1, $2); }
	| VIEW symbol_view_name
		{ $$ = MAKE_NODE (nod_view_obj, (int) 1, $2); }
	| ROLE symbol_role_name
			{ $$ = MAKE_NODE (nod_role_name, (int) 1, $2); }
	;

user_grantee_list : user_grantee
		| user_grantee_list ',' user_grantee
			{ $$ = MAKE_NODE (nod_list, (int) 2, $1, $3); }
		;

/* CVC: In the future we can deprecate the first implicit form since we'll support
explicit grant/revoke for both USER and ROLE keywords & object types. */

user_grantee	: symbol_user_name
		{ $$ = MAKE_NODE (nod_user_name, (int) 1, $1); }
	| USER symbol_user_name
		{ $$ = MAKE_NODE (nod_user_name, (int) 2, $2, NULL); }
	| GROUP symbol_user_name
		{ $$ = MAKE_NODE (nod_user_group, (int) 1, $2); }
	;

role_name_list  : role_name
		| role_name_list ',' role_name
			{ $$ = MAKE_NODE (nod_list, (int) 2, $1, $3); }
		;

role_name   : symbol_role_name
		{ $$ = MAKE_NODE (nod_role_name, (int) 1, $1); }
		;

role_grantee_list  : role_grantee
		| role_grantee_list ',' role_grantee
			{ $$ = MAKE_NODE (nod_list, (int) 2, $1, $3); }
		;

role_grantee   : symbol_user_name
		{ $$ = MAKE_NODE (nod_user_name, (int) 1, $1); }
	| USER symbol_user_name
		{ $$ = MAKE_NODE (nod_user_name, (int) 1, $2); }
		;


/* DECLARE operations */

declare		: DECLARE declare_clause
			{ $$ = $2;}
		;

declare_clause  : FILTER filter_decl_clause
			{ $$ = $2; }
		| EXTERNAL FUNCTION udf_decl_clause
			{ $$ = $3; }
		;


udf_decl_clause : symbol_UDF_name arg_desc_list1 RETURNS return_value1
			ENTRY_POINT sql_string MODULE_NAME sql_string
				{ $$ = MAKE_NODE (nod_def_udf, (int) e_udf_count, 
				$1, $6, $8, MAKE_LIST ($2), $4); }
		;

udf_data_type	: simple_type
		| BLOB
			{ ((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_blob; }
		| CSTRING '(' pos_short_integer ')' charset_clause
			{ 
			((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_cstring; 
			((SQLParse*) sqlParse)->g_field->fld_character_length = (USHORT)(long) $3; }
		;

arg_desc_list1	: 
		 	{ $$ = NULL; }
		| arg_desc_list	
		| '(' arg_desc_list ')'	
		 	{ $$ = $2; }
		;

arg_desc_list	: arg_desc
		| arg_desc_list ',' arg_desc
			{ $$ = MAKE_NODE (nod_list, (int) 2, $1, $3); }
		;

/*arg_desc	: init_data_type udf_data_type
  { $$ = $1; } */
  
arg_desc	: init_data_type udf_data_type param_mechanism
			{ $$ = MAKE_NODE (nod_udf_param, (int) e_udf_param_count,
							  $1, $3); }
		;

param_mechanism :
			{ $$ = NULL; } /* Beware: ddl.cpp converts this to mean FUN_reference. */
		| BY KW_DESCRIPTOR
			{ $$ = MAKE_INTEGER (FUN_descriptor); }
		| BY SCALAR_ARRAY
			{ $$ = MAKE_INTEGER (FUN_scalar_array); }
		| KW_NULL
			{ $$ = MAKE_INTEGER (FUN_ref_with_null); }
		;  

return_value1	: return_value
		| '(' return_value ')'
			{ $$ = $2; }
		;
		
return_value	: init_data_type udf_data_type return_mechanism
			{ $$ = MAKE_NODE (nod_udf_return_value, (int) e_udf_param_count,
							  $1, $3); }
		| PARAMETER pos_short_integer
			{ $$ = MAKE_NODE (nod_udf_return_value, (int) e_udf_param_count,
				NULL, MAKE_CONSTANT ((dsql_str*) $2, CONSTANT_SLONG));}
		;

return_mechanism :
			{ $$ = MAKE_INTEGER (FUN_reference); }
		| BY KW_VALUE
			{ $$ = MAKE_INTEGER (FUN_value); }
		| BY KW_DESCRIPTOR
			{ $$ = MAKE_INTEGER (FUN_descriptor); }
		| FREE_IT
			{ $$ = MAKE_INTEGER ((-1 * FUN_reference)); }
										 /* FUN_refrence with FREE_IT is -ve */
		| BY KW_DESCRIPTOR FREE_IT
			{ $$ = MAKE_INTEGER ((-1 * FUN_descriptor)); }
		;
		
filter_decl_clause : symbol_filter_name INPUT_TYPE blob_filter_subtype OUTPUT_TYPE blob_filter_subtype 
			ENTRY_POINT sql_string MODULE_NAME sql_string
				{ $$ = MAKE_NODE (nod_def_filter, (int) e_filter_count, 
						$1, $3, $5, $7, $9); }
		;

blob_filter_subtype :	symbol_blob_subtype_name
				{ $$ = MAKE_CONSTANT ((dsql_str*) $1, CONSTANT_STRING); }
		|
						signed_short_integer
				{ $$ = MAKE_CONSTANT ((dsql_str*) $1, CONSTANT_SLONG); }
		;

/* CREATE metadata operations */

create	 	: CREATE create_clause
			{ $$ = $2; }
				; 

create_clause	: EXCEPTION exception_clause
			{ $$ = $2; }
		| unique_opt order_direction INDEX symbol_index_name ON simple_table_name index_definition
			{ $$ = MAKE_NODE (nod_def_index, (int) e_idx_count, 
					$1, $2, $4, $6, $7); }
		| PROCEDURE procedure_clause
			{ $$ = $2; }
		| TABLE table_clause
			{ $$ = $2; }
		| TRIGGER def_trigger_clause
			{ $$ = $2; }
		| VIEW view_clause
			{ $$ = $2; }
		| GENERATOR generator_clause
			{ $$ = $2; }
		| SEQUENCE generator_clause
			{ $$ = $2; }
		| DATABASE db_clause
			{ $$ = $2; }
		| DOMAIN   domain_clause
			{ $$ = $2; }
		| SHADOW shadow_clause
			{ $$ = $2; }
		| ROLE role_clause
			{ $$ = $2; }
		| USER symbol_user_name user_clauses
			{ $$ = MAKE_NODE (nod_def_user, 2, $2, MAKE_LIST($3)); }
		;


recreate 	: RECREATE recreate_clause
			{ $$ = $2; }
		;

recreate_clause	: PROCEDURE rprocedure_clause
			{ $$ = $2; }
		| TABLE rtable_clause
			{ $$ = $2; }
		| VIEW rview_clause
			{ $$ = $2; }
/*
		| TRIGGER def_trigger_clause
			{ $$ = $2; }
		| DOMAIN rdomain_clause
			{ $$ = $2; }
*/
		| EXCEPTION rexception_clause
			{ $$ = $2; }
		;

replace	: CREATE OR ALTER replace_clause
			{ $$ = $4; }
		;

replace_clause	: PROCEDURE replace_procedure_clause
			{ $$ = $2; }
		| TRIGGER replace_trigger_clause
			{ $$ = $2; }
/*
		| VIEW replace_view_clause
			{ $$ = $2; }
*/
		| EXCEPTION replace_exception_clause
			{ $$ = $2; }
		;


upgrade	 	: UPGRADE upgrade_clause
			{ $$ = $2; }
				; 

upgrade_clause	: USER symbol_user_name user_clauses
			{ $$ = MAKE_NODE (nod_upg_user, 2, $2, MAKE_LIST($3)); }
				;


/* CREATE EXCEPTION */

exception_clause	: symbol_exception_name sql_string
			{ $$ = MAKE_NODE (nod_def_exception, (int) e_xcp_count, 
						$1, $2); }
		;

rexception_clause	: symbol_exception_name sql_string
			{ $$ = MAKE_NODE (nod_redef_exception, (int) e_xcp_count, 
						$1, $2); }
		;

replace_exception_clause	: symbol_exception_name sql_string
			{ $$ = MAKE_NODE (nod_replace_exception, (int) e_xcp_count, 
						$1, $2); }
		;

alter_exception_clause	: symbol_exception_name sql_string
			{ $$ = MAKE_NODE (nod_mod_exception, (int) e_xcp_count, 
						$1, $2); }
		;


/* CREATE INDEX */

unique_opt	: UNIQUE
			{ $$ = MAKE_NODE (nod_unique, 0, NULL); }
		|
			{ $$ = NULL; }
		;

index_definition : column_list 
			{ $$ = MAKE_LIST ($1); }
		| column_parens 
		| computed_by '(' begin_trigger value end_trigger ')'
			{ $$ = MAKE_NODE (nod_def_computed, 2, $4, $5); }
		;


/* CREATE SHADOW */
shadow_clause	: pos_short_integer manual_auto conditional sql_string
			first_file_length sec_shadow_files
		 	{ $$ = MAKE_NODE (nod_def_shadow, (int) e_shadow_count,
				 $1, $2, $3, $4, $5, MAKE_LIST ($6)); }
		;

manual_auto	: MANUAL
			{ $$ = MAKE_INTEGER (1); }
		| AUTO
			{ $$ = MAKE_INTEGER (0); }
		| 
			{ $$ = MAKE_INTEGER (0); }
		;

conditional	: 
			{ $$ = MAKE_INTEGER (0); }
		| CONDITIONAL
			{ $$ = MAKE_INTEGER (1); }
		;	

first_file_length : 
			{ $$ = (dsql_nod*) 0;}
		| LENGTH equals long_integer page_noise
			{ $$ = $3; }
		;

sec_shadow_files :
		 	{ $$ = NULL; }
		| db_file_list
		;

db_file_list	: db_file
		| db_file_list db_file
			{ $$ = MAKE_NODE (nod_list, (int) 2, $1, $2); }
		;


/* CREATE DOMAIN */

domain_clause	: column_def_name
		as_opt
		data_type
		begin_trigger
		domain_default_opt
		end_trigger
		domain_constraint_clause
		collate_clause
			{ $$ = MAKE_NODE (nod_def_domain, (int) e_dom_count,
										  $1, $5, $6, MAKE_LIST ($7), $8); }
				;

/*
rdomain_clause	: DOMAIN alter_column_name alter_domain_ops
						{ $$ = MAKE_NODE (nod_mod_domain, (int) e_alt_count,
										  $2, MAKE_LIST ($3)); }
*/

as_opt		: AS
				  { $$ = NULL; }
				| 
				  { $$ = NULL; }  
				;

domain_default_opt	: DEFAULT begin_trigger default_value
				{ $$ = $3; }
			|
				{ $$ = NULL; }
			;

domain_constraint_clause	: 
								  { $$ = NULL; }
				| domain_constraint_list
								; 

domain_constraint_list  	: domain_constraint_def
				| domain_constraint_list domain_constraint_def
								  { $$ = MAKE_NODE (nod_list, (int) 2, $1, $2); }
						;
domain_constraint_def		: domain_constraint
				  { $$ = MAKE_NODE (nod_rel_constraint, (int) 2, NULL, $1);}
 
						;	

domain_constraint	  	: null_constraint
				| domain_check_constraint
							;
								
null_constraint			: NOT KW_NULL
								  { $$ = MAKE_NODE (nod_null, (int) 0, NULL); }
								;

domain_check_constraint 	: begin_trigger CHECK '(' search_condition ')' end_trigger
				  { $$ = MAKE_NODE (nod_def_constraint, 
				  (int) e_cnstr_count, MAKE_STRING(NULL_STRING, 0), NULL, 
				  NULL, NULL, $4, NULL, $6, NULL, NULL); }
								;


/* CREATE SEQUENCE/GENERATOR */

generator_clause : symbol_generator_name
			{ $$ = MAKE_NODE (nod_def_generator, (int) e_gen_count, $1); }
		 ;


/* CREATE ROLE */

role_clause : symbol_role_name
			{ $$ = MAKE_NODE (nod_def_role, (int) 1, $1); }
		;

/* CREATE USER */

user_clauses : user_clause
		| user_clauses ',' user_clause
			{ $$ = MAKE_NODE (nod_list, 2, $1, $3); }
		;

user_clause :  PASSWORD sql_string
			{ $$ = MAKE_NODE (nod_password, 1, $2);} 
		| SET PASSWORD sql_string
			{ $$ = MAKE_NODE (nod_password, 1, $3);} 
		;

/* CREATE DATABASE */

db_clause	:  db_name db_initial_desc1 db_rem_desc1
			{ $$ = MAKE_NODE (nod_def_database, (int) e_cdb_count,
				 $1, MAKE_LIST($2), MAKE_LIST ($3));}
		;

equals		:
		| '='
		;

db_name		: sql_string
			{ ((SQLParse*) sqlParse)->log_defined = FALSE;
			  ((SQLParse*) sqlParse)->cache_defined = FALSE;
			  $$ = (dsql_nod*) $1; }
		;

db_initial_desc1 :  
			{$$ = NULL;}
		| db_initial_desc
		;

db_initial_desc : db_initial_option
		| db_initial_desc db_initial_option
			{ $$ = MAKE_NODE (nod_list, 2, $1, $2); }
		; 
 
db_initial_option: KW_PAGE_SIZE equals pos_short_integer 
			{ $$ = MAKE_NODE (nod_page_size, 1, $3);}
		| LENGTH equals long_integer page_noise
			{ $$ = MAKE_NODE (nod_file_length, 1, $3);}
		| USER sql_string
			{ $$ = MAKE_NODE (nod_user_name, 1, $2);} 
		| PASSWORD sql_string	
			{ $$ = MAKE_NODE (nod_password, 1, $2);} 
		| SET NAMES sql_string	
			{ $$ = MAKE_NODE (nod_lc_ctype, 1, $3);} 
		;

db_rem_desc1	:  
			{$$ = NULL;} 
		| db_rem_desc
		;

db_rem_desc	: db_rem_option
		| db_rem_desc db_rem_option
			{ $$ = MAKE_NODE (nod_list, 2, $1, $2); }
		;

db_rem_option   : db_file  
/*   		| db_cache 
		| db_log
		| db_log_option		
*/		
		| DEFAULT CHARACTER SET symbol_character_set_name
			{ $$ = MAKE_NODE (nod_dfl_charset, 1, $4);} 
		| KW_DIFFERENCE KW_FILE sql_string
			{ $$ = MAKE_NODE (nod_difference_file, 1, $3); }
		;
		
/*
db_log_option   : GROUP_COMMIT_WAIT equals long_integer
			{ $$ = MAKE_NODE (nod_group_commit_wait, 1, $3);}
		| CHECK_POINT_LEN equals long_integer
			{ $$ = MAKE_NODE (nod_check_point_len, 1, $3);}
		| NUM_LOG_BUFS equals pos_short_integer
			{ $$ = MAKE_NODE (nod_num_log_buffers, 1, $3);}
		| LOG_BUF_SIZE equals unsigned_short_integer
			{ $$ = MAKE_NODE (nod_log_buffer_size, 1, $3);}
		;

db_log		: db_default_log_spec
			{ if (((SQLParse*) sqlParse)->log_defined)
				yyabandon (-260, isc_log_redef);  
			*/	/* Log redefined */	/*
			  ((SQLParse*) sqlParse)->log_defined = TRUE;
			  $$ = $1; }
		| db_rem_log_spec
			{ if (((SQLParse*) sqlParse)->log_defined)
				yyabandon (-260, isc_log_redef);
			  ((SQLParse*) sqlParse)->log_defined = TRUE;
			  $$ = $1; }
		;	

db_rem_log_spec	: LOGFILE '(' logfiles ')' OVERFLOW logfile_desc
			{ ((SQLParse*) sqlParse)->g_file->fil_flags |= LOG_serial | LOG_overflow; 
			  if (((SQLParse*) sqlParse)->g_file->fil_partitions)
				  yyabandon (-261, isc_partition_not_supp);
		*/	/* Partitions not supported in series of log file specification */	/*
			 $$ = MAKE_NODE (nod_list, 2, $3, $6); }  
		| LOGFILE BASENAME logfile_desc
			{ ((SQLParse*) sqlParse)->g_file->fil_flags |= LOG_serial;
			  if (((SQLParse*) sqlParse)->g_file->fil_partitions)
				  yyabandon (-261, isc_partition_not_supp);
			  $$ = $3; }
		;

db_default_log_spec : LOGFILE 
			{ ((SQLParse*) sqlParse)->g_file = MAKE_FILE(); 
			  ((SQLParse*) sqlParse)->g_file->fil_flags = LOG_serial | LOG_default;
			  $$ = MAKE_NODE (nod_log_file_desc, (int) 1,
						(dsql_nod*) ((SQLParse*) sqlParse)->g_file);}
		;
*/

db_file		: file1 sql_string file_desc1
			{ ((SQLParse*) sqlParse)->g_file->fil_name = (dsql_str*) $2;
			  $$ = (dsql_nod*) MAKE_NODE (nod_file_desc, (int) 1,
						(dsql_nod*) ((SQLParse*) sqlParse)->g_file); }
		;

/*
db_cache	: CACHE sql_string cache_length 
			{ 
			  if (((SQLParse*) sqlParse)->cache_defined)
				  yyabandon (-260, isc_cache_redef);
				  */ /* Cache redefined */ /*
			  ((SQLParse*) sqlParse)->g_file = MAKE_FILE();
			  ((SQLParse*) sqlParse)->g_file->fil_length = (long) $3;
			  ((SQLParse*) sqlParse)->g_file->fil_name = (dsql_str*) $2;
			  ((SQLParse*) sqlParse)->cache_defined = TRUE;
			  $$ = (dsql_nod*) MAKE_NODE (nod_cache_file_desc, (int) 1,
					 (dsql_nod*) ((SQLParse*) sqlParse)->g_file); }
		;
*/

/*
cache_length	: 
			{ $$ = (dsql_nod*) (long) DEF_CACHE_BUFFERS; }
		| LENGTH equals long_integer page_noise
			{ if ((long) $3 < MIN_CACHE_BUFFERS)
				  yyabandon (-239, isc_cache_too_small);
				  */ /* Cache length too small */ /*
			  else 
				  $$ = (dsql_nod*) $3; }
		;

logfiles	: logfile_desc 
		| logfiles ',' logfile_desc
			{ $$ = MAKE_NODE (nod_list, 2, $1, $3); }
		;
logfile_desc	: logfile_name logfile_attrs 
			{ 
				 //check_log_file_attrs(); 
			 $$ = (dsql_nod*) MAKE_NODE (nod_log_file_desc, (int) 1,
												(dsql_nod*) ((SQLParse*) sqlParse)->g_file); }
		;
logfile_name	: sql_string 
			{ ((SQLParse*) sqlParse)->g_file = MAKE_FILE();
			  ((SQLParse*) sqlParse)->g_file->fil_name = (dsql_str*) $1; }
		;
logfile_attrs	: 
		| logfile_attrs logfile_attr
		;

logfile_attr	: KW_SIZE equals long_integer
			{ ((SQLParse*) sqlParse)->g_file->fil_length = (long) $3; }
		| RAW_PARTITIONS equals pos_short_integer
			{ ((SQLParse*) sqlParse)->g_file->fil_partitions = (SSHORT) $3; 
			  ((SQLParse*) sqlParse)->g_file->fil_flags |= LOG_raw; } 
		;
*/		

file1		: KW_FILE
			{ ((SQLParse*) sqlParse)->g_file  = MAKE_FILE ();}
		;

file_desc1	:
		| file_desc
		;

file_desc	: file_clause
		| file_desc file_clause
		;

file_clause	: STARTING file_clause_noise long_integer
			{ ((SQLParse*) sqlParse)->g_file->fil_start = (long) $3;}
		| LENGTH equals long_integer page_noise
			{ ((SQLParse*) sqlParse)->g_file->fil_length = (long) $3;}
		;

file_clause_noise :
		| AT
		| AT PAGE
		;

page_noise	:
		| PAGE
		| PAGES
		;


/* CREATE TABLE */

table_clause	: simple_table_name external_file '(' table_elements ')'
			{ $$ = MAKE_NODE (nod_def_relation, 
				(int) e_drl_count, $1, MAKE_LIST ($4), $2); }
		;

rtable_clause	: simple_table_name external_file '(' table_elements ')'
			{ $$ = MAKE_NODE (nod_redef_relation, 
				(int) e_drl_count, $1, MAKE_LIST ($4), $2); }
		;

external_file	: EXTERNAL KW_FILE sql_string
			{ $$ = $3; }
		| EXTERNAL sql_string
			{ $$ = $2; }
		|
			{ $$ = NULL; }
		;

table_elements	: table_element
		| table_elements ',' table_element
			{ $$ = MAKE_NODE (nod_list, 2, $1, $3); }
		;

table_element	: column_def
		| table_constraint_definition
		;



/* column definition */

column_def	: column_def_name 
			  data_type_or_domain       
			  domain_default_opt
			  end_trigger     
			  column_constraint_clause  
			  collate_clause
			{ $$ = MAKE_NODE (nod_def_field, (int) e_dfl_count, 
							  $1, $3, $4, MAKE_LIST ($5), $6, $2, NULL); }   
		| column_def_name non_array_type def_computed
			{ $$ = MAKE_NODE (nod_def_field, (int) e_dfl_count, 
					$1, NULL, NULL, NULL, NULL, NULL, $3); }   

		| column_def_name def_computed
			{ $$ = MAKE_NODE (nod_def_field, (int) e_dfl_count, 
					$1, NULL, NULL, NULL, NULL, NULL, $2); }   
		;
								 
/* value does allow parens around it, but there is a problem getting the
 * source text
 */

def_computed	: computed_by '(' begin_trigger value end_trigger ')'
			{ 
			((SQLParse*) sqlParse)->g_field->fld_flags |= FLD_computed;
			$$ = MAKE_NODE (nod_def_computed, 2, $4, $5); }
		;

computed_by	: COMPUTED BY
		| COMPUTED
		;

data_type_or_domain	: data_type begin_trigger
						  { $$ = NULL; }
			| simple_column_name begin_string
						  { $$ = MAKE_NODE (nod_def_domain, (int) e_dom_count,
											$1, NULL, NULL, NULL, NULL); }
						;

collate_clause	: COLLATE symbol_collation_name
			{ $$ = $2; }
		|
			{ $$ = NULL; }
		;


column_def_name	: simple_column_name
			{ ((SQLParse*) sqlParse)->g_field_name = $1;
			  ((SQLParse*) sqlParse)->g_field = MAKE_FIELD ($1);
			  $$ = (dsql_nod*) ((SQLParse*) sqlParse)->g_field; }
		;

simple_column_def_name  : simple_column_name
				{ ((SQLParse*) sqlParse)->g_field = MAKE_FIELD ($1);
				  $$ = (dsql_nod*) ((SQLParse*) sqlParse)->g_field; }
			;


data_type_descriptor :	init_data_type data_type
			{ $$ = $1; }
		;

init_data_type :
			{ ((SQLParse*) sqlParse)->g_field = MAKE_FIELD (NULL);
			  $$ = (dsql_nod*) ((SQLParse*) sqlParse)->g_field; }
		;


default_opt	: DEFAULT default_value
			{ $$ = $2; }
		|
			{ $$ = NULL; }
		;

default_value	: constant
		| current_user
		| current_role
		| internal_info
		| null_value
		| datetime_value_expression
		;
				   
column_constraint_clause : 
				{ $$ = NULL; }
			| column_constraint_list
			;

column_constraint_list	: column_constraint_def
				| column_constraint_list column_constraint_def
			{ $$ = MAKE_NODE (nod_list, (int) 2, $1, $2); }
				;

column_constraint_def : constraint_name_opt column_constraint
			{ $$ = MAKE_NODE (nod_rel_constraint, (int) 2, $1, $2);}
		;


column_constraint : NOT KW_NULL 
						{ $$ = MAKE_NODE (nod_null, (int) 1, NULL); }
				  | REFERENCES simple_table_name column_parens_opt
			referential_trigger_action constraint_index_opt
						{ $$ = MAKE_NODE (nod_foreign, (int) e_for_count,
						MAKE_NODE (nod_list, (int) 1, ((SQLParse*) sqlParse)->g_field_name), $2, $3, $4, $5); }

				  | check_constraint
				  | UNIQUE constraint_index_opt
						{ $$ = MAKE_NODE (nod_unique, 2, NULL, $2); }
				  | PRIMARY KEY constraint_index_opt
						{ $$ = MAKE_NODE (nod_primary, (int) e_pri_count, NULL, $3); }
		;
					


/* table constraints */

table_constraint_definition : constraint_name_opt table_constraint
		   { $$ = MAKE_NODE (nod_rel_constraint, (int) 2, $1, $2);}
					  ;

constraint_name_opt : CONSTRAINT symbol_constraint_name
					  { $$ = $2; }
					| { $$ = NULL ;}
					;

table_constraint : unique_constraint
			| primary_constraint
			| referential_constraint
			| check_constraint
		;

unique_constraint	: UNIQUE column_parens constraint_index_opt
			{ $$ = MAKE_NODE (nod_unique, 2, $2, $3); }
		;

primary_constraint	: PRIMARY KEY column_parens constraint_index_opt
			{ $$ = MAKE_NODE (nod_primary, (int) e_pri_count, $3, $4); }
		;

referential_constraint	: FOREIGN KEY column_parens REFERENCES 
			  simple_table_name column_parens_opt 
			  referential_trigger_action constraint_index_opt
			{ $$ = MAKE_NODE (nod_foreign, (int) e_for_count, $3, $5, 
					 $6, $7, $8); }
		;

constraint_index_opt	: USING order_direction INDEX symbol_index_name
			{ $$ = MAKE_NODE (nod_def_index, (int) e_idx_count, 
					NULL, $2, $4, NULL, NULL); }
/*
		| NO INDEX
			{ $$ = NULL; }
*/
		|
			{ $$ = MAKE_NODE (nod_def_index, (int) e_idx_count, 
					NULL, NULL, NULL, NULL, NULL); }
		;

check_constraint : begin_trigger CHECK '(' search_condition ')' end_trigger
			{ $$ = MAKE_NODE (nod_def_constraint, 
				(int) e_cnstr_count, MAKE_STRING(NULL_STRING, 0), NULL, 
				NULL, NULL, $4, NULL, $6, NULL, NULL); }
		;

referential_trigger_action:	
		  update_rule
		  { $$ = MAKE_NODE (nod_ref_upd_del, (int) e_ref_upd_del_count, $1, NULL);} 
		| delete_rule
		  { $$ = MAKE_NODE (nod_ref_upd_del, (int) e_ref_upd_del_count, NULL, $1);}
		| delete_rule update_rule
		  { $$ = MAKE_NODE (nod_ref_upd_del, (int) e_ref_upd_del_count, $2, $1); }
		| update_rule delete_rule
		  { $$ = MAKE_NODE (nod_ref_upd_del, (int) e_ref_upd_del_count, $1, $2);}
		| /* empty */
		  { $$ = NULL;}
		;

update_rule	: ON UPDATE referential_action
		  { $$ = $3;}
		;
delete_rule	: ON KW_DELETE referential_action
		  { $$ = $3;}
		;

referential_action: CASCADE
		  { $$ = MAKE_FLAG_NODE (nod_ref_trig_action, 
			 REF_ACTION_CASCADE, (int) e_ref_trig_action_count, NULL);}
				| SET DEFAULT
		  { $$ = MAKE_FLAG_NODE (nod_ref_trig_action, 
			 REF_ACTION_SET_DEFAULT, (int) e_ref_trig_action_count, NULL);}
		| SET KW_NULL
		  { $$ = MAKE_FLAG_NODE (nod_ref_trig_action, 
			 REF_ACTION_SET_NULL, (int) e_ref_trig_action_count, NULL);}
		| NO ACTION
		  { $$ = MAKE_FLAG_NODE (nod_ref_trig_action, 
			 REF_ACTION_NONE, (int) e_ref_trig_action_count, NULL);}
		;


/* PROCEDURE */


procedure_clause	: symbol_procedure_name input_parameters
			 	  output_parameters
				  AS begin_string
			  local_declaration_list
			  full_proc_block
			  end_trigger
				{ $$ = MAKE_NODE (nod_def_procedure,
						(int) e_prc_count, $1, $2, $3, $6, $7, $8); } 
		;		


rprocedure_clause	: symbol_procedure_name input_parameters
			 	  output_parameters
				  AS begin_string
			  local_declaration_list
			  full_proc_block
			  end_trigger
				{ $$ = MAKE_NODE (nod_redef_procedure,
						(int) e_prc_count, $1, $2, $3, $6, $7, $8); } 
		;		

replace_procedure_clause	: symbol_procedure_name input_parameters
			 	  output_parameters
				  AS begin_string
			  local_declaration_list
			  full_proc_block
			  end_trigger
				{ $$ = MAKE_NODE (nod_replace_procedure,
						(int) e_prc_count, $1, $2, $3, $6, $7, $8); } 
		;		

alter_procedure_clause	: symbol_procedure_name input_parameters
			 	  output_parameters
				  AS begin_string
			  local_declaration_list
			  full_proc_block
			  end_trigger
				{ $$ = MAKE_NODE (nod_mod_procedure,
						(int) e_prc_count, $1, $2, $3, $6, $7, $8); } 
		;		

input_parameters :	'(' input_proc_parameters ')'
			{ $$ = MAKE_LIST ($2); }
		|
			{ $$ = NULL; }
		;

output_parameters :	RETURNS '(' output_proc_parameters ')'
			{ $$ = MAKE_LIST ($3); }
		|
			{ $$ = NULL; }
		;

input_proc_parameters	: input_proc_parameter
		| input_proc_parameters ',' input_proc_parameter
			{ $$ = MAKE_NODE (nod_list, 2, $1, $3); }
		;

input_proc_parameter	: simple_column_def_name non_array_type
				default_par_opt end_trigger 
			{ $$ = MAKE_NODE (nod_def_field, (int) e_dfl_count, 
				$1, $3, $4, NULL, NULL, NULL, NULL); }   
		;
		
output_proc_parameters	: proc_parameter
		| output_proc_parameters ',' proc_parameter
			{ $$ = MAKE_NODE (nod_list, 2, $1, $3); }
		;

proc_parameter	: simple_column_def_name non_array_type
			{ $$ = MAKE_NODE (nod_def_field, (int) e_dfl_count, 
				$1, NULL, NULL, NULL, NULL, NULL, NULL); }   
		;

default_par_opt	: DEFAULT begin_string default_value
			{ $$ = $3; }
		| '=' begin_string default_value
			{ $$ = $3; }
		| begin_string
			{ $$ = (dsql_nod*) NULL; }
		;

local_declaration_list	: local_declarations
			{ $$ = MAKE_LIST ($1); }
		|
			{ $$ = NULL; }
		;

local_declarations	: local_declaration
		| local_declarations local_declaration
			{ $$ = MAKE_NODE (nod_list, 2, $1, $2); }
		;

local_declaration : DECLARE var_decl_opt local_declaration_item ';'
			{ $$ = $3; }
		;

local_declaration_item	: var_declaration_item
		| cursor_declaration_item
		;

var_declaration_item	: column_def_name non_array_type var_init_opt
			{ $$ = MAKE_NODE (nod_def_field, (int) e_dfl_count, 
				$1, $3, NULL, NULL, NULL, NULL, NULL); }
		;

var_decl_opt	: VARIABLE
			{ $$ = NULL; }
		|
			{ $$ = NULL; }
		;

var_init_opt	: '=' default_value
			{ $$ = $2; }
		| default_opt
			{ $$ = $1; }
		;

cursor_declaration_item	: symbol_cursor_name CURSOR FOR '(' select ')'
			{ $$ = MAKE_FLAG_NODE (nod_cursor, NOD_CURSOR_EXPLICIT,
				(int) e_cur_count, $1, $5, NULL, NULL); }
 		;

proc_block	: proc_statement
		| full_proc_block
		;

full_proc_block	: BEGIN full_proc_block_body END
			{ $$ = $2; }
		;

full_proc_block_body	: proc_statements
			{ $$ = MAKE_NODE (nod_block, (int) e_blk_count, MAKE_LIST ($1), NULL); }
		| proc_statements excp_hndl_statements
			{ $$ = MAKE_NODE (nod_block, (int) e_blk_count, MAKE_LIST ($1), MAKE_LIST ($2)); }
		|
			{ $$ = MAKE_NODE (nod_block, (int) e_blk_count, NULL, NULL);}
		;							

proc_statements	: proc_block
		| proc_statements proc_block
			{ $$ = MAKE_NODE (nod_list, 2, $1, $2); }
		;

proc_statement	: assignment ';'
		| insert ';'
		| update ';'
		| delete ';'
		| singleton_select ';'
		| exec_procedure ';'
		| exec_sql ';'
		| exec_into ';'
		| exec_udf ';'
		| excp_statement ';'
		| raise_statement ';'
		| post_event ';'
		| cursor_statement ';'
		| breakleave ';'
		| SUSPEND ';'
			{ $$ = MAKE_NODE (nod_return, (int) e_rtn_count, NULL); }
		| EXIT ';'
			{ $$ = MAKE_NODE (nod_exit, (int) 0, NULL); }
		| if_then_else
		| while
		| for_select
		| for_exec_into
		;

excp_statement	: EXCEPTION symbol_exception_name
			{ $$ = MAKE_NODE (nod_exception_stmt, (int) e_xcp_count, $2, NULL); }
		| EXCEPTION symbol_exception_name value
			{ $$ = MAKE_NODE (nod_exception_stmt, (int) e_xcp_count, $2, $3); }
		;

raise_statement	: EXCEPTION
			{ $$ = MAKE_NODE (nod_exception_stmt, (int) e_xcp_count, NULL, NULL); }
		;

exec_sql	: EXECUTE STATEMENT value
			{ $$ = MAKE_NODE (nod_exec_sql, (int) e_exec_sql_count, $3); }
		;

for_select	: label_opt FOR select INTO variable_list cursor_def DO proc_block
			{ $$ = MAKE_NODE (nod_for_select, (int) e_flp_count, $3,
					  MAKE_LIST ($5), $6, $8, $1); }
		;

for_exec_into	: label_opt FOR EXECUTE STATEMENT value INTO variable_list DO proc_block 
			{ $$ = MAKE_NODE (nod_exec_into, (int) e_exec_into_count, $5, $9, MAKE_LIST ($7), $1); }
		;

exec_into	: EXECUTE STATEMENT value INTO variable_list
			{ $$ = MAKE_NODE (nod_exec_into, (int) e_exec_into_count, $3, (int) 0, MAKE_LIST ($5)); }
		;

if_then_else	: IF '(' search_condition ')' THEN proc_block ELSE proc_block
			{ $$ = MAKE_NODE (nod_if, (int) e_if_count, $3, $6, $8); }
		| IF '(' search_condition ')' THEN proc_block 
			{ $$ = MAKE_NODE (nod_if, (int) e_if_count, $3, $6, NULL); }
		;

post_event	: POST_EVENT value event_argument_opt
			{ $$ = MAKE_NODE (nod_post, (int) e_pst_count, $2, $3); }
		;

event_argument_opt	: /*',' value
			{ $$ = $2; }
		|*/
			{ $$ = NULL; }
		;

singleton_select	: select INTO variable_list
			{ $$ = MAKE_NODE (nod_for_select, (int) e_flp_count, $1,
					  MAKE_LIST ($3), NULL, NULL); }
		;

variable	: ':' symbol_variable_name
			{ $$ = MAKE_NODE (nod_var_name, (int) e_vrn_count, 
							$2); }
		;

variable_list	: variable
 		| column_name
		| variable_list ',' column_name
			{ $$ = MAKE_NODE (nod_list, 2, $1, $3); }
		| variable_list ',' variable
			{ $$ = MAKE_NODE (nod_list, 2, $1, $3); }
		;

while		: label_opt WHILE '(' search_condition ')' DO proc_block
			{ $$ = MAKE_NODE (nod_while, (int) e_while_count, $4, $7, $1); }
		;

label_opt	: symbol_label_name ':'
			{ $$ = MAKE_NODE (nod_label, (int) e_label_count, $1, NULL); }
		|
			{ $$ = NULL; }
		;

breakleave	: KW_BREAK
			{ $$ = MAKE_NODE (nod_breakleave, (int) e_breakleave_count, NULL); }
		| LEAVE
			{ $$ = MAKE_NODE (nod_breakleave, (int) e_breakleave_count, NULL); }
		| LEAVE symbol_label_name
			{ $$ = MAKE_NODE (nod_breakleave, (int) e_breakleave_count,
				MAKE_NODE (nod_label, (int) e_label_count, $2, NULL)); }
		;

cursor_def	: AS CURSOR symbol_cursor_name
			{ $$ = MAKE_FLAG_NODE (nod_cursor, NOD_CURSOR_FOR,
				(int) e_cur_count, $3, NULL, NULL, NULL); }
		|
			{ $$ = NULL; }
		;

excp_hndl_statements	: excp_hndl_statement
		| excp_hndl_statements excp_hndl_statement
			{ $$ = MAKE_NODE (nod_list, 2, $1, $2); }
		;

excp_hndl_statement	: WHEN errors DO proc_block
			{ $$ = MAKE_NODE (nod_on_error, (int) e_err_count,
					MAKE_LIST ($2), $4); }
		;

errors	: err
	| errors ',' err
		{ $$ = MAKE_NODE (nod_list, 2, $1, $3); }
	;

err	: SQLCODE signed_short_integer
		{ $$ = MAKE_NODE (nod_sqlcode, 1, $2); }
	| GDSCODE symbol_gdscode_name
		{ $$ = MAKE_NODE (nod_gdscode, 1, $2); }
	| EXCEPTION symbol_exception_name
		{ $$ = MAKE_NODE (nod_exception, 1, $2); }
	| ANY
		{ $$ = MAKE_NODE (nod_default, 1, NULL); }
	;

cursor_statement	: open_cursor
	| fetch_cursor
	| close_cursor
	;

open_cursor	: OPEN symbol_cursor_name
		{ $$ = MAKE_NODE (nod_cursor_open, (int) e_cur_stmt_count, $2, NULL, NULL); }
	;

close_cursor	: CLOSE symbol_cursor_name
		{ $$ = MAKE_NODE (nod_cursor_close, (int) e_cur_stmt_count, $2, NULL, NULL); }
	;

fetch_cursor	: FETCH fetch_opt symbol_cursor_name INTO variable_list
		{ $$ = MAKE_NODE (nod_cursor_fetch, (int) e_cur_stmt_count, $3, $2, MAKE_LIST ($5)); }
	;

fetch_opt	:
		{ $$ = NULL; }
	;
/*
fetch_opt	: fetch_seek_opt FROM
	;

fetch_seek_opt	:
	| FIRST
		{ $$ = MAKE_NODE (nod_fetch_seek, 2,
				// corresponds to (blr_bof_forward, 0)
				MAKE_INTEGER (3),
				MAKE_INTEGER (0)); }
	| LAST
		{ $$ = MAKE_NODE (nod_fetch_seek, 2,
				// corresponds to (blr_eof_backward, 0)
				MAKE_INTEGER (4),
				MAKE_INTEGER (0)); }
	| PRIOR
		{ $$ = MAKE_NODE (nod_fetch_seek, 2,
				// corresponds to (blr_backward, 1)
				MAKE_INTEGER (2),
				MAKE_INTEGER (1)); }
	| NEXT
		{ $$ = MAKE_NODE (nod_fetch_seek, 2,
				// corresponds to (blr_forward, 1)
				MAKE_INTEGER (1),
				MAKE_INTEGER (1)); }
	| ABSOLUTE value
		{ $$ = MAKE_NODE (nod_fetch_seek, 2,
				// corresponds to (blr_bof_forward, value)
				MAKE_INTEGER (3),
				$2); }
	| RELATIVE value
		{ $$ = MAKE_NODE (nod_fetch_seek, 2,
				// corresponds to (blr_forward, value)
				MAKE_INTEGER (1),
				$2); }
	;
*/
/* Direct EXECUTE PROCEDURE */

exec_procedure : EXECUTE PROCEDURE symbol_procedure_name proc_inputs proc_outputs_opt
			{ $$ = MAKE_NODE (nod_exec_procedure, (int) e_exe_count, 
					$3, $4, $5); }
		;
		
proc_inputs	: value_list
			{ $$ = MAKE_LIST ($1); }
		| '(' value_list ')'
			{ $$ = MAKE_LIST ($2); }
		|
			{ $$ = NULL; }
		;

proc_outputs_opt	: RETURNING_VALUES variable_list
			{ $$ = MAKE_LIST ($2); }
		| RETURNING_VALUES '(' variable_list  ')'
			{ $$ = MAKE_LIST ($3); }
		|
			{ $$ = NULL; }
		;

/* EXECUTE BLOCK */

exec_block : EXECUTE BLOCK block_input_params output_parameters AS 
			local_declaration_list
			full_proc_block
				{ $$ = MAKE_NODE (nod_exec_block,
						  (int) e_exe_blk_count, 
					          $3, $4, $6, $7, MAKE_NODE (nod_all, (int) 0, NULL)); } 
		;

block_input_params :	'(' block_parameters ')'
				{ $$ = MAKE_LIST ($2); }
			|
				{ $$ = NULL; }
			;

block_parameters	: block_parameter
		| block_parameters ',' block_parameter
			{ $$ = MAKE_NODE (nod_list, 2, $1, $3); }
		;

block_parameter	: proc_parameter '=' parameter
			{ $$ = MAKE_NODE (nod_param_val, e_prm_val_count, $1, $3); }   
		;

/* CREATE VIEW */

view_clause	: symbol_view_name column_parens_opt AS begin_string select_expr 
															check_opt end_string
			{ $$ = MAKE_NODE (nod_def_view, (int) e_view_count, 
					  $1, $2, $5, $6, $7); }   
		;		


rview_clause	: symbol_view_name column_parens_opt AS begin_string select_expr 
															check_opt end_string
			{ $$ = MAKE_NODE (nod_redef_view, (int) e_view_count, 
					  $1, $2, $5, $6, $7); }   
		;		

/*
replace_view_clause	: symbol_view_name column_parens_opt AS begin_string select_expr 
															check_opt end_string
			{ $$ = MAKE_NODE (nod_replace_view, (int) e_view_count, 
					  $1, $2, $5, $6, $7); }   
		;		

alter_view_clause	: symbol_view_name column_parens_opt AS begin_string select_expr 
															check_opt end_string
 			{ $$ = MAKE_NODE (nod_mod_view, (int) e_view_count, 
					  $1, $2, $5, $6, $7); }   
 		;		
*/


/* these rules will capture the input string for storage in metadata */

begin_string	: 
			{ ((SQLParse*) sqlParse)->beginning = ((SQLParse*) sqlParse)->ptr; }
		;

end_string	:
			{ $$ = (dsql_nod*) MAKE_STRING(((SQLParse*) sqlParse)->beginning,
				   (((SQLParse*) sqlParse)->ptr == ((SQLParse*) sqlParse)->end) ?
				   ((SQLParse*) sqlParse)->ptr - ((SQLParse*) sqlParse)->beginning : ((SQLParse*) sqlParse)->last_token - ((SQLParse*) sqlParse)->beginning);}
		;

begin_trigger	: 
			{ ((SQLParse*) sqlParse)->beginning = ((SQLParse*) sqlParse)->last_token; }
		;

end_trigger	:
			{ $$ = (dsql_nod*) MAKE_STRING(((SQLParse*) sqlParse)->beginning,
					((SQLParse*) sqlParse)->ptr - ((SQLParse*) sqlParse)->beginning); }
		;


check_opt	: WITH CHECK OPTION
			{ $$ = MAKE_NODE (nod_def_constraint, (int) e_cnstr_count, 
					MAKE_STRING(NULL_STRING, 0), NULL, NULL, NULL, 
					NULL, NULL, NULL, NULL, NULL); }
		|
			{ $$ = 0; }
		;



/* CREATE TRIGGER */

def_trigger_clause : symbol_trigger_name FOR simple_table_name
		trigger_active
		trigger_type
		trigger_position
		begin_trigger
		trigger_action
		end_trigger
			{ $$ = MAKE_NODE (nod_def_trigger, (int) e_trg_count,
				$1, $3, $4, $5, $6, $8, $9, NULL); }
		;

replace_trigger_clause : symbol_trigger_name FOR simple_table_name
		trigger_active
		trigger_type
		trigger_position
		begin_trigger
		trigger_action
		end_trigger
			{ $$ = MAKE_NODE (nod_replace_trigger, (int) e_trg_count,
				$1, $3, $4, $5, $6, $8, $9, NULL); }
		;

trigger_active	: ACTIVE 
			{ $$ = MAKE_INTEGER (0); }
		| INACTIVE
			{ $$ = MAKE_INTEGER (1); }
		|
			{ $$ = NULL; }
		;

trigger_type	: trigger_type_prefix trigger_type_suffix
			{ $$ = MAKE_TRIGGER_TYPE ($1, $2); }
		;

trigger_type_prefix	: BEFORE
			{ $$ = MAKE_INTEGER (0); }
		| AFTER
			{ $$ = MAKE_INTEGER (1); }
		;

trigger_type_suffix	: INSERT
			{ $$ = MAKE_INTEGER (trigger_type_suffix (1, 0, 0)); }
		| UPDATE
			{ $$ = MAKE_INTEGER (trigger_type_suffix (2, 0, 0)); }
		| KW_DELETE
			{ $$ = MAKE_INTEGER (trigger_type_suffix (3, 0, 0)); }
		| INSERT OR UPDATE
			{ $$ = MAKE_INTEGER (trigger_type_suffix (1, 2, 0)); }
		| INSERT OR KW_DELETE
			{ $$ = MAKE_INTEGER (trigger_type_suffix (1, 3, 0)); }
		| UPDATE OR INSERT
			{ $$ = MAKE_INTEGER (trigger_type_suffix (2, 1, 0)); }
		| UPDATE OR KW_DELETE
			{ $$ = MAKE_INTEGER (trigger_type_suffix (2, 3, 0)); }
		| KW_DELETE OR INSERT
			{ $$ = MAKE_INTEGER (trigger_type_suffix (3, 1, 0)); }
		| KW_DELETE OR UPDATE
			{ $$ = MAKE_INTEGER (trigger_type_suffix (3, 2, 0)); }
		| INSERT OR UPDATE OR KW_DELETE
			{ $$ = MAKE_INTEGER (trigger_type_suffix (1, 2, 3)); }
		| INSERT OR KW_DELETE OR UPDATE
			{ $$ = MAKE_INTEGER (trigger_type_suffix (1, 3, 2)); }
		| UPDATE OR INSERT OR KW_DELETE
			{ $$ = MAKE_INTEGER (trigger_type_suffix (2, 1, 3)); }
		| UPDATE OR KW_DELETE OR INSERT
			{ $$ = MAKE_INTEGER (trigger_type_suffix (2, 3, 1)); }
		| KW_DELETE OR INSERT OR UPDATE
			{ $$ = MAKE_INTEGER (trigger_type_suffix (3, 1, 2)); }
		| KW_DELETE OR UPDATE OR INSERT
			{ $$ = MAKE_INTEGER (trigger_type_suffix (3, 2, 1)); }
		;

trigger_position : POSITION nonneg_short_integer
			{ $$ = MAKE_CONSTANT ((dsql_str*) $2, CONSTANT_SLONG); }
		|
			{ $$ = NULL; }
		;

trigger_action : AS begin_trigger local_declaration_list full_proc_block
			{ $$ = MAKE_NODE (nod_list, (int) e_trg_act_count, $3, $4); }
		;

/* ALTER statement */

alter		: ALTER alter_clause
			{ $$ = $2; }
				; 

alter_clause	: EXCEPTION alter_exception_clause
			{ $$ = $2; }
		| TABLE simple_table_name alter_ops
			{ $$ = MAKE_NODE (nod_mod_relation, (int) e_alt_count, 
						$2, MAKE_LIST ($3)); }
/*
 		| VIEW alter_view_clause
 			{ $$ = $2; }
*/
		| TRIGGER alter_trigger_clause
			{ $$ = $2; }
		| PROCEDURE alter_procedure_clause
			{ $$ = $2; }
		| DATABASE init_alter_db alter_db
			{ $$ = MAKE_NODE (nod_mod_database, (int) e_adb_count,
				MAKE_LIST ($3)); }
				| DOMAIN alter_column_name alter_domain_ops
						{ $$ = MAKE_NODE (nod_mod_domain, (int) e_alt_count,
										  $2, MAKE_LIST ($3)); }
		| INDEX alter_index_clause
						{ $$ = MAKE_NODE (nod_mod_index,  e_mod_idx_count, $2); }
		| USER symbol_user_name user_clauses
			{ $$ = MAKE_NODE (nod_mod_user, 2, $2, MAKE_LIST($3)); }
		| SEQUENCE alter_sequence_clause
			{ $$ = $2; }
		;

domain_default_opt2	: DEFAULT begin_trigger default_value
				{ $$ = $3; }
		;

domain_check_constraint2 	: CHECK begin_trigger '(' search_condition ')' end_trigger
				  { $$ = MAKE_NODE (nod_def_constraint, 
				  (int) e_cnstr_count, MAKE_STRING(NULL_STRING, 0), NULL, 
				  NULL, NULL, $4, NULL, $6, NULL, NULL); }
								;

alter_domain_ops	: alter_domain_op
			| alter_domain_ops alter_domain_op
			  { $$ = MAKE_NODE (nod_list, 2, $1, $2); }
			;

alter_domain_op		: SET begin_trigger domain_default_opt2 end_trigger
						  { $$ = MAKE_NODE (nod_def_default, (int) e_dft_count,
						$3, $4); }			  
/*			SET begin_string default_opt end_trigger
						  { $$ = MAKE_NODE (nod_def_default, (int) e_dft_count,
						$3, $4); }
						| begin_trigger default_opt end_trigger
						  { $$ = MAKE_NODE (nod_def_default, (int) e_dft_count,
						$2, $3); }			  */
						| ADD CONSTRAINT domain_check_constraint2
						  { $$ = $3; } 
/*						| ADD CONSTRAINT domain_check_constraint
						  { $$ = $3; }						 */
						| ADD domain_check_constraint
						  { $$ = $2; } 
			| DROP DEFAULT
						  {$$ = MAKE_NODE (nod_del_default, 0, NULL); }
			| DROP CONSTRAINT
						  { $$ = MAKE_NODE (nod_delete_rel_constraint, (int) 1, NULL); }
			| TO simple_column_name
			  { $$ = $2; }
			| TYPE init_data_type non_array_type 
			  { $$ = MAKE_NODE (nod_mod_domain_type, 2, $2); }
			;

alter_ops	: alter_op
		| alter_ops ',' alter_op
			{ $$ = MAKE_NODE (nod_list, 2, $1, $3); }
		;

alter_op	: DROP simple_column_name drop_behaviour
			{ $$ = MAKE_NODE (nod_del_field, 2, $2, $3); }
				| DROP CONSTRAINT symbol_constraint_name
						{ $$ = MAKE_NODE (nod_delete_rel_constraint, (int) 1, $3);}
		| ADD column_def
			{ $$ = $2; }
		| ADD table_constraint_definition
						{ $$ = $2; }
/* CVC: From SQL, field positions start at 1, not zero. Think in ORDER BY, for example. 
		| col_opt simple_column_name POSITION nonneg_short_integer 
			{ $$ = MAKE_NODE (nod_mod_field_pos, 2, $2,
			MAKE_CONSTANT ((dsql_str*) $4, CONSTANT_SLONG)); } */
		| col_opt simple_column_name POSITION pos_short_integer
			{ $$ = MAKE_NODE (nod_mod_field_pos, 2, $2,
				MAKE_CONSTANT ((dsql_str*) $4, CONSTANT_SLONG)); }
		| col_opt alter_column_name TO simple_column_name
			{ $$ = MAKE_NODE (nod_mod_field_name, 2, $2, $4); }
		| col_opt alter_col_name TYPE alter_data_type_or_domain end_trigger 
			{ $$ = MAKE_NODE (nod_mod_field_type, 3, $2, $5, $4); } 
		;

alter_column_name  : keyword_or_column
		   { $$ = MAKE_NODE (nod_field_name, (int) e_fln_count,
						NULL, $1); }
		   ;

/* below are reserved words that could be used as column identifiers
   in the previous versions */

keyword_or_column	: valid_symbol_name
		| ADMIN					/* added in IB 5.0 */
		| COLUMN				/* added in IB 6.0 */
		| EXTRACT
		| YEAR
		| MONTH
		| DAY
		| HOUR
		| MINUTE
		| SECOND
		| TIME
		| TIMESTAMP
		| CURRENT_DATE
		| CURRENT_TIME
		| CURRENT_TIMESTAMP
		| CURRENT_USER			/* added in FB 1.0 */
		| CURRENT_ROLE
		| RECREATE
		| CURRENT_CONNECTION	/* added in FB 1.5 */
		| CURRENT_TRANSACTION
		| BIGINT
		| CASE
		| RELEASE
		| ROW_COUNT
		| SAVEPOINT
		| OPEN					/* added in FB 2.0 */
		| CLOSE
		| FETCH
		| ROWS
		| USING
		| CROSS
		;

col_opt		: ALTER
			{ $$ = NULL; }
		| ALTER COLUMN
			{ $$ = NULL; }
		;

alter_data_type_or_domain	: non_array_type begin_trigger
						  		{ $$ = NULL; }
				| simple_column_name begin_string
						  		{ $$ = MAKE_NODE (nod_def_domain, (int) e_dom_count,
												$1, NULL, NULL, NULL, NULL); }
		;

alter_col_name	: simple_column_name
			{ ((SQLParse*) sqlParse)->g_field_name = $1;
			  ((SQLParse*) sqlParse)->g_field = MAKE_FIELD ($1);
			  $$ = (dsql_nod*) ((SQLParse*) sqlParse)->g_field; }
		;

drop_behaviour	: RESTRICT
			{ $$ = MAKE_NODE (nod_restrict, 0, NULL); }
		| CASCADE
			{ $$ = MAKE_NODE (nod_cascade, 0, NULL); }
		|
			{ $$ = MAKE_NODE (nod_restrict, 0, NULL); }
		;

alter_index_clause	: symbol_index_name ACTIVE
				{ $$ = MAKE_NODE (nod_idx_active, 1, $1); }
			| symbol_index_name INACTIVE
				{ $$ = MAKE_NODE (nod_idx_inactive, 1, $1); }
			;

alter_sequence_clause	: symbol_generator_name RESTART WITH signed_long_integer
			{ $$ = MAKE_NODE (nod_set_generator2, e_gen_id_count, $1,
				MAKE_CONSTANT ((dsql_str*) $4, CONSTANT_SLONG)); }
		| symbol_generator_name RESTART WITH NUMBER64BIT
			{ $$ = MAKE_NODE (nod_set_generator2, e_gen_id_count, $1,
				MAKE_CONSTANT((dsql_str*) $4, CONSTANT_SINT64)); }
		| symbol_generator_name RESTART WITH '-' NUMBER64BIT
			{ $$ = MAKE_NODE (nod_set_generator2, e_gen_id_count, $1,
				MAKE_NODE(nod_negate, 1, MAKE_CONSTANT((dsql_str*) $5, CONSTANT_SINT64))); }
		;


/* ALTER DATABASE */

init_alter_db	: 
			{ ((SQLParse*) sqlParse)->log_defined = FALSE;
			  ((SQLParse*) sqlParse)->cache_defined = FALSE;
			  $$ = NULL; }
		;

alter_db	: db_alter_clause
		| alter_db db_alter_clause
				{ $$ = MAKE_NODE (nod_list, (int) 2, $1, $2); }
		;

db_alter_clause : ADD db_file_list
			{ $$ = $2; }
/*
		| ADD db_cache
			{ $$ = $2; }
				| DROP CACHE
			{ $$ = MAKE_NODE (nod_drop_cache, 0, NULL); }
		| DROP LOGFILE
			{ $$ = MAKE_NODE (nod_drop_log, 0, NULL); }
		| SET  db_log_option_list
			{ $$ = $2; }
		| ADD db_log
			{ $$ = $2; }
*/			
		| ADD KW_DIFFERENCE KW_FILE sql_string
			{ $$ = MAKE_NODE (nod_difference_file, (int) 1, $4); }
		| DROP KW_DIFFERENCE KW_FILE
			{ $$ = MAKE_NODE (nod_drop_difference, 0, NULL); }
		| BEGIN BACKUP
			{ $$ = MAKE_NODE (nod_begin_backup, 0, NULL); }
		| END BACKUP
			{ $$ = MAKE_NODE (nod_end_backup, 0, NULL); }
		;

/*
db_log_option_list : db_log_option
		   | db_log_option_list ',' db_log_option
				{ $$ = MAKE_NODE (nod_list, (int) 2, $1, $3); }
		   ;
*/


/* ALTER TRIGGER */

alter_trigger_clause : symbol_trigger_name trigger_active
		new_trigger_type
		trigger_position
		begin_trigger
		new_trigger_action
		end_trigger
			{ $$ = MAKE_NODE (nod_mod_trigger, (int) e_trg_count,
				$1, NULL, $2, $3, $4, $6, $7, NULL); }
		;

new_trigger_type : trigger_type
		|
			{ $$ = NULL; }
		;

new_trigger_action : trigger_action
		|
			{ $$ = NULL; }
		;

/* DROP metadata operations */
				
drop		: DROP drop_clause
			{ $$ = $2; }
				; 

drop_clause	: EXCEPTION symbol_exception_name
			{ $$ = MAKE_NODE (nod_del_exception, 1, $2); }
		| INDEX symbol_index_name
			{ $$ = MAKE_NODE (nod_del_index, (int) 1, $2); }
		| PROCEDURE symbol_procedure_name
			{ $$ = MAKE_NODE (nod_del_procedure, (int) 1, $2); }
		| TABLE symbol_table_name
			{ $$ = MAKE_NODE (nod_del_relation, (int) 1, $2); }
		| TRIGGER symbol_trigger_name
			{ $$ = MAKE_NODE (nod_del_trigger, (int) 1, $2); }
		| VIEW symbol_view_name
			{ $$ = MAKE_NODE (nod_del_view, (int) 1, $2); }
		| FILTER symbol_filter_name
			{ $$ = MAKE_NODE (nod_del_filter, (int) 1, $2); }
		| DOMAIN symbol_domain_name
			{ $$ = MAKE_NODE (nod_del_domain, (int) 1, $2); }
		| EXTERNAL FUNCTION symbol_UDF_name
			{ $$ = MAKE_NODE (nod_del_udf, (int) 1, $3); }
		| SHADOW pos_short_integer
			{ $$ = MAKE_NODE (nod_del_shadow, (int) 1, $2); }
		| ROLE symbol_role_name
			{ $$ = MAKE_NODE (nod_del_role, (int) 1, $2); }
		| GENERATOR symbol_generator_name
			{ $$ = MAKE_NODE (nod_del_generator, (int) 1, $2); }
		| USER symbol_user_name
			{ $$ = MAKE_NODE (nod_del_user, 2, $2, NULL); }
		| SEQUENCE symbol_generator_name
			{ $$ = MAKE_NODE (nod_del_generator, (int) 1, $2); }
		;


/* these are the allowable datatypes */

data_type	: non_array_type
		| array_type
		;

non_array_type	: simple_type
		| blob_type
		;

array_type	: non_charset_simple_type '[' array_spec ']'
			{ ((SQLParse*) sqlParse)->g_field->fld_ranges = MAKE_LIST ($3);
			  ((SQLParse*) sqlParse)->g_field->fld_dimensions = ((SQLParse*) sqlParse)->g_field->fld_ranges->nod_count / 2;
			  ((SQLParse*) sqlParse)->g_field->fld_element_dtype = ((SQLParse*) sqlParse)->g_field->fld_dtype;
			  $$ = $1; }
		| character_type '[' array_spec ']' charset_clause
			{ ((SQLParse*) sqlParse)->g_field->fld_ranges = MAKE_LIST ($3);
			  ((SQLParse*) sqlParse)->g_field->fld_dimensions = ((SQLParse*) sqlParse)->g_field->fld_ranges->nod_count / 2;
			  ((SQLParse*) sqlParse)->g_field->fld_element_dtype = ((SQLParse*) sqlParse)->g_field->fld_dtype;
			  $$ = $1; }
		;

array_spec	: array_range 
		| array_spec ',' array_range 
			{ $$ = MAKE_NODE (nod_list, (int) 2, $1, $3); }
		;

array_range	: signed_long_integer
				{ if ((long) $1 < 1)
			 		$$ = MAKE_NODE (nod_list, (int) 2, 
					MAKE_CONSTANT ((dsql_str*) $1, CONSTANT_SLONG),
					MAKE_INTEGER (1));
				  else
			 		$$ = MAKE_NODE (nod_list, (int) 2, 
			   		MAKE_INTEGER (1),
					MAKE_CONSTANT ((dsql_str*) $1, CONSTANT_SLONG) ); }
		| signed_long_integer ':' signed_long_integer
				{ $$ = MAKE_NODE (nod_list, (int) 2, 
			 	MAKE_CONSTANT ((dsql_str*) $1, CONSTANT_SLONG),
				MAKE_CONSTANT ((dsql_str*) $3, CONSTANT_SLONG)); }
		;

simple_type	: non_charset_simple_type
		| character_type charset_clause
		;

non_charset_simple_type	: national_character_type
		| numeric_type
		| float_type
		| BIGINT
			{ 
			if (((SQLParse*) sqlParse)->clientDialect < SQL_DIALECT_V6_TRANSITION)
				throw OSRIException (isc_sqlerr, isc_arg_number, (SLONG) -104, 
					isc_arg_gds, isc_sql_dialect_datatype_unsupport,
					isc_arg_number, ((SQLParse*) sqlParse)->clientDialect,
					isc_arg_string, "BIGINT",
					0);
			if (((SQLParse*) sqlParse)->dbDialect < SQL_DIALECT_V6_TRANSITION)
				throw OSRIException (isc_sqlerr, isc_arg_number, (SLONG) -104, 
					isc_arg_gds, isc_sql_db_dialect_dtype_unsupport,
					isc_arg_number, ((SQLParse*) sqlParse)->dbDialect,
					isc_arg_string, "BIGINT",
					0);
			((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_int64; 
			((SQLParse*) sqlParse)->g_field->fld_length = sizeof (SINT64); 
			}
		| integer_keyword
			{ 
			((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_long; 
			((SQLParse*) sqlParse)->g_field->fld_length = sizeof (SLONG); 
			}
		| SMALLINT
			{ 
			((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_short; 
			((SQLParse*) sqlParse)->g_field->fld_length = sizeof (SSHORT); 
			}
		| DATE
			{ 
			((SQLParse*) sqlParse)->ambiguousStatement = true;
			if (((SQLParse*) sqlParse)->clientDialect <= SQL_DIALECT_V5)
				{
				/* Post warning saying that DATE is equivalent to TIMESTAMP */
					ERRD_post_warning (isc_sqlwarn, isc_arg_number, (SLONG) 301, 
											   isc_arg_warning, isc_dtype_renamed, 0);
				((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_timestamp; 
				((SQLParse*) sqlParse)->g_field->fld_length = sizeof (GDS_TIMESTAMP);
				}
			else if (((SQLParse*) sqlParse)->clientDialect == SQL_DIALECT_V6_TRANSITION)
				yyabandon (-104, isc_transitional_date);
			else
				{
				((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_sql_date; 
				((SQLParse*) sqlParse)->g_field->fld_length = sizeof (ULONG);
				}
			}
		| TIME
			{ 
			if (((SQLParse*) sqlParse)->clientDialect < SQL_DIALECT_V6_TRANSITION)
				throw OSRIException (isc_sqlerr, isc_arg_number, (SLONG) -104, 
					isc_arg_gds, isc_sql_dialect_datatype_unsupport,
					isc_arg_number, ((SQLParse*) sqlParse)->clientDialect,
					isc_arg_string, "TIME",
					0);
			if (((SQLParse*) sqlParse)->dbDialect < SQL_DIALECT_V6_TRANSITION)
				throw OSRIException (isc_sqlerr, isc_arg_number, (SLONG) -104, 
					isc_arg_gds, isc_sql_db_dialect_dtype_unsupport,
					isc_arg_number, ((SQLParse*) sqlParse)->dbDialect,
					isc_arg_string, "TIME",
					0);
			((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_sql_time; 
			((SQLParse*) sqlParse)->g_field->fld_length = sizeof (SLONG);
			}
		| TIMESTAMP
			{ 
			((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_timestamp; 
			((SQLParse*) sqlParse)->g_field->fld_length = sizeof (GDS_TIMESTAMP);
			}
		;

integer_keyword	: INTEGER	
		| KW_INT
		;


/* allow a blob to be specified with any combination of 
   segment length and subtype */

blob_type	: BLOB blob_subtype blob_segsize charset_clause
			{ 
			((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_blob; 
			((SQLParse*) sqlParse)->g_field->fld_length = sizeof(ISC_QUAD);
			}
		| BLOB '(' unsigned_short_integer ')'
			{ 
			((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_blob; 
			((SQLParse*) sqlParse)->g_field->fld_length = sizeof(ISC_QUAD);
			((SQLParse*) sqlParse)->g_field->fld_seg_length = (USHORT)(long) $3;
			((SQLParse*) sqlParse)->g_field->fld_sub_type = 0;
			}
		| BLOB '(' unsigned_short_integer ',' signed_short_integer ')'
			{ 
			((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_blob; 
			((SQLParse*) sqlParse)->g_field->fld_length = sizeof(ISC_QUAD);
			((SQLParse*) sqlParse)->g_field->fld_seg_length = (USHORT)(long) $3;
			((SQLParse*) sqlParse)->g_field->fld_sub_type = (USHORT)(long) $5;
			}
		| BLOB '(' ',' signed_short_integer ')'
			{ 
			((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_blob; 
			((SQLParse*) sqlParse)->g_field->fld_length = sizeof(ISC_QUAD);
			((SQLParse*) sqlParse)->g_field->fld_seg_length = 80;
			((SQLParse*) sqlParse)->g_field->fld_sub_type = (USHORT)(long) $4;
			}
		;

blob_segsize	: SEGMENT KW_SIZE unsigned_short_integer
		  	{
			((SQLParse*) sqlParse)->g_field->fld_seg_length = (USHORT)(long) $3;
		  	}
		|
		  	{
			((SQLParse*) sqlParse)->g_field->fld_seg_length = (USHORT) 80;
		  	}
		;

blob_subtype	: SUB_TYPE signed_short_integer
			{
			((SQLParse*) sqlParse)->g_field->fld_sub_type = (USHORT)(long) $2;
			}
		| SUB_TYPE symbol_blob_subtype_name
			{
			((SQLParse*) sqlParse)->g_field->fld_sub_type_name = $2;
			}
		|
			{
			((SQLParse*) sqlParse)->g_field->fld_sub_type = (USHORT) 0;
			}
		;

charset_clause	: CHARACTER SET symbol_character_set_name
			{
			((SQLParse*) sqlParse)->g_field->fld_character_set = $3;
			}
		|
		;


/* character type */


national_character_type	: national_character_keyword '(' pos_short_integer ')'
			{ 
			((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_text; 
			((SQLParse*) sqlParse)->g_field->fld_character_length = (USHORT)(long) $3; 
			((SQLParse*) sqlParse)->g_field->fld_flags |= FLD_national;
			}
		| national_character_keyword
			{ 
			((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_text; 
			((SQLParse*) sqlParse)->g_field->fld_character_length = 1; 
			((SQLParse*) sqlParse)->g_field->fld_flags |= FLD_national;
			}
		| national_character_keyword VARYING '(' pos_short_integer ')'
			{ 
			((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_varying; 
			((SQLParse*) sqlParse)->g_field->fld_character_length = (USHORT)(long) $4; 
			((SQLParse*) sqlParse)->g_field->fld_flags |= FLD_national;
			}
		;

character_type	: character_keyword '(' pos_short_integer ')'
			{ 
			((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_text; 
			((SQLParse*) sqlParse)->g_field->fld_character_length = (USHORT)(long) $3; 
			}
		| character_keyword
			{ 
			((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_text; 
			((SQLParse*) sqlParse)->g_field->fld_character_length = 1; 
			}
		| varying_keyword '(' pos_short_integer ')'
			{ 
			((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_varying; 
			((SQLParse*) sqlParse)->g_field->fld_character_length = (USHORT)(long) $3; 
			}
		;

varying_keyword 	   : VARCHAR
			   | CHARACTER VARYING
			   | KW_CHAR VARYING
			   ;

character_keyword 	   : CHARACTER
			   | KW_CHAR
			   ;

national_character_keyword : NCHAR
			   | NATIONAL CHARACTER
				   | NATIONAL KW_CHAR
				   ;



/* numeric type */

numeric_type	: KW_NUMERIC prec_scale
						{ 
			  ((SQLParse*) sqlParse)->g_field->fld_sub_type = dsc_num_type_numeric;
			}
		| decimal_keyword prec_scale
			{  
			   ((SQLParse*) sqlParse)->g_field->fld_sub_type = dsc_num_type_decimal;
			   if (((SQLParse*) sqlParse)->g_field->fld_dtype == dtype_short)
				{
				((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_long;
				((SQLParse*) sqlParse)->g_field->fld_length = sizeof (SLONG);
				};
			}
		;

prec_scale	: 
			{
			((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_long; 
				((SQLParse*) sqlParse)->g_field->fld_length = sizeof (SLONG); 
			((SQLParse*) sqlParse)->g_field->fld_precision = 9;
				}
		| '(' signed_long_integer ')'
			{		 
			if ( ((long) $2 < 1) || ((long) $2 > 18) )
				yyabandon (-842, isc_precision_err);
				/* Precision most be between 1 and 18. */ 
			if ((long) $2 > 9)
				{
				if ( ( (((SQLParse*) sqlParse)->clientDialect <= SQL_DIALECT_V5) &&
				   (((SQLParse*) sqlParse)->dbDialect	 >  SQL_DIALECT_V5) ) ||
				 ( (((SQLParse*) sqlParse)->clientDialect >  SQL_DIALECT_V5) &&
				   (((SQLParse*) sqlParse)->dbDialect	 <= SQL_DIALECT_V5) ) )
					throw OSRIException (isc_sqlerr,
					   isc_arg_number, (SLONG) -817,
					   isc_arg_gds,
					   isc_ddl_not_allowed_by_db_sql_dial,
					   isc_arg_number, (SLONG) ((SQLParse*) sqlParse)->dbDialect,
					   0);
				if (((SQLParse*) sqlParse)->clientDialect <= SQL_DIALECT_V5)
					{
				((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_double;
				((SQLParse*) sqlParse)->g_field->fld_length = sizeof (double);
					}
				else
					{
				if (((SQLParse*) sqlParse)->clientDialect == SQL_DIALECT_V6_TRANSITION)
					{
					ERRD_post_warning (
					isc_dsql_warn_precision_ambiguous,
					isc_arg_end );
					ERRD_post_warning (
					isc_dsql_warn_precision_ambiguous1,
					isc_arg_end );
					ERRD_post_warning (
					isc_dsql_warn_precision_ambiguous2,
					isc_arg_end );

					}
				((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_int64;
				((SQLParse*) sqlParse)->g_field->fld_length = sizeof (SINT64);
					}
				}
			else 
				if ((long) $2 < 5)
					{
					((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_short; 
					((SQLParse*) sqlParse)->g_field->fld_length = sizeof (SSHORT); 
					}
				else
					{
					((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_long; 
					((SQLParse*) sqlParse)->g_field->fld_length = sizeof (SLONG); 
					}
			((SQLParse*) sqlParse)->g_field->fld_precision = (USHORT)(long) $2;
			}
		| '(' signed_long_integer ',' signed_long_integer ')'
			{ 
			if ( ((long) $2 < 1) || ((long) $2 > 18) )
				yyabandon (-842, isc_precision_err);
				/* Precision should be between 1 and 18 */ 
			if (((long) $4 > (long) $2) || ((long) $4 < 0))
				yyabandon (-842, isc_scale_nogt);
				/* Scale must be between 0 and precision */
			if ((long) $2 > 9)
				{
				if ( ( (((SQLParse*) sqlParse)->clientDialect <= SQL_DIALECT_V5) &&
				   (((SQLParse*) sqlParse)->dbDialect	 >  SQL_DIALECT_V5) ) ||
				 ( (((SQLParse*) sqlParse)->clientDialect >  SQL_DIALECT_V5) &&
				   (((SQLParse*) sqlParse)->dbDialect	 <= SQL_DIALECT_V5) ) )
					throw OSRIException (isc_sqlerr,
					   isc_arg_number, (SLONG) -817,
					   isc_arg_gds,
					   isc_ddl_not_allowed_by_db_sql_dial,
					   isc_arg_number, (SLONG) ((SQLParse*) sqlParse)->dbDialect,
					   0);
				if (((SQLParse*) sqlParse)->clientDialect <= SQL_DIALECT_V5)
					{
				((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_double;
				((SQLParse*) sqlParse)->g_field->fld_length = sizeof (double); 
					}
				else
					{
				if (((SQLParse*) sqlParse)->clientDialect == SQL_DIALECT_V6_TRANSITION)
				  {
					ERRD_post_warning (
					isc_dsql_warn_precision_ambiguous,
					isc_arg_end );
					ERRD_post_warning (
					isc_dsql_warn_precision_ambiguous1,
					isc_arg_end );
					ERRD_post_warning (
					isc_dsql_warn_precision_ambiguous2,
					isc_arg_end );
				  }
				  /* ((SQLParse*) sqlParse)->clientDialect >= SQL_DIALECT_V6 */
				((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_int64;
				((SQLParse*) sqlParse)->g_field->fld_length = sizeof (SINT64);
					}
				}
			else
				{
				if ((long) $2 < 5)
					{
					((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_short; 
					((SQLParse*) sqlParse)->g_field->fld_length = sizeof (SSHORT); 
					}
				else
					{
					((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_long; 
					((SQLParse*) sqlParse)->g_field->fld_length = sizeof (SLONG); 
					}
				}
			((SQLParse*) sqlParse)->g_field->fld_precision = (USHORT)(long) $2;
			((SQLParse*) sqlParse)->g_field->fld_scale = - (SSHORT)(long) $4;
			}
		;

decimal_keyword	: DECIMAL	
		| KW_DEC
		;



/* floating point type */

float_type	: KW_FLOAT precision_opt
			{ 
			if ((long) $2 > 7)
				{
				((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_double;
				((SQLParse*) sqlParse)->g_field->fld_length = sizeof (double); 
				}
			else
				{
				((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_real; 
				((SQLParse*) sqlParse)->g_field->fld_length = sizeof (float);
				}
			}
		| KW_LONG KW_FLOAT precision_opt
			{ 
			((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_double; 
			((SQLParse*) sqlParse)->g_field->fld_length = sizeof (double); 
			}
		| REAL
			{ 
			((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_real; 
			((SQLParse*) sqlParse)->g_field->fld_length = sizeof (float); 
			}
		| KW_DOUBLE PRECISION
			{ 
			((SQLParse*) sqlParse)->g_field->fld_dtype = dtype_double; 
			((SQLParse*) sqlParse)->g_field->fld_length = sizeof (double); 
			}
		;

precision_opt	: '(' nonneg_short_integer ')'
			{ $$ = $2; }
		|
			{ $$ = 0; }
		;



/* SET statements */
set		: set_transaction
		| set_generator
		| set_statistics
		;


set_generator	: SET GENERATOR symbol_generator_name TO signed_long_integer
			{ 
			  $$ = MAKE_NODE (nod_set_generator2,e_gen_id_count,$3,
						MAKE_CONSTANT ((dsql_str*) $5, CONSTANT_SLONG));
			}
				| SET GENERATOR symbol_generator_name TO NUMBER64BIT
						{
			  $$ = MAKE_NODE (nod_set_generator2,e_gen_id_count,$3,
					   MAKE_CONSTANT((dsql_str*)$5, CONSTANT_SINT64));
			}
				| SET GENERATOR symbol_generator_name TO '-' NUMBER64BIT
						{
			  $$ = MAKE_NODE (nod_set_generator2, (int) e_gen_id_count, $3,
					  MAKE_NODE(nod_negate, 1,
							MAKE_CONSTANT((dsql_str*)$6, CONSTANT_SINT64)));
			}
		;


/* transaction statements */

savepoint	: set_savepoint
		| release_savepoint
		| undo_savepoint
		;

set_savepoint : SAVEPOINT symbol_savepoint_name
			{ $$ = MAKE_NODE (nod_user_savepoint, 1, $2); }
		;

release_savepoint	: RELEASE SAVEPOINT symbol_savepoint_name release_only_opt
			{ $$ = MAKE_NODE (nod_release_savepoint, 2, $3, $4); }
		;

release_only_opt	: ONLY
			{ $$ = MAKE_NODE (nod_flag, 0, NULL); }
		|
			{ $$ = 0; }
		;

undo_savepoint : ROLLBACK optional_work TO optional_savepoint symbol_savepoint_name
			{ $$ = MAKE_NODE (nod_undo_savepoint, 1, $5); }
		;

optional_savepoint	: SAVEPOINT
		|
		;

commit		: COMMIT optional_work optional_retain
			{ $$ = MAKE_NODE (nod_commit, 1, $3); }
		;

rollback	: ROLLBACK optional_work
			{ $$ = MAKE_NODE (nod_rollback, 0, NULL); }
		;

optional_work	: WORK
		|
		;

optional_retain	: RETAIN opt_snapshot
			{ $$ = MAKE_NODE (nod_commit_retain, 0, NULL); }
		|
		 	{ $$ = NULL; }
		;

opt_snapshot	: SNAPSHOT
		|
		 	{ $$ = NULL; }
		;

set_transaction	: SET TRANSACTION tran_opt_list_m
			{$$ = MAKE_NODE (nod_trans, 1, MAKE_LIST ($3)); }
		;

tran_opt_list_m	: tran_opt_list	
		|
		 	{ $$ = NULL; }
		;

tran_opt_list	: tran_opt
		| tran_opt_list tran_opt
			{ $$ = MAKE_NODE (nod_list, (int) 2, $1, $2); }
		;

tran_opt	: access_mode 
		| lock_wait 
		| isolation_mode 
		| tbl_reserve_options 
		;

access_mode	: READ ONLY
			{ $$ = MAKE_FLAG_NODE (nod_access, NOD_READ_ONLY, 0, NULL); }
		| READ WRITE
			{ $$ = MAKE_FLAG_NODE (nod_access, NOD_READ_WRITE, 0, NULL); }
		;

lock_wait	: WAIT
			{ $$ = MAKE_FLAG_NODE (nod_wait, NOD_WAIT, 0, NULL); }
		| NO WAIT
			{ $$ = MAKE_FLAG_NODE (nod_wait, NOD_NO_WAIT, 0, NULL); }
		;

isolation_mode	: ISOLATION LEVEL iso_mode
			{ $$ = $3;}
		| iso_mode
		;

iso_mode	: snap_shot
			{ $$ = $1;}
		| READ UNCOMMITTED version_mode
			{ $$ = MAKE_FLAG_NODE (nod_isolation, NOD_READ_COMMITTED, 1, $3); }
		| READ COMMITTED version_mode
			{ $$ = MAKE_FLAG_NODE (nod_isolation, NOD_READ_COMMITTED, 1, $3); }
		;

snap_shot	: SNAPSHOT
			{ $$ = MAKE_FLAG_NODE (nod_isolation, NOD_CONCURRENCY, 0, NULL); }
		| SNAPSHOT TABLE 
			{ $$ = MAKE_FLAG_NODE (nod_isolation, NOD_CONSISTENCY, 0, NULL); }
		| SNAPSHOT TABLE STABILITY
			{ $$ = MAKE_FLAG_NODE (nod_isolation, NOD_CONSISTENCY, 0, NULL); }
		;

version_mode	: VERSION
			{ $$ = MAKE_FLAG_NODE (nod_version, NOD_VERSION, 0, NULL); }
		| NO VERSION
			{ $$ = MAKE_FLAG_NODE (nod_version, NOD_NO_VERSION, 0, NULL); }
		|
			{ $$ = 0; }
		;

tbl_reserve_options: RESERVING restr_list
			{ $$ = MAKE_NODE (nod_reserve, 1, MAKE_LIST ($2)); }
		;

lock_type	: KW_SHARED
			{ $$ = (dsql_nod*) NOD_SHARED; }
		| PROTECTED
			{ $$ = (dsql_nod*) NOD_PROTECTED ; }
		|
			{ $$ = (dsql_nod*) 0; }
		;

lock_mode	: READ
			{ $$ = (dsql_nod*) NOD_READ; }
		| WRITE
			{ $$ = (dsql_nod*) NOD_WRITE; }
		;

restr_list	: restr_option
		| restr_list ',' restr_option
			{ $$ = MAKE_NODE (nod_list, (int) 2, $1, $3); }
		;

restr_option	: table_list table_lock
			{ $$ = MAKE_NODE (nod_table_lock, (int) 2, MAKE_LIST ($1), $2); }
		;

table_lock	: FOR lock_type lock_mode
			{ $$ = MAKE_FLAG_NODE (nod_lock_mode, (SSHORT) ((SSHORT)(long) $2 | (SSHORT)(long) $3), (SSHORT) 0, NULL); }
		|
			{ $$ = 0; }
		;

table_list	: simple_table_name
		| table_list ',' simple_table_name
			{ $$ = MAKE_NODE (nod_list, (int) 2, $1, $3); }
		;


set_statistics	: SET STATISTICS INDEX symbol_index_name
			{$$ = MAKE_NODE (nod_set_statistics, (int)e_stat_count, $4); }
		;


/* SELECT statement */

select		: select_expr for_update_clause lock_clause
			{ $$ = MAKE_NODE (nod_select, (int) e_select_count, $1, $2, $3); }
		;

for_update_clause : FOR UPDATE for_update_list
			{ $$ = MAKE_NODE (nod_for_update, (int) e_fpd_count, $3); }
		|
			{ $$ = NULL; }
		;

for_update_list	: OF column_list
			{ $$ = $2; }
		|
			{ $$ = MAKE_NODE (nod_flag, 0, NULL); }
		;

lock_clause : WITH LOCK
			{ $$ = MAKE_NODE (nod_flag, 0, NULL); }
		|
			{ $$ = NULL; }
		;
		

/* SELECT expression */

select_expr	: select_expr_body order_clause rows_clause
			{ $$ = MAKE_NODE (nod_select_expr, (int) e_sel_count, $1, $2, $3); }
		;

column_select	: select_expr_body order_clause rows_clause
			{ $$ = MAKE_FLAG_NODE (nod_select_expr, NOD_SELECT_EXPR_VALUE,
					(int) e_sel_count, $1, $2, $3); }
		;

column_singleton	: select_expr_body order_clause rows_clause
			{ $$ = MAKE_FLAG_NODE (nod_select_expr, NOD_SELECT_EXPR_VALUE | NOD_SELECT_EXPR_SINGLETON,
					(int) e_sel_count, $1, $2, $3); }
		;

select_expr_body	: query_term
		| select_expr_body UNION distinct_noise query_term
			{ $$ = MAKE_NODE (nod_list, 2, $1, $4); }
		| select_expr_body UNION ALL query_term
			{ $$ = MAKE_FLAG_NODE (nod_list, NOD_UNION_ALL, 2, $1, $4); }
		;

query_term	: query_spec
		;

query_spec	: SELECT limit_clause
			 distinct_clause
			 select_list 
			 from_clause 
			 where_clause 
			 group_clause 
			 having_clause
			 plan_clause
			{ $$ = MAKE_NODE (nod_query_spec, (int) e_qry_count, 
					$2, $3, $4, $5, $6, $7, $8, $9); }
		;											   

begin_limit	: 
			{ ((SQLParse*) sqlParse)->limit_clause = true; }
		;

end_limit	:
			{ ((SQLParse*) sqlParse)->limit_clause = FALSE; }
		;
		
begin_first	: 
			{ ((SQLParse*) sqlParse)->first_detection = true; }
		;

end_first	:
			{ ((SQLParse*) sqlParse)->first_detection = FALSE; }
		;
		
limit_clause	: first_clause skip_clause end_limit
			{ $$ = MAKE_NODE (nod_limit, (int) e_limit_count, $2, $1); }
		|   first_clause end_limit
			{ $$ = MAKE_NODE (nod_limit, (int) e_limit_count, NULL, $1); }
		|   skip_clause 
			{ $$ = MAKE_NODE (nod_limit, (int) e_limit_count, $1, NULL); }
		| 
			{ $$ = 0; }
		;

first_clause	: FIRST long_integer begin_limit
			{ $$ = MAKE_CONSTANT ((dsql_str*) $2, CONSTANT_SLONG); }
		| FIRST '(' value ')' begin_limit
			{ $$ = $3; }
		| FIRST parameter begin_limit
			{ $$ = $2; }
		;

skip_clause	: SKIP long_integer
			{ $$ = MAKE_CONSTANT ((dsql_str*) $2, CONSTANT_SLONG); }
		| SKIP '(' end_limit value ')'
			{ $$ = $4; }
		| SKIP parameter
			{ $$ = $2; }
		;

distinct_clause	: DISTINCT
			{ $$ = MAKE_NODE (nod_flag, 0, NULL); }
		| all_noise 
			{ $$ = 0; }
		;

select_list	: select_items
			{ $$ = MAKE_LIST ($1); }
		| '*'
			{ $$ = 0; }
		;

select_items	: select_item
		| select_items ',' select_item
			{ $$ = MAKE_NODE (nod_list, 2, $1, $3); }
		;

select_item	: value
		| value as_noise symbol_item_alias_name
			{ $$ = MAKE_NODE (nod_alias, 2, $1, $3); }
		;

as_noise : AS
		|
		;

/* FROM clause */

from_clause	: FROM from_list
		 	{ $$ = MAKE_LIST ($2); }
		;

from_list	: table_reference
		| from_list ',' table_reference
			{ $$ = MAKE_NODE (nod_list, 2, $1, $3); }
		;

table_reference	: joined_table
		| table_primary
		;

table_primary	: table_proc
		| derived_table
		| '(' joined_table ')'
			{ $$ = $2; }
		;

/* AB: derived table support */
derived_table :
		'(' select_expr ')' as_noise correlation_name derived_column_list
			{ $$ = MAKE_NODE(nod_derived_table, (int) e_derived_table_count, $2, $5, $6); }
		;

correlation_name : symbol_table_alias_name
		|
			{ $$ = NULL; }
		;

derived_column_list : '(' alias_list ')'
			{ $$ = MAKE_LIST ($2); }
		|
			{ $$ = NULL; }
		;

alias_list : symbol_item_alias_name
		| alias_list ',' symbol_item_alias_name
			{ $$ = MAKE_NODE (nod_list, 2, $1, $3); }
		;

joined_table	: cross_join
		| natural_join
		| qualified_join
		;

cross_join	: table_reference CROSS JOIN table_primary
			{ $$ = MAKE_NODE (nod_join, (int) e_join_count, $1,
				MAKE_NODE (nod_join_inner, (int) 0, NULL), $4, NULL); }
		;

natural_join	: table_reference NATURAL join_type JOIN table_primary
			{ $$ = MAKE_NODE (nod_join, (int) e_join_count, $1, $3, $5,
					MAKE_NODE (nod_flag, 0, NULL)); }
		;

qualified_join	: table_reference join_type JOIN table_reference join_specification
			{ $$ = MAKE_NODE (nod_join, (int) e_join_count, $1, $2, $4, $5); }
		;

join_specification	: join_condition
		| named_columns_join
		;

join_condition	: ON search_condition
			{ $$ = $2; }
		;

named_columns_join	: USING '(' column_list ')'
			{ $$ = MAKE_LIST ($3); }
		;

table_proc	: symbol_procedure_name table_proc_inputs as_noise symbol_table_alias_name
			{ $$ = MAKE_NODE (nod_rel_proc_name, 
					(int) e_rpn_count, $1, $4, $2); }
		| symbol_procedure_name table_proc_inputs
			{ $$ = MAKE_NODE (nod_rel_proc_name, 
					(int) e_rpn_count, $1, NULL, $2); }
		;

table_proc_inputs	: '(' value_list ')'
				{ $$ = MAKE_LIST ($2); }
			|
				{ $$ = NULL; }
			;

table_name	: simple_table_name
		| symbol_table_name symbol_table_alias_name
			{ $$ = MAKE_NODE (nod_relation_name, 
						(int) e_rln_count, $1, $2); }
		;						 

simple_table_name: symbol_table_name
			{ $$ = MAKE_NODE (nod_relation_name, 
						(int) e_rln_count, $1, NULL); }
		;

join_type	: INNER
			{ $$ = MAKE_NODE (nod_join_inner, (int) 0, NULL); }
		| LEFT outer_noise
			{ $$ = MAKE_NODE (nod_join_left, (int) 0, NULL); }
		| RIGHT outer_noise
			{ $$ = MAKE_NODE (nod_join_right, (int) 0, NULL); }
		| FULL outer_noise
			{ $$ = MAKE_NODE (nod_join_full, (int) 0, NULL); }
		|
			{ $$ = MAKE_NODE (nod_join_inner, (int) 0, NULL); }
		;

outer_noise	: OUTER
		|
		;


/* other clauses in the select expression */

group_clause	: GROUP BY group_by_list
			{ $$ = MAKE_LIST ($3); }
		|
			{ $$ = NULL; }
		;

group_by_list	: group_by_item
		| group_by_list ',' group_by_item
			{ $$ = MAKE_NODE (nod_list, 2, $1, $3); }
		;

/* Except aggregate-functions are all expressions supported in group_by_item, 
   they are caught inside pass1.cpp */
group_by_item : value
		;

having_clause	: HAVING search_condition
			{ $$ = $2; }
		|
			{ $$ = NULL; }
		;

where_clause	: WHERE search_condition
		 	{ $$ = $2; }
		| 
			{ $$ = NULL; }
		;


/* PLAN clause to specify an access plan for a query */

plan_clause	: PLAN plan_expression
			{ $$ = $2; }
		|
			{ $$ = NULL; }
		;

plan_expression	: plan_type '(' plan_item_list ')'
			{ $$ = MAKE_NODE (nod_plan_expr, 2, $1, MAKE_LIST ($3)); }
		;

plan_type	: JOIN
			{ $$ = 0; }
		| SORT MERGE
			{ $$ = MAKE_NODE (nod_merge, (int) 0, NULL); }
		| MERGE
			{ $$ = MAKE_NODE (nod_merge, (int) 0, NULL); }

		/* for now the SORT operator is a no-op; it does not 
		   change the place where a sort happens, but is just intended 
		   to read the output from a SET PLAN */
		| SORT
			{ $$ = 0; }
		|
			{ $$ = 0; }
		;

plan_item_list	: plan_item
		| plan_item ',' plan_item_list
			{ $$ = MAKE_NODE (nod_list, 2, $1, $3); }
		;

plan_item	: table_or_alias_list access_type
			{ $$ = MAKE_NODE (nod_plan_item, 2, MAKE_LIST ($1), $2); }
		| plan_expression
		;

table_or_alias_list : symbol_table_name
		| symbol_table_name table_or_alias_list
			{ $$ = MAKE_NODE (nod_list, 2, $1, $2); }
		;

access_type	: NATURAL
			{ $$ = MAKE_NODE (nod_natural, (int) 0, NULL); }
		| INDEX '(' index_list ')'
			{ $$ = MAKE_NODE (nod_index, 1, MAKE_LIST ($3)); }
		| ORDER symbol_index_name extra_indices_opt
			{ $$ = MAKE_NODE (nod_index_order, 2, $2, $3); }
		;

index_list	: symbol_index_name
		| symbol_index_name ',' index_list
			{ $$ = MAKE_NODE (nod_list, 2, $1, $3); }
		;

extra_indices_opt	: INDEX '(' index_list ')'
			{ $$ = MAKE_LIST ($3); }
		|
			{ $$ = 0; }
		;

/* ORDER BY clause */

order_clause	: ORDER BY order_list
			{ $$ = MAKE_LIST ($3); }
		|
			{ $$ = 0; }
		;

order_list	: order_item
		| order_list ',' order_item
			{ $$ = MAKE_NODE (nod_list, 2, $1, $3); }
		;

order_item	: value order_direction nulls_clause
			{ $$ = MAKE_NODE (nod_order, (int) e_order_count, $1, $2, $3); }
		;

order_direction	: ASC
			{ $$ = 0; }
		| DESC
			{ $$ = MAKE_NODE (nod_flag, 0, NULL); }
		|
			{ $$ = 0; }
		;

nulls_placement : FIRST
			{ $$ = MAKE_CONSTANT((dsql_str*) NOD_NULLS_FIRST, CONSTANT_SLONG); }
		| LAST
			{ $$ = MAKE_CONSTANT((dsql_str*) NOD_NULLS_LAST, CONSTANT_SLONG); }
		;

nulls_clause : NULLS begin_first nulls_placement end_first
			{ $$ = $3; }
		|
			{ $$ = 0; }
		;

/* ROWS clause */

rows_clause	: ROWS value
			/* equivalent to FIRST value */
			{ $$ = MAKE_NODE (nod_rows, (int) e_rows_count, NULL, $2); }
		| ROWS value TO value
			/* equivalent to FIRST (upper_value - lower_value + 1) SKIP (lower_value - 1) */
			{ $$ = MAKE_NODE (nod_rows, (int) e_rows_count,
				MAKE_NODE (nod_subtract, 2, $2,
					MAKE_CONSTANT ((dsql_str*) 1, CONSTANT_SLONG)),
				MAKE_NODE (nod_add, 2,
					MAKE_NODE (nod_subtract, 2, $4, $2),
					MAKE_CONSTANT ((dsql_str*) 1, CONSTANT_SLONG))); }
		|
			{ $$ = NULL; }
		;


/* INSERT statement */
/* IBO hack: replace column_parens_opt by ins_column_parens_opt. */
insert		: INSERT INTO simple_table_name ins_column_parens_opt VALUES '(' value_list ')'
			{ $$ = MAKE_NODE (nod_insert, (int) e_ins_count, 
			  $3, MAKE_LIST ($4), MAKE_LIST ($7), NULL); }
		| INSERT INTO simple_table_name ins_column_parens_opt select_expr
			{ $$ = MAKE_NODE (nod_insert, (int) e_ins_count, $3, $4, NULL, $5); }
		;


/* DELETE statement */

delete		: delete_searched
		| delete_positioned
		;

delete_searched	: KW_DELETE FROM table_name where_clause plan_clause order_clause rows_clause
			{ $$ = MAKE_NODE (nod_delete, (int) e_del_count, $3, $4, $5, $6, $7, NULL); }
		;

delete_positioned : KW_DELETE FROM table_name cursor_clause
			{ $$ = MAKE_NODE (nod_delete, (int) e_del_count, $3, NULL, NULL, NULL, NULL, $4); }
		;


/* UPDATE statement */

update		: update_searched
		| update_positioned
		;

update_searched	: UPDATE table_name SET assignments where_clause plan_clause order_clause rows_clause
			{ $$ = MAKE_NODE (nod_update, (int) e_upd_count,
				$2, MAKE_LIST ($4), $5, $6, $7, $8, NULL); }
		  	;

update_positioned : UPDATE table_name SET assignments cursor_clause
			{ $$ = MAKE_NODE (nod_update, (int) e_upd_count,
				$2, MAKE_LIST ($4), NULL, NULL, NULL, NULL, $5); }
		;

cursor_clause	: WHERE CURRENT OF symbol_cursor_name
			{ $$ = MAKE_NODE (nod_cursor, (int) e_cur_count, $4, NULL, NULL, NULL); }
		;


/* Assignments */

assignments	: assignment
		| assignments ',' assignment
			{ $$ = MAKE_NODE (nod_list, 2, $1, $3); }
		;

assignment	: update_column_name '=' value
			{ $$ = MAKE_NODE (nod_assign, 2, $3, $1); }
		;

exec_udf	: udf
			{ $$ = MAKE_NODE (nod_assign, 2, $1, MAKE_NODE (nod_null, 0, NULL)); }
		;


/* BLOB get and put */

blob_io			: READ BLOB simple_column_name FROM simple_table_name filter_clause_io segment_clause_io
			{ $$ = MAKE_NODE (nod_get_segment, (int) e_blb_count, $3, $5, $6, $7); }
				| INSERT BLOB simple_column_name INTO simple_table_name filter_clause_io segment_clause_io
			{ $$ = MAKE_NODE (nod_put_segment, (int) e_blb_count, $3, $5, $6, $7); }
		;

filter_clause_io	: FILTER FROM blob_subtype_value_io TO blob_subtype_value_io
			{ $$ = MAKE_NODE (nod_list, 2, $3, $5); }
		| FILTER TO blob_subtype_value_io
			{ $$ = MAKE_NODE (nod_list, 2, NULL, $3); }
		|
		;

blob_subtype_value_io : blob_subtype_io
		| parameter
		;

blob_subtype_io	: signed_short_integer
			{ $$ = MAKE_CONSTANT ((dsql_str*) $1, CONSTANT_SLONG); }
		;

segment_clause_io	: MAX_SEGMENT segment_length_io
			{ $$ = $2; }
		|
			{ $$ = NULL; }
		;

segment_length_io	: unsigned_short_integer
			{ $$ = MAKE_CONSTANT ((dsql_str*) $1, CONSTANT_SLONG); }
		| parameter
		;


/* column specifications */

column_parens_opt : column_parens
		|
			{ $$ = NULL; }
		;

column_parens	: '(' column_list ')'
			{ $$ = MAKE_LIST ($2); }
		;

column_list	: simple_column_name
		| column_list ',' simple_column_name
			{ $$ = MAKE_NODE (nod_list, 2, $1, $3); }
		;

/* begin IBO hack */
ins_column_parens_opt : ins_column_parens
		|
			{ $$ = NULL; }
		;

ins_column_parens	: '(' ins_column_list ')'
			{ $$ = MAKE_LIST ($2); }
		;

ins_column_list	: update_column_name
		| ins_column_list ',' update_column_name
			{ $$ = MAKE_NODE (nod_list, 2, $1, $3); }
		;
/* end IBO hack */

column_name	 : simple_column_name
		| symbol_table_alias_name '.' symbol_column_name
			{ $$ = MAKE_NODE (nod_field_name, (int) e_fln_count, 
							$1, $3); }
		| symbol_table_alias_name '.' '*'
			{ $$ = MAKE_NODE (nod_field_name, (int) e_fln_count, 
							$1, NULL); }
		;

simple_column_name : symbol_column_name
			{ $$ = MAKE_NODE (nod_field_name, (int) e_fln_count,
						NULL, $1); }
		;

update_column_name : simple_column_name
/* CVC: This option should be deprecated! The only allowed syntax should be
Update...set column = expr, without qualifier for the column. */
		| symbol_table_alias_name '.' symbol_column_name
			{ $$ = MAKE_NODE (nod_field_name, (int) e_fln_count, 
							$1, $3); }
		;

/* boolean expressions */

search_condition : trigger_action_predicate
		| NOT trigger_action_predicate
			{ $$ = MAKE_NODE (nod_not, 1, $2); }
		| simple_search_condition
		| search_condition OR search_condition
			{ $$ = MAKE_NODE (nod_or, 2, $1, $3); }
		| search_condition AND search_condition
			{ $$ = MAKE_NODE (nod_and, 2, $1, $3); }
		;

bracable_search_condition : simple_search_condition
		| NOT trigger_action_predicate
			{ $$ = MAKE_NODE (nod_not, 1, $2); }
		| bracable_search_condition OR search_condition
			{ $$ = MAKE_NODE (nod_or, 2, $1, $3); }
		| bracable_search_condition AND search_condition
			{ $$ = MAKE_NODE (nod_and, 2, $1, $3); }
		/* Special cases. Need help from lexer to parse the grammar */
		/*| special_trigger_action_predicate -- handled by lexer */
		| special_trigger_action_predicate OR search_condition
			{ $$ = MAKE_NODE (nod_or, 2, $1, $3); }
		| special_trigger_action_predicate AND search_condition
			{ $$ = MAKE_NODE (nod_and, 2, $1, $3); }			
		;

simple_search_condition : predicate
		| '(' bracable_search_condition ')'
			{ $$ = $2; }
		| NOT simple_search_condition
			{ $$ = MAKE_NODE (nod_not, 1, $2); }
		;
		
predicate : comparison_predicate
		| distinct_predicate
		| between_predicate
		| like_predicate
		| in_predicate
		| null_predicate
		| quantified_predicate
		| exists_predicate
		| containing_predicate
		| starting_predicate
		| singular_predicate;


/* comparisons */

comparison_predicate : value '=' value
			{ $$ = MAKE_NODE (nod_eql, 2, $1, $3); }
		| value '<' value
			{ $$ = MAKE_NODE (nod_lss, 2, $1, $3); }
		| value '>' value
			{ $$ = MAKE_NODE (nod_gtr, 2, $1, $3); }
		| value GEQ value
			{ $$ = MAKE_NODE (nod_geq, 2, $1, $3); }
		| value LEQ value
			{ $$ = MAKE_NODE (nod_leq, 2, $1, $3); }
		| value NOT_GTR value
			{ $$ = MAKE_NODE (nod_leq, 2, $1, $3); }
		| value NOT_LSS value
			{ $$ = MAKE_NODE (nod_geq, 2, $1, $3); }
		| value NEQ value
			{ $$ = MAKE_NODE (nod_neq, 2, $1, $3); }
		;


/* quantified comparisons */

quantified_predicate : value '=' ALL '(' column_select ')'
		{ $$ = MAKE_NODE (nod_eql_all, 2, $1, $5); }
	| value '<' ALL '(' column_select ')'
		{ $$ = MAKE_NODE (nod_lss_all, 2, $1, $5); }
	| value '>' ALL '(' column_select ')'
		{ $$ = MAKE_NODE (nod_gtr_all, 2, $1, $5); }
	| value GEQ ALL '(' column_select ')'
		{ $$ = MAKE_NODE (nod_geq_all, 2, $1, $5); }
	| value LEQ ALL '(' column_select ')'
		{ $$ = MAKE_NODE (nod_leq_all, 2, $1, $5); }
	| value NOT_GTR ALL '(' column_select ')'
		{ $$ = MAKE_NODE (nod_leq_all, 2, $1, $5); }
	| value NOT_LSS ALL '(' column_select ')'
		{ $$ = MAKE_NODE (nod_geq_all, 2, $1, $5); }
	| value NEQ ALL '(' column_select ')'
		{ $$ = MAKE_NODE (nod_neq_all, 2, $1, $5); }
	| value '=' some '(' column_select ')'
		{ $$ = MAKE_NODE (nod_eql_any, 2, $1, $5); }
	| value '<' some '(' column_select ')'
		{ $$ = MAKE_NODE (nod_lss_any, 2, $1, $5); }
	| value '>' some '(' column_select ')'
		{ $$ = MAKE_NODE (nod_gtr_any, 2, $1, $5); }
	| value GEQ some '(' column_select ')'
		{ $$ = MAKE_NODE (nod_geq_any, 2, $1, $5); }
	| value LEQ some '(' column_select ')'
		{ $$ = MAKE_NODE (nod_leq_any, 2, $1, $5); }
	| value NOT_GTR some '(' column_select ')'
		{ $$ = MAKE_NODE (nod_leq_any, 2, $1, $5); }
	| value NOT_LSS some '(' column_select ')'
		{ $$ = MAKE_NODE (nod_geq_any, 2, $1, $5); }
	| value NEQ some '(' column_select ')'
		{ $$ = MAKE_NODE (nod_neq_any, 2, $1, $5); }
	;

some		: SOME
		| ANY
		;


/* other predicates */

distinct_predicate : value IS DISTINCT FROM value
		{ $$ = MAKE_NODE (nod_not, 1, MAKE_NODE (nod_equiv, 2, $1, $5)); }
	| value IS NOT DISTINCT FROM value
		{ $$ = MAKE_NODE (nod_equiv, 2, $1, $6); }
	;

between_predicate : value BETWEEN value AND value
		{ $$ = MAKE_NODE (nod_between, 3, $1, $3, $5); }
	| value NOT BETWEEN value AND value
		{ $$ = MAKE_NODE (nod_not, 1, MAKE_NODE (nod_between, 
						3, $1, $4, $6)); }
		;

like_predicate	: value LIKE value
		{ $$ = MAKE_NODE (nod_like, 2, $1, $3); }
	| value NOT LIKE value
		{ $$ = MAKE_NODE (nod_not, 1, MAKE_NODE (nod_like, 2, $1, $4)); }
	| value LIKE value ESCAPE value
		{ $$ = MAKE_NODE (nod_like, 3, $1, $3, $5); }
	| value NOT LIKE value ESCAPE value
		{ $$ = MAKE_NODE (nod_not, 1, MAKE_NODE (nod_like, 
						3, $1, $4, $6)); }
		;

in_predicate	: value KW_IN in_predicate_value
		{ $$ = MAKE_NODE (nod_eql_any, 2, $1, $3); }
	| value NOT KW_IN in_predicate_value
		{ $$ = MAKE_NODE (nod_not, 1, MAKE_NODE (nod_eql_any, 2, $1, $4)); }
		;

containing_predicate	: value CONTAINING value
		{ $$ = MAKE_NODE (nod_containing, 2, $1, $3); }
	| value NOT CONTAINING value
		{ $$ = MAKE_NODE (nod_not, 1, MAKE_NODE (nod_containing, 2, $1, $4)); }
		;

starting_predicate	: value STARTING value
		{ $$ = MAKE_NODE (nod_starting, 2, $1, $3); }
	| value NOT STARTING value
		{ $$ = MAKE_NODE (nod_not, 1, MAKE_NODE (nod_starting, 2, $1, $4)); }
	| value STARTING WITH value
		{ $$ = MAKE_NODE (nod_starting, 2, $1, $4); }
	| value NOT STARTING WITH value
		{ $$ = MAKE_NODE (nod_not, 1, MAKE_NODE (nod_starting, 2, $1, $5)); }
		;

exists_predicate : EXISTS '(' select_expr ')'
		{ $$ = MAKE_NODE (nod_exists, 1, $3); }
		;

singular_predicate : SINGULAR '(' select_expr ')'
		{ $$ = MAKE_NODE (nod_singular, 1, $3); }
		;

null_predicate	: value IS KW_NULL
		{ $$ = MAKE_NODE (nod_missing, 1, $1); }
	| value IS NOT KW_NULL
		{ $$ = MAKE_NODE (nod_not, 1, MAKE_NODE (nod_missing, 1, $1)); }
	;

trigger_action_predicate	: INSERTING
		{ $$ = MAKE_NODE (nod_eql, 2,
					MAKE_NODE (nod_internal_info, (int) e_internal_info_count,
						MAKE_INTEGER (internal_trigger_action)),
						MAKE_INTEGER (1)); }
	| UPDATING
		{ $$ = MAKE_NODE (nod_eql, 2,
					MAKE_NODE (nod_internal_info, (int) e_internal_info_count,
						MAKE_INTEGER (internal_trigger_action)),
						MAKE_INTEGER (2)); }
	| DELETING
		{ $$ = MAKE_NODE (nod_eql, 2,
					MAKE_NODE (nod_internal_info, (int) e_internal_info_count,
						MAKE_INTEGER (internal_trigger_action)),
						MAKE_INTEGER (3)); }
	;

special_trigger_action_predicate	: KW_INSERTING
		{ $$ = MAKE_NODE (nod_eql, 2,
					MAKE_NODE (nod_internal_info, (int) e_internal_info_count,
						MAKE_INTEGER (internal_trigger_action)),
						MAKE_INTEGER (1)); }
	| KW_UPDATING
		{ $$ = MAKE_NODE (nod_eql, 2,
					MAKE_NODE (nod_internal_info, (int) e_internal_info_count,
						MAKE_INTEGER (internal_trigger_action)),
						MAKE_INTEGER (2)); }
	| KW_DELETING
		{ $$ = MAKE_NODE (nod_eql, 2,
					MAKE_NODE (nod_internal_info, (int) e_internal_info_count,
						MAKE_INTEGER (internal_trigger_action)),
						MAKE_INTEGER (3)); }
	;

/* set values */

in_predicate_value	: table_subquery
		| '(' value_list ')'
			{ $$ = MAKE_LIST ($2); }
		;

table_subquery	: '(' column_select ')'
			{ $$ = $2; } 
		;


/* value types */

value	: column_name
		| array_element
		| function
		| u_constant
		| parameter
		| variable
		| cast_specification
		| case_expression
		| next_value_expression
		| udf
		| '-' value
			{ $$ = MAKE_NODE (nod_negate, 1, $2); }
				| '+' value
						{ $$ = $2; }
		| value '+' value
			{ 
			  if (((SQLParse*) sqlParse)->clientDialect >= SQL_DIALECT_V6_TRANSITION)
				  $$ = MAKE_NODE (nod_add2, 2, $1, $3);
			  else
				  $$ = MAKE_NODE (nod_add, 2, $1, $3);
			}
		| value CONCATENATE value
			{ $$ = MAKE_NODE (nod_concatenate, 2, $1, $3); }
		| value COLLATE symbol_collation_name
			{ $$ = MAKE_NODE (nod_collate, (int) e_coll_count, (dsql_nod*) $3, $1); }
		| value '-' value
			{ 
			  if (((SQLParse*) sqlParse)->clientDialect >= SQL_DIALECT_V6_TRANSITION)
				  $$ = MAKE_NODE (nod_subtract2, 2, $1, $3);
			  else 
				  $$ = MAKE_NODE (nod_subtract, 2, $1, $3);
			}
		| value '*' value
			{ 
			  if (((SQLParse*) sqlParse)->clientDialect >= SQL_DIALECT_V6_TRANSITION)
				   $$ = MAKE_NODE (nod_multiply2, 2, $1, $3);
			  else
				   $$ = MAKE_NODE (nod_multiply, 2, $1, $3);
			}
		| value '/' value
			{
			  if (((SQLParse*) sqlParse)->clientDialect >= SQL_DIALECT_V6_TRANSITION)
				  $$ = MAKE_NODE (nod_divide2, 2, $1, $3);
			  else
				  $$ = MAKE_NODE (nod_divide, 2, $1, $3);
			}
		| '(' value ')'
			{ $$ = $2; }
		| '(' column_singleton ')'
			{ $$ = $2; }
		| current_user
		| current_role
		| internal_info
		| DB_KEY
			{ $$ = MAKE_NODE (nod_dbkey, 1, NULL); }
		| symbol_table_alias_name '.' DB_KEY
			{ $$ = MAKE_NODE (nod_dbkey, 1, $1); }
				| KW_VALUE
						{ 
			  $$ = MAKE_NODE (nod_dom_value, 0, NULL);
						}
		| datetime_value_expression
		| null_value
		;

datetime_value_expression : CURRENT_DATE
			{ 
			if (((SQLParse*) sqlParse)->clientDialect < SQL_DIALECT_V6_TRANSITION)
				throw OSRIException (isc_sqlerr, isc_arg_number, (SLONG) -104, 
					isc_arg_gds, isc_sql_dialect_datatype_unsupport,
					isc_arg_number, ((SQLParse*) sqlParse)->clientDialect,
					isc_arg_string, "DATE",
					0);
			if (((SQLParse*) sqlParse)->dbDialect < SQL_DIALECT_V6_TRANSITION)
				throw OSRIException (isc_sqlerr, isc_arg_number, (SLONG) -104, 
					isc_arg_gds, isc_sql_db_dialect_dtype_unsupport,
					isc_arg_number, ((SQLParse*) sqlParse)->dbDialect,
					isc_arg_string, "DATE",
					0);
			$$ = MAKE_NODE (nod_current_date, 0, NULL);
			}
		| CURRENT_TIME
			{ 
			if (((SQLParse*) sqlParse)->clientDialect < SQL_DIALECT_V6_TRANSITION)
				throw OSRIException (isc_sqlerr, isc_arg_number, (SLONG) -104, 
					isc_arg_gds, isc_sql_dialect_datatype_unsupport,
					isc_arg_number, ((SQLParse*) sqlParse)->clientDialect,
					isc_arg_string, "TIME",
					0);
			if (((SQLParse*) sqlParse)->dbDialect < SQL_DIALECT_V6_TRANSITION)
				throw OSRIException (isc_sqlerr, isc_arg_number, (SLONG) -104, 
					isc_arg_gds, isc_sql_db_dialect_dtype_unsupport,
					isc_arg_number, ((SQLParse*) sqlParse)->dbDialect,
					isc_arg_string, "TIME",
					0);
			$$ = MAKE_NODE (nod_current_time, 0, NULL);
			}
		| CURRENT_TIMESTAMP
			{ $$ = MAKE_NODE (nod_current_timestamp, 0, NULL); }
		;

array_element   : column_name '[' value_list ']'
			{ $$ = MAKE_NODE (nod_array, (int) e_ary_count, $1, MAKE_LIST ($3)); }
		;

value_list	: value
		| value_list ',' value
			{ $$ = MAKE_NODE (nod_list, 2, $1, $3); }
		;

constant	: u_constant
		| '-' u_numeric_constant
			{ $$ = MAKE_NODE (nod_negate, 1, $2); }
		;

u_numeric_constant : NUMERIC
			{ $$ = MAKE_CONSTANT ((dsql_str*) $1, CONSTANT_STRING); }
		| NUMBER
			{ $$ = MAKE_CONSTANT ((dsql_str*) $1, CONSTANT_SLONG); }
		| FLOAT_NUMBER
			{ $$ = MAKE_CONSTANT ((dsql_str*) $1, CONSTANT_DOUBLE); }
		| NUMBER64BIT
			{ $$ = MAKE_CONSTANT ((dsql_str*) $1, CONSTANT_SINT64); }
		| SCALEDINT
			{ $$ = MAKE_CONSTANT ((dsql_str*) $1, CONSTANT_SINT64); }
		;

u_constant	: u_numeric_constant
		| sql_string
			{ $$ = MAKE_STRING_CONSTANT ((dsql_str*) $1, ((SQLParse*) sqlParse)->att_charset); }
		| DATE STRING
			{ 
			if (((SQLParse*) sqlParse)->clientDialect < SQL_DIALECT_V6_TRANSITION)
				throw OSRIException (isc_sqlerr, isc_arg_number, (SLONG) -104, 
					isc_arg_gds, isc_sql_dialect_datatype_unsupport,
					isc_arg_number, ((SQLParse*) sqlParse)->clientDialect,
					isc_arg_string, "DATE",
					0);
			if (((SQLParse*) sqlParse)->dbDialect < SQL_DIALECT_V6_TRANSITION)
				throw OSRIException (isc_sqlerr, isc_arg_number, (SLONG) -104, 
					isc_arg_gds, isc_sql_db_dialect_dtype_unsupport,
					isc_arg_number, ((SQLParse*) sqlParse)->dbDialect,
					isc_arg_string, "DATE",
					0);
			$$ = MAKE_CONSTANT ((dsql_str*) $2, CONSTANT_DATE);
			}
		| TIME STRING
			{
			if (((SQLParse*) sqlParse)->clientDialect < SQL_DIALECT_V6_TRANSITION)
				throw OSRIException (isc_sqlerr, isc_arg_number, (SLONG) -104, 
					isc_arg_gds, isc_sql_dialect_datatype_unsupport,
					isc_arg_number, ((SQLParse*) sqlParse)->clientDialect,
					isc_arg_string, "TIME",
					0);
			if (((SQLParse*) sqlParse)->dbDialect < SQL_DIALECT_V6_TRANSITION)
				throw OSRIException (isc_sqlerr, isc_arg_number, (SLONG) -104, 
					isc_arg_gds, isc_sql_db_dialect_dtype_unsupport,
					isc_arg_number, ((SQLParse*) sqlParse)->dbDialect,
					isc_arg_string, "TIME",
					0);
			$$ = MAKE_CONSTANT ((dsql_str*) $2, CONSTANT_TIME);
			}
		| TIMESTAMP STRING
			{ $$ = MAKE_CONSTANT ((dsql_str*) $2, CONSTANT_TIMESTAMP); }
		;

parameter	: '?'
			{ $$ = MAKE_PARAMETER (); }
		;

current_user	: USER
			{ $$ = MAKE_NODE (nod_user_name, 0, NULL); }
		| CURRENT_USER
			{ $$ = MAKE_NODE (nod_user_name, 0, NULL); }
		;

current_role	: CURRENT_ROLE
			{ $$ = MAKE_NODE (nod_current_role, 0, NULL); }
		;

internal_info	: CURRENT_CONNECTION
			{ $$ = MAKE_NODE (nod_internal_info, (int) e_internal_info_count,
						MAKE_INTEGER (internal_connection_id)); }
		| CURRENT_TRANSACTION
			{ $$ = MAKE_NODE (nod_internal_info, (int) e_internal_info_count,
						MAKE_INTEGER (internal_transaction_id)); }
		| GDSCODE
			{ $$ = MAKE_NODE (nod_internal_info, (int) e_internal_info_count,
						MAKE_INTEGER (internal_gdscode)); }
		| SQLCODE
			{ $$ = MAKE_NODE (nod_internal_info, (int) e_internal_info_count,
						MAKE_INTEGER (internal_sqlcode)); }
		| ROW_COUNT
			{ $$ = MAKE_NODE (nod_internal_info, (int) e_internal_info_count,
						MAKE_INTEGER (internal_rows_affected)); }
		;

sql_string	: STRING			/* string in current charset */
			{ $$ = $1; }
		| INTRODUCER STRING		/* string in specific charset */
			{ ((dsql_str*) $2)->str_charset = (TEXT *) $1;
			  $$ = $2; }
		;

signed_short_integer	:	nonneg_short_integer
		| '-' neg_short_integer
			{ $$ = (dsql_nod*) - (long) $2; }
		;

nonneg_short_integer	: NUMBER
			{ if ((long) $1 > SHRT_POS_MAX)
				yyabandon (-842, isc_expec_short);
				/* Short integer expected */
			  $$ = $1;}
		;

neg_short_integer : NUMBER
			{ if ((long) $1 > SHRT_NEG_MAX)
				yyabandon (-842, isc_expec_short);
				/* Short integer expected */
			  $$ = $1;}
		;

pos_short_integer : nonneg_short_integer
			{ if ((long) $1 == 0)
				yyabandon (-842, isc_expec_positive);
				/* Positive number expected */
			  $$ = $1;}
		;

unsigned_short_integer : NUMBER
			{ if ((long) $1 > SHRT_UNSIGNED_MAX)
				yyabandon (-842, isc_expec_ushort);
				/* Unsigned short integer expected */
			  $$ = $1;}
		;

signed_long_integer	:	long_integer
		| '-' long_integer
			{ $$ = (dsql_nod*) - (long) $2; }
		;

long_integer	: NUMBER
			{ $$ = $1;}
		;
	
/* functions */

function	: aggregate_function
	| numeric_value_function
	| string_value_function
	;
	
aggregate_function	: COUNT '(' '*' ')'
			{ $$ = MAKE_NODE (nod_agg_count, 0, NULL); }
		| COUNT '(' all_noise value ')'
			{ $$ = MAKE_NODE (nod_agg_count, 1, $4); }
		| COUNT '(' DISTINCT value ')'
			{ $$ = MAKE_FLAG_NODE (nod_agg_count,
									   NOD_AGG_DISTINCT, 1, $4); }
		| SUM '(' all_noise value ')'
			{ 
			  if (((SQLParse*) sqlParse)->clientDialect >= SQL_DIALECT_V6_TRANSITION)
				  $$ = MAKE_NODE (nod_agg_total2, 1, $4);
			  else
				  $$ = MAKE_NODE (nod_agg_total, 1, $4);
			}
		| SUM '(' DISTINCT value ')'
			{ 
			  if (((SQLParse*) sqlParse)->clientDialect >= SQL_DIALECT_V6_TRANSITION)
				  $$ = MAKE_FLAG_NODE (nod_agg_total2,
						   NOD_AGG_DISTINCT, 1, $4);
			  else
				  $$ = MAKE_FLAG_NODE (nod_agg_total,
						   NOD_AGG_DISTINCT, 1, $4);
			}
		| AVG '(' all_noise value ')'
			{ 
			  if (((SQLParse*) sqlParse)->clientDialect >= SQL_DIALECT_V6_TRANSITION)
				  $$ = MAKE_NODE (nod_agg_average2, 1, $4);
			  else
				  $$ = MAKE_NODE (nod_agg_average, 1, $4);
			}
		| AVG '(' DISTINCT value ')'
			{ 
			  if (((SQLParse*) sqlParse)->clientDialect >= SQL_DIALECT_V6_TRANSITION)
				  $$ = MAKE_FLAG_NODE (nod_agg_average2,
						   NOD_AGG_DISTINCT, 1, $4);
			  else
				  $$ = MAKE_FLAG_NODE (nod_agg_average,
						   NOD_AGG_DISTINCT, 1, $4);
			}
		| MINIMUM '(' all_noise value ')'
			{ $$ = MAKE_NODE (nod_agg_min, 1, $4); }
		| MINIMUM '(' DISTINCT value ')'
			{ $$ = MAKE_NODE (nod_agg_min, 1, $4); }
		| MAXIMUM '(' all_noise value ')'
			{ $$ = MAKE_NODE (nod_agg_max, 1, $4); }
		| MAXIMUM '(' DISTINCT value ')'
			{ $$ = MAKE_NODE (nod_agg_max, 1, $4); }
		;

numeric_value_function	: extract_expression
		;

extract_expression	: EXTRACT '(' timestamp_part FROM value ')'
			{ $$ = MAKE_NODE (nod_extract, (int) e_extract_count, $3, $5); }
		;

string_value_function	:  substring_function
		| KW_UPPER '(' value ')'
			{ $$ = MAKE_NODE (nod_upcase, 1, $3); }
		;

substring_function	: SUBSTRING '(' value FROM value string_length_opt ')'
			/* SQL spec requires numbering to start with 1,
			   hence we decrement the first parameter to make it
			   compatible with the engine's implementation */
			{ $$ = MAKE_NODE (nod_substr, (int) e_substr_count, $3,
				MAKE_NODE (nod_subtract, 2, $5,
					MAKE_INTEGER (1)), $6); }
		;

string_length_opt	: FOR value
			{ $$ = $2; }
		|
			{ $$ = MAKE_INTEGER (SHRT_POS_MAX); }
		;

udf		: symbol_UDF_name '(' value_list ')'
			{ $$ = MAKE_NODE (nod_udf, 2, $1, $3); }
		| symbol_UDF_name '(' ')'
			{ $$ = MAKE_NODE (nod_udf, 1, $1); }
		;

cast_specification	: CAST '(' value AS data_type_descriptor ')'
			{ $$ = MAKE_NODE (nod_cast, (int) e_cast_count, $5, $3); }
		;

/* case expressions */

case_expression	: case_abbreviation
		| case_specification
		;

case_abbreviation	: NULLIF '(' value ',' value ')'
			{ $$ = MAKE_NODE (nod_searched_case, 2, 
				MAKE_NODE (nod_list, 2, MAKE_NODE (nod_eql, 2, $3, $5), 
				MAKE_NODE (nod_null, 0, NULL)), $3); }
		| IIF '(' search_condition ',' value ',' value ')'
			{ $$ = MAKE_NODE (nod_searched_case, 2, 
				MAKE_NODE (nod_list, 2, $3, $5), $7); }
		| COALESCE '(' value ',' value_list ')'
			{ $$ = MAKE_NODE (nod_coalesce, 2, $3, $5); }
		;

case_specification	: simple_case
		| searched_case
		;

simple_case	: CASE case_operand simple_when_clause END
			{ $$ = MAKE_NODE (nod_simple_case, 3, $2, MAKE_LIST($3), MAKE_NODE (nod_null, 0, NULL)); }
		| CASE case_operand simple_when_clause ELSE case_result END
			{ $$ = MAKE_NODE (nod_simple_case, 3, $2, MAKE_LIST($3), $5); }
		;

simple_when_clause	: WHEN when_operand THEN case_result
				{ $$ = MAKE_NODE (nod_list, 2, $2, $4); }
			| simple_when_clause WHEN when_operand THEN case_result
				{ $$ = MAKE_NODE (nod_list, 2, $1, MAKE_NODE (nod_list, 2, $3, $5)); }
			;

searched_case	: CASE searched_when_clause END
			{ $$ = MAKE_NODE (nod_searched_case, 2, MAKE_LIST($2), MAKE_NODE (nod_null, 0, NULL)); }
		| CASE searched_when_clause ELSE case_result END
			{ $$ = MAKE_NODE (nod_searched_case, 2, MAKE_LIST($2), $4); }
		;

searched_when_clause	: WHEN search_condition THEN case_result
			{ $$ = MAKE_NODE (nod_list, 2, $2, $4); }
		| searched_when_clause WHEN search_condition THEN case_result
			{ $$ = MAKE_NODE (nod_list, 2, $1, MAKE_NODE (nod_list, 2, $3, $5)); }
		;

when_operand	: value
		;

case_operand	: value
		;

case_result	: value
		;

/* next value expression */

next_value_expression	: NEXT KW_VALUE FOR symbol_generator_name
			{ $$ = MAKE_NODE (nod_gen_id, 2, $4,
					MAKE_CONSTANT((dsql_str*) 1, CONSTANT_SLONG)); }
		| GEN_ID '(' symbol_generator_name ',' value ')'
			{ 
			  if (((SQLParse*) sqlParse)->clientDialect >= SQL_DIALECT_V6_TRANSITION)
				  $$ = MAKE_NODE (nod_gen_id2, 2, $3, $5);
			  else
				  $$ = MAKE_NODE (nod_gen_id, 2, $3, $5);
			}
		;


timestamp_part	: YEAR
			{ $$ = MAKE_INTEGER (blr_extract_year); }
		| MONTH
			{ $$ = MAKE_INTEGER (blr_extract_month); }
		| DAY
			{ $$ = MAKE_INTEGER (blr_extract_day); }
		| HOUR
			{ $$ = MAKE_INTEGER (blr_extract_hour); }
		| MINUTE
			{ $$ = MAKE_INTEGER (blr_extract_minute); }
		| SECOND
			{ $$ = MAKE_INTEGER (blr_extract_second); }
		| WEEKDAY
			{ $$ = MAKE_INTEGER (blr_extract_weekday); }
		| YEARDAY
			{ $$ = MAKE_INTEGER (blr_extract_yearday); }
		;

all_noise	: ALL
		|
		;

distinct_noise	: DISTINCT
		|
		;

null_value	: KW_NULL
			{ $$ = MAKE_NODE (nod_null, 0, NULL); }
		;



/* Performs special mapping of keywords into symbols */

symbol_UDF_name	: SYMBOL
	;

symbol_blob_subtype_name	: valid_symbol_name
	;

symbol_character_set_name	: valid_symbol_name
	;

symbol_collation_name	: valid_symbol_name
	;

symbol_column_name	: valid_symbol_name
	;

symbol_constraint_name	: valid_symbol_name
	;

symbol_cursor_name	: valid_symbol_name
	;

symbol_domain_name	: valid_symbol_name
	;

symbol_exception_name	: valid_symbol_name
	;

symbol_filter_name	: valid_symbol_name
	;

symbol_gdscode_name	: valid_symbol_name
	;

symbol_generator_name	: valid_symbol_name
	;

symbol_index_name	: valid_symbol_name
	;

symbol_item_alias_name	: valid_symbol_name
	;

symbol_label_name	: valid_symbol_name
	;

symbol_procedure_name	: valid_symbol_name
	;

symbol_role_name	: valid_symbol_name
	;

symbol_table_alias_name	: valid_symbol_name
	;

symbol_table_name	: valid_symbol_name
	;

symbol_trigger_name	: valid_symbol_name
	;

symbol_user_name	: valid_symbol_name
	;

symbol_variable_name	: valid_symbol_name
	;

symbol_view_name	: valid_symbol_name
	;

symbol_savepoint_name	: valid_symbol_name
	;

/* symbols */

valid_symbol_name	: SYMBOL
	| non_reserved_word
	;

/* list of non-reserved words */

non_reserved_word :
	ACTION					/* added in IB 5.0 */
	| CASCADE
	| FREE_IT
	| RESTRICT
	| ROLE
	| TYPE					/* added in IB 6.0 */
	| KW_BREAK				/* added in FB 1.0 */
	| KW_DESCRIPTOR
	| SUBSTRING
	| COALESCE				/* added in FB 1.5 */
	| LAST
	| LEAVE
	| LOCK
	| NULLIF
	| NULLS
	| STATEMENT
	| INSERTING
	| UPDATING
	| DELETING
/*  | FIRST | SKIP -- this is handled by the lexer. */
	| BLOCK
	| BACKUP				/* added in FB 2.0 */
	| KW_DIFFERENCE
	| IIF
	| SCALAR_ARRAY
	| WEEKDAY
	| YEARDAY
	| SEQUENCE
	| NEXT
	| RESTART
	;

%%


static void yyerror(const TEXT* error_string)
{
/**************************************
 *
 *	y y e r r o r
 *
 **************************************
 *
 * Functional description
 *	The argument passed to this function is ignored. Therefore, messages like
 *  "syntax error" and "yacc stack overflow" are never seen.
 *
 **************************************/

	throw SQLSyntaxError();
}


static void yyabandon (SSHORT		sql_code,
					   ISC_STATUS	error_symbol)
{
/**************************************
 *
 *	y y a b a n d o n
 *
 **************************************
 *
 * Functional description
 *	Abandon the parsing outputting the supplied string
 *
 **************************************/

	throw OSRIException (isc_sqlerr, isc_arg_number, (SLONG) sql_code, 
		isc_arg_gds, error_symbol, 0);
}

/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		sqlParse->c
 *	DESCRIPTION:	Lexical routine
 *
 */


void LEX_dsql_init (void)
{
/**************************************
 *
 *	L E X _ d s q l _ i n i t
 *
 **************************************
 *
 * Functional description
 *	Initialize LEX for processing.  This is called only once
 *	per session.
 *
 **************************************/
 
	for (const TOK* token = KEYWORD_getTokens(); token->tok_string; ++token)
		{
		DSQL_SYM symbol = FB_NEW_RPT(*DSQL_permanent_pool, 0) dsql_sym;
		symbol->sym_string = (TEXT *) token->tok_string;
		symbol->sym_length = strlen(token->tok_string);
		symbol->sym_type = SYM_keyword;
		symbol->sym_keyword = token->tok_ident;
		symbol->sym_version = token->tok_version;
		dsql_str* str_ = FB_NEW_RPT(*DSQL_permanent_pool, symbol->sym_length) dsql_str;
		str_->str_length = symbol->sym_length;
		strncpy((char*)str_->str_data, (char*)symbol->sym_string, symbol->sym_length);
		symbol->sym_object = (void *) str_;
		HSHD_insert(symbol);
		}
}



static void prepare_console_debug (int level, int *yydeb)
{
/*************************************
 *
 *	p r e p a r e _ c o n s o l e _ d e b u g
 * 
 *************************************
 *
 * Functional description
 *	Activate debug info. In WinNT, redirect the standard
 *	output so one can see the generated information.
 *	Feel free to add your platform specific code.
 *
 *************************************/
#ifdef DSQL_DEBUG
	DSQL_debug = level;
#endif
	if (level >> 8)
		*yydeb = level >> 8;
}


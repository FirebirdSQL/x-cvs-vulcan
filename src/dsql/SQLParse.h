/*
 *  
 *     The contents of this file are subject to the Initial 
 *     Developer's Public License Version 1.0 (the "License"); 
 *     you may not use this file except in compliance with the 
 *     License. You may obtain a copy of the License at 
 *     http://www.ibphoenix.com/idpl.html. 
 *
 *     Software distributed under the License is distributed on 
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 *     express or implied.  See the License for the specific 
 *     language governing rights and limitations under the License.
 *
 *     The contents of this file or any work derived from this file
 *     may not be distributed under any other license whatsoever 
 *     without the express prior written permission of the original 
 *     author.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 2004 James A. Starkey
 *  All Rights Reserved.
 */

#ifndef _SQLPARSE_H
#define _SQLPARSE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../dsql/dsql_nodes.h"

const int MAX_TOKEN_LEN = 256;
static const char INTERNAL_FIELD_NAME[] = "DSQL internal"; /* NTX: placeholder */

class dsql_fld;
class dsql_fil;
class dsql_nod;
//class dsql_lls;
class dsql_str;
class Stack;

struct thread_db;

class SQLParse
{
public:
	SQLParse(int client_dialect, int db_dialect, int parser_version);
	~SQLParse(void);

	dsql_nod*	parse(thread_db* threadData, int sqlLength, const char* sql, int charset);
	int			yylex(dsql_nod **yylval);
	void		punt(int sqlCode, int errorCode);
	void		stackNodes(dsql_nod* node, Stack* stack);
	void		deleteNodes(void);
	dsql_nod*	makeParameter(void);
	dsql_nod*	makeNode(NOD_TYPE type, int count, ...);
	dsql_nod*	makeFlagNode(NOD_TYPE type, int flag, int count, ...);
	dsql_nod*	makeList(dsql_nod* node);
	dsql_fil*	makeFile(void);
	dsql_fld*	makeField(dsql_nod* field_name);
	dsql_nod*	makeDsqlNode(NOD_TYPE type, int count);
	
	int			clientDialect;
	int			dbDialect;
	int			parserVersion;
	int			yydebug;
	bool		ambiguousStatement;
	thread_db*	threadData;
	dsql_nod*	parseTree;
	dsql_nod*	nodes;
	
	/* This is, in fact, parser state. Not used in lexer itself */
	
	dsql_fld* g_field;
	dsql_fil* g_file;
	dsql_nod* g_field_name;
	SSHORT log_defined, cache_defined;
	int dsql_debug;
	
	/* Actual lexer state begins from here */
	
	const TEXT*		beginning;
	const TEXT*		ptr;
	const TEXT*		end;
	const TEXT*		last_token;
	const TEXT*		line_start;
	const TEXT*		last_token_bk;
	const TEXT*		line_start_bk;
	SSHORT			lines, att_charset;
	SSHORT			lines_bk;
	int				prev_keyword, prev_prev_keyword;
	USHORT			param_number;
	ISC_STATUS		*statusVector;
	
	/* Fields to handle FIRST/SKIP as non-reserved keywords */
	
	bool limit_clause; /* We are inside of limit clause. Need to detect SKIP after FIRST */
	bool first_detection; /* Detect FIRST unconditionally */
	
	/* Fields to handle INSERTING/UPDATING/DELETING as non-reserved keywords */
	
	bool brace_analysis; /* When this is true lexer is informed not to swallow braces around INSERTING/UPDATING/DELETING */
	dsql_nod* makeConstant(dsql_str* string, dsql_constant_type type);
	dsql_nod* makeTriggerType(dsql_nod* arg1, dsql_nod* arg2);
	dsql_str* makeString(const char* string, int length);
	dsql_nod* makeStringConstant(dsql_str* constant, int characterSet);
	dsql_nod* takeSyntaxTree(void);
	dsql_nod* makeConstant(SLONG constant);
	//void postWarning(ISC_STATUS status, ...);
	int getToken(dsql_nod** yylval);
	SQLParse& operator =(const SQLParse& source);
	SQLParse(const SQLParse& source);
	void copy(const SQLParse& source);
};

#endif


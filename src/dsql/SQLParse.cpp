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
 *	2004-03-11 Derived from parse.y by James A. Starkey
 */
 
#include <stdarg.h>
#include "firebird.h"
#include "../jrd/common.h"
#include "SQLParse.h"
#include "SQLSyntaxError.h"
#include "../dsql/dsql.h"
#include "../dsql/alld_proto.h"
#include "../dsql/errd_proto.h"
#include "../dsql/hsh_proto.h"
#include "../dsql/make_proto.h"
#include "../dsql/parse_proto.h"
#include "../dsql/keywords.h"
#include "../dsql/chars.h"
#include "iberror.h"
#include "OSRIException.h"
#include "gds_proto.h"
#include "TempSpace.h"
#if defined _AIX || defined MVS
#undef PAGE_SIZE
#endif
#include "parse.h"

#define CHECK_BOUND(to,string)			if ((to - string) >= MAX_TOKEN_LEN) punt (-104, isc_token_too_long)
#define CHECK_COPY_INCR(to,ch,string)	{ CHECK_BOUND(to,string); *to++ = ch; }

SQLParse::SQLParse(int client_dialect, int db_dialect, int parser_version)
{
	clientDialect = client_dialect;
	dbDialect = db_dialect;
	parserVersion = parser_version;
	yydebug = 0;
	nodes = NULL;
}

SQLParse::~SQLParse(void)
{
	deleteNodes();
}

dsql_nod* SQLParse::parse(thread_db* threadStuff, int sqlLength, const char* sql, int charset)
{
	threadData = threadStuff;
	deleteNodes();
	line_start = ptr = sql;
	end = sql + sqlLength;
	lines = 1;
	att_charset = charset;
	line_start_bk = line_start;
	lines_bk = lines;
	param_number = 1;
	prev_keyword = -1;
	prev_prev_keyword = -1;
	limit_clause = false;	
	first_detection = false;
	brace_analysis = false;
	
#ifdef DSQL_DEBUG
	if (DSQL_debug & 32)
		dsql_trace("Source DSQL string:\n%.*s", sqlLength, sql);
#endif

	ambiguousStatement = false;
	
	//return dsql_yyparse(clientDialect, dbDialect, parserVersion, &ambiguousStatement);
	
	try
		{
		parseTree = NULL;
		
		if (dsql_yyparse (this))
			return NULL;
		
		return parseTree;
		}
	catch (SQLSyntaxError& exception)
		{
		exception;
		if (ptr >= end)
			throw OSRIException (isc_sqlerr, isc_arg_number, (SLONG) -104,
				isc_arg_gds, isc_command_end_err,	/* Unexpected end of command */
				0);
		else
			throw OSRIException (isc_sqlerr, isc_arg_number, (SLONG) -104,
				isc_arg_gds, isc_dsql_token_unk_err,
				isc_arg_number, lines,
				isc_arg_number, last_token - line_start + 1, /*CVC: +1*/
				isc_arg_gds, isc_random,
				isc_arg_cstring, ptr - last_token, last_token, 0);
		}
	
	return false;
}

int SQLParse::yylex(dsql_nod **yylval)
{
	UCHAR	tok_class;
	char  string[MAX_TOKEN_LEN];
	SSHORT	c;

	/* Find end of white space and skip comments */

	for (;;)
		{
		if (ptr >= end)
			return -1;
			
		c = *ptr++;

		/* Process comments */

		if (c == '\n') 
			{
			lines++;
			line_start = ptr;
			continue;
			}

		if ((c == '-') && (*ptr == '-')) 
			{
			/* single-line */
			ptr++;
			while (ptr < end) 
				{
				if ((c = *ptr++) == '\n') 
					{
					lines++;
					line_start = ptr /* + 1*/; /* CVC: +1 left out. */
					break;
					}
				}
			if (ptr >= end)
				return -1;
			continue;
			}
		else if ((c == '/') && (*ptr == '*')) 
			{
			/* multi-line */
			ptr++;
			while (ptr < end) 
				{
				if ((c = *ptr++) == '*') 
					{
					if (*ptr == '/')
						break;
					}
				if (c == '\n') 
					{
					lines++;
					line_start = ptr /* + 1*/; /* CVC: +1 left out. */
					}
				}
			if (ptr >= end)
				return -1;
			ptr++;
			continue;
			}

		tok_class = classes[c];

		if (!(tok_class & CHR_WHITE))
			break;
		}

	/* Depending on tok_class of token, parse token */

	last_token = ptr - 1;

	if (tok_class & CHR_INTRODUCER)
		{
		/* The Introducer (_) is skipped, all other idents are copied
		 * to become the name of the character set
		 */
		char* p = string;
		
		for (; ptr < end && classes[*ptr] & CHR_IDENT; ptr++)
			{
			if (ptr >= end)
				return -1;
			CHECK_COPY_INCR(p, UPPER7(*ptr), string);
			}
		
		CHECK_BOUND(p, string);
		*p = 0;

		/* make a string value to hold the name, the name 
		 * is resolved in pass1_constant */

		*yylval = (dsql_nod*) (makeString(string, p - string))->str_data;

		return INTRODUCER;
		}

	/* parse a quoted string, being sure to look for double quotes */

	if (tok_class & CHR_QUOTE)
		{
		TempSpace temp (sizeof(string), string);
		
		for (;;)
			{
			if (ptr >= end)
				return -1;

			/* *ptr is quote - if next != quote we're at the end */
			
			if ((*ptr == c) && ((++ptr == end) || (*ptr != c)))
				break;
			
			temp.addByte(*ptr++);
			}
			
		if (c == '"')
			{
			ambiguousStatement = true; /* string delimited by double quotes could be
					**   either a string constant or a SQL delimited
					**   identifier, therefore marks the SQL
					**   statement as ambiguous  */
					
			if (clientDialect == SQL_DIALECT_V6_TRANSITION)
				punt (-104, isc_invalid_string_constant);
			
			if (clientDialect >= SQL_DIALECT_V6)
				{
				/***
				if ((p - buffer) >= MAX_TOKEN_LEN)
					punt (-104, isc_token_too_long);
				***/
				
				*yylval = (dsql_nod*) makeString((char*) temp.space, temp.length);
				//dsql_str* delimited_id_str = (dsql_str*) yylval;
				dsql_str* delimited_id_str = *(dsql_str**) yylval;
				delimited_id_str->str_flags |= STR_delimited_id;
				
				return SYMBOL;
				}
			}
			
		//*yylval = (dsql_nod*) makeString(buffer, p - buffer);
		*yylval = (dsql_nod*) makeString((char*) temp.space, temp.length);

		return STRING;
		}

	/* 
	 * Check for a numeric constant, which starts either with a digit or with
	 * a decimal point followed by a digit.
	 * 
	 * This code recognizes the following token types:
	 * 
	 * NUMBER: string of digits which fits into a 32-bit integer
	 * 
	 * NUMBER64BIT: string of digits whose value might fit into an SINT64,
	 *   depending on whether or not there is a preceding '-', which is to
	 *   say that "9223372036854775808" is accepted here.
	 *
	 * SCALEDINT: string of digits and a single '.', where the digits
	 *   represent a value which might fit into an SINT64, depending on
	 *   whether or not there is a preceding '-'.
	 *
	 * FLOAT: string of digits with an optional '.', and followed by an "e"
	 *   or "E" and an optionally-signed exponent.
	 *
	 * NOTE: we swallow leading or trailing blanks, but we do NOT accept
	 *   embedded blanks:
	 *
	 * Another note: c is the first character which need to be considered,
	 *   ptr points to the next character.
	 *
	 * YAN: Really long digit strings that contain a decimal point value
	 *      should be acceptable even if they can't be represented with
	 *      exact precision - the "." tells us the user expects floating
	 *      point.  So as long as there's a "." then we don't error out
	 *      with digit strings that won't fit in a UINT64.
	 */

	//fb_assert(ptr <= end);

	if ((tok_class & CHR_DIGIT) || ((c == '.') && (ptr < end) && (classes[*ptr] & CHR_DIGIT)))
		{
		/* The following variables are used to recognize kinds of numbers. */

		bool have_error	 = false;	/* syntax error or value too large */
		bool have_digit	 = false;	/* we've seen a digit			  */
		bool have_decimal   = false;	/* we've seen a '.'				*/
		bool have_exp	   = false;	/* digit ... [eE]				  */
		bool have_exp_sign  = false; /* digit ... [eE] {+-]			 */
		bool have_exp_digit = false; /* digit ... [eE] ... digit		*/
		bool should_be_float = false; /* Digit string long enough to need float representation? */
		UINT64	number		 = 0;
		UINT64	limit_by_10	= MAX_SINT64 / 10;

		for (--ptr ; ptr < end ; ptr++)
			{
			c = *ptr;
			
			if (have_exp_digit && (! (classes[c]  & CHR_DIGIT)))
				/* First non-digit after exponent and digit terminates
				 the token. */
				break;
			else if (have_exp_sign && (! (classes[c]  & CHR_DIGIT)))
				{
				/* only digits can be accepted after "1E-" */
				have_error = true;
				break;
				}
			else if (have_exp)
				{
				/* We've seen e or E, but nothing beyond that. */
				if ( ('-' == c) || ('+' == c) )
					have_exp_sign = true;
				else if ( classes[c]  & CHR_DIGIT )
					/* We have a digit: we haven't seen a sign yet,
					but it's too late now. */
					have_exp_digit = have_exp_sign  = true;
				else
					{
					/* end of the token */
					have_error = true;
					break;
					}
				}
			else if ('.' == c)
				{
				if (!have_decimal)
					have_decimal = true;
				else
					{
					have_error = true;
					break;
					}
				}
			else if (classes[c] & CHR_DIGIT)
				{
				/* Before computing the next value, make sure there will be
				   no overflow.  If there is, we accept it but note that it
				   will require downstream conversion as a FLOAT_NUMBER. */

				have_digit = true;
				
				if (number >= limit_by_10)
					should_be_float = true;
				else
					number = number * 10 + ( c - '0' );
			    }	
			else if ( (('E' == c) || ('e' == c)) && have_digit )
				have_exp = true;
			else
				/* Unexpected character: this is the end of the number. */
				break;
			}

		/* We're done scanning the characters: now return the right kind
		   of number token, if any fits the bill. */

		/* If we have a really long digit string, then the user must have */
		/* used floating notation ("." or "e").  If s/he didn't then it's */
		/* an error since the integer value can't be stored in a UINT64.  */

		if( !have_error )
			{
			if( should_be_float && !(have_exp || have_decimal ))
				have_error = true;
			}
		
		if (!have_error)
			{
			//fb_assert(have_digit);

			if (have_exp_digit || should_be_float)
				{
				*yylval = (dsql_nod*) makeString(last_token, ptr - last_token);
				last_token_bk = last_token;
				line_start_bk = line_start;
				lines_bk = lines;

				return FLOAT_NUMBER;
				}
			else if (!have_exp)
				{

				/* We should return some kind (scaled-) integer type
				   except perhaps in dialect 1. */

				if (!have_decimal && (number <= MAX_SLONG))
					{
					*yylval = (dsql_nod*) (long) number;
					return NUMBER;
					}
				else
					{
					/* We have either a decimal point with no exponent
					   or a string of digits whose value exceeds MAX_SLONG:
					   the returned type depends on the client dialect,
					   so warn of the difference if the client dialect is
					   SQL_DIALECT_V6_TRANSITION.
					*/

					if (SQL_DIALECT_V6_TRANSITION == clientDialect)
						{
						/* Issue a warning about the ambiguity of the numeric
						 * numeric literal.  There are multiple calls because
						 * the message text exceeds the 119-character limit
						 * of our message database.
						 */
						ERRD_post_warning( isc_dsql_warning_number_ambiguous,
							   isc_arg_cstring, ptr - last_token, last_token,
							   isc_arg_end );
						ERRD_post_warning( isc_dsql_warning_number_ambiguous1,
							   isc_arg_end );
						}

					*yylval = (dsql_nod*) makeString(last_token, ptr - last_token);

					last_token_bk = last_token;
					line_start_bk = line_start;
					lines_bk = lines;

					if (clientDialect < SQL_DIALECT_V6_TRANSITION)
						return FLOAT_NUMBER;
					else if (have_decimal)
						return SCALEDINT;
					else
						return NUMBER64BIT;
					}
				} /* else if (!have_exp) */
			} /* if (!have_error) */

		/* we got some kind of error or overflow, so don't recognize this
		 * as a number: just pass it through to the next part of the lexer.
		 */
		}

	/* Restore the status quo ante, before we started our unsuccessful
	   attempt to recognize a number. */
	   
	ptr = last_token;
	c   = *ptr++;
	
	/* We never touched tok_class, so it doesn't need to be restored. */
	/* end of number-recognition code */


	if (tok_class & CHR_LETTER)
		{
		char* p = string;
		CHECK_COPY_INCR(p, UPPER (c), string);
		
		for (; ptr < end && classes[*ptr] & CHR_IDENT; ptr++)
			{
			if (ptr >= end)
				return -1;
			CHECK_COPY_INCR(p, UPPER (*ptr), string);
			}

		CHECK_BOUND(p, string);
		*p = 0;
		dsql_sym* sym = HSHD_lookup (NULL, (TEXT *) string, (SSHORT)(p - string), SYM_keyword, parserVersion);
		
		if (sym)
			{
			/* 13 June 2003. Nickolay Samofatov
			* Detect INSERTING/UPDATING/DELETING as non-reserved keywords.
			* We need to help parser from lexer because our grammar is not LARL(1) in this case
			*/
			if (prev_keyword == '(' && !brace_analysis &&
				(sym->sym_keyword == INSERTING ||
				 sym->sym_keyword == UPDATING ||
				 sym->sym_keyword == DELETING
				) &&
				/* Produce special_trigger_action_predicate only where we can handle it -
				  in search conditions */
				(prev_prev_keyword == '(' || prev_prev_keyword == NOT || prev_prev_keyword == AND ||
				 prev_prev_keyword == OR || prev_prev_keyword == ON || prev_prev_keyword == HAVING ||
				 prev_prev_keyword == WHERE || prev_prev_keyword == WHEN) )
				{			
				SQLParse state = *this;
				int nextToken = state.yylex(yylval);
				
				if (nextToken == OR || nextToken == AND) 
					{
					switch(sym->sym_keyword) 
						{
						case INSERTING:
							*yylval = (dsql_nod*) sym->sym_object;
							return KW_INSERTING;
						case UPDATING:
							*yylval = (dsql_nod*) sym->sym_object;
							return KW_UPDATING;
						case DELETING:
							*yylval = (dsql_nod*) sym->sym_object;
							return KW_DELETING;
						}
					}
				}
				
			/* 23 May 2003. Nickolay Samofatov
			 * Detect FIRST/SKIP as non-reserved keywords
			 * 1. We detect FIRST or SKIP as keywords if they appear just after SELECT and
			 *   immediately before parameter mark ('?'), opening brace ('(') or number
			  * 2. We detect SKIP as a part of FIRST/SKIP clause the same way
			 * 3. We detect FIRST if we are explicitly asked for (such as in NULLS FIRST/LAST clause)
			 * 4. In all other cases we return them as SYMBOL
			 */
			 
			if ((sym->sym_keyword == FIRST && !first_detection) || sym->sym_keyword == SKIP)
				{
				if (prev_keyword == SELECT || limit_clause) 
					{
					SQLParse state = *this;
					int nextToken = state.yylex(yylval);
					//lex = savedState;
					
					if (nextToken != NUMBER && nextToken != '?' && nextToken != '(') 
						{
						*yylval = (dsql_nod*) makeString(string, p - string);
						last_token_bk = last_token;
						line_start_bk = line_start;
						lines_bk = lines;
						
						return SYMBOL;
						}
					else 
						{
						*yylval = (dsql_nod*) sym->sym_object;
						last_token_bk = last_token;
						line_start_bk = line_start;
						lines_bk = lines;
						
						return sym->sym_keyword;
						}
					} /* else fall down and return token as SYMBOL */
				}
			else 
				{
				*yylval = (dsql_nod*) sym->sym_object;
				last_token_bk = last_token;
				line_start_bk = line_start;
				lines_bk = lines;
				return sym->sym_keyword;
				}
			}
			
		*yylval = (dsql_nod*) makeString(string, p - string);
		last_token_bk = last_token;
		line_start_bk = line_start;
		lines_bk = lines;
		
		return SYMBOL;
		}

	/* Must be punctuation -- test for double character punctuation */

	if (last_token + 1 < end)
		{
		dsql_sym* sym = HSHD_lookup (NULL, last_token, 2, SYM_keyword, parserVersion);
		
		if (sym)
			{
			++ptr;
			return sym->sym_keyword;
			}
		}
		
	/* We need to swallow braces around INSERTING/UPDATING/DELETING keywords */
	/* This algorithm is not perfect, but it is ok for now. 
	  It should be dropped when BOOLEAN datatype is introduced in Firebird */

	if ( c == '(' && !brace_analysis && 
		/* 1) We need to swallow braces in all boolean expressions
		   2) We may swallow braces in ordinary expressions 
		   3) We should not swallow braces after special tokens 
			 like IF, FIRST, SKIP, VALUES and 30 more other	   
		*/
		(prev_keyword == '(' || prev_keyword == NOT || prev_keyword == AND || prev_keyword == OR ||
		 prev_keyword == ON || prev_keyword == HAVING || prev_keyword == WHERE || prev_keyword == WHEN) )
		{
		SQLParse savedState (*this);	
		brace_analysis = true;
		int openCount = 0;
		int nextToken;
		
		do 
			{
			openCount++;
			nextToken = yylex(yylval);
			} while (nextToken == '(');
			
		dsql_nod* temp_val = *yylval;
		
		if (nextToken == INSERTING || nextToken == UPDATING || nextToken == DELETING)
			{
			/* Skip closing braces. */
			while ( openCount && yylex(yylval) == ')')
				openCount--;

			if (openCount) 
				/* Not enough closing braces. Restore status quo. */
				*this = savedState;
			else 
				{
				/* Cool! We successfully swallowed braces ! */
				
				brace_analysis = false;
				*yylval = temp_val;
				
				/* Check if we need to handle LR(2) grammar case */
				
				if (prev_keyword == '(' &&
					/* Produce special_trigger_action_predicate only where we can handle it -
					  in search conditions */
					(prev_prev_keyword == '(' || prev_prev_keyword == NOT || prev_prev_keyword == AND ||
					 prev_prev_keyword == OR || prev_prev_keyword == ON || prev_prev_keyword == HAVING ||
					 prev_prev_keyword == WHERE || prev_prev_keyword == WHEN) )
					{
					savedState = *this;
					int token = yylex(yylval);
					*this = savedState;
					
					if (token == OR || token == AND) 
						{
						switch(nextToken) 
							{
							case INSERTING:
								return KW_INSERTING;
							case UPDATING:
								return KW_UPDATING;
							case DELETING:
								return KW_DELETING;
							}
						}
					}
				return nextToken;
				}
			}
		else 
			/* Restore status quo. */
			*this = savedState;
		}

	/* Single character punctuation are simply passed on */

	return c;
}

void SQLParse::punt(int sqlCode, int errorCode)
{
	throw OSRIException (isc_sqlerr, isc_arg_number, sqlCode, 
			   isc_arg_gds, errorCode, 0);
}

dsql_nod* SQLParse::makeParameter(void)
{
	//TSQL tdsql = GET_THREAD_DATA;

	dsql_nod* node = makeDsqlNode (nod_parameter, 1);
	node->nod_line = (USHORT) lines_bk;
	node->nod_column = (USHORT) (last_token_bk - line_start_bk + 1);
	node->nod_count = 1;
	node->nod_arg[0] = (dsql_nod*)(long) param_number++;

	return node;
}

dsql_nod* SQLParse::makeNode(NOD_TYPE type, int count, ...)
{
	dsql_nod* node = makeDsqlNode (type, count);
	node->nod_line = (USHORT) lines_bk;
	node->nod_column = (USHORT) (last_token_bk - line_start_bk + 1);
	dsql_nod** p = node->nod_arg;
	va_list	ptr;
	VA_START (ptr, count);

	while (--count >= 0)
		*p++ = va_arg (ptr, dsql_nod*);

	return node;
}

dsql_nod* SQLParse::makeFlagNode(NOD_TYPE type, int flag, int count, ...)
{
	dsql_nod* node = makeDsqlNode (type, count);
	node->nod_flags = flag;
	node->nod_line = (USHORT) lines_bk;
	node->nod_column = (USHORT) (last_token_bk - line_start_bk + 1);
	dsql_nod** p = node->nod_arg;
	va_list	ptr;
	VA_START (ptr, count);

	while (--count >= 0)
		*p++ = va_arg (ptr, dsql_nod*);

	return node;
}

dsql_nod* SQLParse::makeList(dsql_nod* node)
{
	if (!node)
		return node;

	Stack stack;
	stackNodes (node, &stack);
	USHORT l = stack.count();
	
	dsql_nod* old  = node;
	node = makeDsqlNode (nod_list, l);
	node->nod_type  = nod_list;
	node->nod_flags = old->nod_flags;
	dsql_nod** ptr = node->nod_arg + node->nod_count;

	/***
	while (stack)
		*--ptr = (dsql_nod*) LLS_POP (&stack);
	***/
	
	FOR_STACK(dsql_nod*, node, &stack)
		*--ptr = node;
	END_FOR;
	
	return node;
}

void SQLParse::stackNodes(dsql_nod* node, Stack* stack)
{
	if (node->nod_type != nod_list)
		{
		LLS_PUSH (node, stack);
		return;
		}

	/* To take care of cases where long lists of nodes are in a chain
	   of list nodes with exactly one entry, this algorithm will look
	   for a pattern of repeated list nodes with two entries, the first
	   being a list node and the second being a non-list node.   Such
	   a list will be reverse linked, and then re-reversed, stacking the
	   non-list nodes in the process.   The purpose of this is to avoid
	   massive recursion of this function. */

	dsql_nod* start_chain = node;
	dsql_nod* end_chain = NULL;
	dsql_nod* curr_node = node;
	dsql_nod* next_node = node->nod_arg[0];
	
	while ( curr_node->nod_count == 2 &&
			curr_node->nod_arg[0]->nod_type == nod_list &&
			curr_node->nod_arg[1]->nod_type != nod_list &&
			next_node->nod_arg[0]->nod_type == nod_list &&
			next_node->nod_arg[1]->nod_type != nod_list)
		{

		/* pattern was found so reverse the links and go to next node */

		dsql_nod* save_link = next_node->nod_arg[0];
		next_node->nod_arg[0] = curr_node;
		curr_node = next_node;
		next_node = save_link;
		end_chain = curr_node;
		}

	/* see if any chain was found */

	if (end_chain)
		{

		/* first, handle the rest of the nodes */
		/* note that next_node still points to the first non-pattern node */

		stackNodes( next_node, stack);

		/* stack the non-list nodes and reverse the chain on the way back */
		
		curr_node = end_chain;
		
		while (true)
			{
			LLS_PUSH( curr_node->nod_arg[1], stack);
			if ( curr_node == start_chain)
				break;
			dsql_nod* save_link = curr_node->nod_arg[0];
			curr_node->nod_arg[0] = next_node;
			next_node = curr_node;
			curr_node = save_link;
			}
		return;
		}

	dsql_nod** ptr = node->nod_arg;
	
	for (const dsql_nod* const* const end = ptr + node->nod_count; ptr < end; ptr++)
		stackNodes (*ptr, stack);
}

dsql_fil* SQLParse::makeFile(void)
{
	dsql_fil* temp_file = FB_NEW(*threadData->tsql_default) dsql_fil;

	return temp_file;
}

dsql_fld* SQLParse::makeField(dsql_nod* field_name)
{
	if (field_name == NULL)
		{
		dsql_fld* field = new dsql_fld;
			//FB_NEW_RPT(*tdsql->tsql_default, sizeof (INTERNAL_FIELD_NAME)) dsql_fld;
		//strcpy (field->fld_name, (TEXT*) INTERNAL_FIELD_NAME);
		field->fld_name = INTERNAL_FIELD_NAME;
		return field;
		}
		
	const dsql_str* string = (dsql_str*) field_name->nod_arg[1];
	dsql_fld* field = new dsql_fld;
		//FB_NEW_RPT(*tdsql->tsql_default, strlen ((SCHAR*) string->str_data)) dsql_fld;
	//strcpy (field->fld_name, (TEXT*) string->str_data);
	field->fld_name =  string->str_data;

	return field;
}

void SQLParse::deleteNodes(void)
{
	for (dsql_nod *node; node = nodes;)
		{
		nodes = node->nod_next;
		delete node;
		}
}

dsql_nod* SQLParse::makeDsqlNode(NOD_TYPE type, int count)
{
	dsql_nod* node = FB_NEW_RPT(*threadData->tsql_default, count) dsql_nod;
	node->nod_type = type;
	node->nod_count = count;
	node->nod_next = nodes;
	nodes = node;
	
	return node;
}

dsql_nod* SQLParse::makeConstant(dsql_str* string, dsql_constant_type type)
{
	return MAKE_constant (threadData, string, type);
}

dsql_nod* SQLParse::makeConstant(SLONG constant)
{
	return MAKE_constant (threadData, (dsql_str*)(long) constant, CONSTANT_SLONG);
}

dsql_nod* SQLParse::makeTriggerType(dsql_nod* arg1, dsql_nod* arg2)
{
	return MAKE_trigger_type (threadData, arg1, arg2);
}

dsql_str* SQLParse::makeString(const char* string, int length)
{
	//return MAKE_string (threadData, string, length);
	//return MAKE_tagged_string(threadData, str, length, NULL);
	dsql_str* node = FB_NEW_RPT(*threadData->tsql_default, length) dsql_str;
	node->str_charset = NULL;
	node->str_length  = length;
	memcpy(node->str_data, string, length);

	return node;
}

dsql_nod* SQLParse::makeStringConstant(dsql_str* constant, int characterSet)
{
	//return MAKE_str_constant (threadData, constant, characterSet);
	dsql_nod* node = FB_NEW_RPT(*threadData->tsql_default, 1) dsql_nod;
	node->nod_type = nod_constant;
	node->nod_desc.dsc_dtype = dtype_text;
	node->nod_desc.dsc_sub_type = 0;
	node->nod_desc.dsc_scale = 0;
	node->nod_desc.dsc_length = static_cast<USHORT>(constant->str_length);
	node->nod_desc.dsc_address = reinterpret_cast<UCHAR*>(constant->str_data);
	node->nod_desc.dsc_ttype = characterSet;
	// carry a pointer to the constant to resolve character set in pass1 
	node->nod_arg[0] = (dsql_nod*) constant;

	return node;
}

dsql_nod* SQLParse::takeSyntaxTree(void)
{
	dsql_nod *node = nodes;
	nodes = NULL;
	
	return node;
}

/***
void SQLParse::ERRD_post_warning(ISC_STATUS status, ...)
{
	va_list args;
	VA_START(args, status);
}
***/


int SQLParse::getToken(dsql_nod** yylval)
{
	int keyWord = yylex(yylval);
	prev_prev_keyword = prev_keyword;
	prev_keyword = keyWord;
	
	return keyWord;
}

SQLParse& SQLParse::operator =(const SQLParse& source)
{
	copy (source);
	
	return *this;
}

SQLParse::SQLParse(const SQLParse& source)
{
	nodes = NULL;
	copy (source);
}

void SQLParse::copy(const SQLParse& source)
{
	clientDialect = source.clientDialect;
	dbDialect = source.dbDialect;
	parserVersion = source.parserVersion;
	yydebug = source.yydebug;
	ambiguousStatement = source.ambiguousStatement;
	threadData = source.threadData;
	parseTree = source.parseTree;
	
	g_field = source.g_field;
	g_file = source.g_file;
	g_field_name = source.g_field_name;
	log_defined  = source.log_defined;
	cache_defined = source.cache_defined;
	dsql_debug = source.dsql_debug;
	
	beginning = source.beginning;
	ptr = source.ptr;
	end = source.end;
	last_token = source.last_token;
	line_start = source.line_start;
	last_token_bk = source.last_token_bk;
	line_start_bk = source.line_start_bk;
	lines = source.lines;
	att_charset = source.att_charset;
	lines_bk = source.lines_bk;
	prev_keyword = source.prev_keyword;
	prev_prev_keyword = source.prev_prev_keyword;
	param_number = source.param_number;
	statusVector = source.statusVector;
	
	limit_clause = source.limit_clause; 
	first_detection = source.first_detection;
}

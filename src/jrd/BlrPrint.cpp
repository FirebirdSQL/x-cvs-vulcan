#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "fbdev.h"
#include "BlrPrint.h"
#include "common.h"
#include "blr.h"
#include "../jrd/gdsassert.h"
#include "OSRIException.h"
#include "iberror.h"

/* BLR Pretty print stuff */

#define PRINT_VERB 	print_verb (level)
#define PRINT_LINE	print_line ((SSHORT) offset)
#define PRINT_BYTE	print_byte ()
#define PRINT_CHAR	print_char ()
#define PRINT_WORD	print_word ()
#define PRINT_COND	print_cond ()
#define PRINT_DTYPE	print_dtype ()
#define PRINT_JOIN	print_join ()
#define BLR_BYTE	*(ctl_blr)++
#define BLR_PEEK	(*ctl_blr)
#define PUT_BYTE(byte)	*(ctl_ptr)++ = byte

#define op_line		 1
#define op_verb		 2
#define op_byte		 3
#define op_word		 4
#define op_pad		 5
#define op_dtype	 6
#define op_message	 7
#define op_literal	 8
#define op_begin	 9
#define op_map		 10
#define op_args		 11
#define op_union	 12
#define op_indent	 13
#define op_join		 14
#define op_parameters	 15
#define op_error_handler 16
#define op_set_error	 17
#define op_literals		18
#define op_relation		20
#define op_exec_into	21
#define op_stall		22

static const UCHAR
	/* generic print formats */
	zero[]		= { op_line, 0 },
	one[]		= { op_line, op_verb, 0},
	two[]		= { op_line, op_verb, op_verb, 0},
	three[]		= { op_line, op_verb, op_verb, op_verb, 0},
	field[]		= { op_byte, op_byte, op_literal, op_pad, op_line, 0},
	byte[]		= { op_byte, op_line, 0},
	byte_args[] = { op_byte, op_line, op_args, 0},
	byte_byte[] = { op_byte, op_byte, op_line, 0},
	byte_verb[] = { op_byte, op_line, op_verb, 0},
	byte_verb_verb[] = { op_byte, op_line, op_verb, op_verb, 0},
	byte_literal[] = { op_byte, op_literal, op_line, 0},
	byte_byte_verb[] = { op_byte, op_byte, op_line, op_verb, 0},
	parm[]		= { op_byte, op_word, op_line, 0},	/* also field id */

	parm2[]		= { op_byte, op_word, op_word, op_line, 0},
	parm3[]		= { op_byte, op_word, op_word, op_word, op_line, 0},

	/* formats specific to a verb */
	begin[]		= { op_line, op_begin, op_verb, 0},
	literal[]	= { op_dtype, op_literal, op_line, 0},
	message[]	= { op_byte, op_word, op_line, op_message, 0},
	rse[]		= { op_byte, op_line, op_begin, op_verb, 0},
	relation[]	= { op_byte, op_literal, op_pad, op_byte, op_line, 0},
	relation2[] = { op_byte, op_literal, op_line, op_indent, op_byte,
					op_literal, op_pad, op_byte, op_line, 0},
	aggregate[] = { op_byte, op_line, op_verb, op_verb, op_verb, 0},
	rid[]		= { op_word, op_byte, op_line, 0},
	rid2[]		= { op_word, op_byte, op_literal, op_pad, op_byte, op_line, 0},
	union_ops[] = { op_byte, op_byte, op_line, op_union, 0},
    map[]  	    = { op_word, op_line, op_map, 0},
	function[]	= { op_byte, op_literal, op_byte, op_line, op_args, 0},
	gen_id[]	= { op_byte, op_literal, op_line, op_verb, 0},
	declare[]	= { op_word, op_dtype, op_line, 0},
	variable[]	= { op_word, op_line, 0},
	indx[]		= { op_line, op_verb, op_indent, op_byte, op_line, op_args, 0},
	find[]		= { op_byte, op_verb, op_verb, op_indent, op_byte, op_line, op_args, 0},
	seek[]		= { op_line, op_verb, op_verb, 0},
	join[]		= { op_join, op_line, 0},
	exec_proc[] = { op_byte, op_literal, op_line, op_indent, op_word, op_line,
					op_parameters, op_indent, op_word, op_line, op_parameters, 0},
	procedure[] = { op_byte, op_literal, op_pad, op_byte, op_line, op_indent,
					op_word, op_line, op_parameters, 0},
	pid[]		= { op_word, op_pad, op_byte, op_line, op_indent, op_word,
					op_line, op_parameters, 0},
	error_handler[] = { op_word, op_line, op_error_handler, 0},
	set_error[] = { op_set_error, op_line, 0},
	cast[]		= { op_dtype, op_line, op_verb, 0},
	indices[]	= { op_byte, op_line, op_literals, 0},
	lock_relation[] = { op_line, op_indent, op_relation, op_line, op_verb, 0},
	range_relation[] = { op_line, op_verb, op_indent, op_relation, op_line, 0},
	extract[]	= { op_line, op_byte, op_verb, 0},
	user_savepoint[]	= { op_byte, op_byte, op_literal, op_line, 0},
	exec_into[] = { op_word, op_line, op_indent, op_exec_into, 0},
	for_stall[] = {  op_line, op_stall, op_verb, op_verb, 0 };

#include "../jrd/blp.h"

BlrPrint::BlrPrint(const UCHAR* blr,
				   FPTR_PRINT_CALLBACK routine,
				   void* user_arg, SSHORT language)
{
	if (!(ctl_routine = routine))
		ctl_routine = defaultPrint;
		
	ctl_user_arg = user_arg;
	ctl_blr = ctl_blr_start = blr;
	ctl_ptr = ctl_buffer;
	ctl_language = language;
	
}

BlrPrint::~BlrPrint(void)
{
}


void BlrPrint::format(const char* string, ...)
{
/**************************************
 *
 *	b l r _ f o r m a t
 *
 **************************************
 *
 * Functional description
 *	Format an utterance.
 *
 **************************************/
	va_list ptr;

	VA_START(ptr, string);
	vsprintf(ctl_ptr, string, ptr);
	while (*ctl_ptr)
		ctl_ptr++;
}


void BlrPrint::indent(SSHORT level)
{
/**************************************
 *
 *	b l r _ i n d e n t
 *
 **************************************
 *
 * Functional description
 *	Indent for pretty printing.
 *
 **************************************/

	level *= 3;
	while (--level >= 0)
		PUT_BYTE(' ');
}



void BlrPrint::print_blr(UCHAR operator_)
{
/**************************************
 *
 *	b l r _ p r i n t _ b l r
 *
 **************************************
 *
 * Functional description
 *	Print a blr item.
 *
 **************************************/
	SCHAR *p;

	if (operator_ > FB_NELEM(blr_table) ||
		!(p = (SCHAR *) /* const_cast */ blr_table[operator_].blr_string))
		error("*** blr operator %d is undefined ***",
				  (int) operator_);

	format("blr_%s, ", p);
}


SCHAR BlrPrint::print_byte()
{
/**************************************
 *
 *	b l r _ p r i n t _ b y t e
 *
 **************************************
 *
 * Functional description
 *	Print a byte as a numeric value and return same.
 *
 **************************************/
	const UCHAR v = BLR_BYTE;
	format((ctl_language) ? "chr(%d), " : "%d, ",
			   (int) v);

	return v;
}


SCHAR BlrPrint::print_char()
{
/**************************************
 *
 *	b l r _ p r i n t _ c h a r
 *
 **************************************
 *
 * Functional description
 *	Print a byte as a numeric value and return same.
 *
 **************************************/
	SCHAR c;
	UCHAR v;

	v = c = BLR_BYTE;
	const bool printable = (c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z') ||
	    (c >= '0' && c <= '9') ||
	     c == '$' ||
	     c == '_';

	if (printable)
		format("'%c',", (char) c);
	else if (ctl_language)
		format("chr(%d),", (int) v);
	else
		format("%d,", (int) c);

	return c;
}


void BlrPrint::print_cond()
{
/**************************************
 *
 *	b l r _ p r i n t _ c o n d
 *
 **************************************
 *
 * Functional description
 *	Print an error condition.
 *
 **************************************/
	SSHORT n;

	const USHORT ctype = BLR_BYTE;

	switch (ctype) {
	case blr_gds_code:
		format("blr_gds_code, ");
		n = PRINT_BYTE;
		while (--n >= 0)
			PRINT_CHAR;
		break;

	case blr_exception:
		format("blr_exception, ");
		n = PRINT_BYTE;
		while (--n >= 0)
			PRINT_CHAR;
		break;

	case blr_sql_code:
		format("blr_sql_code, ");
		PRINT_WORD;
		break;

	case blr_default_code:
		format("blr_default_code, ");
		break;

	default:
		error("*** invalid condition type ***");
		break;
	}
	return;
}


int BlrPrint::print_dtype()
{
/**************************************
 *
 *	b l r _ p r i n t _ d t y p e
 *
 **************************************
 *
 * Functional description
 *	Print a datatype sequence and return the length of the
 *	data described.
 *
 **************************************/
	SSHORT length;

	const USHORT dtype = BLR_BYTE;

/* Special case blob (261) to keep down the size of the
   jump table */
	const TEXT* string;

	switch (dtype) 
		{
		case blr_short:
			string = "short";
			length = 2;
			break;

		case blr_long:
			string = "long";
			length = 4;
			break;

		case blr_int64:
			string = "int64";
			length = 8;
			break;

		case blr_quad:
			string = "quad";
			length = 8;
			break;

		case blr_timestamp:
			string = "timestamp";
			length = 8;
			break;

		case blr_sql_time:
			string = "sql_time";
			length = 4;
			break;

		case blr_sql_date:
			string = "sql_date";
			length = 4;
			break;

		case blr_float:
			string = "float";
			length = 4;
			break;

		case blr_double:
			{
				string = "double";

				/* for double literal, return the length of the numeric string */
				const UCHAR v1 = *(ctl_blr);
				const UCHAR v2 = *(ctl_blr + 1);
				length = ((v2 << 8) | v1) + 2;
				break;
			}

		case blr_d_float:
			string = "d_float";
			length = 8;
			break;

		case blr_text:
			string = "text";
			break;

		case blr_cstring:
			string = "cstring";
			break;

		case blr_varying:
			string = "varying";
			break;

		case blr_text2:
			string = "text2";
			break;

		case blr_cstring2:
			string = "cstring2";
			break;

		case blr_varying2:
			string = "varying2";
			break;

		default:
			error("*** invalid data type ***");
			break;
		}

	format("blr_%s, ", string);

	switch (dtype) 
		{
		case blr_text:
			length = PRINT_WORD;
			break;

		case blr_varying:
			length = PRINT_WORD + 2;
			break;

		case blr_short:
		case blr_long:
		case blr_quad:
		case blr_int64:
			PRINT_BYTE;
			break;

		case blr_text2:
			PRINT_WORD;
			length = PRINT_WORD;
			break;

		case blr_varying2:
			PRINT_WORD;
			length = PRINT_WORD + 2;
			break;

		case blr_cstring2:
			PRINT_WORD;
			length = PRINT_WORD;
			break;

		default:
			if (dtype == blr_cstring)
				length = PRINT_WORD;
			break;
		}

	return length;
}


void BlrPrint::print_join()
{
/**************************************
 *
 *	b l r _ p r i n t _ j o i n
 *
 **************************************
 *
 * Functional description
 *	Print a join type.
 *
 **************************************/
	const TEXT *string;
	const USHORT join_type = BLR_BYTE;

	switch (join_type) 
		{
		case blr_inner:
			string = "inner";
			break;

		case blr_left:
			string = "left";
			break;

		case blr_right:
			string = "right";
			break;

		case blr_full:
			string = "full";
			break;

		default:
			error("*** invalid join type ***");
			break;
		}

	format("blr_%s, ", string);
}


SLONG BlrPrint::print_line(SSHORT offset)
{
/**************************************
 *
 *	b l r _ p r i n t _ l i n e
 *
 **************************************
 *
 * Functional description
 *	Invoke callback routine to print (or do something with) a line.
 *
 **************************************/

	*ctl_ptr = 0;

	(*ctl_routine)(ctl_user_arg, offset, ctl_buffer);
	ctl_ptr = ctl_buffer;

	return ctl_blr - ctl_blr_start;
}


void BlrPrint::print_verb(SSHORT level)
{
/**************************************
 *
 *	b l r _ p r i n t _ v e r b
 *
 **************************************
 *
 * Functional description
 *	Primary recursive routine to print BLR.
 *
 **************************************/
	SLONG offset = ctl_blr - ctl_blr_start;
	indent(level);
	UCHAR blr_operator = BLR_BYTE;

	if ((SCHAR) blr_operator == (SCHAR) blr_end) 
		{
		format("blr_end, ");
		PRINT_LINE;
		return;
		}

	print_blr (blr_operator);
	level++;
	const UCHAR *ops = blr_table[blr_operator].blr_operators;
	SSHORT n;

	while (*ops)
		switch (*ops++) 
			{
			case op_verb:
				PRINT_VERB;
				break;

			case op_line:
				offset = PRINT_LINE;
				break;

			case op_byte:
				n = PRINT_BYTE;
				break;

			case op_word:
				n = PRINT_WORD;
				break;

			case op_pad:
				PUT_BYTE(' ');
				break;

			case op_dtype:
				n = PRINT_DTYPE;
				break;

			case op_literal:
				while (--n >= 0)
					PRINT_CHAR;
				break;

			case op_join:
				PRINT_JOIN;
				break;

			case op_message:
				while (--n >= 0) 
					{
					indent(level);
					PRINT_DTYPE;
					offset = PRINT_LINE;
					}
				break;

			case op_parameters:
				level++;
				while (--n >= 0)
					PRINT_VERB;
				level--;
				break;

			case op_error_handler:
				while (--n >= 0) 
					{
					indent(level);
					PRINT_COND;
					offset = PRINT_LINE;
					}
				break;

			case op_set_error:
				PRINT_COND;
				break;

			case op_indent:
				indent(level);
				break;

			case op_begin:
				while ((SCHAR) * (ctl_blr) != (SCHAR) blr_end)
					PRINT_VERB;
				break;

			case op_map:
				while (--n >= 0) 
					{
					indent(level);
					PRINT_WORD;
					offset = PRINT_LINE;
					PRINT_VERB;
					}
				break;

			case op_args:
				while (--n >= 0)
					PRINT_VERB;
				break;

			case op_literals:
				while (--n >= 0) 
					{
					indent(level);
					SSHORT n2 = PRINT_BYTE;
					while (--n2 >= 0)
						PRINT_CHAR;
					offset = PRINT_LINE;
					}
				break;

			case op_union:
				while (--n >= 0) 
					{
					PRINT_VERB;
					PRINT_VERB;
					}
				break;

			case op_relation:
				blr_operator = BLR_BYTE;
				print_blr(blr_operator);
				if (blr_operator != blr_relation && blr_operator != blr_rid)
					error("*** blr_relation or blr_rid must be object of blr_lock_relation, %d found ***",
							(int) blr_operator);

				if (blr_operator == blr_relation) 
					{
					n = PRINT_BYTE;
					while (--n >= 0)
						PRINT_CHAR;
					}
				else
					PRINT_WORD;
				break;
			
			case op_stall:
				if (BLR_PEEK == blr_stall)
					PRINT_VERB;
				break;
				
			case op_exec_into: 
				{
				PRINT_VERB;
				if (PRINT_BYTE)
					PRINT_VERB;
				while (--n >= 0)
					PRINT_VERB;
				break;
				}

			default:
				fb_assert(FALSE);
				break;
			}
}


int BlrPrint::print_word()
{
/**************************************
 *
 *	b l r _ p r i n t _ w o r d
 *
 **************************************
 *
 * Functional description
 *	Print a VAX word as a numeric value an return same.
 *
 **************************************/
	const UCHAR v1 = BLR_BYTE;
	const UCHAR v2 = BLR_BYTE;
	format((ctl_language) ? "chr(%d),chr(%d), " : "%d,%d, ",
			   (int) v1, (int) v2);

	return (v2 << 8) | v1;
}

void BlrPrint::error(const TEXT* string, ...)
{
/**************************************
 *
 *	b l r _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	Report back an error message and unwind.
 *
 **************************************/
	USHORT offset;
	va_list args;

	VA_START(args, string);
	format(string, args);
	offset = 0;
	PRINT_LINE;
	//firebird::status_exception::raise(-1);
	throw OSRIException(isc_random, isc_arg_string, "error during blr print", 0);
}

void BlrPrint::print(void)
{
	const SSHORT version = BLR_BYTE;

	if ((version != blr_version4) && (version != blr_version5))
		error("*** blr version %d is not supported ***",
					(int) version);

	format((version == blr_version4) ? "blr_version4," : "blr_version5,");

	SSHORT level = 0;
	SLONG offset = 0;
	PRINT_LINE;
	PRINT_VERB;

	offset = ctl_blr - ctl_blr_start;
	const SCHAR eoc = BLR_BYTE;

	if (eoc != blr_eoc)
		error("*** expected end of command, encounted %d ***",
					(int) eoc);

	format("blr_eoc");
	PRINT_LINE;
}

void BlrPrint::defaultPrint(void* arg, SSHORT offset, const TEXT* line)
{
	printf("%4d %s\n", offset, line);
}

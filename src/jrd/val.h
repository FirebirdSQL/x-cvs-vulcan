/*
 *	PROGRAM:	JRD access method
 *	MODULE:		val.h
 *	DESCRIPTION:	Definitions associated with value handling
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
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#ifndef JRD_VAL_H
#define JRD_VAL_H

#include "../jrd/jrd_blks.h"
#include "../include/fb_blk.h"
#include "../include/fb_vector.h"

#include "../jrd/dsc.h"

class Transaction;

#define FLAG_BYTES(n)	(((n + BITS_PER_LONG) & ~((ULONG)BITS_PER_LONG - 1)) >> 3)

const UCHAR DEFAULT_DOUBLE	= dtype_double;

#ifdef VMS
const UCHAR SPECIAL_DOUBLE	= dtype_d_float;
#define CNVT_TO_DFLT(x)	MTH$CVT_D_G (x)
#define CNVT_FROM_DFLT(x)	MTH$CVT_G_D (x)

#endif

#ifndef REQUESTER
class Format : public pool_alloc<type_fmt>
{
public:
	Format(MemoryPool& p, int len) : fmt_desc(len, p, type_fmt)
		{
		}
		
	static Format* newFmt(MemoryPool& p, int len = 0)
		{ return FB_NEW(p) Format(p, len); }

	USHORT fmt_length;
	USHORT fmt_count;
	USHORT fmt_version;
	firebird::vector<dsc> fmt_desc;
	typedef firebird::vector<dsc>::iterator fmt_desc_iterator;
	typedef firebird::vector<dsc>::const_iterator fmt_desc_const_iterator;
};
//typedef fmt* FMT;
#endif /* REQUESTER */

#define MAX_FORMAT_SIZE		65535

typedef vary VARY;

/* A macro to define a local vary stack variable of a given length
   Usage:  VARY_STR(5)	my_var;        */

#define VARY_STR(x)	struct { USHORT vary_length; UCHAR vary_string [x]; }

#ifndef REQUESTER
/* Function definition block */

// Parameter passing mechanism. Also used for returning values, except for scalar_array.
enum FUN_T {
		FUN_value,
		FUN_reference,
		FUN_descriptor,
		FUN_blob_struct,
		FUN_scalar_array,
		FUN_ref_with_null
};

struct fun_repeat {
	DSC fun_desc;			/* Datatype info */
	FUN_T fun_mechanism;	/* Passing mechanism */
};

class Symbol;

class UserFunction : public pool_alloc_rpt<fun_repeat, type_fun>
{
    public:
	JString		fun_exception_message;	/* message containing the exception error message */
	UserFunction* fun_homonym;	/* Homonym functions */
	Symbol*		fun_symbol;		/* Symbol block */
	int			(*fun_entrypoint) ();	/* Function entrypoint */
	USHORT		fun_count;			/* Number of arguments (including return) */
	USHORT		fun_args;			/* Number of input arguments */
	USHORT		fun_return_arg;		/* Return argument */
	USHORT		fun_type;			/* Type of function */
	ULONG		fun_temp_length;		/* Temporary space required */
    fun_repeat fun_rpt[1];
};

// Those two defines seems an intention to do something that wasn't completed.
// UDfs that return values like now or boolean Udfs. See rdb$functions.rdb$function_type.
//#define FUN_value	0
//#define FUN_boolean	1

/* Blob passing structure */
// CVC: Moved to fun.epp where it belongs.

/* Scalar array descriptor */

struct scalar_array_desc {
	DSC sad_desc;
	SLONG sad_dimensions;
	struct sad_repeat {
		SLONG sad_lower;
		SLONG sad_upper;
	} sad_rpt[1];
};
#endif /* REQUESTER */



#define ADS_VERSION_1	1
#ifdef __cplusplus
#define ADS_LEN(count)	(sizeof (struct ads) + (count - 1) * sizeof (struct ads::ads_repeat))
#else
#define ADS_LEN(count)	(sizeof (struct ads) + (count - 1) * sizeof (struct ads_repeat))
#endif

#ifndef REQUESTER
class ArrayField : public pool_alloc_rpt<ads::ads_repeat, type_arr>
{
    public:
	UCHAR*			arr_data;				/* Data block, if allocated */
	class blb*		arr_blob;				/* Blob for data access */
	Transaction*	arr_transaction;		/* Parent transaction block */
	class ArrayField* arr_next;			/* Next array in transaction */
	Request*		arr_request;			/* request */
	SLONG			arr_effective_length;	/* Length of array instance */
	USHORT			arr_desc_length;		/* Length of array descriptor */
	SLONG			temporaryId;			/* Temporary array id */
	struct ads		arr_desc;				/* Array descriptor */
};

#endif /* REQUESTER */


#endif /* JRD_VAL_H */


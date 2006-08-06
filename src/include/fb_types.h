/* The contents of this file are subject to the Interbase Public
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
 *
 *
 *   Contributor(s):
 *       Mike Nordel <tamlin@algonet.se>
 *       Mark O'Donohue <mark.odonohue@ludwig.edu.au>
 *
 *  This file, fb_types.h was created from ibase.h
 *
 *
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "OS/2" port
 *
 */


#ifndef INCLUDE_FB_TYPES_H
#define INCLUDE_FB_TYPES_H

#include <stddef.h>

#if defined(WIN32) || defined(_WIN32)

#if !defined(_INTPTR_T_DEFINED)
typedef long intptr_t;
typedef unsigned long uintptr_t;
#endif

#else
#if !defined(SSA_OS_MVS) || defined(MVS_VULCAN) /* MVS headers conflict with system headers */
#include <inttypes.h>
#endif

#endif

/******************************************************************/
/* Define type, export and other stuff based on c/c++ and Windows */
/******************************************************************/
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
	#define  ISC_EXPORT	__stdcall
	#define  ISC_EXPORT_VARARG	__cdecl
#else
	#define  ISC_EXPORT
	#define  ISC_EXPORT_VARARG
#endif

/*******************************************************************/
/* 64 bit Integers                                                 */
/*******************************************************************/

#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__)) && !defined(__GNUC__)
#ifndef _BASETSD_H_ /* avoid redefinition from windows headers */
typedef int					INT32;
typedef unsigned int		UINT32;
typedef __int64				INT64;
#endif
typedef __int64				ISC_INT64;
typedef unsigned __int64	ISC_UINT64;
#else
typedef int						INT32;
typedef unsigned int			UINT32;
typedef long long int			INT64;
//typedef unsigned long long int	UINT64; /* conflicts on H6I */
typedef uint64_t				UINT64;
typedef long long int			ISC_INT64;
typedef unsigned long long int	ISC_UINT64;
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
/* too bad the Windows header files define SLONG and ULONG like this */
typedef long SLONG;
typedef unsigned long ULONG;
#else
typedef INT32 SLONG;
typedef UINT32 ULONG;
#endif

#if SIZEOF_LONG == 8
// EKU: Firebird requires (S)LONG to be 32 bit
typedef INT64 SQUAD;
typedef UINT64 UQUAD;
#define NATIVE_QUAD
typedef unsigned int FB_API_HANDLE;
#else
typedef struct {
	SLONG high;
	ULONG low;
} SQUAD;
typedef unsigned int FB_API_HANDLE;
#endif // SIZEOF_LONG == 8



struct GDS_QUAD_t {
	INT32 gds_quad_high;
	UINT32 gds_quad_low;
};

typedef struct GDS_QUAD_t GDS_QUAD;
typedef struct GDS_QUAD_t ISC_QUAD;

#define	isc_quad_high	gds_quad_high
#define	isc_quad_low	gds_quad_low

// Basic data types

// typedef signed char SCHAR;
// TMN: TODO It seems SCHAR is used just about *everywhere* where a plain
// "char" is really intended. This currently forces us to this bad definition.
//
typedef char SCHAR;

typedef unsigned char UCHAR;
typedef short SSHORT;
typedef unsigned short USHORT;

// Substitution of API data types 

typedef SCHAR ISC_SCHAR;
typedef UCHAR ISC_UCHAR;
typedef SSHORT ISC_SHORT;
typedef USHORT ISC_USHORT;
typedef SLONG ISC_LONG;
typedef ULONG ISC_ULONG;

//
// TMN: some misc data types from all over the place
//
struct vary
{
	USHORT vary_length;
	char   vary_string[1];
};
// TMN: Currently we can't do this, since remote uses a different
// definition of VARY than the rest of the code! :-<
//typedef vary* VARY;

struct lstring
{
	ULONG	lstr_length;
	ULONG	lstr_allocated;
	UCHAR*	lstr_address;
};
typedef struct lstring LSTRING;


typedef unsigned char BOOLEAN;
typedef char TEXT;				/* To be expunged over time */
/*typedef unsigned char STEXT;	Signed text - not used
typedef unsigned char UTEXT;	Unsigned text - not used */
typedef unsigned char BYTE;		/* Unsigned byte - common */
/*typedef char SBYTE;			Signed byte - not used */
typedef intptr_t ISC_STATUS;
typedef intptr_t IPTR;
typedef uintptr_t U_IPTR;
typedef void (*FPTR_VOID) ();
typedef void (*FPTR_VOID_PTR) (void*);
typedef int (*FPTR_INT) ();
typedef int (*FPTR_INT_VOID_PTR) (void*);

#ifdef _WIN32
#define INTERLOCK_TYPE	long
#endif

#ifndef INTERLOCK_TYPE
#define INTERLOCK_TYPE	int
#endif

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*FPTR_PRINT_CALLBACK) (void*, SSHORT, const char*);

#ifdef __cplusplus
}
#endif


/* Used for isc_version */
typedef void (*FPTR_VERSION_CALLBACK)(void*, const char*);
/* Used for isc_que_events and internal functions */
typedef void (*FPTR_EVENT_CALLBACK)(void*, USHORT, const UCHAR*);

// The type of JRD's ERR_post, DSQL's ERRD_post & post_error,
// REMOTE's move_error & GPRE's post_error.
typedef void (*FPTR_ERROR) (ISC_STATUS, ...);

typedef ULONG RCRD_OFFSET;
typedef USHORT FLD_LENGTH;
typedef int (*lock_ast_t)(void*);

#define ISC_STATUS_LENGTH	20
typedef ISC_STATUS ISC_STATUS_ARRAY[ISC_STATUS_LENGTH];

/* Number of elements in an array */
#define FB_NELEM(x)	((int)(sizeof(x) / sizeof(x[0])))
#define FB_ALIGN(n, b) ((n + b - 1) & ~(b - 1))

#endif /* INCLUDE_FB_TYPES_H */


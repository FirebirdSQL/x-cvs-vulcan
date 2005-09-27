/* $Id$ */
/*
 *      PROGRAM:        JRD access method
 *      MODULE:         common.h
 *      DESCRIPTION:    Common descriptions for all GDS programs
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
 * Added TCP_NO_DELAY option for superserver on Linux
 * FSG 16.03.2001
 *
 * 2001.07.06 Sean Leyne - Code Cleanup, removed "#ifdef READONLY_DATABASE"
 *                         conditionals, as the engine now fully supports
 *                         readonly databases.
 *
 * 2001.11.20  Ann Harrison - make 64bitio.h conditional on not windows.
 *
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete ports:
 *                          - MAC ("MAC", "MAC_AUX" and "MAC_CP" defines)
 *                          - EPSON, XENIX, DELTA, IMP, NCR3000, M88K
 *                          - NT Power PC and HP9000 s300
 *
 * 2002.04.16  Paul Beach - HP10 and unistd.h
 *
 * 2002.10.27 Sean Leyne - Completed removal of obsolete "DG_X86" port
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "UNIXWARE" port
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "Ultrix" port
 *
 * 2002.10.28 Sean Leyne - Completed removal of obsolete "DGUX" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "DecOSF" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "SGI" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */
/*
$Id$
*/

#ifndef JRD_COMMON_H
#define JRD_COMMON_H

//#include "fbdev.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifndef INCLUDE_FB_MACROS_H
#include "../include/fb_macros.h"
#endif

#ifndef INCLUDE_FB_TYPES_H
#include "../include/fb_types.h"
#endif


/*
  do not use links in source code to maintain platform neutraility
*/

#ifdef PROD_BUILD
#ifdef DEV_BUILD
#undef DEV_BUILD
#endif
#endif

#ifdef SUPERSERVER
#define GOVERNOR 1
#define CANCEL_OPERATION
#define FB_ARCHITECTURE isc_info_db_class_server_access
#else
#define FB_ARCHITECTURE isc_info_db_class_classic_access
#endif


/*****************************************************
* Linux for Intel platforms 
*****************************************************/
#ifdef LINUX
#include "linux.h"
#endif /* LINUX */

/*****************************************************
* SINIX-Z 5.42
*****************************************************/
#ifdef SINIXZ
#include "sinixz.h"
#endif /* SINIXZ */

/*****************************************************
* Darwin Platforms 
*****************************************************/
#ifdef DARWIN
#include "darwin.h"
#endif /* Darwin Platforms */


/*****************************************************
* FreeBSD for Intel platforms 
*****************************************************/
#ifdef FREEBSD
#include "freebsd.h"
#endif /* FREEBSD */

/*****************************************************
* NetBSD 
*****************************************************/
#ifdef NETBSD
#include "netbsd.h"
#endif /* NETBSD */



/*****************************************************
 * SUN platforms--the 386i is obsolete 
*****************************************************/

#ifdef sun
#include "sun.h"
#endif /* sun */


/*****************************************************
* HP/UX platforms 
*****************************************************/

#if (defined(hpux) || defined(HPUX))
#include "hpux.h"
#endif /* hpux */


/*****************************************************
* DEC VAX/VMS and AlphaVMS 
*****************************************************/
#ifdef __VMS
#include "jrd_vms.h"
#endif /* VMS */



/*****************************************************
* IBM AIX RS/6000 and IBM AIX PowerPC 
*****************************************************/

#ifdef _AIX						/* IBM AIX */
#include "aix.h"
#endif /* IBM AIX */

/*****************************************************
* IBM MVS
*****************************************************/

#ifdef MVS						/* IBM MVS */
#include "jrd_mvs.h"
#endif /* IBM MVS */


/*****************************************************
* Windows NT 
*****************************************************/

#ifdef WIN_NT
#include "win32.h"
#endif /* WIN_NT */

// 23 Sep 2002, skidder, ALLOC_LIB_MEMORY moved here,
// DEBUG_GDS_ALLOC definition removed because allocators 
// do not (and can not) include this file,
// but use DEBUG_GDS_ALLOC. Hence DEBUG_GDS_ALLOC should be defined
// globally by now and moved to autoconf-generated header later
#ifdef DEBUG_GDS_ALLOC
#define ALLOC_LIB_MEMORY(size)   gds__alloc_debug ((size),(TEXT *)__FILE__,(ULONG)__LINE__)
#endif

/*****************************************************
* SCO 
*****************************************************/
#ifdef SCO_EV
#include "sco.h"
#endif /* SCO_EV */

/*****************************************************
 * UNIX
*****************************************************/
#ifdef UNIX
#define NO_CHECKSUM     1
#define SYS_ARG		isc_arg_unix
#endif /* UNIX */



/* Turn off NFS checking everywhere for now */
#undef NO_NFS
#define NO_NFS


/* various declaration modifiers */

#ifndef API_ROUTINE
#define API_ROUTINE
#define API_ROUTINE_VARARG
#define INTERNAL_API_ROUTINE	API_ROUTINE
#endif

#ifndef CLIB_ROUTINE
#define CLIB_ROUTINE
#endif

#ifndef THREAD_ROUTINE
#define THREAD_ROUTINE
#endif



/* alignment macros */

#ifndef OLD_ALIGNMENT
#ifdef I386
/* Using internal alignment optimal for 386 processor and above
 */
/*#define ALIGNMENT       4*/
/*#define DOUBLE_ALIGN    8*/
#endif
#endif

#ifndef ALIGNMENT
/*#define ALIGNMENT       2*/
#error must define ALIGNMENT for your system
#endif

#ifndef SHIFTLONG
/* Number of shifts needed to convert between char and LONG */
#define SHIFTLONG       2
#define BITS_PER_LONG   32
#define LOG2_BITS_PER_LONG      5
#endif

#ifndef DOUBLE_ALIGN
/*#define DOUBLE_ALIGN    4*/
#error must define DOUBLE_ALIGN for your system
#endif



/* common return values */

#ifndef FINI_OK
#define FINI_OK         0
#define FINI_ERROR      1
#define STARTUP_ERROR   2		/* this is also used in iscguard.h, make sure these match */
#endif

#ifndef TRUE
#define TRUE            1
#endif
#ifndef FALSE
#define FALSE           0
#endif
#define FB_SUCCESS         0
#define FB_FAILURE         1

/* sys/paramh.h : compatibility purposes */
#ifndef NOFILE
#ifdef __VMS
#define NOFILE      32
#else
#define NOFILE      20
#endif
#endif

/* data type definitions */


#ifndef INT64_DEFINED			/* 64 bit */
typedef long long int SINT64;
// typedef unsigned long long int UINT64; /* already defined in fb_types.h */
#else
#undef INT64_DEFINED
#endif


#ifndef ATOM_DEFINED			/* 32 or 64 bit */
typedef long SATOM;
typedef unsigned long UATOM;
#else
#undef ATOM_DEFINED
#endif

#ifndef ISC_TIMESTAMP_DEFINED
typedef SLONG ISC_DATE;
typedef ULONG ISC_TIME;
typedef struct
{
	ISC_DATE timestamp_date;
	ISC_TIME timestamp_time;
} ISC_TIMESTAMP;
#define ISC_TIMESTAMP_DEFINED
#endif	/* ISC_TIMESTAMP_DEFINED */

#define GDS_DATE	ISC_DATE
#define GDS_TIME	ISC_TIME
#define GDS_TIMESTAMP	ISC_TIMESTAMP


#ifndef BLOB_PTR
#define BLOB_PTR        UCHAR
#endif


#ifndef SLONGFORMAT
#define SLONGFORMAT	"d"
#define ULONGFORMAT	"u"
#define XLONGFORMAT "X"
#define xLONGFORMAT "x"
#endif

//format for __LINE__
#ifndef LINEFORMAT
#define LINEFORMAT "ld"
#endif

/* variable argument definitions */

#define VA_START(list,parmN)    va_start (list, parmN)

/* conditional compilation macros */



/* MAX and MIN for datatypes */

#define MAX_UCHAR		((UCHAR)0xFF)
#define MIN_UCHAR		0x00

#define MAX_SCHAR		0x7F
#define MIN_SCHAR		(-MAX_SCHAR-1)

#define MAX_USHORT		((USHORT)0xFFFF)
#define MIN_USHORT		0x0000

#define MAX_SSHORT		0x7FFF
#define MIN_SSHORT		(-MAX_SSHORT-1)

#define MAX_ULONG		((ULONG)0xFFFFFFFF)
#define MIN_ULONG		0x00000000

#define MAX_SLONG		0x7FFFFFFF
#define MIN_SLONG		(-MAX_SLONG-1)

#define MAX_UINT64              ((UINT64) QUADCONST(0xFFFFFFFFFFFFFFFF))
#define MIN_UINT64              QUADCONST(0x0000000000000000)

#define MAX_SINT64              QUADCONST(0x7FFFFFFFFFFFFFFF)
#define MIN_SINT64              (-MAX_SINT64-1)



/* commonly used macros */

#ifndef MAX
#define MAX(a,b)                (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b)                (((a) < (b)) ? (a) : (b))
#endif

#define UPPER(c)                (((c) >= 'a' && (c)<= 'z') ? (c) - 'a' + 'A' : (c))
#define LOWWER(c)               (((c) >= 'A' && (c)<= 'Z') ? (c) - 'A' + 'a' : (c))

/* UPPER7 & LOWWER7 are to be used when the data is known to
   be 7-bit ASCII, eg: internal data, OS data.  NOT user data.  */

#define UPPER7(c)               (((c) >= 'a' && (c)<= 'z') ? (c) - 'a' + 'A' : (c))
#define LOWWER7(c)              (((c) >= 'A' && (c)<= 'Z') ? (c) - 'A' + 'a' : (c))

#ifndef ROUNDUP
#define ROUNDUP(n,b)            (((n) + (b) - 1) & ~((b) - 1))
#endif
#define ROUNDUP_LONG(len)       ROUNDUP (len, sizeof (SLONG))

#define JRD_BUGCHK 15			/* facility code for bugcheck messages */
#ifndef OFFSET
#define OFFSET(struct,fld)      ((IPTR) &((struct) NULL)->fld)
#define OFFSETA(struct,fld)     ((IPTR) ((struct) NULL)->fld)
#endif

#ifndef ODS_ALIGNMENT
#define ODS_ALIGNMENT           4
#endif

#ifndef FORMAT_ALIGNMENT
// Alignment for items in record format. Used for databases after ODS11.
// Always 64-bit to ensure ODS compatibility with 64-bit versions of the engine
#define FORMAT_ALIGNMENT           8
#endif

#ifndef SYSCALL_INTERRUPTED
#define SYSCALL_INTERRUPTED(err)        ((err) == EINTR)
#endif



/* data conversion macros */

#ifndef AOF32L
#define AOF32L(l)               &l
#endif



/* data movement and allocation macros */

#ifndef MOVE_FAST
#define MOVE_FAST(from,to,length)       MOV_fast (from, to, (ULONG) (length))
#endif

#ifndef MOVE_FASTER
#define MOVE_FASTER(from,to,length)     MOV_faster (from, to, (ULONG) (length))
#endif

#ifndef MEMMOVE
/* Use character by character copy function */
#define MEMMOVE(from,to,length)       MOV_fast (from, to, (ULONG) (length))
#endif

#ifndef MOVE_CLEAR
#define MOVE_CLEAR(to,length)           MOV_fill (to, (ULONG) (length))
#endif

#ifndef ALLOC_LIB_MEMORY
#define ALLOC_LIB_MEMORY(size)          gds__alloc (size)
#endif

#ifndef FREE_LIB_MEMORY
#define FREE_LIB_MEMORY(block)          gds__free (block)
#endif

// This macros are used to workaround shortage of standard conformance
// in Microsoft compilers. They could be replaced with normal procedure
// and generic macro if MSVC would support C99-style __VA_ARGS__
#define DEFINE_TRACE_ROUTINE(routine) void routine(const char* message, ...)

#ifdef HAVE_VSNPRINTF
#define VSNPRINTF(a,b,c,d) vsnprintf(a,b,c,d)
#else
#define VSNPRINTF(a,b,c,d) vsprintf(a,c,d)
#endif

#define IMPLEMENT_TRACE_ROUTINE(routine, subsystem) \
void routine(const char* message, ...) { \
	static const char name_facility[] = subsystem ","; \
	char buffer[1000]; \
	strcpy(buffer, name_facility); \
	char *ptr = buffer + sizeof(name_facility) - 1; \
	va_list params; \
	va_start(params, message); \
	VSNPRINTF(ptr, sizeof(buffer) - sizeof(name_facility), message, params); \
	va_end(params); \
	gds__trace(buffer); \
}

#ifdef DEV_BUILD

/* Define any debugging symbols and macros here.  This
   ifdef will be executed during development builds. */

#define TRACE(msg)				gds__trace (msg)

#ifndef DEV_REPORT
#define DEV_REPORT(msg)         gds__log (msg)
#endif

#ifndef BREAKPOINT
#define BREAKPOINT(x)           GDS_breakpoint(x)
// fwd. decl. the function itself
#ifdef __cplusplus
extern "C" {
#endif
void GDS_breakpoint(int);
#ifdef __cplusplus
} /* extern "C" */
#endif
#endif

#endif /* DEV_BUILD */

#ifndef DEV_BUILD
#ifndef DEV_REPORT
#define DEV_REPORT(msg)         gds__log (msg)
#endif
#ifndef BREAKPOINT
#define BREAKPOINT(x)			/* nothing */
#endif
#ifndef TRACE
#define TRACE(msg)				/* nothing */
#endif
#endif


/* commonly used buffer sizes for dynamic buffer allocation */

#define BUFFER_XLARGE   2048
#define BUFFER_LARGE    1024
#define BUFFER_MEDIUM   512
#define BUFFER_SMALL    256
#define BUFFER_TINY     128

/* The default lseek offset type.  Changed from nothing to (off_t) to correctly support 64 bit IO */
#ifndef LSEEK_OFFSET_CAST
#define LSEEK_OFFSET_CAST (off_t)
#endif

#ifndef DOUBLE_MULTIPLY
#define DOUBLE_MULTIPLY(a,b)    (((double) (a)) * ((double) (b)))
#endif

#ifndef DOUBLE_DIVIDE
#define DOUBLE_DIVIDE(a,b)      (((double) (a)) / ((double) (b)))
#endif

#ifndef ISC_EXT_LIB_PATH_ENV
#define ISC_EXT_LIB_PATH_ENV	"INTERBASE_EXT_LIB_PATH"
#endif

/* switch name and state table.  This structure should be used in all
 * command line tools to facilitate parsing options.*/
typedef struct in_sw_tab_t {
	int 		in_sw;
	int 		in_spb_sw;
	const TEXT *in_sw_name;
	ULONG 		in_sw_value;			/* alice specific field */
	ULONG 		in_sw_requires;		/* alice specific field */
	ULONG 		in_sw_incompatibilities;	/* alice specific field */
	USHORT 		in_sw_state;
	USHORT 		in_sw_msg;
	USHORT 		in_sw_min_length;
	const TEXT *in_sw_text;

} *IN_SW_TAB;

#ifndef HAVE_WORKING_VFORK
#define vfork fork
#endif

static const TEXT FB_SHORT_MONTHS[][4] =
{
	"Jan", "Feb", "Mar",
	"Apr", "May", "Jun",
	"Jul", "Aug", "Sep",
	"Oct", "Nov", "Dec",
	"\0"
};

static const TEXT* FB_LONG_MONTHS_UPPER[] =
{
	"JANUARY",
	"FEBRUARY",
	"MARCH",
	"APRIL",
	"MAY",
	"JUNE",
	"JULY",
	"AUGUST",
	"SEPTEMBER",
	"OCTOBER",
	"NOVEMBER",
	"DECEMBER",
	0
};

enum LockType {
	None,
    Exclusive,
	Shared,
	Invalid
	};

#ifdef MEMMGR
//#include "MemMgr.h"
#include "MemoryManager.h"
#endif



#endif /* JRD_COMMON_H */

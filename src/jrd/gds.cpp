/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		gds.cpp
 *	DESCRIPTION:	User callable routines
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
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "IMP" port
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "M88K" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 * 2003.05.11 Nickolay Samofatov - rework temp stuff
 *
 */

#define IO_RETRY	20

// 11 Sept 2002 Nickolay Samofatov
// this defined in included dsc.h
//#define ISC_TIME_SECONDS_PRECISION		10000L
//#define ISC_TIME_SECONDS_PRECISION_SCALE	-4

#include "fbdev.h"
#include "../jrd/common.h"
#include "ibase.h"
#include "../jrd/ib_stdio.h"
#include <stdlib.h>
#include <string.h>
#include "../jrd/common.h"
#include "../jrd/gdsassert.h"
#include "../jrd/file_params.h"
#include "../jrd/msg_encode.h"
#include "../jrd/iberr.h"
#include "../jrd/gds_proto.h"
#include "BlrPrint.h"
#include "StatusPrint.h"
#include "MsgFile.h"
#include "Mutex.h"
#include "Sync.h"
#include "MemMgr.h"

//#include "../common/classes/alloc.h"

#include "../jrd/misc_proto.h"
#include "../jrd/constants.h"

//#include "../common/classes/locks.h"
#include "Configuration.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include <errno.h>

#include <stdarg.h>
#include "../jrd/jrd_time.h"
#include "../jrd/misc.h"

#if defined(WIN_NT)
#include <windows.h>
#include <io.h> // umask, close, lseek, read, open, _sopen
#include <process.h>
#include <sys/types.h>
#include <sys/timeb.h>
#endif

#ifdef __VMS
#include <fcntl.h>

#else /* !VMS */

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#ifndef O_RDWR
#include <fcntl.h>
#endif

#ifdef UNIX
#if (defined SUPERSERVER) || (defined SOLARIS)
#include <sys/mman.h>
#include <sys/resource.h>
#include "err_proto.h"
#endif
#endif

#endif /* VMS */



#ifdef SUPERSERVER
static const TEXT gdslogid[] = " (Server)";
#else
#ifdef SUPERCLIENT
static const TEXT gdslogid[] = " (Client)";
#else
static const TEXT gdslogid[] = "";
#endif
#endif

static const char * FB_PID_FILE = "fb_%d";

#include "gen/sql_code.h"
#include "gen/sql_state.h"
#include "../jrd/thd.h"
//#include "../jrd/gdsold.h"
#include "../jrd/msg.h"
#include "../jrd/fil.h"
#include "../jrd/gds_proto.h"
#include "../jrd/isc_proto.h"
#ifndef REQUESTER
#include "../jrd/os/isc_i_proto.h"
#endif

#ifdef WIN_NT
#define _WINSOCKAPI_
#define DIR_SEPARATOR		'\\'
#include <share.h>
#include "thd_proto.h"
#include "err_proto.h"
#undef leave
#endif /* WIN_NT */

#ifndef DIR_SEPARATOR
#define DIR_SEPARATOR		'/'
#endif

// Number of times to try to generate new name for temporary file
#define MAX_TMPFILE_TRIES 256

static const char *ib_prefix = 0;
static char *ib_prefix_lock = 0;
static char *ib_prefix_msg = 0;

//#include "gen/msgs.h"

#ifndef FOPEN_APPEND_TYPE
#define FOPEN_APPEND_TYPE	"a"
#endif

#ifndef O_BINARY
#define O_BINARY		0
#endif

#ifndef GENERIC_SQLCODE
#define GENERIC_SQLCODE		-999
#endif

#ifndef GENERIC_SQLSTATE
#define GENERIC_SQLSTATE	"HY000"
#endif

static char ib_prefix_val[MAXPATHLEN];
static char ib_prefix_lock_val[MAXPATHLEN];
static char ib_prefix_msg_val[MAXPATHLEN];


#ifndef INCLUDE_FB_TYPES_H
#include "../include/fb_types.h"
#endif



#ifdef DEV_BUILD
void GDS_breakpoint(int);
#endif


static void		init(void);
static int		yday(const tm*);

static void		ndate(SLONG, tm*);
static GDS_DATE	nday(const tm*);
static void		sanitize(TEXT*);

static void		safe_concat_path(TEXT* destbuf, const TEXT* srcbuf);

/* Generic cleanup handlers */

typedef struct clean
{
	struct clean*	clean_next;
	void			(*clean_routine)(void*);
	void*			clean_arg;
} *CLEAN;

static CLEAN		cleanup_handlers = NULL;
static MsgFile		*defaultMsgFile;
static bool			initialized = false;
static SyncObject	syncObject;

#ifdef DEBUG_GDS_ALLOC
void* API_ROUTINE gds__alloc_debug(SLONG size_request,
                                   const TEXT* filename,
                                   ULONG lineno)
{
	//return getDefaultMemoryPool()->allocate(size_request, 0, filename, lineno);
	return new UCHAR [size_request];
}
#else
void * API_ROUTINE gds__alloc_debug (SLONG size_request,
									 const TEXT* filename,
									 ULONG lineno)
{
	return new UCHAR [size_request];
}
void* API_ROUTINE gds__alloc(SLONG size_request)
{
	//return getDefaultMemoryPool()->allocate(size_request);
	return new UCHAR [size_request];
}
#endif

ULONG API_ROUTINE gds__free(void* blk) 
{
	//getDefaultMemoryPool()->deallocate(blk);
	delete [] (UCHAR*) blk;
	return 0;
}

#ifdef UNIX
static SLONG gds_pid = 0;
#endif

/* VMS structure to declare exit handler */

#ifdef VMS
static SLONG exit_status = 0;
static struct
{
	SLONG link;
	int (*exit_handler) ();
	SLONG args;
	SLONG arg[1];
} exit_description;
#endif



#define FB_ENV			"FIREBIRD"
#define FB_LOCK_ENV		"FIREBIRD_LOCK"
#define FB_MSG_ENV		"FIREBIRD_MSG"
#define FB_TMP_ENV		"FIREBIRD_TMP"

#ifdef WIN_NT
#define EXPAND_PATH(relative, absolute)		_fullpath(absolute, relative, MAXPATHLEN)
#define COMPARE_PATH(a,b)			_stricmp(a,b)
#else
#ifdef __VMS
#define EXPAND_PATH(relative, absolute)		strcpy(absolute,relative)
#else
#define EXPAND_PATH(relative, absolute)		realpath(relative, absolute)
#endif
#define COMPARE_PATH(a,b)			strcmp(a,b)
#endif


// This function is very crude, but signal-safe.
// CVC: This function converts and ULONG into a string. I renamed the param
// maxlen to minlen because it represents the minimum length the number will
// be printed in. However, this means buffer should be large enough to contain
// any ULONG (11 bytes) and thus prevent a buffer overflow. If minlen >= 11,
// then buffer should have be of size = (minlen + 1). A small anomaly means
// that when "value" is zero, only filler is stored in buffer: no ACSCII(48)
// appears in buffer, unless filler itself is ASCII(48) == '0'.
void gds__ulstr(char* buffer, ULONG value, const int minlen, const char filler)
{
	ULONG n = value;
	int c = 0;
	while (n) {
		n = n / 10;
		c++;
	}
	if (minlen > c)
		c = minlen;
	char* p = buffer + c;
	while (value) {
		*--p = '0' + (value % 10);
		value = value / 10;
	}
	while (p != buffer) {
		*--p = filler;
	}
	buffer[c] = 0;
}


ISC_STATUS API_ROUTINE gds__decode(ISC_STATUS code, USHORT* fac, USHORT* class_)
{
/**************************************
 *
 *	g d s _ $ d e c o d e
 *
 **************************************
 *
 * Functional description
 *	Translate a status codes from system dependent form to
 *	network form.
 *
 **************************************/

	if (!code)
		return FB_SUCCESS;

	if ((code & ISC_MASK) != ISC_MASK)
		return code;

	*fac = GET_FACILITY(code);
	*class_ = GET_CLASS(code);
	
	return GET_CODE(code);
}


void API_ROUTINE isc_decode_date(const ISC_QUAD* date, void* times_arg)
{
/**************************************
 *
 *	i s c _ d e c o d e _ d a t e
 *
 **************************************
 *
 * Functional description
 *	Convert from internal timestamp format to UNIX time structure.
 *
 *	Note: this API is historical - the prefered entrypoint is
 *	isc_decode_timestamp
 *
 **************************************/
	isc_decode_timestamp((const GDS_TIMESTAMP*) date, times_arg);
}


void API_ROUTINE isc_decode_sql_date(const GDS_DATE* date, void* times_arg)
{
/**************************************
 *
 *	i s c _ d e c o d e _ s q l _ d a t e
 *
 **************************************
 *
 * Functional description
 *	Convert from internal DATE format to UNIX time structure.
 *
 **************************************/
	tm* times = (struct tm*) times_arg;
	memset(times, 0, sizeof(*times));

	ndate(*date, times);
	times->tm_yday = yday(times);
	if ((times->tm_wday = (*date + 3) % 7) < 0)
		times->tm_wday += 7;
}


void API_ROUTINE isc_decode_sql_time(const GDS_TIME* sql_time, void* times_arg)
{
/**************************************
 *
 *	i s c _ d e c o d e _ s q l _ t i m e
 *
 **************************************
 *
 * Functional description
 *	Convert from internal TIME format to UNIX time structure.
 *
 **************************************/
	tm* times = (struct tm*) times_arg;
	memset(times, 0, sizeof(*times));

	const ULONG minutes = *sql_time / (ISC_TIME_SECONDS_PRECISION * 60);
	times->tm_hour = minutes / 60;
	times->tm_min = minutes % 60;
	times->tm_sec = (*sql_time / ISC_TIME_SECONDS_PRECISION) % 60;
}


void API_ROUTINE isc_decode_timestamp(const GDS_TIMESTAMP* date, void* times_arg)
{
/**************************************
 *
 *	i s c _ d e c o d e _ t i m e s t a m p
 *
 **************************************
 *
 * Functional description
 *	Convert from internal timestamp format to UNIX time structure.
 *
 *	Note: the date arguement is really an ISC_TIMESTAMP -- however the
 *	definition of ISC_TIMESTAMP is not available from all the source
 *	modules that need to use isc_encode_timestamp
 *
 **************************************/
	tm* times = (struct tm*) times_arg;
	memset(times, 0, sizeof(*times));

	ndate(date->timestamp_date, times);
	times->tm_yday = yday(times);
	if ((times->tm_wday = (date->timestamp_date + 3) % 7) < 0)
		times->tm_wday += 7;

	const ULONG minutes = date->timestamp_time / (ISC_TIME_SECONDS_PRECISION * 60);
	times->tm_hour = minutes / 60;
	times->tm_min = minutes % 60;
	times->tm_sec = (date->timestamp_time / ISC_TIME_SECONDS_PRECISION) % 60;
}


ISC_STATUS API_ROUTINE gds__encode(ISC_STATUS code, USHORT facility)
{
/**************************************
 *
 *	g d s _ $ e n c o d e
 *
 **************************************
 *
 * Functional description
 *	Translate a status codes from network format to system
 *	dependent form.
 *
 **************************************/

	if (!code)
		return FB_SUCCESS;

	return ENCODE_ISC_MSG(code, facility);
}


void API_ROUTINE isc_encode_date(const void* times_arg, ISC_QUAD* date)
{
/**************************************
 *
 *	i s c _ e n c o d e _ d a t e
 *
 **************************************
 *
 * Functional description
 *	Convert from UNIX time structure to internal timestamp format.
 *
 *	Note: This API is historical -- the prefered entrypoint is
 *	isc_encode_timestamp
 *
 **************************************/
	isc_encode_timestamp(times_arg, (GDS_TIMESTAMP*) date);
}


void API_ROUTINE isc_encode_sql_date(const void* times_arg, GDS_DATE* date)
{
/**************************************
 *
 *	i s c _ e n c o d e _ s q l _ d a t e
 *
 **************************************
 *
 * Functional description
 *	Convert from UNIX time structure to internal date format.
 *
 **************************************/

	*date = nday((const struct tm*) times_arg);
}


void API_ROUTINE isc_encode_sql_time(const void* times_arg, GDS_TIME* isc_time)
{
/**************************************
 *
 *	i s c _ e n c o d e _ s q l _ t i m e
 *
 **************************************
 *
 * Functional description
 *	Convert from UNIX time structure to internal TIME format.
 *
 **************************************/
	const tm* times = (const struct tm*) times_arg;
	*isc_time = ((times->tm_hour * 60 + times->tm_min) * 60 +
				 times->tm_sec) * ISC_TIME_SECONDS_PRECISION;
}


void API_ROUTINE isc_encode_timestamp(const void* times_arg, GDS_TIMESTAMP* date)
{
/**************************************
 *
 *	i s c _ e n c o d e _ t i m e s t a m p
 *
 **************************************
 *
 * Functional description
 *	Convert from UNIX time structure to internal timestamp format.
 *
 *	Note: the date arguement is really an ISC_TIMESTAMP -- however the
 *	definition of ISC_TIMESTAMP is not available from all the source
 *	modules that need to use isc_encode_timestamp
 *
 **************************************/
	int size = sizeof (ISC_TIMESTAMP);
	const tm* times = (const struct tm*) times_arg;

	date->timestamp_date = nday(times);
	date->timestamp_time =
		((times->tm_hour * 60 + times->tm_min) * 60 +
		 times->tm_sec) * ISC_TIME_SECONDS_PRECISION;
}


#ifdef DEV_BUILD

void GDS_breakpoint(int parameter)
{
/**************************************
 *
 *	G D S _ b r e a k p o i n t 
 *
 **************************************
 *
 * Functional description
 *	Just a handy place to put a debugger breakpoint
 *	Calls to this can then be embedded for DEV_BUILD
 *	using the BREAKPOINT(x) macro.
 *
 **************************************/
/* Put a breakpoint here via the debugger */
}
#endif


SINT64 API_ROUTINE isc_portable_integer(const UCHAR* ptr, SSHORT length)
{
/**************************************
 *
 *	i s c _ p o r t a b l e _ i n t e g e r
 *
 **************************************
 *
 * Functional description
 *	Pick up (and convert) a Little Endian (VAX) style integer 
 *      of length 1, 2, 4 or 8 bytes to local system's Endian format.
 *
 *   various parameter blocks (i.e., dpb, tpb, spb) flatten out multibyte
 *   values into little endian byte ordering before network transmission.
 *   programs need to use this function in order to get the correct value
 *   of the integer in the correct Endian format.
 *
 * **Note**
 *
 *   This function is similar to gds__vax_integer() in functionality.
 *   The difference is in the return type. Changed from SLONG to SINT64
 *   Since gds__vax_integer() is a public API routine, it could not be
 *   changed. This function has been made public so gbak can use it.
 *
 **************************************/
	SINT64 value;
	SSHORT shift;

	fb_assert(length <= 8);
	value = shift = 0;

	while (--length >= 0) {
		value += ((SINT64) *ptr++) << shift;
		shift += 8;
	}

	return value;
}

#ifdef DEBUG_GDS_ALLOC

void API_ROUTINE gds_alloc_flag_unfreed(void *blk)
{
/**************************************
 *
 *	g d s _ a l l o c _ f l a g _ u n f r e e d
 *
 **************************************
 *
 * Functional description
 *	Flag a buffer as "known to be unfreed" so we
 *	don't report it in gds_alloc_report
 *
 **************************************/
// JMB: need to rework this for the new pools
// Skidder: Not sure we need to rework this routine. 
// What we really need is to fix all memory leaks including very old.
}
#endif // DEBUG_GDS_ALLOC


#ifdef DEBUG_GDS_ALLOC

void API_ROUTINE gds_alloc_report(ULONG flags, const char* filename, int lineno)
{
/**************************************
 *
 *	g d s _ a l l o c _ r e p o r t 
 *
 **************************************
 *
 * Functional description
 *	Print buffers that might be memory leaks.
 *	Or that might have been clobbered.
 *
 **************************************/
// Skidder: Calls to this function must be replaced with MemoryPool::print_contents
}
#endif // DEBUG_GDS_ALLOC


/* CVC: See comment below. Basically, it provides the needed const correctness,
but throws away the const to make the callee happy, knowing that the callee
indeed treats vector as it was a pointer with the const prefix.
Maybe this function could be private and thus made inline? */

SLONG API_ROUTINE gds_interprete_cpp(char* const s, const ISC_STATUS** vector)
{
/**************************************
 *
 *	g d s _ i n t e r p r e t e _ c p p
 *
 **************************************
 *
 * Functional description
 *	Translate a status code with arguments to a string.  Return the
 *	length of the string while updating the vector address.  If the
 *	message is null (end of messages) or invalid, return 0;
 *
 **************************************/
	return gds__interprete(s, const_cast<ISC_STATUS**>(vector));
}

/* CVC: This non-const signature is needed for compatibility. The reason is
that, unlike int* that can be assigned to const int* transparently to the caller,
int** CANNOT be assigned to const int**, so applications would have to be
fixed to provide such const int**; while it may be more correct from the
semantic POV, we can't estimate how much work it's for app developers.
Therefore, we go back to the old signature and provide gds_interprete_cpp
for compliance, to be used inside the engine, too. September, 2003. */

SLONG API_ROUTINE gds__interprete(char* s, ISC_STATUS** vector)
{
/**************************************
 *
 *	g d s _ $ i n t e r p r e t e
 *
 **************************************
 *
 * Functional description
 *	Translate a status code with arguments to a string.  Return the
 *	length of the string while updating the vector address.  If the
 *	message is null (end of messages) or invalid, return 0;
 *
 **************************************/
 
	StatusPrint printer;
	
	return printer.interpretStatus (1024, s, (const ISC_STATUS**) vector);
}

SLONG API_ROUTINE fb_interpret(char* s, int bufsize, const ISC_STATUS** vector)
{
	StatusPrint printer;
	
	return printer.interpretStatus (bufsize, s, vector);
}

/* CVC: This special function for ADA has been restored to non-const vector,
 too, in case its usage was broken. */
 
void API_ROUTINE gds__interprete_a(
								   SCHAR* s,
								   SSHORT* length,
								   ISC_STATUS* vector, SSHORT* offset)
{
/**************************************
 *
 *	g d s _ $ i n t e r p r e t e _ a
 *
 **************************************
 *
 * Functional description
 *	Translate a status code with arguments to a string.  Return the
 *	length of the string while updating the vector address.  If the
 *	message is null (end of messages) or invalid, return 0;
 *	Ada being an unsatisfactory excuse for a language, fall back on
 *	the concept of indexing into the vector.
 *
 **************************************/
	ISC_STATUS *v = vector + *offset;
	*length = (SSHORT) gds__interprete(s, &v);
	*offset = v - vector;
}


#define	SECS_PER_HOUR	(60 * 60)
#define	SECS_PER_DAY	(SECS_PER_HOUR * 24)

#ifdef WIN_NT
//Firebird::Spinlock trace_mutex;
HANDLE trace_file_handle = INVALID_HANDLE_VALUE;
#endif

void API_ROUTINE gds__trace_raw(const char* text, unsigned int length)
{
/**************************************
 *
 *	g d s _ t r a c e _ r a w
 *
 **************************************
 *
 * Functional description
 *	Write trace event to a log file
 *  This function tries to be async-signal safe
 *
 **************************************/
 
	if (!length) 
		length = strlen(text);

	Sync sync (&syncObject, "gds__trace_raw");
	sync.lock (Exclusive);
	
#ifdef WIN_NT
	// Note: thread-safe code

	// Nickolay Samofatov, 12 Sept 2003. Windows open files extremely slowly. 
	// Slowly enough to make such trace useless. Thus we cache file handle !
	
	//trace_mutex.enter();
	
	while (true) 
		{
		if (trace_file_handle == INVALID_HANDLE_VALUE) 
			{
			TEXT name[MAXPATHLEN];
			gds__prefix(name, LOGFILE);
			// We do not care to close this file. 
			// It will be closed automatically when our process terminates.
			trace_file_handle = CreateFile(name, GENERIC_WRITE, 
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (trace_file_handle == INVALID_HANDLE_VALUE)
				break;
			}
		DWORD bytesWritten;
		SetFilePointer(trace_file_handle, 0, NULL, FILE_END);
		WriteFile(trace_file_handle, text, length, &bytesWritten, NULL);

		if (bytesWritten != length) 
			{
			// Handle the case when file was deleted by another process on Win9x
			// On WinNT we are not going to notice that fact :(
			CloseHandle(trace_file_handle);
			trace_file_handle = INVALID_HANDLE_VALUE;
			continue;
			}
			break;
		}
	//trace_mutex.leave();
#else

	TEXT name[MAXPATHLEN];

	// This function is not truly signal safe now.
	// It calls string::c_str() and may call getenv(), not good.
	// We can only hope that failure is unlikely in it...
	gds__prefix(name, LOGFILE);

	// Note: signal-safe code
	int file = open(name, O_CREAT | O_APPEND | O_WRONLY, 0660);
	if (file == -1) return;
	write(file, text, length);
	close(file);
#endif
}

void API_ROUTINE gds__trace(const TEXT * text)
{
/**************************************
 *
 *	g d s _ t r a c e
 *
 **************************************
 *
 * Functional description
 *	Post trace event to a log file. Function records date and time
 *  This function tries to be async-signal safe
 *
 **************************************/
	
	const time_t now = time((time_t *)0); // is specified in POSIX to be signal-safe
	
	// 07 Sept 2003, Nickolay Samofatov.
	// Since we cannot call ctime/localtime_r or anything else like this from 
	// signal hanlders we need to decode time by hand. 

	const int days = now / SECS_PER_DAY;
	int rem = now % SECS_PER_DAY;
	tm today;
    today.tm_hour = rem / SECS_PER_HOUR;
    rem %= SECS_PER_HOUR;
    today.tm_min = rem / 60;
    today.tm_sec = rem % 60;

	ndate(days + 40617 /* Number of first day of the Epoch in GDS counting */,
		  &today);

	char buffer[1024]; // 1K should be enough for the trace message
	char* p = buffer;
	gds__ulstr(p, today.tm_year+1900, 4, '0'); p+=4;
	*p++ = '-';
	gds__ulstr(p, today.tm_mon, 2, '0'); p+=2;
	*p++ = '-';
	gds__ulstr(p, today.tm_mday, 2, '0'); p+=2;
	*p++ = 'T';
	gds__ulstr(p, today.tm_hour, 2, '0'); p+=2;
	*p++ = ':';
	gds__ulstr(p, today.tm_min, 2, '0'); p+=2;
	*p++ = ':';
	gds__ulstr(p, today.tm_sec, 2, '0'); p+=2;
	*p++ = ' ';
	gds__ulstr(p, 
#ifdef WIN_NT
#ifdef SUPERSERVER
			     GetCurrentThreadId(),
#else
				 GetCurrentProcessId(),				   
#endif
#else
				 getpid(),
#endif
				 5, ' '); p += 5;
	*p++ = ' ';
	strcpy(p, text); p += strlen(p);
	strcat(p, "\n"); p += strlen(p);
	gds__trace_raw(buffer, p - buffer);
}

void API_ROUTINE gds__log(const TEXT* text, ...)
{
/**************************************
 *
 *	g d s _ l o g
 *
 **************************************
 *
 * Functional description
 *	Post an event to a log file.
 *
 **************************************/
	va_list ptr;
	time_t now;
	TEXT name[MAXPATHLEN];

#ifdef HAVE_GETTIMEOFDAY
#ifndef GETTIMEOFDAY
#define GETTIMEOFDAY(tv)	gettimeofday (tv, 0);
#endif
	struct timeval tv;
	GETTIMEOFDAY(&tv);
	now = tv.tv_sec;
#else
	now = time((time_t *)0);
#endif

	gds__prefix(name, LOGFILE);
	const int oldmask = umask(0111);
	Sync sync (&syncObject, "gds__log");
	sync.lock (Exclusive);
	IB_FILE* file = ib_fopen(name, FOPEN_APPEND_TYPE);
	
	if (file != NULL)
		{
		ib_fprintf(file, "\n%s%s\t%.25s\t", 
				   ISC_get_host(name, MAXPATHLEN), gdslogid, ctime(&now));
		VA_START(ptr, text);
		ib_vfprintf(file, text, ptr);
		ib_fprintf(file, "\n\n");
		ib_fclose(file);
		}

	umask(oldmask);
}


void API_ROUTINE gds__log_status(const TEXT* database,
	const ISC_STATUS* status_vector)
{
/**************************************
 *
 *	g d s _ $ l o g _ s t a t u s
 *
 **************************************
 *
 * Functional description
 *	Log error to error log.
 *
 **************************************/
	TEXT* const buffer = (TEXT*) gds__alloc((SLONG) BUFFER_XLARGE);
/* FREE: at procedure exit */
	if (!buffer)				/* NOMEM: */
		return;
		
#ifdef DEV_BUILD
	const size_t db_len = strlen(database);
	fb_assert(db_len < BUFFER_XLARGE - 2);
	fb_assert(db_len < MAXPATHLEN);
#endif

	const int interpreted_line_length = 80; // raw estimation

	TEXT* p = buffer;
	const TEXT* const end = p + BUFFER_XLARGE - interpreted_line_length;
	const int max_db_len = int(BUFFER_XLARGE - 12);
	sprintf(p, "Database: %.*s", max_db_len, (database) ? database : "");

	do {
		while (*p)
			p++;
		*p++ = '\n';
		*p++ = '\t';
	} while (p < end && gds_interprete_cpp(p, &status_vector));

	p[-2] = 0;
	gds__log(buffer, 0);
	gds__free((SLONG *) buffer);
}


int API_ROUTINE gds__msg_close(void *handle)
{
/**************************************
 *
 *	g d s _ $ m s g _ c l o s e
 *
 **************************************
 *
 * Functional description
 *	Close the message file and zero out static variable.
 *
 **************************************/

	if (handle)
		delete (MsgFile*) handle;
	else if (defaultMsgFile)
		{
		delete defaultMsgFile;
		defaultMsgFile = NULL;
		}
	
	return 0;
}


SSHORT API_ROUTINE gds__msg_format(void*       handle,
								   USHORT      facility,
								   USHORT      number,
								   USHORT      length,
								   TEXT*       buffer,
								   const TEXT* arg1,
								   const TEXT* arg2,
								   const TEXT* arg3,
								   const TEXT* arg4,
								   const TEXT* arg5)
{
/**************************************
 *
 *	g d s _ $ m s g _ f o r m a t
 *
 **************************************
 *
 * Functional description
 *	Lookup and format message.  Return as much of formatted string
 *	as fits in caller's buffer.
 *
 **************************************/
	SLONG size = (SLONG) (((arg1) ? MAX_ERRSTR_LEN : 0) +
						  ((arg2) ? MAX_ERRSTR_LEN : 0) +
						  ((arg3) ? MAX_ERRSTR_LEN : 0) +
						  ((arg4) ? MAX_ERRSTR_LEN : 0) +
						  ((arg5) ? MAX_ERRSTR_LEN : 0) + MAX_ERRMSG_LEN);

	size = (size < length) ? length : size;
	TEXT* formatted = (TEXT *) gds__alloc((SLONG) size);

	if (!formatted)				/* NOMEM: */
		return -1;

	/* Let's assume that the text to be output will never be shorter
	   than the raw text of the message to be formatted.  Then we can
	   use the caller's buffer to temporarily hold the raw text. */

	const int n = gds__msg_lookup(handle, facility, number, length, buffer, NULL);

	if (n > 0 && n < length)
		{
		sprintf(formatted, buffer, arg1, arg2, arg3, arg4, arg5);
		}
	else
		{
		sprintf(formatted, "can't format message %d:%d -- ", facility, number);
		TEXT* p;
		
		if (n == -1)
			strcat(formatted, "message text not found");
		else if (n == -2) 
			{
			strcat(formatted, "message file ");
			
			for (p = formatted; *p;)
				p++;
				
			gds__prefix_msg(p, MSG_FILE);
			strcat(p, " not found");
			}
		else 
			{
			for (p = formatted; *p;)
				p++;
				
			sprintf(p, "message system code %d", n);
			}
		}

	const USHORT l = strlen(formatted);
	const TEXT* const end = buffer + length - 1;

	for (const TEXT* p = formatted; *p && buffer < end;)
		*buffer++ = *p++;

	*buffer = 0;
	gds__free((SLONG *) formatted);
	
	return ((n > 0) ? l : -l);
}


static MsgFile *getDefaultMsgFile()
{
/**************************************
 *
 *	g e t D e f a u l t M s g F i l e
 *
 **************************************
 *
 * Functional description
 *   Find and/or open default message file.
 *
 **************************************/
	Sync sync (&syncObject, "getDefaultMsgFile");
	sync.lock (Shared);

	if (defaultMsgFile)
		return defaultMsgFile;

	sync.unlock();
	sync.lock (Exclusive);

	if (defaultMsgFile)
		return defaultMsgFile;
	
	defaultMsgFile = new MsgFile;
	const TEXT* p = getenv("ISC_MSGS");
	
	if (p && defaultMsgFile->openMsgFile (p) == 0)
		return defaultMsgFile;


	/* Try declared language of this attachment */
	/* This is not quite the same as the language declared on the
		READY statement */

	TEXT msg_file [MAXPATHLEN];
	
	if (p = getenv("LC_MESSAGES"))
		{
		//sanitize(p); // CVC: Sanitizing environment variable???
		TEXT translated_msg_file[sizeof(MSG_FILE_LANG) + LOCALE_MAX + 1];
		sprintf(translated_msg_file, MSG_FILE_LANG, p);
		gds__prefix_msg(msg_file, translated_msg_file);
		
		if (defaultMsgFile->openMsgFile (msg_file) == 0)
			return defaultMsgFile;
		}
		
	gds__prefix_msg(msg_file, MSG_FILE);
	
	if (defaultMsgFile->openMsgFile (msg_file) == 0)
		return defaultMsgFile;
	
	delete defaultMsgFile;
	defaultMsgFile = NULL;
	
	return NULL;
}


SSHORT API_ROUTINE gds__msg_lookup(void* handle,
								   USHORT facility,
								   USHORT number,
								   USHORT length,
								   TEXT* buffer, USHORT* flags)
{
/**************************************
 *
 *	g d s _ $ m s g _ l o o k u p
 *
 **************************************
 *
 * Functional description
 *	Lookup a message.  Return as much of the record as fits in the
 *	callers buffer.  Return the FULL size of the message, or negative
 *	number if we can't find the message.
 *
 **************************************/
 
	MsgFile *msgFile = (handle) ? (MsgFile*) handle : getDefaultMsgFile();
	
	if (!msgFile)
		return -1;
	
	return msgFile->lookupMsg (facility, number, length, buffer, flags);
}


int API_ROUTINE gds__msg_open(void** handle, const TEXT* filename)
{
/**************************************
 *
 *	g d s _ $ m s g _ o p e n
 *
 **************************************
 *
 * Functional description
 *	Try to open a message file.  If successful, return a status of 0
 *	and update a message handle.
 *
 **************************************/
 
	MsgFile *msgFile = new MsgFile;
	int ret = msgFile->openMsgFile (filename);
	
	if (ret == 0)
		*handle = msgFile;

	return ret;
}


void API_ROUTINE gds__msg_put(void* handle,
							  USHORT facility,
							  USHORT number,
							  const TEXT* arg1, const TEXT* arg2,
							  const TEXT* arg3, const TEXT* arg4,
							  const TEXT* arg5)
{
/**************************************
 *
 *	g d s _ $ m s g _ p u t
 *
 **************************************
 *
 * Functional description
 *	Lookup and format message.  Return as much of formatted string
 *	as fits in callers buffer.
 *
 **************************************/
	TEXT formatted[512];

	gds__msg_format(handle, facility, number, sizeof(TEXT) * BUFFER_MEDIUM,
					formatted, arg1, arg2, arg3, arg4, arg5);
	gds__put_error(formatted);
}


SLONG API_ROUTINE gds__get_prefix(SSHORT arg_type, const TEXT* passed_string)
{
/**************************************
 *
 *	g d s _ $ g e t _ p r e f i x
 *
 **************************************
 *
 * Functional description
 *	Find appropriate InterBase command line arguments 
 *	for Interbase file prefix.
 *
 *      arg_type is 0 for $INTERBASE, 1 for $INTERBASE_LOCK 
 *      and 2 for $INTERBASE_MSG
 *
 *      Function returns 0 on success and -1 on failure
 **************************************/
	int count = 0;

	if (arg_type < IB_PREFIX_TYPE || arg_type > IB_PREFIX_MSG_TYPE)
		return ((SLONG) - 1);

	char* prefix_ptr;
	
	switch (arg_type) 
		{
		case IB_PREFIX_TYPE:
			ib_prefix = prefix_ptr = ib_prefix_val;
			break;
			
		case IB_PREFIX_LOCK_TYPE:
			prefix_ptr = ib_prefix_lock = ib_prefix_lock_val;
			break;
			
		case IB_PREFIX_MSG_TYPE:
			prefix_ptr = ib_prefix_msg = ib_prefix_msg_val;
			break;
			
		default:
			return -1;
		}
		
/* the command line argument was 'H' for interbase home */

	while (*prefix_ptr++ = *passed_string++) 
		{
		/* if the next character is space, newline or carriage return OR
		   number of characters exceeded */
		if (*passed_string == ' ' || *passed_string == 10
			|| *passed_string == 13 || (count == MAXPATHLEN)) 
			{
			*(prefix_ptr++) = '\0';
			break;
				}
			count++;
			}
		
	if (!count) 
		{
		prefix_ptr = NULL;
		return -1;
		}
		
	return 0;
}


#ifndef VMS
void API_ROUTINE gds__prefix(TEXT* resultString, const TEXT* file)
{
/**************************************
 *
 *	g d s _ $ p r e f i x	( n o n - V M S )
 *
 **************************************
 *
 * Functional description
 *	Find appropriate file prefix.
 *	Override conditional defines with
 *	the enviroment variable FIREBIRD if it is set.
 *
 **************************************/
	resultString[0] = 0;

	if (ib_prefix == NULL)
		{
		if (!(ib_prefix = getenv ("VULCAN")))
			ib_prefix = getenv ("FB_ENV");
		if (!ib_prefix || !ib_prefix [0])
			{
			const char *regPrefix = Configuration::getRootDirectory();
			size_t len = strlen(regPrefix);

			if (len > 0) 
				{
				if (len > sizeof(ib_prefix_val)) 
					ib_perror("ib_prefix path size too large - truncated");                      
				strncpy(ib_prefix_val, regPrefix, sizeof(ib_prefix_val) -1);
				ib_prefix_val[sizeof(ib_prefix_val) -1] = 0;
				ib_prefix = ib_prefix_val;
				}
			else 
				{
				ib_prefix = FB_PREFIX;
				strcat(ib_prefix_val, ib_prefix);
				}

			ib_prefix = ib_prefix_val;
			}
		}
		
	strcat(resultString, ib_prefix);
	safe_concat_path(resultString, file);
}
#endif /* !defined(VMS) */


#ifdef VMS
void API_ROUTINE gds__prefix(TEXT* string, const TEXT* root)
{
/**************************************
 *
 *	g d s _ $ p r e f i x	( V M S )
 *
 **************************************
 *
 * Functional description
 *	Find appropriate InterBase file prefix.
 *	Override conditional defines with
 *	the enviroment variable INTERBASE if it is set.
 *
 **************************************/
	if (*root != '[') {
		strcpy(string, root);
		return;
	}

/* Check for the existence of an InterBase logical name.  If there is 
   one use it, otherwise use the system directories. */
	TEXT temp[256];
	if (ISC_expand_logical_once(ISC_LOGICAL, sizeof(ISC_LOGICAL) - 2, temp)) {
		strcpy(string, ISC_LOGICAL);
		strcat(string, root);
		return;
	}

	TEXT* p = temp;
	for (const TEXT* q = root; *p = UPPER7(*q); q++)
		if (*p++ == ']')
			break;

	const SSHORT len = p - temp;
	for (ISC_VMS_PREFIX prefix = trans_prefix; prefix->isc_prefix; prefix++)
		if (!strncmp(temp, prefix->isc_prefix, len)) {
			strcpy(string, prefix->vms_prefix);
			strcat(string, &root[len]);
			return;
		}

	strcpy(string, &root[len]);
}
#endif


#ifndef VMS
void API_ROUTINE gds__prefix_lock(TEXT* string, const TEXT* root)
{
/********************************************************
 *
 *	g d s _ $ p r e f i x _ l o c k	( n o n - V M S )
 *
 ********************************************************
 *
 * Functional description
 *	Find appropriate Firebird lock file prefix.
 *	Override conditional defines with the enviroment
 *      variable FIREBIRD_LOCK if it is set.
 *
 **************************************/
	string[0] = 0;

	if (ib_prefix_lock == NULL) 
		{
		if (!(ib_prefix_lock = getenv(FB_LOCK_ENV))) 
			{
#ifdef EMBEDDED
			ib_prefix_lock = ib_prefix_lock_val;
			gds__temp_dir(ib_prefix_lock);
			// Generate filename based on the current PID
			TEXT tmp_buf[MAXPATHLEN];
			sprintf(tmp_buf, FB_PID_FILE, getpid());
			sprintf(tmp_buf, root, tmp_buf);
			root = tmp_buf;
#else
			ib_prefix_lock = ib_prefix_lock_val;
			gds__prefix(ib_prefix_lock, "");
#endif
			}
		else 
			{
			strcat(ib_prefix_lock_val, ib_prefix_lock);
			ib_prefix_lock = ib_prefix_lock_val;
			}
		}
		
	strcat(string, ib_prefix_lock);
	safe_concat_path(string, root);
}
#endif


#ifdef VMS
void API_ROUTINE gds__prefix_lock(TEXT* string, const TEXT* root)
{
/************************************************
 *
 *	g d s _ $ p r e f i x _ l o c k	( V M S )
 *
 ************************************************
 *
 * Functional description
 *	Find appropriate Firebird lock file prefix.
 *	Override conditional defines with the enviroment
 *      variable FIREBIRD_LOCK if it is set.
 *
 *************************************************/
	if (*root != '[') {
		strcpy(string, root);
		return;
	}

/* Check for the existence of a Firebird logical name.  If there is 
   one use it, otherwise use the system directories. */
	TEXT temp[256];
	if (ISC_expand_logical_once
		(ISC_LOGICAL_LOCK, sizeof(ISC_LOGICAL_LOCK) - 2, temp)) {
		strcpy(string, ISC_LOGICAL_LOCK);
		strcat(string, root);
		return;
	}

	TEXT* p = temp;
	for (const TEXT* q = root; *p = UPPER7(*q); q++)
		if (*p++ == ']')
			break;

	const SSHORT len = p - temp;
	for (ISC_VMS_PREFIX prefix = trans_prefix; prefix->isc_prefix; prefix++)
		if (!strncmp(temp, prefix->isc_prefix, len)) {
			strcpy(string, prefix->vms_prefix);
			strcat(string, &root[len]);
			return;
		}

	strcpy(string, &root[len]);
}
#endif

#ifndef VMS
void API_ROUTINE gds__prefix_msg(TEXT* string, const TEXT* root)
{
/********************************************************
 *
 *      g d s _ $ p r e f i x _ m s g ( n o n - V M S )
 *
 ********************************************************
 *
 * Functional description
 *      Find appropriate Firebird message file prefix.
 *      Override conditional defines with the enviroment
 *      variable FIREBIRD_MSG if it is set.
 *
 *
 **************************************/
	string[0] = 0;

	if (ib_prefix_msg == NULL) 
		{
		if (!(ib_prefix_msg = getenv(FB_MSG_ENV))) 
			{
			ib_prefix_msg = ib_prefix_msg_val;
			gds__prefix(ib_prefix_msg, "");
			}
		else 
			{
			strcat(ib_prefix_msg_val, ib_prefix_msg);
			ib_prefix_msg = ib_prefix_msg_val;
			}
		}
		
	strcat(string, ib_prefix_msg);
	safe_concat_path(string, root);
}
#endif

#ifdef VMS
void API_ROUTINE gds__prefix_msg(TEXT* string, const TEXT* root)
{
/************************************************
 *
 *      g d s _ $ p r e f i x _ m s g ( V M S )
 *
 ************************************************
 *
 * Functional description
 *      Find appropriate Firebird message file prefix.
 *      Override conditional defines with the enviroment
 *      variable FIREBIRD_MSG if it is set.
 *
 *************************************************/
	if (*root != '[') {
		strcpy(string, root);
		return;
	}


/* Check for the existence of an InterBase logical name.  If there is
   one use it, otherwise use the system directories. */

/* ISC_LOGICAL_MSG macro needs to be defined, check non VMS version of routine
   for functionality. */
	TEXT temp[256];
	if (ISC_expand_logical_once
		(ISC_LOGICAL_MSG, sizeof(ISC_LOGICAL_MSG) - 2, temp)) {
		strcpy(string, ISC_LOGICAL_MSG);
		strcat(string, root);
		return;
	}

	TEXT* p = temp;
	for (const TEXT* q = root; *p = UPPER7(*q); q++)
		if (*p++ == ']')
			break;

	const SSHORT len = p - temp;
	for (ISC_VMS_PREFIX prefix = trans_prefix; prefix->isc_prefix; prefix++)
		if (!strncmp(temp, prefix->isc_prefix, len)) {
			strcpy(string, prefix->vms_prefix);
			strcat(string, &root[len]);
			return;
		}

	strcpy(string, &root[len]);
}
#endif


ISC_STATUS API_ROUTINE gds__print_status(const ISC_STATUS* vec)
{
/**************************************
 *
 *	g d s _ p r i n t _ s t a t u s
 *
 **************************************
 *
 * Functional description
 *	Interprete a status vector.
 *
 **************************************/
 
	StatusPrint printer;
	
	return printer.printStatus (vec);
}


USHORT API_ROUTINE gds__parse_bpb(USHORT bpb_length,
								  const UCHAR* bpb,
								  USHORT* source,
								  USHORT* target)
{
/**************************************
 *
 *	g d s _ p a r s e _ b p b
 *
 **************************************
 *
 * Functional description
 *	Get blob type, source sub_type and target sub-type
 *	from a blob parameter block.
 *
 **************************************/

  /* SIGN ERROR */

	return gds__parse_bpb2(bpb_length, bpb, (SSHORT*)source, (SSHORT*)target, NULL, NULL);
}


USHORT API_ROUTINE gds__parse_bpb2(USHORT bpb_length,
								   const UCHAR* bpb,
								   SSHORT* source,
								   SSHORT* target,
								   USHORT* source_interp,
								   USHORT* target_interp)
{
/**************************************
 *
 *	g d s _ p a r s e _ b p b 2
 *
 **************************************
 *
 * Functional description
 *	Get blob type, source sub_type and target sub-type
 *	from a blob parameter block.
 *	Basically gds__parse_bpb() with additional:
 *	source_interp and target_interp.
 *
 **************************************/
	USHORT type = 0;
	*source = *target = 0;

	if (source_interp)
		*source_interp = 0;
	if (target_interp)
		*target_interp = 0;

	if (!bpb_length || !bpb)
		return type;

	const UCHAR* p = bpb;
	const UCHAR* const end = p + bpb_length;

	if (*p++ != isc_bpb_version1)
		return type;

	while (p < end) {
		const UCHAR op = *p++;
		const USHORT length = *p++;
		switch (op) {
		case isc_bpb_source_type:
			*source = (USHORT) gds__vax_integer(p, length);
			break;

		case isc_bpb_target_type:
			*target = (USHORT) gds__vax_integer(p, length);
			break;

		case isc_bpb_type:
			type = (USHORT) gds__vax_integer(p, length);
			break;

		case isc_bpb_source_interp:
			if (source_interp)
				*source_interp = (USHORT) gds__vax_integer(p, length);
			break;

		case isc_bpb_target_interp:
			if (target_interp)
				*target_interp = (USHORT) gds__vax_integer(p, length);
			break;

		default:
			break;
		}
		p += length;
	}

	return type;
}


long API_ROUTINE gds__ftof(const SCHAR* string,
							const USHORT length1,
							SCHAR* field,
							const USHORT length2)
{
/**************************************
 *
 *	g d s _ f t o f
 *
 **************************************
 *
 * Functional description
 *	Move a fixed length string to a fixed length string.
 *	This is typically generated by the preprocessor to
 *	move strings around.
 *
 **************************************/
	USHORT fill = 0;
	// CVC: a logic bug rendered the fill > 0 test useless
	if (length2 > length1)
		fill = length2 - length1;

	USHORT l = MIN(length1, length2);
	if (l > 0)
		do {
			*field++ = *string++;
		} while (--l);

	if (fill > 0)
		do {
			*field++ = ' ';
		} while (--fill);

	return 0;
}


int API_ROUTINE gds__print_blr(const UCHAR* blr,
							   FPTR_PRINT_CALLBACK routine,
							   void* user_arg, SSHORT language)
{
/**************************************
 *
 *	g d s _ $ p r i n t _ b l r
 *
 **************************************
 *
 * Functional description
 *	Pretty print blr thru callback routine.
 *
 **************************************/
 
	try 
		{
		if (!routine) 
			{
			routine = gds__default_printer;
			user_arg = NULL;
			}
			
		BlrPrint blrPrint (blr, routine, user_arg, language);
		blrPrint.print();
		}
	catch (const std::exception&) 
		{
		return -1;
		}

	return 0;
}


void API_ROUTINE gds__put_error(const TEXT* string)
{
/**************************************
 *
 *	g d s _ p u t _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	Put out a line of error text.
 *
 **************************************/

#ifdef VMS
#define PUT_ERROR
	struct dsc$descriptor_s desc;

	ISC_make_desc(string, &desc, 0);
	lib$put_output(&desc);
#endif

#ifdef PUT_ERROR
#undef PUT_ERROR
#else
	ib_fputs(string, ib_stderr);
	ib_fputc('\n', ib_stderr);
	ib_fflush(ib_stderr);
#endif
}


void API_ROUTINE gds__qtoq(const void* quad_in, void* quad_out)
{
/**************************************
 *
 *	g d s _ q t o q
 *
 **************************************
 *
 * Functional description
 *	Move a quad value to another quad value.  This
 *      call is generated by the preprocessor when assigning
 *      quad values in FORTRAN.
 *
 **************************************/

	*((ISC_QUAD*) quad_out) = *((ISC_QUAD*) quad_in);
}


void API_ROUTINE gds__register_cleanup(FPTR_VOID_PTR routine, void* arg)
{
/**************************************
 *
 *	g d s _ r e g i s t e r _ c l e a n u p
 *
 **************************************
 *
 * Functional description
 *	Register a cleanup handler.
 *
 **************************************/

	/* 
	 * Ifdef out for windows client.  We have not implemented any way of 
	 * determining when a task ends, therefore this never gets called.
	 */
	 
	Sync sync (&syncObject, "gds__register_cleanup");
	sync.lock (Exclusive);

	if (!initialized)
		init();

	CLEAN obj = new clean;
	obj->clean_next = cleanup_handlers;
	cleanup_handlers = obj;
	obj->clean_routine = routine;
	obj->clean_arg = arg;
	
}


SLONG API_ROUTINE gds__sqlcode(const ISC_STATUS* status_vector)
{
/**************************************
 *
 *	g d s _ s q l c o d e
 *
 **************************************
 *
 * Functional description
 *	Translate GDS error code to SQL error code.  This is a little
 *	imprecise (to say the least) because we don't know the proper
 *	SQL codes.  One must do what what can; stiff upper lip, and all
 *	that.
 *
 *	Scan a status vector - if we find gds__sqlerr in the "right"
 *	positions, return the code that follows.  Otherwise, for the
 *	first code for which there is a non-generic SQLCODE, return it.
 *
 **************************************/
	if (!status_vector) {
		DEV_REPORT("gds__sqlcode: NULL status vector");
		return GENERIC_SQLCODE;
	}

	bool have_sqlcode = false;
	SLONG sqlcode = GENERIC_SQLCODE;	/* error of last resort */

/* SQL code -999 (GENERIC_SQLCODE) is generic, meaning "no other sql code
 * known".  Now scan the status vector, seeing if there is ANY sqlcode
 * reported.  Make note of the first error in the status vector who's 
 * SQLCODE is NOT -999, that will be the return code if there is no specific
 * sqlerr reported.
 */

	const ISC_STATUS* s = status_vector;
	while (*s != isc_arg_end)
	{
		if (*s == isc_arg_gds)
		{
			s++;
			if (*s == isc_sqlerr)
			{
				return *(s + 2);
			}

			if (!have_sqlcode)
				{
				// Now check the hard-coded mapping table of gds_codes to
				// sql_codes
				const SLONG gdscode = status_vector[1];

				if (gdscode) {
					for (int i = 0; gds__sql_code[i].gds_code; ++i) {
						if (gdscode == gds__sql_code[i].gds_code) {
							if (gds__sql_code[i].sql_code != GENERIC_SQLCODE) {
								sqlcode = gds__sql_code[i].sql_code;
								have_sqlcode = true;
							}
							break;
						}
					}
				}
				else {
					sqlcode = 0;
					have_sqlcode = true;
				}
			}
			s++;
		}
		else if (*s == isc_arg_cstring)
			s += 3;				// skip: isc_arg_cstring <len> <ptr>
		else
			s += 2;				// skip: isc_arg_* <item>
	}

	return sqlcode;
}


void API_ROUTINE gds__sqlcode_s(const ISC_STATUS* status_vector, ULONG* sqlcode)
{
/**************************************
 *
 *	g d s _ s q l c o d e _ s
 *
 **************************************
 *
 * Functional description
 *	Translate GDS error code to SQL error code.  This is a little
 *	imprecise (to say the least) because we don't know the proper
 *	SQL codes.  One must do what what can; stiff upper lip, and all
 *	that.  THIS IS THE COBOL VERSION.  (Some cobols don't have
 *	return values for calls...
 *
 **************************************/

	*sqlcode = gds__sqlcode(status_vector);
}


void API_ROUTINE fb_sqlstate(char* const sqlstate, const ISC_STATUS* status_vector)
{
/**************************************
 *
 *	f b _ s q l s t a t e
 *
 **************************************
 *
 * Functional description
 *	Translate GDS error code to SQL State. We'll check to see if
 * we have an exact match, and if so that's great. Maybe we'll
 * get lucky and the caller already put a SQLSTATE in the
 * status vector. It could happen.
 *
 *	Scan a status vector - if we find gds__sqlerr in the "right"
 *	positions, return the code that follows.  Otherwise, for the
 *	first code for which there is a non-generic SQLCODE, return it.
 *
 **************************************/
	strcpy (sqlstate, "HY000"); // error of last resort
	if (!status_vector) {
		DEV_REPORT("fb_sqlstate: NULL status vector");
		return;
	}

	SLONG gdscode = status_vector[1];
	if (gdscode == 0) {
		// status_vector[1] == 0 is no error, by definition
		strcpy (sqlstate, "00000");
		return;
	}

	const ISC_STATUS* s = status_vector;
	bool have_sqlstate = false;

	// step #1, maybe we already have a SQLSTATE stuffed in the status vector
	while ((*s != isc_arg_end) && (!have_sqlstate))
	{
		if (*s == isc_arg_sql_state) {
			strcpy (sqlstate, (char*) *(s + 1)); // easy, next argument points to sqlstate string
			have_sqlstate = true;
		}
		else if (*s == isc_arg_cstring) {
			s += 3; // skip: isc_arg_cstring <len> <ptr>
		}
		else {
			s += 2; // skip: isc_arg_* <item>
		}
	}
	
	if (have_sqlstate)
		return;

	// step #2, see if we can find a mapping.
	gdscode = 0;
	s = status_vector;
	while ((*s != isc_arg_end) && (!have_sqlstate)) {
		if (*s == isc_arg_gds) {
			s++;
			gdscode = (const SLONG) *s;
			if (gdscode != 0) {
				if (!(gdscode == isc_random || gdscode == isc_sqlerr))	{
					// random is useless - it's just "%s". skip it
					// sqlerr (sqlcode) is useless for determining sqlstate. skip it
					
					// implement a binary search for array gds__sql_state[]
					int first=0;
					int last=sizeof(gds__sql_state)/sizeof(*gds__sql_state);
					while (first <= last) {
						int mid = (first + last) / 2;
						if (gdscode > gds__sql_state[mid].gds_code)
							first = mid + 1;
						else if (gdscode < gds__sql_state[mid].gds_code)
							last = mid - 1;
						else {
							// found it!!!

							// we get 00000 for info messages like "Table %"
							// these are completely ignored
							if (!(strcmp ("00000", (char *) gds__sql_state[mid].sql_state) == 0)) {
								strcpy (sqlstate, (char *) gds__sql_state[mid].sql_state);

								// 42000 is general syntax error, and HY000 is general API error.
								// we may be able to find something more precise if we keep scanning.
								if (!(strcmp ("42000", sqlstate) == 0 || strcmp ("HY000", sqlstate) == 0) ) {
									have_sqlstate = true;
								}
							}
							break;
						}
					} // while
				}
				s++;
			}
		}
		else if (*s == isc_arg_cstring) {
			s += 3; // skip: isc_arg_cstring <len> <ptr>
		}
		else {
			s += 2; // skip: isc_arg_* <item>
		}

	} // while

	// sqlstate will be exact match, or
	// 42000 for no_meta_update, or
	// HY000 if we didn't find a match
	return;
}

void API_ROUTINE gds__temp_dir(TEXT* buffer)
{
/**************************************
 *
 *      g d s _ _ t e m p _ d i r
 *
 **************************************
 *
 * Functional description
 *      Return temporary directory.
 *
 **************************************/
	const TEXT* directory = getenv(FB_TMP_ENV);

	if (!directory) 
		{
#ifdef WIN_NT
		TEXT temp_dir[MAXPATHLEN];
		const DWORD len = GetTempPath(sizeof(temp_dir), temp_dir);
		// This checks "TEMP" and "TMP" environment variables
		if (len && len < sizeof(temp_dir))
			directory = temp_dir;
#else
		directory = getenv("TMP");
#endif
		}
		
	if (!directory || strlen(directory) >= MAXPATHLEN)
		directory = WORKFILE;
		
	strcpy(buffer, directory);
}

	
int API_ROUTINE gds__temp_file(const TEXT* string,
					 TEXT* expanded_string, TEXT* dir, BOOLEAN unlink_flag)
{
/**************************************
 *
 *      g d s _ _ t e m p _ f i l e
 *
 **************************************
 *
 * Functional description
 *      Create and open a temp file with a given location.
 *      Unless the address of a buffer for the expanded file name string is
 *      given, make up the file "pre-deleted". Return -1 on failure.
 *      If unlink_flag is TRUE than file is marked as pre-deleted even if 
 *      expanded_string is not NULL.
 * NOTE 
 *		Function returns a file descriptor.  It used to be a piece of crap with
 *		the following horseshit:
 *
 *      Function returns untyped handle that needs to be casted to either IB_FILE
 *      or used as file descriptor. This is ugly and needs to be fixed probably 
 *      via introducing two functions with different return types.
 *
 **************************************/
	TEXT temp_dir[MAXPATHLEN];

	const TEXT* directory = dir;

	if (!directory) 
		{
		gds__temp_dir(temp_dir);
		directory = temp_dir;
		}

	if (strlen(directory) >= MAXPATHLEN - strlen(string) - strlen(TEMP_PATTERN) - 2)
		return -1;

	int result;

#ifdef WIN_NT
	/* These are the characters used in temporary filenames.  */
	static const char letters[] = "abcdefghijklmnopqrstuvwxyz0123456789";

	_timeb t;
	_ftime(&t);
	__int64 randomness = t.time;
	randomness *= 1000;
	randomness += t.millitm;
	
	for (int tryCount = 0; tryCount < MAX_TMPFILE_TRIES; tryCount++) 
		{
		char file_name[MAXPATHLEN];
		strcpy(file_name, directory);
		if (file_name[strlen(file_name) - 1] != '\\')
			strcat(file_name, "\\");
		strcat(file_name, string);
		char suffix[] = TEMP_PATTERN;
		__int64 temp = randomness;

		for (size_t i = 0; i < sizeof(suffix) - 1; i++) 
			{
			suffix[i] = letters[temp % (sizeof(letters) - 1)];
			temp /= sizeof(letters) - 1;
			}

		strcat(file_name, suffix);
		if (expanded_string)
			strcpy(expanded_string, file_name);

		result = _sopen(file_name, _O_CREAT | _O_TRUNC | _O_RDWR | 
			_O_BINARY | _O_SHORT_LIVED | _O_NOINHERIT | _O_EXCL |
			(expanded_string && !unlink_flag ? 0 : _O_TEMPORARY),
			_SH_DENYRW, _S_IREAD|_S_IWRITE);

		if ((int)result != -1 || (errno != EACCES && errno != EEXIST))
			break;
		randomness++;
		}
		
	if (result == -1) 
		return result;
		
#else
	TEXT file_name[MAXPATHLEN];
	strcpy(file_name, directory);
	
	if (file_name[strlen(file_name) - 1] != '/')
		strcat(file_name, "/");
		
	strcat(file_name, string);
	strcat(file_name, TEMP_PATTERN);
	
#ifdef HAVE_MKSTEMP
	result = mkstemp(file_name);
#else
	if (mktemp(file_name) == (char *)0)
		return -1;

	do 
		{
		result = (void *)open(file_name, O_RDWR | O_EXCL| O_CREAT);
		} while (result == -1 && errno == EINTR);
#endif
	if (result == -1)
		return result;

	if (expanded_string)
		strcpy(expanded_string, file_name);

	if (!expanded_string || unlink_flag)
		unlink(file_name);
#endif

	return result;
}


void API_ROUTINE gds__unregister_cleanup(FPTR_VOID_PTR routine, void *arg)
{
/**************************************
 *
 *	g d s _ u n r e g i s t e r _ c l e a n u p
 *
 **************************************
 *
 * Functional description
 *	Unregister a cleanup handler.
 *
 **************************************/
	CLEAN clean;

	for (CLEAN* clean_ptr = &cleanup_handlers; clean = *clean_ptr;clean_ptr = &clean->clean_next) 
        if (clean->clean_routine == routine && clean->clean_arg == arg) 
			{
			*clean_ptr = clean->clean_next;
			//FREE_LIB_MEMORY(clean);
			delete clean;
			break;
			}
}


#ifndef VMS
BOOLEAN API_ROUTINE gds__validate_lib_path(const TEXT* module,
										   const TEXT* ib_env_var,
										   TEXT* resolved_module,
										   SLONG length)
{
/**************************************
 *
 *	g d s _ $ v a l i d a t e _ l i b _ p a t h	( n o n - V M S )
 *
 **************************************
 *
 * Functional description
 *	Find the external library path variable.
 *	Validate that the path to the library module name 
 *	in the path specified.  If the external lib path
 *	is not defined then accept any path, and return 
 *	TRUE. If the module is in the path then return TRUE 
 * 	else, if the module is not in the path return FALSE.
 *
 **************************************/
	TEXT* ib_ext_lib_path = getenv(ib_env_var);
	if (!ib_ext_lib_path) {
		strncpy(resolved_module, module, length);
		return TRUE;		/* The variable is not defined. Return TRUE */
	}

	TEXT abs_module[MAXPATHLEN];
	if (EXPAND_PATH(module, abs_module)) {
		/* Extract the path from the absolute module name */
		const TEXT* q = NULL;
		for (const TEXT* mp = abs_module; *mp; mp++)
			if ((*mp == '\\') || (*mp == '/'))
				q = mp;

		TEXT abs_module_path[MAXPATHLEN];
		memset(abs_module_path, 0, MAXPATHLEN);
		strncpy(abs_module_path, abs_module, q - abs_module);

		/* Check to see if the module path is in the lib path
		   if it is return TRUE.  If it does not find it, then
		   the module path is not valid so return FALSE */

		TEXT abs_path[MAXPATHLEN];
		TEXT path[MAXPATHLEN];

		// CVC: Warning, strtok is changing the environment variable!
		// If this function is called again, only the first substring will be found.
		// Multiple engines in the same machine => shudder.
		const TEXT* token = strtok(ib_ext_lib_path, ";");
		while (token != NULL) {
			strcpy(path, token);
			/* make sure that there is no traing slash on the path */
			TEXT* p = path + strlen(path);
			if ((p != path) && ((p[-1] == '/') || (p[-1] == '\\')))
				p[-1] = 0;
			if ((EXPAND_PATH(path, abs_path))
				&& (!COMPARE_PATH(abs_path, abs_module_path))) 
			{
				strncpy(resolved_module, abs_module, length);
				return TRUE;
			}
			token = strtok(NULL, ";");
		}
	}
	return FALSE;
}
#endif


#ifdef VMS
BOOLEAN API_ROUTINE gds__validate_lib_path(const TEXT* module,
										   const TEXT* ib_env_var,
										   TEXT* resolved_module,
										   SLONG length)
{
/**************************************
 *
 *	g d s _ $ v a l i d a t e _ l i b _ p a t h	( V M S )
 *
 **************************************
 *
 * Functional description
 *	Find the InterBase external library path variable.
 *	Validate that the path to the library module name 
 *	in the path specified.  If the external lib path
 *	is not defined then accept any path, and return true.
 * 	If the module is not in the path return FALSE.
 *
 **************************************/
	TEXT *p, *q;
	TEXT path[MAXPATHLEN];
	TEXT abs_module_path[MAXPATHLEN];
	TEXT abs_module[MAXPATHLEN];

/* Check for the existence of an InterBase logical name.  If there is 
   one use it, otherwise use the system directories. */

	COMPILER ERROR ! BEFORE DOING A VMS POST PLEASE
		MAKE SURE THAT THIS FUNCTION WORKS THE SAME WAY
		AS THE NON -
		VMS ONE
		DOES.if (!
				 (ISC_expand_logical_once
				  (ib_env_var, strlen(ib_env_var) - 2, path))) return TRUE;

	if (ISC_expand_logical_once(module, strlen(module) - 2, abs_module)) {
		/* Extract the path from the module name */
		for (p = abs_module, q = NULL; *p; p++)
			if (*p == ']')
				q = p;

		memset(abs_module_path, 0, MAXPATHLEN);
		strncpy(abs_module_path, abs_module, q - abs_module + 1);

		/* Check to see if the module path is the same as lib path
		   if it is return TURE.  If not then the module path is 
		   not valid so return FALSE */
		if (!strcmp(path, abs_module_path))
			return TRUE;
	}
	return FALSE;
}
#endif


SLONG API_ROUTINE gds__vax_integer(const UCHAR* ptr, SSHORT length)
{
/**************************************
 *
 *	g d s _ $ v a x _ i n t e g e r
 *
 **************************************
 *
 * Functional description
 *	Pick up (and convert) a VAX style integer of length 1, 2, or 4
 *	bytes.
 *
 **************************************/
	SLONG value = 0;
	SSHORT shift = 0;

	while (--length >= 0) {
		value += ((SLONG) * ptr++) << shift;
		shift += 8;
	}

	return value;
}


void API_ROUTINE gds__vtof(
						   const SCHAR* string,
						   SCHAR* field, USHORT length)
{
/**************************************
 *
 *	g d s _ v t o f
 *
 **************************************
 *
 * Functional description
 *	Move a null terminated string to a fixed length
 *	field.  The call is primarily generated  by the
 *	preprocessor.
 *
 **************************************/

	while (*string) {
		*field++ = *string++;
		if (--length <= 0)
			return;
	}

	if (length > 0)
		do {
			*field++ = ' ';
		} while (--length);
}


void API_ROUTINE gds__vtov(const SCHAR* string, char* field, SSHORT length)
{
/**************************************
 *
 *	g d s _ v t o v
 *
 **************************************
 *
 * Functional description
 *	Move a null terminated string to a fixed length
 *	field.  The call is primarily generated  by the
 *	preprocessor.  Unless gds__vtof, the target string
 *	is null terminated.
 *
 **************************************/

	--length;

	while ((*field++ = *string++) != 0)
		if (--length <= 0) {
			*field = 0;
			return;
		}
}


void API_ROUTINE isc_print_sqlerror(SSHORT sqlcode, const ISC_STATUS* status)
{
/**************************************
 *
 *	i s c _ p r i n t _ s q l e r r o r
 *
 **************************************
 *
 * Functional description
 *	Given an sqlcode, give as much info as possible.
 *      Decide whether status is worth mentioning.
 *
 **************************************/
	TEXT error_buffer[192];

	sprintf(error_buffer, "SQLCODE: %d\nSQL ERROR:\n", sqlcode);
	TEXT* p = error_buffer;
	while (*p)
		p++;
	isc_sql_interprete(sqlcode, p,
					   (SSHORT) (sizeof(error_buffer) - (p - error_buffer) -
								 2));
	while (*p)
		p++;
	*p++ = '\n';
	*p = 0;
	gds__put_error(error_buffer);

	if (status && status[1]) {
		gds__put_error("ISC STATUS: ");
		gds__print_status(status);
	}
}


void API_ROUTINE isc_sql_interprete(
									SSHORT sqlcode,
									TEXT* buffer, SSHORT length)
{
/**************************************
 *
 *	i s c _ s q l _ i n t e r p r e t e
 *
 **************************************
 *
 * Functional description
 *	Given an sqlcode, return a buffer full of message
 *	This is very simple because all we have is the
 *      message number, and the sqlmessages have as yet no
 *	arguments
 *
 *	NOTE: As of 21-APR-1999, sqlmessages HAVE arguments hence use
 *	      an empty string instead of NULLs.
 *	      
 **************************************/
	const TEXT* str = "";

	if (sqlcode < 0)
		gds__msg_format(0, 13, (USHORT) (1000 + sqlcode), length, buffer,
						str, str, str, str, str);
	else
		gds__msg_format(0, 14, sqlcode, length, buffer,
						str, str, str, str, str);
}


#ifdef VMS
int unlink(SCHAR* file)
{
/**************************************
 *
 *	u n l i n k
 *
 **************************************
 *
 * Functional description
 *	Get rid of a file and all of its versions.
 *
 **************************************/
	int status;
	struct dsc$descriptor_s desc;

	ISC_make_desc(file, &desc, 0);

	for (;;) {
		status = lib$delete_file(&desc);
		if (!(status & 1))
			break;
	}

	if (!status)
		return 0;
	else if (status != RMS$_FNF)
		return -1;
	else
		return 0;
}
#endif


void gds__cleanup(void)
{
/**************************************
 *
 *	c l e a n u p
 *
 **************************************
 *
 * Functional description
 *	Exit handler for image exit.
 *
 **************************************/
#ifdef UNIX
	if (gds_pid != getpid())
		return;
#endif

	gds__msg_close(NULL);

        CLEAN clean;
	while (clean = cleanup_handlers) 
		{
		cleanup_handlers = clean->clean_next;
		FPTR_VOID_PTR routine = clean->clean_routine;
		void* arg = clean->clean_arg;

		/* We must free the handler before calling it because there
		   may be a handler (and is) that frees all memory that has
		   been allocated. */

		//FREE_LIB_MEMORY(clean);
		if (cleanup_handlers)
			delete clean;
			
		(*routine)(arg);
		}

#ifdef V4_THREADING
/* V4_DESTROY; */
#endif
	initialized = false;
}


static void init(void)
{
/**************************************
 *
 *	i n i t
 *
 **************************************
 *
 * Functional description
 *	Do anything necessary to initialize module/system.
 *
 **************************************/
	if (initialized)
		return;

#ifdef VMS
	exit_description.exit_handler = cleanup;
	exit_description.args = 1;
	exit_description.arg[0] = &exit_status;
	ISC_STATUS status = sys$dclexh(&exit_description);
#endif

#ifdef UNIX
	gds_pid = getpid();
#ifdef SUPERSERVER
#if (defined SOLARIS || defined HPUX || defined LINUX)
	{
		/* Increase max open files to hard limit for Unix
		   platforms which are known to have low soft limits. */

		struct rlimit old;

		if (!getrlimit(RLIMIT_NOFILE, &old) && old.rlim_cur < old.rlim_max) {
			struct rlimit new_;
			new_.rlim_cur = new_.rlim_max = old.rlim_max;
			if (!setrlimit(RLIMIT_NOFILE, &new_))
			{
#if _FILE_OFFSET_BITS == 64 && defined SOLARIS
				gds__log("64 bit i/o support is on.");
				gds__log("Open file limit increased from %lld to %lld",
						 old.rlim_cur, new_.rlim_cur);
		       
#else
				gds__log("Open file limit increased from %d to %d",
						 old.rlim_cur, new_.rlim_cur);
#endif
			}
		}
	}
#endif
#endif /* SUPERSERVER */
#endif /* UNIX */

	atexit(gds__cleanup);

	initialized = true;

#ifndef REQUESTER
	ISC_signal_init();
#endif

	/* V4_GLOBAL_MUTEX_UNLOCK; */
}


static int yday(const struct tm* times)
{
/**************************************
 *
 *	y d a y
 *
 **************************************
 *
 * Functional description
 *	Convert a calendar date to the day-of-year.
 *
 *	The unix time structure considers
 *	january 1 to be Year day 0, although it
 *	is day 1 of the month.   (Note that QLI,
 *	when printing Year days takes the other
 *	view.)   
 *
 **************************************/
	SSHORT day = times->tm_mday;
	const SSHORT month = times->tm_mon;
	const SSHORT year = times->tm_year + 1900;

	--day;

	day += (214 * month + 3) / 7;

	if (month < 2)
		return day;

	if (year % 4 == 0 && year % 100 != 0 || year % 400 == 0)
		--day;
	else
		day -= 2;

	return day;
}


static void ndate(SLONG nday, tm* times)
{
/**************************************
 *
 *	n d a t e
 *
 **************************************
 *
 * Functional description
 *	Convert a numeric day to [day, month, year].
 *
 * Calenders are divided into 4 year cycles.
 * 3 Non-Leap years, and 1 leap year.
 * Each cycle takes 365*4 + 1 == 1461 days.
 * There is a further cycle of 100 4 year cycles.
 * Every 100 years, the normally expected leap year
 * is not present.  Every 400 years it is.
 * This cycle takes 100 * 1461 - 3 == 146097 days
 * The origin of the constant 2400001 is unknown.
 * The origin of the constant 1721119 is unknown.
 * The difference between 2400001 and 1721119 is the
 * number of days From 0/0/0000 to our base date of
 * 11/xx/1858. (678882)
 * The origin of the constant 153 is unknown.
 *
 * This whole routine has problems with ndates
 * less than -678882 (Approx 2/1/0000).
 *
 **************************************/
	nday -= 1721119 - 2400001;
	const SLONG century = (4 * nday - 1) / 146097;
	nday = 4 * nday - 1 - 146097 * century;
	SLONG day = nday / 4;

	nday = (4 * day + 3) / 1461;
	day = 4 * day + 3 - 1461 * nday;
	day = (day + 4) / 4;

	SLONG month = (5 * day - 3) / 153;
	day = 5 * day - 3 - 153 * month;
	day = (day + 5) / 5;

	SLONG year = 100 * century + nday;

	if (month < 10)
		month += 3;
	else {
		month -= 9;
		year += 1;
	}

	times->tm_mday = (int) day;
	times->tm_mon = (int) month - 1;
	times->tm_year = (int) year - 1900;
}


static GDS_DATE nday(const tm* times)
{
/**************************************
 *
 *	n d a y
 *
 **************************************
 *
 * Functional description
 *	Convert a calendar date to a numeric day
 *	(the number of days since the base date).
 *
 **************************************/
	const SSHORT day = times->tm_mday;
	SSHORT month = times->tm_mon + 1;
	SSHORT year = times->tm_year + 1900;

	if (month > 2)
		month -= 3;
	else {
		month += 9;
		year -= 1;
	}

	const SLONG c = year / 100;
	const SLONG ya = year - 100 * c;

	return (GDS_DATE) (((SINT64) 146097 * c) / 4 +
					   (1461 * ya) / 4 +
					   (153 * month + 2) / 5 + day + 1721119 - 2400001);
}


static void sanitize(TEXT* locale)
{
/**************************************
 *
 *	s a n i t i z e
 *
 **************************************
 *
 * Functional description
 *	Clean up a locale to make it acceptable for use in file names
 *      for Windows NT, PC.
 *	replace any period with '_' for NT or PC.
 *
 **************************************/

	while (*locale) {
		if (*locale == '.')
			*locale = '_';
		locale++;
	}
}

static void safe_concat_path(TEXT *resultString, const TEXT *appendString)
{
/**************************************
 *
 *	s a f e _ c o n c a t _ p a t h
 *
 **************************************
 *
 * Functional description
 *	Safely appends appendString to resultString using paths rules.
 *  resultString must be at least MAXPATHLEN size.
 *
 **************************************/
	int len = strlen(resultString);
	if (resultString[len - 1] != DIR_SEPARATOR && len < MAXPATHLEN - 1) {
		resultString[len++] = DIR_SEPARATOR;
		resultString[len] = 0;
	}
	int alen = strlen(appendString);
	if (len + alen > MAXPATHLEN - 1)
		alen = MAXPATHLEN - 1 - len;
	fb_assert(alen >= 0);
	memcpy(&resultString[len], appendString, alen);
	resultString[len + alen] = 0;
}

void gds__default_printer(void* arg, SSHORT offset, const TEXT* line)
{
	ib_printf("%4d %s\n", offset, line);
}

void gds__trace_printer(void* arg, SSHORT offset, const TEXT* line)
{
	// Assume that line is not too long
	char buffer[PRETTY_BUFFER_SIZE + 10];
	char* p = buffer;
	gds__ulstr(p, offset, 4, ' ');
	p += strlen(p);
	*p++ = ' ';
	strcpy(p, line); p += strlen(p);
	*p++ = '\n';
	*p = 0;
	gds__trace_raw(buffer);
}

#ifdef DEBUG_GDS_ALLOC
#undef gds__alloc

void* API_ROUTINE gds__alloc(SLONG size)
{
/**************************************
 *
 *	g d s _ $ a l l o c     (alternate debug entrypoint)
 *
 **************************************
 *
 * Functional description
 *
 *	NOTE: This function should be the last in the file due to
 *	the undef of gds__alloc above.
 *
 *	For modules not recompiled with DEBUG_GDS_ALLOC, this provides
 *	an entry point to malloc again.
 *
 **************************************/
	return gds__alloc_debug(size, "-- Unknown --", 0);
}
#endif



#define NO_NFS

#define MOVE_FAST(from,to,length)       memcpy (to, from, (int) (length))
#define MOVE_FASTER(from,to,length)     memcpy (to, from, (int) (length))
#define MOVE_CLEAR(to,length)           memset (to, 0, (int) (length))
#define MEMMOVE(from,to,length)         memmove ((void *)to, (void *)from, (size_t) length)

#define SYS_ARG		isc_arg_win32
#define SLONGFORMAT	"ld"
#define ULONGFORMAT	"lu"
#define XLONGFORMAT "lX"
#define xLONGFORMAT "lx"

//format for __LINE__
#define LINEFORMAT "d"

typedef __int64 SINT64;
typedef unsigned __int64 UINT64;
#define INT64_DEFINED

/* The following define is the prefix to go in front of a "d" or "u"
   format item in a ib_printf() format string, to indicate that the argument
   is an SINT64 or UINT64. */
#define QUADFORMAT "I64"
/* The following macro creates a quad-sized constant, possibly one
   which is too large to fit in a long int.  The Microsoft compiler does
   not permit the LL suffix which some other platforms require, but it
   handles numbers up to the largest 64-bit integer correctly without such
   a suffix, so the macro definition is trivial. */
#ifdef MINGW // needed for gcc 3.3.1
#define QUADCONST(n) (n##LL)
#else
#define QUADCONST(n) (n)
#endif

#ifdef _X86_
#ifndef I386
#define I386
#endif
#define IMPLEMENTATION  isc_info_db_impl_isc_winnt_x86 /* 50 */
#endif

#define                 IEEE
#define VA_START(list,parmN)    va_start (list, parmN)
#define API_ROUTINE     __stdcall
#define API_ROUTINE_VARARG      __cdecl
#define CLIB_ROUTINE    __cdecl
#define THREAD_ROUTINE  __stdcall
#define INTERNAL_API_ROUTINE	API_ROUTINE

#define SYNC_WRITE_DEFAULT      1

#ifndef MAXPATHLEN
#ifdef MAX_PATH
#define MAXPATHLEN MAX_PATH
#else
#define MAXPATHLEN 260
#endif
#endif



#ifndef hpux
#define hpux
#endif

#define KILLER_SIGNALS
#define UNIX            1
#define CURSES_KEYPAD   1

/*#define ALIGNMENT       8*/
/*#define DOUBLE_ALIGN    8*/
#define IMPLEMENTATION  isc_info_db_impl_isc_hp_ux /* 31 */

#define                 IEEE
#pragma OPT_LEVEL 1
// 16-Apr-2002 HP10 in unistd.h Paul Beach
//#define setreuid(ruid,euid)     setresuid (ruid, euid, -1)
//#define setregid(rgid,egid)     setresgid (rgid, egid, -1)

/* The following define is the prefix to go in front of a "d" or "u"
   format item in a ib_printf() format string, to indicate that the argument
   is an SINT64 or UINT64. */
#define QUADFORMAT "ll"
/* The following macro creates a quad-sized constant, possibly one
   which is too large to fit in a long int. */
#define QUADCONST(n) (n##LL)

#define MEMMOVE(from,to,length)       memmove ((void *)to, (void *)from, (size_t) length)
#define MOVE_FAST(from,to,length)       memcpy (to, from, (int) (length))
#define MOVE_FASTER(from,to,length)     memcpy (to, from, (int) (length))
#define MOVE_CLEAR(to,length)           memset (to, 0, (int) (length))


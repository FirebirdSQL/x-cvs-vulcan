#define QUADFORMAT "ll"
#define QUADCONST(n) (n##LL)
#define SLONGFORMAT "ld"
#define ULONGFORMAT "lu"
#define XLONGFORMAT "lX"
#define xLONGFORMAT "lx"


//format for __LINE__
#define LINEFORMAT "d"

#ifdef SUPERSERVER
#define SET_TCP_NO_DELAY
#endif

#define KILLER_SIGNALS

#define VA_START(list,parmN)    va_start (list, parmN)
#define UNIX    1
#define IEEE    1

#ifdef __x86_64
#define AMD64
#endif

#ifdef AMD64
#define IMPLEMENTATION  isc_info_db_impl_linux_amd64 
#endif /* i386 */

#ifdef i386
#define I386    1
#define IMPLEMENTATION  isc_info_db_impl_i386 /* 60  next higher unique number, See you later  */
#endif /* i386 */

#ifdef sparc
#define IMPLEMENTATION  isc_info_db_impl_linux_sparc /* 65  */
#endif /* sparc */

#define MEMMOVE(from,to,length)		memmove ((void *)to, (void *)from, (size_t) length)
#define MOVE_FAST(from,to,length)       memcpy (to, from, (int) (length))
#define MOVE_FASTER(from,to,length)     memcpy (to, from, (int) (length))
#define MOVE_CLEAR(to,length)           memset (to, 0, (int) (length))


/* Defined KILLER_SIGNALS for Sun - as we were getting lots of lockups
 * using pipe server.
 * 1995-February-24 David Schnepper
 */
#define KILLER_SIGNALS

#ifdef SOLARIS

/* This is NOT defined when building the special shared-pipe library
 * which customers can use to avoid the problems with signals & threads
 * in Solaris
 */

//#define SOLARIS_MT	1

/*  Define the following only on platforms whose standard I/O
 *  implementation is so weak that we wouldn't be able to fopen
 *  a file whose underlying file descriptor would be > 255.
 *  Hey, we're not running on PDP-11's any more: would it kill you
 *  to use a short instead of a char to hold the fileno?  :-(
 */
 
/* Why we (solarises) need to rewrite old BSD stdio
   so many times I suggest stdIO from 
   http://www.research.att.com/sw/tools/sfio/ 
*/       
/* 	Need to use full sfio not just stdio emulation to fix
	file descriptor number limit. nmcc Dec2002
*/
#if (!defined(SFIO) && defined(SUPERSERVER))
#error "need to use SFIO"
#endif

#ifdef SOLX86
#define LSEEK_OFFSET_CAST (off_t)
#endif

#define MEMMOVE(from,to,length)       memmove ((void *)to, (void *)from, (size_t) length)
/*********   Reason for introducing MEMMOVE macro.

  void *memcpy( void *s1, const void *s2, size_t n);
  void *memmove( void *s1, const void *s2, size_t n);

  The memcpy() function copies n characters from the string pointed to by the
  s2 parameter into the location pointed to by the s1 parameter.  When copy-
  ing overlapping strings, the behavior of this function is unreliable.

  The memmove() function copies n characters from the string at the location
  pointed to by the s2 parameter to the string at the location pointed to by
  the s1 parameter.  Copying takes place as though the n number of characters
  from string s2 are first copied into a temporary location having n bytes
  that do not overlap either of the strings pointed to by s1 and s2. Then, n
  number of characters from the temporary location are copied to the string
  pointed to by s1. Consequently, this operation is nondestructive and
  proceeds from left to right.
  The above text is taken from the Digital UNIX man pages.

     For maximum portability, memmove should be used when the memory areas
     indicated by s1 and s2 may overlap, and memcpy used for faster copying
     between non-overlapping areas.

**********/

/* The following define is the prefix to go in front of a "d" or "u"
   format item in a ib_printf() format string, to indicate that the argument
   is an SINT64 or UINT64. */
#define QUADFORMAT "ll"
/* The following macro creates a quad-sized constant, possibly one
   which is too large to fit in a long int. */
#define QUADCONST(n) (n##LL)

#else /* SOLARIS */

#define BSD_UNIX        1

#endif /* SOLARIS */

#define UNIX            1
#define                 IEEE

#ifdef sparc
/*#define ALIGNMENT       4*/
/*#define DOUBLE_ALIGN    8*/
#define IMPLEMENTATION  isc_info_db_impl_isc_sun4 /* 30 */
#else /* sparc */

#ifdef i386
#define I386            1
#define IMPLEMENTATION  isc_info_db_impl_isc_sun_386i  /* 32 */
#else /* i386 */
#define IMPLEMENTATION  isc_info_db_impl_isc_sun_68k /* 28 */
#endif /* i386 */

#endif /* sparc */

#define MOVE_FAST(from,to,length)       memcpy (to, from, (int) (length))
#define MOVE_FASTER(from,to,length)     memcpy (to, from, (int) (length))
#define MOVE_CLEAR(to,length)           memset (to, 0, (int) (length))

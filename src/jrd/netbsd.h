
#if defined(__i386__)
/*#define ALIGNMENT     4*/
/*#define DOUBLE_ALIGN  4*/

#define IEEE  1
#define I386  1
#define IMPLEMENTATION        isc_info_db_impl_netbsd /* 62 */

#define QUADFORMAT "ll"
#define QUADCONST(n) (n##LL)
#else /* !__i386__ */
#error Please add support for other ports
#endif

#define UNIX  1

#define KILLER_SIGNALS
#define NO_NFS					/* no MTAB_OPEN or MTAB_CLOSE in isc_file.c */

#define MEMMOVE(from,to,length)     memmove ((void *)to, (void *)from, (size_t) length)
#define MOVE_FAST(from,to,length)       memcpy (to, from, (int) (length))
#define MOVE_FASTER(from,to,length)     memcpy (to, from, (int) (length))
#define MOVE_CLEAR(to,length)           memset (to, 0, (int) (length))


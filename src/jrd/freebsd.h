
/* EKU: obsolete, replaced by _FILE_OFFSET_BITS
#ifndef UNIX_64_BIT_IO
#define UNIX_64_BIT_IO
#endif
*/

/*#define ALIGNMENT     4*/
/*#define DOUBLE_ALIGN  4*/

#define UNIX  1
#define IEEE  1
#define I386  1
#define IMPLEMENTATION    isc_info_db_impl_freebsd   /* 61 */

#define QUADFORMAT "ll"
#define QUADCONST(n) (n##LL)
#define KILLER_SIGNALS
#define NO_NFS					/* no MTAB_OPEN or MTAB_CLOSE in isc_file.c */

#define MEMMOVE(from,to,length)     memmove ((void *)to, (void *)from, (size_t) length)
#define MOVE_FAST(from,to,length)       memcpy (to, from, (int) (length))
#define MOVE_FASTER(from,to,length)     memcpy (to, from, (int) (length))
#define MOVE_CLEAR(to,length)           memset (to, 0, (int) (length))


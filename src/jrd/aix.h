#ifndef _POWER					/* IBM RS/6000 */
#define AIX
#define KILLER_SIGNALS
#define UNIX            1
#define CURSES_KEYPAD   1
/*#define ALIGNMENT       4*/
#define IMPLEMENTATION  isc_info_db_impl_isc_rt_aix /* 35 */
#define                 IEEE
#define MEMMOVE(from,to,length)       memmove ((void *)to, (void *)from, (size_t) length)
#define MOVE_FAST(from,to,length)       memcpy (to, from, (int) (length))
#define MOVE_FASTER(from,to,length)     memcpy (to, from, (int) (length))
#define MOVE_CLEAR(to,length)           memset (to, 0, (int) (length))
#define SYSCALL_INTERRUPTED(err)        (((err) == EINTR) || ((err) == ERESTART))	/* pjpg 20001102 */
#else /* AIX PowerPC */
#define AIX_PPC
#define KILLER_SIGNALS
#define UNIX            1
#define CURSES_KEYPAD   1
/*#define ALIGNMENT       4*/
#define IMPLEMENTATION  isc_info_db_impl_isc_rt_aix /* 35 */
#define                 IEEE
#define MEMMOVE(from,to,length)       memmove ((void *)to, (void *)from, (size_t) length)
#define MOVE_FAST(from,to,length)       memcpy (to, from, (int) (length))
#define MOVE_FASTER(from,to,length)     memcpy (to, from, (int) (length))
#define MOVE_CLEAR(to,length)           memset (to, 0, (int) (length))
#define SYSCALL_INTERRUPTED(err)        (((err) == EINTR) || ((err) == ERESTART))	/* pjpg 20001102 */

#define VA_START(list,parmN)    va_start (list, parmN)	/* TMC 081700 */
#define QUADFORMAT "ll"			/* TMC 081700 */
#define QUADCONST(n) (n##LL)	/* TMC 081700 */

#endif /* IBM PowerPC */


/* EKU: obsolete, replaced by _FILE_OFFSET_BITS
#ifndef UNIX_64_BIT_IO
#define UNIX_64_BIT_IO
#endif
*/
//format for __LINE__
#define LINEFORMAT "d"

#define SLONGFORMAT	"ld"
#define ULONGFORMAT "lu"
#define XLONGFORMAT "lX"
#define xLONGFORMAT "lx"

/*#define ALIGNMENT       4*/
/*#define DOUBLE_ALIGN    4*/
#define BSD_UNIX        1
#define UNIX            1
#define IMPLEMENTATION  63
#define IEEE
#define QUADCONST(n) (n##LL)
#define QUADFORMAT "q"
#define MAP_ANONYMOUS
#define MAP_ANNON
#define LSEEK_OFFSET_CAST (off_t)

#define MEMMOVE(from,to,length)		memmove ((void *)to, (void *)from, (size_t)length)
#define MOVE_FAST(from,to,length)	memcpy (to, from, (int) (length))
#define MOVE_FASTER(from,to,length)	memcpy (to, from, (int) (length))
#define MOVE_CLEAR(to,length)		memset (to, 0, (int) (length))


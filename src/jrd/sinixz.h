#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* These prototypes are missing in the system header files :-( */
int gettimeofday (struct timeval *tp);
int munmap(void * addr, size_t len);
int gethostname(char *name, size_t len);
int socket(int domain, int type, int protocol);
int connect(int s, struct sockaddr *name, int namelen);
int send(int s, void *msg, int len, int flags);
int recv(int s, void *buf, int len, int flags);
int strcasecmp(const char *s1, const char *s2);
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *execptfds, struct timeval *timeout);
int getsockopt(int s, int level, int optname, char *optval, int *optlen);
int setsockopt(int s, int level, int optname, char *optval, int optlen);
int bind(int s, struct sockaddr *name, int namelen);
int listen(int s, int backlog);
int accept(int s, struct sockaddr *ddr, int *addrlen);
int getsockname(int s, struct sockaddr *name, int *namelen);
int setsockname(int s, struct sockaddr *name, int *namelen);
int getpeername(int s, struct sockaddr *name, int *namelen);
int shutdown(int s, int how);
int syslog(int pri, char *fmt, ...);

#ifdef __cplusplus
    }
#endif

#include <dlfcn.h>
#define dlopen(a,b)		dlopen((char *)(a),(b))
#define dlsym(a,b)		dlsym((a), (char *)(b))

#include <signal.h>
#include <sys/siginfo.h>

struct sinixz_sigaction
  {
    int sa_flags;
    union
      {
        /* Used if SA_SIGINFO is not set.  */
        void (*sa_handler)(int);
        /* Used if SA_SIGINFO is set.  */
        void (*sa_sigaction) (int, siginfo_t *, void *);
      }
    __sigaction_handler;
#define sa_handler		__sigaction_handler.sa_handler
#define sa_sigaction		__sigaction_handler.sa_sigaction
    sigset_t sa_mask;
    int sa_resv[2];
  };

static inline int sinixz_sigaction(int sig, const struct sinixz_sigaction *act,
                                   struct sinixz_sigaction *oact)
{
  return sigaction(sig, (struct sigaction*)act, (struct sigaction*)oact);
}

// Re-define things actually
#define sigaction		sinixz_sigaction

#define QUADFORMAT "ll"
#define QUADCONST(n) (n##LL)

/*#define ALIGNMENT	4*/
/*#define DOUBLE_ALIGN	8*/

#ifdef SUPERSERVER
#define SET_TCP_NO_DELAY
#endif

#define KILLER_SIGNALS

#define VA_START(list,parmN)    va_start (list, parmN)
#define UNIX    1
#define IEEE    1

#ifdef i386
#define I386    1
/* Change version string into SINIXZ */
#define IMPLEMENTATION  isc_info_db_impl_sinixz  /* 64 */
#endif /* i386 */

#define setreuid(ruid,euid)     setuid(euid)
#define setregid(rgid,egid)     setgid(egid)

#define MEMMOVE(from,to,length)		memmove ((void *)to, (void *)from, (size_t) length)
#define MOVE_FAST(from,to,length)       memcpy (to, from, (int) (length))
#define MOVE_FASTER(from,to,length)     memcpy (to, from, (int) (length))
#define MOVE_CLEAR(to,length)           memset (to, 0, (int) (length))

//format for __LINE__
#define LINEFORMAT "d"

#define SLONGFORMAT "ld"
#define ULONGFORMAT "lu"
#define XLONGFORMAT "lX"
#define xLONGFORMAT "lx"

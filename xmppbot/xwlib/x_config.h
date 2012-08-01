#ifndef X_CONFIG_H_
#define X_CONFIG_H_

#include "x_trace_defs.h"

#ifdef ANDROID
#include "x_config_android.h"
#elif defined (IOS)
#include "x_config_ios.h"
#else
#define HAVE_STRDUP 1
#define HAVE_STRLEN 1
#define HAVE_STRCPY 1
#define HAVE_INET_NTOP 1
/* #undef HAVE_INET_NTOA */

#ifdef WIN32
/* #undef HAVE_MSWSOCK_H */
/* #undef HAVE_WINSOCK2_H */
/* #undef HAVE_WSA_RECVMSG */
#else
#define HAVE_PTHREAD_H 1
#define HAVE_SYS_UIO_H 1
#define HAVE_RECVMSG 1
#define HAVE_SENDMSG 1
#endif

/* #undef HAVE_CLOCK_GETTIME */
/* #undef HAVE_CLOCK_SYSCALL */
#define HAVE_DLFCN_H 1
#define HAVE_EPOLL_CTL 1
#define HAVE_EVENTFD 1
/* #undef HAVE_FLOOR */
#define HAVE_INOTIFY_INIT 1
#define HAVE_INTTYPES_H 1
/* #undef HAVE_KQUEUE */
/* #undef HAVE_LIBRT */
#define HAVE_MEMORY_H 1
#define HAVE_NANOSLEEP 1
#define HAVE_POLL 1
#define HAVE_POLL_H 1
/* #undef HAVE_PORT_CREATE */
/* #undef HAVE_PORT_H */
#define HAVE_SELECT 1
/* #undef HAVE_SIGNALFD */
/* #undef HAVE_STDINT_H */
/* #undef HAVE_STDLIB_H */
/* #undef HAVE_STRINGS_H */
/* #undef HAVE_STRING_H */
#define HAVE_SYS_EPOLL_H 1
#define HAVE_SYS_EVENTFD_H 1
/* #undef HAVE_SYS_EVENT_H */
#define HAVE_SYS_INOTIFY_H 1
#define HAVE_SYS_SELECT_H 1
#define HAVE_SYS_SIGNALFD_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_UNISTD_H 1
#define HAVE_GETOPT_H 1 
#define HAVE_GETOPT_LONG 1 
/* #undef HAVE_GETTID */
#define HAVE_SYSCALL 1 

/* #undef LT_OBJDIR */
/* #undef PACKAGE */
/* #undef PACKAGE_BUGREPORT */
/* #undef PACKAGE_NAME */
/* #undef PACKAGE_STRING */
/* #undef PACKAGE_TARNAME */
/* #undef PACKAGE_URL */
/* #undef PACKAGE_VERSION */
/* #undef VERSION */
#endif /* ANDROID */

#endif /* X_CONFIG_H_ */

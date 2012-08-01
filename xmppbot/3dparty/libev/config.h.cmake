#ifdef ANDROID
#include "config_android.h"
#elif defined (IOS)
#include "config_ios.h"
#else
#cmakedefine HAVE_CLOCK_GETTIME 1
#cmakedefine HAVE_CLOCK_SYSCALL 1
#cmakedefine HAVE_DLFCN_H 1
#cmakedefine HAVE_EPOLL_CTL 1
#cmakedefine HAVE_EVENTFD 1
#cmakedefine HAVE_FLOOR 1
#cmakedefine HAVE_INOTIFY_INIT 1
#cmakedefine HAVE_INTTYPES_H 1
#cmakedefine HAVE_KQUEUE 1
#cmakedefine HAVE_LIBRT 1
#cmakedefine HAVE_MEMORY_H 1
#cmakedefine HAVE_NANOSLEEP 1
#cmakedefine HAVE_POLL 1
#cmakedefine HAVE_POLL_H 1
#cmakedefine HAVE_PORT_CREATE 1
#cmakedefine HAVE_PORT_H 1
#cmakedefine HAVE_SELECT 1
#cmakedefine HAVE_SIGNALFD 1
#cmakedefine HAVE_STDINT_H 1
#cmakedefine HAVE_STDLIB_H 1
#cmakedefine HAVE_STRINGS_H 1
#cmakedefine HAVE_STRING_H 1
#cmakedefine HAVE_SYS_EPOLL_H 1
#cmakedefine HAVE_SYS_EVENTFD_H 1
#cmakedefine HAVE_SYS_EVENT_H 1
#cmakedefine HAVE_SYS_INOTIFY_H 1
#cmakedefine HAVE_SYS_SELECT_H 1
#cmakedefine HAVE_SYS_SIGNALFD_H 1
#cmakedefine HAVE_SYS_STAT_H 1
#cmakedefine HAVE_SYS_TYPES_H 1
#cmakedefine HAVE_UNISTD_H 1
#cmakedefine LT_OBJDIR 1
#cmakedefine PACKAGE 1
#cmakedefine PACKAGE_BUGREPORT 1
#cmakedefine PACKAGE_NAME 1
#cmakedefine PACKAGE_STRING 1
#cmakedefine PACKAGE_TARNAME 1
#cmakedefine PACKAGE_URL 1
#cmakedefine PACKAGE_VERSION 1
#cmakedefine VERSION 1

#endif /* ANDROID */

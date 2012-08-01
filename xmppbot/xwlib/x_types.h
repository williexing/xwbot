#ifndef X_TYPES_H_
#define X_TYPES_H_

#include "x_config.h"

#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>

#if defined (WIN32) || defined(_WIN32) || defined(_WINDLL) || defined(WINDOWS)
#ifndef _O_SYNC
#	define O_SYNC         0
#else
#	define O_SYNC         _O_SYNC
#endif
#	define WIN32 1
#	define GETTID 0
#	include <winsock2.h>
#	include <ws2tcpip.h>
//#	include <wspiapi.h>
#	include <windows.h>
#	include <mswsock.h>
#	include <io.h>
#	include <share.h>
#   include <memory.h>
#   include <tchar.h>
#	ifndef __DISABLE_MULTITHREAD__
	typedef void* THREAD_T;
        typedef void (*t_func)(void*);
#		include <process.h>
#	endif /* __DISABLE_MULTITHREAD__ */
#ifndef EWOULDBLOCK
#		define EWOULDBLOCK WSAEWOULDBLOCK
#endif /* EWOULDBLOCK */
#else /* defined (WIN32) || defined(_WINDLL) || defined(WINDOWS */
#	include <alloca.h>
#	include <unistd.h>
#	include <sys/socket.h>
#	include <sys/un.h>
#	include <netinet/in.h>
#	include <arpa/inet.h>
#	include <netdb.h>
#	include <sys/syscall.h>
#ifdef HAVE_GETTID
#       define GETTID gettid()
#elif defined(SYS_gettid) && defined(HAVE_SYSCALL)
#       define GETTID syscall(SYS_gettid)
#else
#       define GETTID 0
#endif /* HAV_GETTID */
#ifndef __DISABLE_MULTITHREAD__
#	include <strings.h>
#	include <pthread.h>
	typedef pthread_t THREAD_T;
	typedef void *(*t_func)(void*);
	typedef int SOCKET;
#endif /* __DISABLE_MULTITHREAD__ */
#endif /* defined (WIN32) || defined(_WINDLL) || defined(WINDOWS */

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#else
struct iovec
{
	void *iov_base; /* Pointer to data.  */
	size_t iov_len; /* Length of data.  */
};

struct msghdr {
	void *msg_name;
	int msg_namelen;
	struct iovec *msg_iov;
	int msg_iovlen;
};
#endif

#ifdef WIN32
typedef struct _WSAMSG _gobee_iomsg_t;
typedef struct _WSABUF x_iobuf_t;
#define X_IOV_DATA(x) ((x_iobuf_t *)x)->buf
#define X_IOV_LEN(x) ((x_iobuf_t *)x)->len
#	define _gobee_iomsg_name(x) (((_gobee_iomsg_t *)x)->name)
#	define _gobee_iomsg_namelen(x) (((_gobee_iomsg_t *)x)->namelen)
#	define _gobee_iomsg_iov(x) (((_gobee_iomsg_t *)x)->lpBuffers)
#	define _gobee_iomsg_iovlen(x) (((_gobee_iomsg_t *)x)->dwBufferCount)
#	define _gobee_iomsg_control(x) (((_gobee_iomsg_t *)x)->Control.buf)
#	define _gobee_iomsg_controllen(x) (((_gobee_iomsg_t *)x)->Control.len)
typedef LPDWORD _gobee_size_t;
#else
typedef struct msghdr _gobee_iomsg_t;
typedef struct iovec x_iobuf_t;
#define X_IOV_DATA(x) ((x_iobuf_t *)x)->iov_base
#define X_IOV_LEN(x) ((x_iobuf_t *)x)->iov_len
#	define _gobee_iomsg_name(x) (((_gobee_iomsg_t *)x)->msg_name)
#	define _gobee_iomsg_namelen(x) (((_gobee_iomsg_t *)x)->msg_namelen)
#	define _gobee_iomsg_iov(x) (((_gobee_iomsg_t *)x)->msg_iov)
#	define _gobee_iomsg_iovlen(x) (((_gobee_iomsg_t *)x)->msg_iovlen)
#	define _gobee_iomsg_control(x) (((_gobee_iomsg_t *)x)->msg_control)
#	define _gobee_iomsg_controllen(x) (((_gobee_iomsg_t *)x)->msg_controllen)
typedef size_t _gobee_size_t;
#endif

#ifdef _MSC_VER
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef __int16 int_least16_t;
typedef unsigned __int16 uint16_t;
typedef SSIZE_T ssize_t;
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C"
  {
#endif

/**
 * DOUBLE LINKED LIST DEFINITION
 */
typedef struct list_entry
{
  struct list_entry *next;
  struct list_entry *prev;
} list_entry_t;

/**
 * HASH TABLE DEFINITIONS
 */

/*#define HTABLE_DEBUG*/

typedef char *VAL;
typedef char *KEY;

#ifndef BOOL
#define BOOL int
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define HTABLESIZE	64

/* entry */
struct ht_cell
{
  struct ht_cell *next; /* a pointer to the next element */
  KEY key; /* key    */
  VAL val; /* value  */
};

typedef struct ht_cell x_obj_attr_t;
typedef struct ht_cell x_obj_listener_t;

/* iterator */
struct ht_celliter
{
  int index;
  struct ht_cell *ptr;
};

/**/
struct x_transport
{
  int
  (*wr)(struct x_transport *, char *buf, unsigned int len, int flags);
  int
  (*on_wr)(struct x_transport *, char *buf, unsigned int len, int flags);
  int
  (*rd)(struct x_transport *, char *buf, unsigned int len, int flags);
};

/** @} */

/**********************/
/* ENUMS */
/**********************/
typedef enum
{
  X_NONE = 0, X_CREAT, X_RDONLY, X_RDWR, X_WRONLY, X_FLAG_NUM
} X_FLAG;

/* useful macroses */
#define BUG() do { printf("BUG()\n"); *((volatile int *)10) = 10; } while(0)
#define BUG_ON(x)\
  do { \
    if (x) { \
        printf("BUG_ON()\n"); \
        BUG(); \
    } \
  } while (0)

/* DEBUGGING MACROS */
#if (defined(_DEBUG) || defined(_DEBUG_) \
|| defined(DEBUG) || defined (__DEBUG__) || defined(__DEBUG))
#define __DEBUGGING_MACROS__ 1
#else
#undef __DEBUGGING_MACROS__
#define PROFILING 1
#endif

/*#define ANDROID 1*/
#ifdef __DEBUGGING_MACROS__
#       if defined(DEBUG_PRFX)
#               if defined(ANDROID)
#                       include <jni.h>
#                       include <string.h>
#                       include <android/log.h>
#                       define ENTER __android_log_print(ANDROID_LOG_DEBUG,DEBUG_PRFX, \
                              DEBUG_PRFX" %s::%s():%d ENTER\n",__FILE__,__FUNCTION__,__LINE__)
#                       define ERROR __android_log_print(ANDROID_LOG_DEBUG,DEBUG_PRFX, \
                              DEBUG_PRFX" %s::%s():%d ERROR\n",__FILE__,__FUNCTION__,__LINE__)
#                       define EXIT __android_log_print(ANDROID_LOG_DEBUG,DEBUG_PRFX, \
                              DEBUG_PRFX" %s::%s():%d EXIT\n",__FILE__,__FUNCTION__,__LINE__)
#                       define TRACE(fmt,...) __android_log_print(ANDROID_LOG_DEBUG,DEBUG_PRFX, \
                              DEBUG_PRFX" %s():%d " fmt, __FUNCTION__,__LINE__, ## __VA_ARGS__)
#               else
#                       include <stdio.h>
#                       define ENTER printf("%d : " DEBUG_PRFX "%s::%s():%d ENTER\n",(int)GETTID,__FILE__,__FUNCTION__,__LINE__)
#                       define ERROR printf("%d : " DEBUG_PRFX "%s::%s():%d ERROR\n",(int)GETTID,__FILE__,__FUNCTION__,__LINE__)
#                       define EXIT printf("%d : " DEBUG_PRFX "%s::%s():%d EXIT\n",(int)GETTID,__FILE__,__FUNCTION__,__LINE__)
#                       ifdef __STRICT_ANSI__
#                             define TRACE printf("%s",DEBUG_PRFX); printf
#                       else
#                             define TRACE(fmt,...) printf("%d : " DEBUG_PRFX "%s()::%d " fmt,(int)GETTID, __FUNCTION__,__LINE__, ## __VA_ARGS__)
#                       endif
#               endif /* ANDROID*/
#       else /* defined(DEBUG_PRFX) */
#               define ENTER do {} while (0)
#               define EXIT do {} while (0)
#               define ERROR do {} while (0)
#       	ifdef __STRICT_ANSI__
#                       define TRACE printf
#               else
#                       define TRACE(fmt,...) do {} while (0)
#               endif
#       endif /* defined(DEBUG_PRFX) */
#else
#       define ENTER do {} while (0)
#       define EXIT do {} while (0)
#       define ERROR do {} while (0)
#       ifdef __STRICT_ANSI__
#               define TRACE printf
#       else
#               define TRACE(fmt,...) do {} while (0)
#       endif
#endif

/* check trace level */
#if TRACE_LEVEL < 2
#       define ENTER do {} while (0)
#       define EXIT do {} while (0)
#endif

#if !defined(__DISABLE_MULTITHREAD__) && \
    (defined(WIN32) || defined(_WINDLL) || defined(WINDOWS))
  #define CS2CS(x) &(x)
  #define CS_DECL(x) CRITICAL_SECTION x
  #define CS_INIT(x) InitializeCriticalSection(x)
  #define CS_INIT_ERRCHK(x) InitializeCriticalSection(x)
  #define CS_RELEASE(x) DeleteCriticalSection(x)
  #define CS_LOCK(x) EnterCriticalSection(x)
  #define CS_UNLOCK(x) LeaveCriticalSection(x)
//#elif defined(__DISABLE_MULTITHREAD__) && defined(HAVE_PTHREAD_H)
#elif !defined(__DISABLE_MULTITHREAD__) && defined(HAVE_PTHREAD_H)
  #define CS_DECL(x) pthread_mutex_t x;
  #define CS_INIT(x) { \
    /* printf("%d CS_INIT: %p\n",GETTID,x); */ \
    pthread_mutex_init(x,NULL); }
  #define CS_INIT_ERRCHK(x) { \
    /* printf("%d CS_INIT_ERRCHK: %p\n",GETTID,x); */ \
    pthread_mutex_init(x,NULL);}
  #define CS_RELEASE(x) { \
    /* printf("%d CS_RELEASE: %p, %s:%d\n",GETTID,x,__FILE__,__LINE__); */ \
    pthread_mutex_destroy(x); }
  #define CS_LOCK(x) { \
    /* printf("%d CS_LOCK: %p, %s:%d\n",GETTID,x,__FILE__,__LINE__); */ \
    pthread_mutex_lock(x); }
  #define CS_UNLOCK(x) { \
    /* printf("%d CS_UNLOCK: %p, %s:%d\n",GETTID,x,__FILE__,__LINE__); */ \
    pthread_mutex_unlock(x); }
  #define CS2CS(x) &(x)
#else /* __DISABLE_MULTITHREAD__ */
  #define CS_DECL(x)
  #define CS_INIT(x) do {} while (0)
  #define CS_RELEASE(x) do {} while (0)
  #define CS_LOCK(x) do {} while (0)
  #define CS_UNLOCK(x) do {} while (0)
  #define CS2CS(x) (x)
  #define CS_DISABLED 1
#endif /* __DISABLE_MULTITHREAD__ */

/*/////////////////////////////////////////////////// */
/*/////////////////////////////////////////////////// */
/*/////////////// PLUGIN DEFINITIONS //////////////// */
/*/////////////////////////////////////////////////// */
/*/////////////////////////////////////////////////// */
#ifdef __STATIC__
#define __x_plugin_visibility 
#else
#define __x_plugin_visibility /* static */
#endif
      
#if (defined(__GNUC__) && __GNUC__ >= 4) || defined (__clang__) || defined (__llvm__)
typedef void (*plugin_t)(void);
#       define __plugin_init __attribute__((constructor))
#       define PLUGIN_INIT(x) void __plugin_init x (void)

#elif defined (__GNUC__)
typedef void (*plugin_t)(void);
#		define __plugin_init
#       define __plugin_ptr __attribute__((__section__(".ctors")))
#       define PLUGIN_INIT(x) plugin_t * __plugin_ptr ptr_##x \
                = (plugin_t *)x

#elif defined (WIN32) || defined(_WINDLL) \
	|| defined(WINDOWS) || defined(_WINDOWS)
	
	typedef void (__cdecl *plugin_t)(void);
#	if defined(_WINDLL__)
#		define __plugin_init
/* #       define __plugin_ptr */
#       define PLUGIN_INIT(x) \
/*	BOOLEAN WINAPI DllMain(HINSTANCE hDllHandle,DWORD nReason,LPVOID Reserved) {\
    switch (nReason) { \
        case DLL_PROCESS_ATTACH: \
            DisableThreadLibraryCalls( hDllHandle ); \
			((plugin_t)x)();\
            break; \
        case DLL_PROCESS_DETACH: \
            break; \
		} \
	} */
#	else
#		define __plugin_init
#       define __plugin_ptr __pragma(section(".ctors$m",read,write,execute)) __declspec(allocate(".ctors$m"))
#       define PLUGIN_INIT(x) __plugin_ptr static const plugin_t _ini_ptr_##x \
                = (plugin_t)x
#       define __plugin_sect_start__ __pragma(section(".ctors$a",read,write,execute)) __declspec(allocate(".ctors$a"))
#       define __plugin_sect_end__ __pragma(section(".ctors$z",read,write,execute)) __declspec(allocate(".ctors$z"))
#	endif /* _WINDLL */
#else
#       define PLUGIN_INIT(x)
#endif

#ifndef offsetof
#define offsetof(type,member) (size_t)((char *)&((type *)0)->member)
#endif

/*/////////////////////////////////////////////////// */
/*/////////////////////////////////////////////////// */
/*////////////// END PLUGIN DEFINITIONS ///////////// */
/*/////////////////////////////////////////////////// */
/*/////////////////////////////////////////////////// */

/**
 * Class definition macro
 */
#if (!defined(__cplusplus) && !defined(__STRICT_ANSI__)) || defined(_ISOC99_SOURCE)
#define DECLARE_CLASS(classname,classtruct,...) \
  static struct xobjectclass  classname##_##objclass = \
  { \
    .cname = #classname, \
    .psize = (unsigned int)(sizeof(classtruct) \
        - sizeof(x_object)), ##__VA_ARGS__ \
  }; \
  static __plugin_init void \
  classname##_##_init(void) \
  { \
    ENTER; \
    x_class_register(classname##_##objclass.cname, &classname##_##objclass); \
    EXIT; \
    return; \
  } \
  PLUGIN_INIT(classname##_##init);
#endif /* __STRICT_ANSI__ */

#ifdef __cplusplus
}
#endif

#endif

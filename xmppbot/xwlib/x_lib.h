/*
 * Copyright (c) 2011, artemka
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * xwbot
 * Created on: Jun 5, 2011
 *     Author: artemka
 *
 */


#ifndef X_LIB_H_
#define X_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "x_types.h"

#define X_LIB_MAX_NETADDRLEN 128
#define X_LIB_BUFLEN 256

/** starting capacity of a string */
#define X_STR_BUFSIZ 0x20
#define X_STR_MEMLEN_ALIGN(strlen) (size_t)((strlen + (X_STR_BUFSIZ - 1))&~((size_t)(X_STR_BUFSIZ - 1)))

#if defined (WIN32) || defined(_WINDLL) || defined(WINDOWS)
#define WIN32
typedef unsigned __int32 u_int32_t;
typedef __int32 int32_t;
#ifndef __inline__
#define __inline__ __inline
#endif /*__inline__*/

#       define EXPORT_SYMBOL __declspec( dllexport )
#       define IMPORT_SYMBOL __declspec( dllimport )
#       define HIDE_SYMBOL
#       define __NOINLINE__ __declspec( noinline )
#       define X_DEPRECATED __declspec(deprecated)
#       define UNUSED 
#		define EXPORT_CLASS __declspec( dllexport )
#elif defined(__GNUC__) && __GNUC__ >= 4
#       define EXPORT_SYMBOL __attribute__ ((visibility("default")))
#       define IMPORT_SYMBOL
#       define HIDE_SYMBOL __attribute__((visibility("hidden")))
#       define __NOINLINE__ __attribute__((noinline))
#       define X_DEPRECATED __attribute__ ((deprecated))
#       define UNUSED __attribute__ ((unused))
#		define EXPORT_CLASS __attribute__ ((visibility("default")))
#else
#       define EXPORT_SYMBOL
#       define IMPORT_SYMBOL
#       define HIDE_SYMBOL
#       define __NOINLINE__
#       define X_DEPRECATED
#       define UNUSED
#		define EXPORT_CLASS
#endif /* WIN32,_WINDLL, __GNUC__ ... */

/**
 * ctype wrappers
 */
EXPORT_SYMBOL int x_isupper(int c);
EXPORT_SYMBOL int x_islower(int c);
EXPORT_SYMBOL int x_isalpha(int c);
EXPORT_SYMBOL int x_isdigit(int c);
EXPORT_SYMBOL int x_isalnum(int c);
EXPORT_SYMBOL int x_isspace(int c);
EXPORT_SYMBOL int x_isblank(int c);

/**
 * stdlib wrappers
 */
#include <stdlib.h>
#include <stdio.h>

#include "x_config.h"

#if defined (WIN32) || defined(_WINDLL) || defined(WINDOWS)

#include <direct.h>
#include <malloc.h>
#define x_strcmp strcmp
#define x_strcasecmp _stricmp
#define x_strncasecmp _strnicmp
#define x_strchr strchr
#define x_strncmp strncmp
#define x_snprintf _snprintf
#define x_sprintf _sprintf
//#define x_strlen strlen
#define x_putchar putchar
#define random rand
// #define x_strcpy strcpy
#define x_strncpy strncpy
#define x_write _write
#define x_read _read
#define x_memset memset
#define x_memcpy memcpy
#define mkdir(x,y) _mkdir(x)
#define x_strdup _strdup
#define strerror_r(e,s,l) strerror_s(s,l,e)

#else /* (WIN32) || defined(_WINDLL) || defined(WINDOWS) */

#include <sys/stat.h>
// #include <sys/types.h>
#define x_strcmp strcmp
#define x_strcasecmp strcasecmp
#define x_strncasecmp strncasecmp
#define x_strchr strchr
#define x_strrchr strrchr
#define x_strncmp strncmp
#define x_snprintf snprintf
#define x_sprintf sprintf
#define x_putchar putchar
#define x_strncpy strncpy
#define x_write write
#define x_read read
#define x_memset memset
#define x_memcpy memcpy

#endif /* (WIN32) || defined(_WINDLL) || defined(WINDOWS) */

#define x_strstr strstr

#ifdef HAVE_SENDMSG
#	define x_send_msg sendmsg
#else
	EXPORT_SYMBOL int x_send_msg(SOCKET sockfd, _gobee_iomsg_t *mh, int flags);
#endif

#ifdef HAVE_RECVMSG
#       define x_recv_msg recvmsg
#else
        EXPORT_SYMBOL int x_recv_msg(SOCKET sockfd, _gobee_iomsg_t *mh, int flags);
#endif

#ifdef HAVE_INET_NTOP
#	define x_inet_ntop inet_ntop 
#else
	EXPORT_SYMBOL char *x_inet_ntop (int family, void *src, char *buf, size_t buflen);
#endif

#ifdef HAVE_INET_ATON
#	define x_inet_aton inet_aton
#else
	EXPORT_SYMBOL int x_inet_aton (char* addr, struct in_addr* dest);
#endif

#if (defined(_DEBUG) || defined(_DEBUG_) \
 || defined(DEBUG) || defined (__DEBUG__) || defined(__DEBUG) \
 || defined(PROFILING))
EXPORT_SYMBOL void *x_local_malloc(size_t);
EXPORT_SYMBOL void *x_local_realloc(void *, size_t);
EXPORT_SYMBOL void x_local_free(void *);
#define x_malloc x_local_malloc
#define x_realloc x_local_realloc
#define x_free x_local_free
#else
#define x_malloc malloc
#define x_realloc realloc
#define x_free free
#endif

#ifndef HAVE_STRDUP
EXPORT_SYMBOL char *x_strdup(const char *in);
#else
#ifndef x_strdup
#define x_strdup strdup
#endif
#endif

#ifndef HAVE_STRLEN
EXPORT_SYMBOL unsigned int x_strlen(const char *in);
#else
#define x_strlen strlen
#endif

#ifndef HAVE_STRCPY
EXPORT_SYMBOL char *x_strcpy(char *dest, const char *src);
#else
#define x_strcpy strcpy
#endif

EXPORT_SYMBOL int x_mkdir_p(const char *dirname);

#ifdef __cplusplus
}
#endif

#endif /* X_LIB_H_ */

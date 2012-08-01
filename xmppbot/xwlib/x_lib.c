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

#include "x_types.h"
#include "x_lib.h"

int
x_isupper(int c)
{
  unsigned char _c = (unsigned char) c;
  return (_c >= 'A' && _c <= 'Z');
}

int
x_islower(int c)
{
  unsigned char _c = (unsigned char) c;
  return (_c >= 'a' && _c <= 'z');
}

int
x_isalpha(int c)
{
  return (x_isupper(c) || x_islower(c));
}

int
x_isdigit(int c)
{
  unsigned char _c = (unsigned char) c;
  return (_c >= '0' && _c <= '9');
}

int
x_isalnum(int c)
{
  return (x_isalpha(c) || x_isdigit(c));
}

int
x_isspace(int c)
{
  unsigned char _c = (unsigned char) c;
  return (_c == ' ' || _c == '\f' || _c == '\n' || _c == '\r' || _c == '\t'
      || _c == '\v');
}

int
x_isblank(int c)
{
  unsigned char _c = (unsigned char) c;
  return (_c == ' ' || _c == '\t');
}

#ifndef HAVE_STRDUP
EXPORT_SYMBOL char *
x_strdup(const char *in)
  {
    size_t len;
    char *out = NULL;
    if (in == NULL)
    return NULL;

    len = x_strlen(in);
    out = (char *) x_malloc(X_STR_MEMLEN_ALIGN(len + 1));

    if (out != NULL)
      {
        x_memcpy(out, in, len);
      }
    out[len] = '\0';

    return out;
  }
#endif

#ifndef HAVE_STRCPY
EXPORT_SYMBOL char *
x_strcpy(char *dest, const char *src)
  {
    while ((*dest++ = *src++))
    ;
    return dest;
  }
#endif

#ifndef HAVE_STRLEN
EXPORT_SYMBOL unsigned int
x_strlen(const char *in)
  {
    unsigned int res = 0;
    while (in[res++])
    ;
    return --res;
  }
#endif

EXPORT_SYMBOL int
x_mkdir_p(const char *dirname)
{
  return mkdir(dirname, 755);
}

#if (defined(_DEBUG) || defined(_DEBUG_) \
|| defined(DEBUG) || defined (__DEBUG__) || defined(__DEBUG) \
	|| defined(PROFILING))
static FILE *mfd = NULL;

EXPORT_SYMBOL void *
x_local_malloc(size_t _siz)
{
  void *p;
  int siz = X_STR_MEMLEN_ALIGN(_siz);

#if !defined(ANDROID) && !defined (IOS)
  if (mfd)
    {
      fprintf(mfd, "%d : > M: (%d)->(%d)\n", GETTID, _siz, siz);
    }
#endif

  p = malloc(siz);

#if !defined(ANDROID) && !defined (IOS)
  if (!mfd)
    {
      TRACE("Creating memory usage trace log...\n");
      mfd = fopen("memuse.log", "w+");
    }
  fprintf(mfd, "%d : M:%p:%d\n", GETTID, p, _siz);
#endif

  return p;
}

EXPORT_SYMBOL void *
x_local_realloc(void *p, size_t _siz)
{
  void *_p;
  int siz = X_STR_MEMLEN_ALIGN(_siz);

#if !defined(ANDROID) && !defined (IOS)
  if (mfd)
    {
      fprintf(mfd, "%d : > R: (%p:%d)->(%d)\n", GETTID, p, _siz, siz);
    }
#endif

  _p = realloc(p, siz);
#if !defined(ANDROID) && !defined (IOS)
  if (mfd)
    {
      fprintf(mfd, "%d : R:%p:%p:%d\n", GETTID, p, _p, siz);
    }
#endif
  return _p;
}

EXPORT_SYMBOL void
x_local_free(void *p)
{
#if !defined(ANDROID) && !defined (IOS)
  if (mfd)
    {
      fprintf(mfd, "%d : F:%p\n", GETTID, p);
      fflush(mfd);
    }
#endif

  BUG_ON(!p);

  free(p);
}

#endif /* DEBUG */

/**
 @todo Make it dependent on HAVE_INET_NTOP
 */
#ifndef HAVE_INET_NTOP
char *
x_inet_ntop(int family, void *src, char *buf, size_t buflen)
  {
    char *addr;
    addr = inet_ntoa(((struct sockaddr_in *) src)->sin_addr);
    if (addr)
    strncpy(buf, addr, buflen - 1);
    else
    buf[0] = '\0';
    return buf;
  }
#endif /* HAVE_INET_NTOP */

#ifndef HAVE_INET_ATON
int
x_inet_aton(char* addr, struct in_addr* dest)
{
  int a[4];
  if (sscanf(addr, "%d.%d.%d.%d", &a[0], &a[1], &a[2], &a[3]) != 4)
    return 0;
  dest->s_addr = a[0] | a[1] << 8 | a[2] << 16 | a[3] << 24;
  return 1;
}
#endif

#ifndef HAVE_SENDMSG
int
x_send_msg(SOCKET s, _gobee_iomsg_t *mh, int flags)
  {
    int err = -ENODEV;
#if defined(WIN32)
    int bytes=0;

    err = WSASendTo((SOCKET)s,mh->lpBuffers,mh->dwBufferCount,
        &bytes,flags,mh->name,mh->namelen,NULL,NULL);

#endif
    return err;
  }
#endif /* HAVE_SENDMSG */

#ifndef HAVE_RECVMSG
int
x_recv_msg(SOCKET s, _gobee_iomsg_t *mh, int flags)
  {
    int err = -ENODEV;

    /* get message */
#if defined(WIN32)
    int bytes=0;

    err = WSARecvFrom((SOCKET)s,mh->lpBuffers,mh->dwBufferCount,
        &bytes,flags,mh->name,mh->namelen,NULL,NULL);

#endif

    return err;
  }
#endif /* HAVE_SENDMSG */


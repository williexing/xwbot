/*
 * x_utils.c
 *
 *  Created on: 17.11.2010
 *      Author: artur
 */

#include <sys/types.h>
#include <string.h>
/*#include <sys/stat.h>*/
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <netdb.h>
#include <ctype.h>

#undef DEBUG_PRFX

#include "x_config.h"

#if TRACE_XUTILS_ON
#define DEBUG_PRFX "[x_utils] "
#endif

#include "x_types.h"
#include "x_lib.h"
#include "x_utils.h"

#ifdef WIN32
struct sockaddr **__x_bus_get_local_addr_list2(void)
  {
    return NULL;
  }
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <sys/ioctl.h>

#ifdef LINUX
#include <linux/if.h>

/**
 * @todo Make this global and remove __x_bus_get_local_addr_list()
 */
struct sockaddr **
icectl_get_local_ips(void)
  {
    int i;
    size_t siz;
    struct ifreq *ifr;
    struct ifconf ifc;
    int sock;
    int numif;
    struct sockaddr **addrarray;

    ENTER;

    ifc.ifc_len = 0;
    ifc.ifc_ifcu.ifcu_req = NULL;

    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) < 0)
      {
        TRACE("ERR: socket() %s",strerror(errno));
        return NULL;
      }

    if (ioctl(sock, SIOCGIFCONF, &ifc) < 0)
      {
        TRACE("ERR: ioctl() %s",strerror(errno));
        return NULL;
      }

    if ((ifr = x_malloc(ifc.ifc_len)) == NULL)
      {
        TRACE("ERR: malloc() %s",strerror(errno));
        return NULL;
      }

    ifc.ifc_ifcu.ifcu_req = ifr;

    if (ioctl(sock, SIOCGIFCONF, &ifc) < 0)
      {
        TRACE("ERR: ioctl2() %s",strerror(errno));
        return NULL;
      }

    close(sock);

    numif = ifc.ifc_len / sizeof(struct ifreq);

    siz = sizeof(struct sockaddr *) * (numif + 1) + sizeof(struct sockaddr)
    * numif;
    addrarray = x_malloc(siz);
    TRACE("Allocated 0x%x bytes at %p\n", siz, addrarray);

    addrarray[numif] = 0; /* terminator */

    TRACE("Found %d addresses\n", numif);

    for (i = 0; i < numif; i++)
      {
        addrarray[i] = (struct sockaddr *) &addrarray[numif + 1] + i
        * sizeof(struct sockaddr);

        struct ifreq *r = &ifr[i];
        struct sockaddr_in *sin = (struct sockaddr_in *) &r->ifr_addr;

        TRACE("(0x%x) %p(%p),%p,%p\n", sizeof(struct sockaddr),
            &addrarray[0],&addrarray[numif + 1], addrarray[i], sin);

        if (r->ifr_addr.sa_family == AF_INET || r->ifr_addr.sa_family == AF_INET6)
          {
            x_memcpy((void *) addrarray[i], (void *) &r->ifr_addr,
                sizeof(struct sockaddr));

            if (r->ifr_addr.sa_family == AF_INET)
              {
                TRACE("%-8s\n", &r->ifr_name[0]);
                //  TRACE("%s\n",inet_ntoa(sin->sin_addr));
              }
          }
      }

    x_free(ifr);

    EXIT;
    return addrarray;
  }

#endif

#ifndef ANDROID
#include <sys/types.h>
#include <ifaddrs.h>
/**
 * @todo Fix this function. Move it to xwlib
 */
struct sockaddr **
__x_bus_get_local_addr_list2(void)
{
  struct sockaddr **addrarray;
  char *ptr;
//  struct addrinfo hints;
//  struct addrinfo *res, *_res;
//  char myhostname[128];
  int family, s;

  struct ifaddrs *ifaddr, *ifa = NULL;

  char host[NI_MAXHOST];
  int err = -1;
  int i = 0;
  int numaddr = 0;

  ENTER;

  TRACE("ENTER\n");

  err = getifaddrs(&ifaddr);
  BUG_ON(err);
  if (err)
    {
      TRACE("ERROR! Getting address list\n");
      goto ADDRLIST2_EXIT;
    }

  TRACE("1: ifaddr=%p\n", ifaddr);

  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
      family = ifa->ifa_addr->sa_family;
      if (family == AF_INET || family == AF_INET6)
        numaddr++;

      /*
       TRACE("%s  address family: %d%s\n", ifa->ifa_name, family, (family
       == AF_PACKET) ? " (AF_PACKET)" : (family == AF_INET) ? " (AF_INET)"
       : (family == AF_INET6) ? " (AF_INET6)" : "");
       */

      /* For an AF_INET* interface address, display the address */
      if (family == AF_INET || family == AF_INET6)
        {
          s = getnameinfo(ifa->ifa_addr,
              (family == AF_INET) ? sizeof(struct sockaddr_in) :
                  sizeof(struct sockaddr_in6), host, NI_MAXHOST, NULL, 0,
              NI_NUMERICHOST);
          if (s != 0)
            {
              TRACE("getnameinfo() failed: %s\n", gai_strerror(s));
              exit(EXIT_FAILURE);
            }

          TRACE("\taddress: <%s>\n", host);
        }
    }

  TRACE("Found %d addresses\n", numaddr);

  addrarray = (struct sockaddr **)x_malloc(
      sizeof(struct sockaddr *) * (numaddr + 1)
          + sizeof(struct sockaddr) * (numaddr + 1));

  addrarray[numaddr] = 0; /* terminator */
  ptr = (char *)&addrarray[numaddr + 1];

  for (ifa = ifaddr, i=0; ifa != NULL && i < numaddr; ifa = ifa->ifa_next)
    {
      family = ifa->ifa_addr->sa_family;
      if (family == AF_INET || family == AF_INET6)
        {
          addrarray[i] = (struct sockaddr *)(
                      ((unsigned long long)ptr)
                     + i * sizeof(struct sockaddr));
          x_memcpy(addrarray[i], (void *) ifa->ifa_addr,
              sizeof(struct sockaddr));
          i++;
        }
    }

  TRACE("2: Freeing address list: %p\n", ifaddr);

  freeifaddrs(ifaddr);

  EXIT;
  TRACE("2: returning...\n");
  ADDRLIST2_EXIT: return addrarray;
}
#endif /* ANDROID */

/**
 * @todo Fix this function. Move it to xwlib
 */
struct sockaddr **
__x_bus_get_local_addr_list(void)
{
  int err = -1;
  int i;
  struct sockaddr **addrarray;
  struct addrinfo hints;
  struct addrinfo *res, *_res;
  char myhostname[128];
  int numaddr = 0;

  ENTER;

  gethostname(myhostname, sizeof(myhostname) - 1);

  TRACE("Determining local IPs for %s...\n", myhostname);

  x_memset(&hints, 0, sizeof(struct addrinfo));
  /* set-up hints structure */
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
#ifdef ANDROID
  hints.ai_flags = AI_CANONNAME | AI_ADDRCONFIG;
#else
  hints.ai_flags = AI_CANONNAME | AI_ADDRCONFIG | AI_V4MAPPED;
#endif

  TRACE("Determining local IPs for %s (flags[0x%x])...\n",
      myhostname, hints.ai_flags);
  err = getaddrinfo(myhostname, NULL, &hints, &_res);
  if (err)
    {
      TRACE("ERROR!: %s\n", gai_strerror(err));
    }
  else
    {
      for (res = _res; res; res = res->ai_next)
        {
          numaddr++;
#if 0
          struct sockaddr_in *sa = (struct sockaddr_in *) res->ai_addr;
          if ((sa->sin_addr.s_addr & htonl(0x7f000000)) == htonl(0x7f000000))
            {
              /* skip loopback interfaces */
              continue;
            }
          else
            {
              inet_ntoa(sa->sin_addr);
              switch (res->ai_addr->sa_family)
                {
                  case AF_INET:
                  break;
                  case AF_INET6:
                  break;
                  default:
                  break;
                }
              break;
            }
#endif
        }

      addrarray = x_malloc(
          sizeof(struct sockaddr *) * (numaddr + 1)
              + sizeof(struct sockaddr) * (numaddr + 1));

      addrarray[numaddr] = 0; /* terminator */

      TRACE("Found %d network addresses\n", numaddr);

      for (res = _res; res; res = res->ai_next)
        {
          for (i = 0; i < numaddr; i++)
            {
              TRACE("Ifnum %d: %s\n",
                  i, inet_ntoa(((struct sockaddr_in *)res->ai_addr)->sin_addr));
              addrarray[i] = (struct sockaddr *) &addrarray[numaddr + 1]
                  + i * sizeof(struct sockaddr);
              x_memcpy(addrarray[i], (void *) res->ai_addr,
                  sizeof(struct sockaddr));
            }

        }

      TRACE("Freeing addrinfo list...\n");
      freeaddrinfo(_res);
    }

  EXIT;
  TRACE("Ok!\n");
  return addrarray;
}
#endif /* WIN32 */

/**
 * DOUBLE LINKED LIST IMPLEMENTATION
 */EXPORT_SYMBOL void
xw_list_init(struct list_entry *l)
{
  l->prev = l;
  l->next = l;
}

EXPORT_SYMBOL void
xw_list_insert(struct list_entry *l, struct list_entry *e)
{
  e->prev = l;
  e->next = l->next;
  l->next = e;
  e->next->prev = e;
}

EXPORT_SYMBOL void
xw_list_remove(struct list_entry *e)
{
  e->prev->next = e->next;
  e->next->prev = e->prev;
}

EXPORT_SYMBOL int
xw_list_is_empty(struct list_entry *e)
{
  return e->next == e;
}

/**
 * Attribute list API
 */
static x_obj_attr_t *
get_attr_cell(KEY key, x_obj_attr_t *head)
{
  x_obj_attr_t *p;

  if (!key)
    return NULL;

  for (p = head->next; p; p = p->next)
    {
      if (p->key && EQKEY(p->key, key))
        return p;
    }

  return NULL;
}

int
setattr(KEY k, VAL v, x_obj_attr_t *head)
{
  x_obj_attr_t *p;

  /* delete attribute on no value */
  if (!v)
    {
      delattr(k, head);
      return 0;
    }

  /* is there a link with such a key ? */
  if ((p = get_attr_cell(k, head)) == NULL)
    { /* no */
      p = (x_obj_attr_t *) x_malloc(sizeof(x_obj_attr_t));

      if (!p)
        return -1;

      SETKEY(p->key, k);
      p->next = head->next;
      head->next = p;
    }
  else
    {
      /* yes: change the value */
      FREEVAL(p->val);
    }

  SETVAL(p->val, v);
  return 0;
}

const x_string_t
getattr(KEY key, x_obj_attr_t *head)
{
  x_obj_attr_t *p;

  if (!key || !head)
    return NULL;

  for (p = head->next; p; p = p->next)
    {
      if (p->key && EQKEY(p->key, key))
        return p->val;
    }

  return NULL;
}

int
delattr(KEY k, x_obj_attr_t *head)
{
  x_obj_attr_t *p = NULL, *prev = NULL;

  for (p = head->next, prev = head; p; prev = p, p = p->next)
    if (EQKEY(p->key, k))
      {
        prev->next = p->next;

        FREEVAL(p->val);
        FREEKEY(p->key);
        x_free(p);
        return 0;
      }
  return -1;
}

void
attr_list_clear(x_obj_attr_t *head)
{
  x_obj_attr_t *p, *_p;

  p = head->next;
  head->next = NULL;
  
  for (; p;)
    {
      _p = p->next;
      if (p->val)
      {
        FREEVAL(p->val);
      }
      
      if (p->key)
      {
        FREEKEY(p->key);
      }
      
      x_free(p);      
      p = _p;
    }

}

void
attr_list_init(x_obj_attr_t *head)
{
  head->next = NULL;
  head->key = NULL;
  head->val = NULL;
}

int
attr_list_is_empty(x_obj_attr_t *head)
{
  return !(head->next && (head->next != head));
}

#ifdef ATTRLIST_DEBUG
void print_attr_list(x_obj_attr_t *head)
  {
    x_obj_attr_t *p;

    printf("attrs = [\n");

    for (p = head->next; p;p=p->next)
    printf("'%s'=>'%s',\n",p->key,p->val);

    printf("]\n");

  }
#endif

/**
 * HASH TABLE IMPLEMENTATION
 */

struct ht_cell **
ht_create_table(void)
{
  return (struct ht_cell **) x_malloc(sizeof(struct ht_cell *) * HTABLESIZE);
}

int
ht_hash(KEY key)
{
  unsigned int i = 0;
  char *keysrc = key;
  int hash;

  while (*key)
    {
      i = (i << 1) | (i >> 15); /* ROL */
      i ^= *key++;
    }
  hash = i % HTABLESIZE;

  TRACE("hash(%s)=%d\n", (char *)keysrc, hash);
  return hash;
}

/* ht_get value by key from hashtable */
struct ht_cell *
ht_get_cell(KEY key, struct ht_cell **hashtable, int *indx)
{
  struct ht_cell *p;

  /* pointer must be valid */
  /*assert(indx != NULL);*/
  *indx = ht_hash(key);

  for (p = hashtable[*indx]; p; p = p->next)
    if (EQKEY(p->key, key))
      return p;

  return NULL;
}

/* ht_get value by key from hashtable */
VAL
ht_get(KEY key, struct ht_cell **hashtable, int *indx)
{
  struct ht_cell *p;

  /* pointer must be valid */
  /*assert(indx != NULL);*/
  *indx = ht_hash(key);

  for (p = hashtable[*indx]; p; p = p->next)
    {
      if (EQKEY(p->key, key))
        return p->val;
    }

  return NULL;
}

/* ht_get value by key from hashtable (ignore key case) */
VAL
ht_lowcase_get(KEY key, struct ht_cell **hashtable, int *indx)
{
  struct ht_cell *p;
  char buf[512] = {0};
  char *_b = buf;

  while(*key) *_b++ = tolower(*key++);
  *_b = '\0';

  /* pointer must be valid */
  /*assert(indx != NULL);*/
  *indx = ht_hash(buf);

  for (p = hashtable[*indx]; p; p = p->next)
    {
      if (EQ(p->key, buf))
        return p->val;
    }

  return NULL;
}

/* insert the pair key-value into the table */
void
ht_set(KEY key, VAL val, struct ht_cell **hashtable)
{
  struct ht_cell *p;
  int hash;

  /* is there a link with such a key ? */
  if ((p = ht_get_cell(key, hashtable, &hash)) == NULL)
    { /* no */
      if (!(p = (struct ht_cell *) x_malloc(sizeof(struct ht_cell))))
        return;
      SETKEY(p->key, key);
      p->next = hashtable[hash];
      hashtable[hash] = p;
    }

#ifdef FIXME_AS_SOON_AS_POSSIBLE
  else /* yes: change the value */
  FREEVAL(p->val);
  SETVAL(p->val, val);
#else
  /* val in common is a pointer */
  p->val = val;
#endif
}

/* del pair by key */
int
ht_del(KEY key, struct ht_cell **hashtable)
{
  int indx = ht_hash(key);
  struct ht_cell *p, *prev = NULL;

  if ((p = hashtable[indx]) == NULL)
    return 0;

  for (; p; prev = p, p = p->next)
    if (EQKEY(p->key, key))
      {
#ifdef FIXME_AS_SOON_AS_POSSIBLE
        FREEVAL(p->val);
#endif
        FREEKEY(p->key);
        if (p == hashtable[indx]) /* the head of the list */
          hashtable[indx] = p->next;
        else
          prev->next = p->next;
        x_free((void *) p);
        return 1; /* deleted */
      }
  return 0; /* not found */
}

/* return the next pair */
struct ht_cell *
ht_next(struct ht_celliter *ci, struct ht_cell **hashtable)
{
  struct ht_cell *result;

  while ((result = ci->ptr) == NULL)
    {
      if (++(ci->index) >= HTABLESIZE)
        return NULL; /* no more */
      ci->ptr = hashtable[ci->index];
    }
  ci->ptr = result->next;

  return result;
}

/* iterator initalizer */
struct ht_cell *
ht_nullifyiter(struct ht_celliter *ci, struct ht_cell **hashtable)
{
  ci->index = -1;
  ci->ptr = NULL;
  return ht_next(ci, hashtable);
}

#ifdef HTABLE_DEBUG

/* print pair */
void ht_printcell(struct ht_cell *ptr)
  {
    putchar('(');
    printf( KEYFMT, ptr->key );
    putchar(',');
    printf( VALFMT, ptr->val );
    putchar(')');
  }

/* print table */
void ht_printtable(struct ht_cell **hashtable)
  {
    struct ht_cell *p;
    int i;

    printf("----TABLE CONTENTS----\n");
    for (i=0; i < HTABLESIZE; i++)
    if ((p = hashtable[i]) != NULL)
      {
        printf( "%d: ", i);
        for (; p; p=p->next)
        ht_printcell(p), putchar(' ');
        putchar('\n');
      }
  }

/* Usage example: table of filename-size pairs from the current dir */
void ht_testtable(void)
  {
    struct ht_cell **htable;
    struct ht_celliter ci;
    struct ht_cell *cl;
    char key[40], value[40];
    struct ht_cell *val;
    FILE *fp;
    char *s;
    int hash;

    htable = ht_create_table();
    if (htable == NULL)
      {
        printf("unable to create hash table - no memory left\n");
        return;
      }

    /* popen() ht_get the output of the command from the 1st arg */
    fp = popen( "ls -s", "r" );
    while ( fscanf( fp, "%s%s", value, key) == 2 )
    ht_set(key, value, htable);
    pclose(fp);

    /* proccess user input */
    for (;;)
      {
        printf( "command: " );

        /* EOF */
        if (!gets(key))
        break;

        /* -KEY :delete     */
        if (*key == '-')
          {
            printf(ht_del(key+1, htable) ? "OK\n" : "not in the table\n");
            continue;
          }

        /* = :print table */
        if ( !*key || !strcmp(key, "="))
          {
            ht_printtable(htable);
            continue;
          }

        /* KEY=VALUE :add */
        if(s = strchr(key, '='))
          {
            *s++ = '\0';
            ht_set(key, s, htable);
            continue;
          }

        /* KEY :find value */
        if ((val = ht_get(key, htable, &hash)) == NULL)
        printf("no such key\n");
        else
          {
            printf(VALFMT, val->val);
            putchar('\n');
          }
      }

    for (cl = ht_nullifyiter(&ci, htable); cl; cl = ht_next(&ci, htable))
    ht_printcell(cl), putchar('\n');
  }

#endif /*HTABLE_DEBUG */

int
set_sock_to_non_blocking(int s)
{
  int z;
  int res = -1;
#ifdef WIN32
  u_long iMode = 1;
  res = ioctlsocket(s, FIONBIO, &iMode);
#else
  z = fcntl(s, F_GETFL, 0);
  fcntl(s, F_SETFL, z | O_NONBLOCK);
#endif
  return res;
}

int
unset_sock_to_non_blocking(int s)
{
  int z;
  int res = -1;
#ifdef WIN32
  u_long iMode = 0;
  res = ioctlsocket(s, FIONBIO, &iMode);
#else
  z = fcntl(s, F_GETFL, 0);
  fcntl(s, F_SETFL, z & ~O_NONBLOCK);
#endif
  return res;
}

int
x_thread_run(THREAD_T *tid, t_func func, void *fdata)
{
  int err = -1;
#ifndef __DISABLE_MULTITHREAD__
#       ifdef WIN32
  tid = _beginthread(func, 0, fdata);
  if (!tid)
    {
      err = -1;
    }
  else
    {
      err = 0;
    }
#       else
  err = pthread_create(tid, NULL, func, fdata);
#       endif
#else /* __DISABLE_MULTITHREAD__ */
#       error "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
#endif /* __DISABLE_MULTITHREAD__ */
  return err;
}


/**
 * x_stun_server.c
 *
 *  Created on: Jan 17, 2012
 *      Author: artemka
 */

#undef DEBUG_PRFX
#define DEBUG_PRFX "[x_stunserver] "
#include <xwlib/x_types.h>
#include <xwlib/x_obj.h>
#include <xwlib/x_utils.h>

#include "x_stun.h"

typedef struct x_stun_server
{
  x_object xobj;
  struct
  {
    int state;
    uint32_t cr0;
  } regs;
} x_stunserver;

EXPORT_SYMBOL void
x_lib_print_netaddr(struct sockaddr *addr)
{
  char strbuf[X_LIB_MAX_NETADDRLEN];
  uint16_t portnum = 0;

  if (addr->sa_family == AF_INET)
    portnum = ((struct sockaddr_in *) addr)->sin_port;
  else if (addr->sa_family == AF_INET6)
    portnum = ((struct sockaddr_in6 *) addr)->sin6_port;

  portnum = ntohs(portnum);

  strbuf[X_LIB_MAX_NETADDRLEN - 1] = 0;
  x_inet_ntop(addr->sa_family, (void *) addr,
      &strbuf[0], X_LIB_MAX_NETADDRLEN - 1);
  TRACE("NETWORK ADDR: %s:%d\n", strbuf, ntohs(portnum));

  return;
}

/**
 * @param hostname Target hostname for shich address is discovered
 * @param family Address family hint (ex. AF_INET, AF_INET6 etc..)
 * @param socktype Socket type hint (ex. SOCK_STREAM, SOCK_DGRAM, ... etc.)
 * @param dstaddr Caller's address holder
 * @param adrlen Caller set length of address holder
 */
EXPORT_SYMBOL int
x_lib_getaddrinfo(const char *hostname, const char *service, int family,
    int socktype, struct sockaddr *dstadr, socklen_t *adrlen)
{
  struct addrinfo hints;
  struct addrinfo *res = NULL;
  int err = -1;

  ENTER;

  if (!dstadr || !adrlen || !*adrlen)
    {
      TRACE("Invalid param!\n");
      return -EINVAL;
    }

  x_memset(&hints, 0, sizeof(struct addrinfo));

  /* set-up hints structure */
  hints.ai_family = family;
  hints.ai_socktype = socktype;

  TRACE("GETTING ADDRESS FOR: '%s:%s'\n", hostname, service);

  err = getaddrinfo(hostname, service, &hints, &res);
  if (err)
    {
      TRACE("ERROR!! getaddrinfo(): %s", gai_strerror(err));
      goto early_exit;
    }

  TRACE("GOT ADDRESS FOR: '%s:%s'\n", hostname, service);

  *adrlen = (*adrlen > res->ai_addrlen) ? *adrlen : res->ai_addrlen;
  x_memcpy(dstadr, res->ai_addr, (size_t) *adrlen);

  if (res)
    freeaddrinfo(res);
  early_exit:
  EXIT;
  return err;
}

/**
 * @todo This function should be called by timer and thus \
 * must save/restore its context each time it is fired.
 */
static void UNUSED
x_stunserver_proto_start(x_object *xobj)
{
  int i = 0;
  struct iovec iov[4];
  const char *srv = NULL;
  struct sockaddr dst_addr;
  socklen_t alen = sizeof(struct sockaddr);
  x_obj_attr_t attrs =
    { 0, 0, 0 };
  stun_hdr_t *req = NULL;
  struct sockaddr **local_addrs;
  const char *_stunport_s;

  ENTER;

  /*  local_addrs = icectl_get_local_ips();*/
#ifdef ANDROID
  local_addrs = __x_bus_get_local_addr_list();
#else
  local_addrs = __x_bus_get_local_addr_list2();
#endif

    {
      srv = x_object_getenv(xobj, "stunserver");
      _stunport_s = x_object_getenv(xobj, "stunport");

      TRACE("STUN SERVER IN USE: '%s:%s'\n", srv, _stunport_s);

      if (srv)
        {
          x_lib_getaddrinfo(srv, _stunport_s, AF_INET, 0, &dst_addr, &alen);
          x_lib_print_netaddr(&dst_addr);

          if (!((struct sockaddr_in *) &dst_addr)->sin_port)
            {
              ((struct sockaddr_in *) &dst_addr)->sin_port
                  = STUN_PORT_DEFAULT_N;
            }

          req = get_stun_simple_request();

          iov[2].iov_base = (void *) req;
          iov[2].iov_len = (size_t) (ntohs(req->mlen) + sizeof(stun_hdr_t));
          iov[0].iov_base = (void *) &dst_addr;
          iov[0].iov_len = sizeof(struct sockaddr_in);
        }
    }

    {
      if (req)
        {
          /* set control type attribute */
          setattr(X_STRING("type"), X_STRING("control"), &attrs);

          while (local_addrs[i])
            {
              iov[1].iov_base = local_addrs[i];
              iov[1].iov_len = sizeof(struct sockaddr_in);

              /* send */
              x_object_try_writev(x_object_get_parent(xobj), &iov[0],
                  sizeof(iov) / sizeof(struct iovec), NULL);

              /* send on control port */
              x_object_try_writev(x_object_get_parent(xobj), &iov[0],
                  sizeof(iov) / sizeof(struct iovec), &attrs);

              i++;
            }

          x_free(req);
        }
    }

  x_free(local_addrs);

  EXIT;
}

static void
x_stunserver_on_local_candidate(x_object *o, const char *addr_str,
    unsigned int portnum)
{
  char portbuf[32];
  x_object *tmp;

  ENTER;

  tmp = x_object_new("candidate");
  /*
   x_object_setattr("rel-addr", inet_ntoa(to));
   x_object_setattr("rel-port", ntohs(portnum));
   */
  x_object_setattr(tmp, "ip", addr_str);
  x_snprintf(portbuf, (sizeof(portbuf)) - 1, "%d", ntohs(portnum));
  x_object_setattr(tmp, "port", portbuf);

  /* send new gathered candidate to the otherend */
  x_object_send_down(x_object_get_parent(o), tmp, NULL);

  EXIT;
}

/**
 * @todo __icectl_route_stun_packet() need to be refactored \
 * and splitted into small functions
 */
static void
x_stunserver_route_stun_packet(x_object *o, struct sockaddr *to,
    socklen_t tolen, struct sockaddr *from, socklen_t *fromlen,
    stun_hdr_t *_stun_hdr)
{
  stun_packet_t *_stun_pkt;

  ENTER;
  _stun_pkt = stun_packet_from_hdr(_stun_hdr, NULL);

  switch (ntohs(_stun_hdr->ver_type.raw))
    {
  case STUN_RESP:
    {
      int anum = 0; /* attribute counter */
      char trans_id[STUN_TRANS_ID_LEN + 1];

      for (; anum < _stun_pkt->attrnum; anum++)
        {
          switch (_stun_pkt->attrs[anum].type)
            {
          case 0x4:
          case 0x5:
            break;
          case 0x1:
            {
              uint16_t portnum = *(uint16_t *) &_stun_pkt->attrs[anum].data[2];

              struct in_addr mapaddr =
                  *(struct in_addr *) &_stun_pkt->attrs[anum].data[4];

              x_strncpy(trans_id, _stun_hdr->trans_id, STUN_TRANS_ID_LEN);

              TRACE ("My mapped address: %s:%d (trans_id='%s')\n",
                  inet_ntoa(mapaddr),ntohs(portnum),trans_id);

              x_stunserver_on_local_candidate(o, inet_ntoa(mapaddr), ntohs(
                  portnum));
            }
            break;
          default:
            break;
            }
        }
    }

    break;

  case STUN_REQ:
    TRACE("Request from %s\n",
        inet_ntoa(((struct sockaddr_in *)from)->sin_addr)
    );
    break;
    }

  x_free(_stun_pkt);

  EXIT;
}

/**
 * simple stun packet detector
 */
static __inline BOOL
__is_stun_packet(void *buf, uint32_t len)
{
  stun_hdr_t *_stun_hdr = (stun_hdr_t *) (void *) buf;

  ENTER;
  if (/* _stun_hdr->ver_type.v == 0 && */_stun_hdr->mcookie
      == STUN_NET_M_COOKIE)
    {
      EXIT;
      return TRUE;
    }
  EXIT;
  return FALSE;
}

/**
 * This takes newly received stun packet data.
 * The iov vector should contain at least two
 * records, where first record is treated as
 * a source networking adddress of a packet.
 *
 * @param o stun server x_object
 * @param iov data vector
 * @param iovcnt number of records in iov
 * @param attr additional attributes
 *
 */
static int
x_stunserver_try_writev(x_object *o, const struct iovec *iov, int iovcnt,
    x_obj_attr_t *attr)
{
  ENTER;

  if (iovcnt < 2)
    {
      EXIT;
      return -1;
    }

#if 1
  if (!__is_stun_packet(iov[2].iov_base, iov[2].iov_len))
    {
      EXIT;
      BUG();
      return -1;
    }
#endif

  x_stunserver_route_stun_packet(o,
      (struct sockaddr *) (void *) iov[0].iov_base, (socklen_t) iov[0].iov_len,
      (struct sockaddr *) (void *) iov[1].iov_base,
      (socklen_t *) &(iov[1].iov_len), (stun_hdr_t *) (void *) iov[2].iov_base);

  EXIT;

  return 1;
}

static BOOL
x_stunserver_equals(x_object *o, x_obj_attr_t *_o)
{
  ENTER;
  return TRUE;

}

static int
x_stunserver_on_append(x_object *so, x_object *parent)
{
  ENTER;
  x_stunserver_proto_start(so);
  EXIT;
  return 0;
}

static void
x_stunserver_on_child_append(x_object *o, x_object *child)
{
  ENTER;
  if (EQ(x_object_get_name(child),"candidate"))
    {

    }
  EXIT;
  return;
}

static x_object *
x_stunserver_on_assign(x_object *o, x_obj_attr_t *attrs)
{
  ENTER;

  EXIT;
  return o;
}

static void
x_stunserver_exit(x_object *o)
{
  ENTER;
  printf("%s:%s():%d\n",__FILE__,__FUNCTION__,__LINE__);
  EXIT;
}

static struct xobjectclass x_stunserver_class;

__x_plugin_visibility
__plugin_init void
x_stunserver_init(void)
{
  ENTER;

  x_memset(&x_stunserver_class, 0, sizeof(x_stunserver_class));

  x_stunserver_class.cname = X_STRING("__stunserver");
  x_stunserver_class.psize = sizeof(struct x_stun_server) - sizeof(x_object);
  x_stunserver_class.match = &x_stunserver_equals;
  x_stunserver_class.on_assign = &x_stunserver_on_assign;
  x_stunserver_class.on_append = &x_stunserver_on_append;
  x_stunserver_class.on_child_append = &x_stunserver_on_child_append;
  x_stunserver_class.finalize = &x_stunserver_exit;
  x_stunserver_class.try_writev = &x_stunserver_try_writev;

  TRACE("Sizeof struct x_stun_server %d bytes,"
      " x_object size %d bytes\n",
      (int)sizeof(struct x_stun_server),
      (int)sizeof(x_object));

#if 1
  x_class_register_ns(x_stunserver_class.cname, &x_stunserver_class, "jingle");
#else
  x_class_register(x_stunserver_class.cname, &x_stunserver_class);
#endif
  EXIT;
}

PLUGIN_INIT(x_stunserver_init);


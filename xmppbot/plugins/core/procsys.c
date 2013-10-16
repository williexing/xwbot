/*
 * bind.c
 *
 *  Created on: Jul 28, 2011
 *      Author: artemka
 */
#undef DEBUG_PRFX
#include <x_config.h>
#if TRACE_PROCSYS_ON
#define DEBUG_PRFX "[procsys] "
#endif
#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include <xmppagent.h>


static void
__procsys_collect_network_ifaces(x_object *netdir)
{
  int i = 0;
  char addrstrbuf[64];
  x_object *tmp;
  struct sockaddr **local_addrs;

  ENTER;

  _REFGET(netdir);

#ifdef ANDROID
  local_addrs = __x_bus_get_local_addr_list();
#else
  local_addrs = __x_bus_get_local_addr_list2();
#endif

  for (i = 0; local_addrs[i]; i++)
    {
      void *src = NULL;
      socklen_t len;

      if (local_addrs[i]->sa_family == AF_INET)
        {
          struct sockaddr_in *ip4addr = (struct sockaddr_in *) (void *) local_addrs[i];
          if (ip4addr->sin_addr.s_addr == INADDR_LOOPBACK
              || ip4addr->sin_addr.s_addr == htonl(INADDR_LOOPBACK))
            continue;

          src = (void *) &ip4addr->sin_addr;
          len = sizeof(struct sockaddr_in);
        }
      else if (local_addrs[i]->sa_family == AF_INET6)
        {
          struct sockaddr_in6 *ip6addr = (struct sockaddr_in6 *) (void *) local_addrs[i];
          src = (void *) &ip6addr->sin6_addr;
          len = sizeof(struct sockaddr_in6);
        }
      else
        {
          continue;
        }

      x_inet_ntop(local_addrs[i]->sa_family, src, addrstrbuf, len);

      /* dataport candidate */
      tmp = _GNEW(_XS("$interface"),NULL);
      BUG_ON(!tmp);
      TRACE("ADDING IP ADDRESS '%s'\n", &addrstrbuf[0]);
      _ASET(tmp, _XS("ip"), &addrstrbuf[0]);
      _ASET(tmp, _XS("type"), (local_addrs[i]->sa_family == AF_INET6)
          ? _XS("IPv6") : _XS("IPv4"));

      _INS(netdir, tmp);

    }

//  x_free(local_addrs);

  _REFPUT(netdir,NULL);

  EXIT;
}

static void
___procsys_on_x_path_event(void *_obj)
{
  x_object *tmpo;
//  x_object *obj = _obj;

  ENTER;

  if (_obj)
    {
      tmpo = x_object_add_path(_obj, _XS("$networking"), NULL);
      __procsys_collect_network_ifaces(tmpo);
    }

  EXIT;
}

static struct x_path_listener procsys_bus_listener;

__x_plugin_visibility
__plugin_init void
procsys_init(void)
{

//  presence_class.cname = "networking";
//  presence_class.on_append = &presence_on_append;
//  x_class_register(presence_class.cname, &presence_class);

  procsys_bus_listener.on_x_path_event = &___procsys_on_x_path_event;

  x_path_listener_add("__proc", &procsys_bus_listener);
  return;
}

PLUGIN_INIT(procsys_init)
;


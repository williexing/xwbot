/*
 * Copyright (c) 2010, Artur Emagulov
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
 * Created on: Sep 21, 2010
 *     Author: Artur Emagulov
 *
 */

#include <stdio.h>
#include <string.h>

/*
 #include <alloca.h>
 #include <strings.h>
 */

#undef DEBUG_PRFX
#include <xwlib/x_config.h>
#if TRACE_XDISCO_ON
#define DEBUG_PRFX "[bus] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>

#include <xmppagent.h>

struct discovery_object
{
  x_object super;
  struct
  {
    int state;
    int id_counter;
    u_int32_t cr0;
  } regs;
};

/* matching function */
static BOOL
___disco_query_match(x_object *o, x_obj_attr_t *_o)
{
  const char *xmlns;
  ENTER;

  xmlns = getattr("xmlns", _o);
  if ((x_strncasecmp(xmlns, "http://jabber.org/protocol/disco",
      strlen("http://jabber.org/protocol/disco")) == 0))
    return TRUE;
  else
    return FALSE;
}

static x_object *
___get_query_response(x_object *procdir, int reqtype, const char *item,
    const char *node)
{
  x_object *msg = NULL;
  x_object *tmp, *ctx;
  x_object *itemnode;
  char *xmlns;
  char *_node = (char *) node;

  const char *jid = x_object_getenv(procdir, "jid");

  ENTER;
  TRACE("%p,jid=%s,%s,%s\n", (void *)procdir, jid, item, node);

  msg = _NEW("query","gobee");

  xmlns = alloca((item ? strlen(item) : 0) + strlen(
          "http://jabber.org/protocol/disco") + 2);
  sprintf(xmlns, "http://jabber.org/protocol/disco#%s", item ? item : "");
  _ASET(msg, "xmlns", xmlns);
  _ASET(msg, "node", node);

  if (!x_strcasecmp(_node, "http://jabber.org/protocol/commands"))
    _node = "commands/";

  ctx = x_object_from_path(procdir, _node);
  TRACE("procdir='%s'\n", _GETNM(procdir));
  TRACE("commands name='%s'\n", _GETNM(ctx));

  /* add each proc entry */
  for (tmp = _CHLD(ctx, NULL); tmp; tmp = _SBL(tmp))
    {
      itemnode = _NEW("item","gobee");
      //x_object_setattr(itemnode, "jid", jid);
      _ASET(itemnode, "node", _GETNM(tmp));
      _ASET(itemnode, "name", _AGET(tmp, "name"));
      _ASET(itemnode, "jid", _ENV(procdir, "jid"));

      _INS(msg, itemnode);
    }

  EXIT;
  return msg;
}

/* assignment handler */
static x_object *
___disco_query_on_assign(x_object *o, x_obj_attr_t *attrs)
{
  const char *xmlns;
  const char *attr;
  x_object *feature, *query;
  x_object *msg;
  char *item;
  ENTER;

  if (!o->bus)
    TRACE("Unknown bus => %p\n", o->bus);

  xmlns = getattr("xmlns", attrs);
  TRACE("Item request -> %s\n", xmlns);

  if (!xmlns)
    goto __assignout;

  item = x_strchr(xmlns, '#');
  if (!item)
    goto __assignout;

  item++;

  if (EQ(item, "info"))
    {
      if (attr = getattr("node", attrs))
        {
          TRACE("Object name -> %s\n", _GETNM(o));
          TRACE("Bus name -> %s\n", _GETNM(o->bus));
          TRACE("Node name -> %s\n", attr);

          msg = _NEW("iq","gobee");
          _ASET(msg, "type", "result");
          _ASET(msg, "from", _ENV(X_OBJECT(o), "jid"));
          _ASET(msg, "to", _ENV(X_OBJECT(o), "from"));
          _ASET(msg, "id", _ENV(X_OBJECT(o), "id"));

          query = _NEW("query","gobee");
          _ASET(query, "xmlns", xmlns);

          /**
           * @todo Fixme ugly hack
           */
          if (x_strstr(attr, "video-v1"))
            {
              feature = _NEW("feature","caps");
              _ASET(feature, "var",
                  "http://www.google.com/xmpp/protocol/video/v1");
              _INS(query, feature);
            }
          else if (x_strstr(attr, "voice-v1"))
            {
              feature = _NEW("feature","caps");
              _ASET(feature, "var",
                  "http://www.google.com/xmpp/protocol/voice/v1");
              _INS(query, feature);
            }
          else
            {

            }

          _INS(msg, query);
          x_object_send_down(X_OBJECT(o->bus), msg, NULL);
        }
    }
  else if (EQ(item, "items"))
    {
      attr = getattr("node", attrs);
      TRACE("Object name -> %s\n", x_object_get_name(o));
      TRACE("Bus name -> %s\n", x_object_get_name(o->bus));

      msg = _NEW("iq","gobee");
      _ASET(msg, "type", "result");
      _ASET(msg, "from", _ENV(X_OBJECT(o), "jid"));
      _ASET(msg, "to", _ENV(X_OBJECT(o), "from"));
      _ASET(msg, "id", _ENV(X_OBJECT(o), "id"));

      _INS(msg,
          ___get_query_response(x_object_get_child( o->bus, "__proc"), 0, item, attr));

      x_object_send_down(X_OBJECT(o->bus), msg, NULL);
    }

  EXIT;
  __assignout: return o;
}

static void
___disco_query_exit(x_object *this__)
{
  //  struct discovery_object *ft = (struct discovery_object *) this__;
  ENTER;
  printf("%s:%s():%d\n",__FILE__,__FUNCTION__,__LINE__);
  EXIT;
}

static struct xobjectclass disco_query_class;

static void
___on_x_path_event(void *_obj)
{
  x_object *obj = _obj;
  ENTER;
  TRACE("Event on '%s'\n", x_object_get_name(obj));
  x_object_mkpath(obj, "__proc");
  x_object_mkpath(obj, "__usr");
  x_object_mkpath(obj, "__var");
  x_object_mkpath(obj, "__lib");
  x_object_mkpath(obj, "__sys");
  EXIT;
}

static struct x_path_listener x_bus_listener;

__x_plugin_visibility
__plugin_init void
disco_proto_init(void)
{
  disco_query_class.cname = "query";
  disco_query_class.psize = (unsigned int) (sizeof(struct discovery_object)
      - sizeof(x_object));
  disco_query_class.match = &___disco_query_match;
  disco_query_class.on_assign = &___disco_query_on_assign;
  disco_query_class.finalize = &___disco_query_exit;

  x_class_register_ns(disco_query_class.cname, &disco_query_class,
      "http://jabber.org/protocol/disco");

  x_bus_listener.on_x_path_event = &___on_x_path_event;

  x_path_listener_add("/", &x_bus_listener);
  return;
}

PLUGIN_INIT(disco_proto_init)
;

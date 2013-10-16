/*
 * bind.c
 *
 *  Created on: Jul 28, 2011
 *      Author: artemka
 */

#include <stdio.h>
#include <string.h>

#undef DEBUG_PRFX
#define DEBUG_PRFX "[bind] "
#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>

#include <xmppagent.h>

/* matching function */
static BOOL
bind_match(x_object *o, x_obj_attr_t *_o)
{
  const char *xmlns;
  ENTER;

  xmlns = getattr("xmlns", _o);
  if (xmlns && EQ(xmlns,"urn:ietf:params:xml:ns:xmpp-bind"))
    return TRUE;
  else
    return FALSE;
}

enum
{
  BIND_SENT = 1, BIND_OK = 2,
};

static int
bind_on_append(x_object *so, x_object *parent)
{
  ENTER;

  EXIT;
  return 0;
}

static x_object *
bind_on_assign(x_object *this__, x_obj_attr_t *attrs)
{
  int id;
  x_string_t val;
  x_object *msg;
  x_object *tmp;
  struct x_bus *bus = (struct x_bus *) this__->bus;
  ENTER;

//  val = (int) ht_get("bind", &bus->wall[0], &id);
  val = _AGET(this__,"$bind_state");
  if (val && EQ(val,"BIND_SENT"))
    {

    }
  else if (val && EQ(val,"BIND_OK"))
    {

    }
  else
    {
      msg = _GNEW("bind",NULL);
      _ASET(msg, "xmlns", "urn:ietf:params:xml:ns:xmpp-bind");

      tmp = _GNEW("resource",NULL);
      x_string_write(&tmp->content, _ENV(this__,"resource"),
          x_strlen(_ENV(this__,"resource")));

      _INS(msg, tmp);

      tmp = _GNEW("iq",NULL);
      _ASET(tmp, "type", "set");
      _ASET(tmp, "id", "bind_1");
      _INS(tmp, msg);

//      ht_set("bind", (VAL) BIND_SENT, &bus->wall[0]);
      _ASET(this__, "$bind_state", "BIND_SENT");
      x_object_send_down(X_OBJECT(bus), tmp, NULL);
      _REFPUT(tmp, NULL);
    }

  EXIT;
  return this__;
}

static void
bind_exit(x_object *this__)
{
  x_object *msg;
  x_object *_m;
  x_object *tmp;
  x_object *o;
  x_string_t state;
  char *ptr;
  struct x_bus *bus = (struct x_bus *) this__->bus;
  ENTER;

  state = _AGET(this__,"$bind_state");

  if (state && EQ(state,"BIND_SENT"))
    {
      o = _CHLD(this__, "jid");
      if (o)
        {
          _ASET(this__, "$bind_state", "BIND_OK");

          TRACE("New JID:=\"%s\"\n", o->content.cbuf);

          /* set new xmppbot jid */
          _ASET(X_OBJECT(bus), "jid", o->content.cbuf);

          /* update resource name */
          if ((ptr = x_strrchr(o->content.cbuf, '/')) != NULL)
            _ASET(X_OBJECT(bus), "resource", ++ptr);

          tmp = _CHLD(X_OBJECT(bus), "stream:stream");
          if (tmp)
            {
              /* send session initiate */
              _m = _GNEW("iq",NULL);
              _ASET(_m, "type", "set");
              _ASET(_m, "id", "bind_2");

              msg = _GNEW("session",NULL);
              _ASET(msg, "xmlns", "urn:ietf:params:xml:ns:xmpp-session");
              _INS(_m, msg);

              x_object_send_down(tmp, _m, NULL);
              _REFPUT(_m, NULL);

              /* append presence object to stream */
              msg = x_object_new("presence");
              x_object_append_child(tmp, msg);
            }
        }
    }

  EXIT;
}

static struct xobjectclass bind_class;

__x_plugin_visibility
__plugin_init void
bind_init(void)
{

  bind_class.cname = "bind";
  bind_class.psize = 0;
  bind_class.match = &bind_match;
  bind_class.on_assign = &bind_on_assign;
  bind_class.on_append = &bind_on_append;
  bind_class.commit = &bind_exit;

  x_class_register_ns(bind_class.cname, &bind_class,
      "urn:ietf:params:xml:ns:xmpp-bind");
  return;
}
PLUGIN_INIT(bind_init)
;


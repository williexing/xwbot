/*
 * bind.c
 *
 *  Created on: Jul 28, 2011
 *      Author: artemka
 */
#undef DEBUG_PRFX

#include <x_config.h>
#if TRACE_PRESENCE_ON
#define DEBUG_PRFX "[presence] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include <ev.h>

#include <xmppagent.h>

typedef struct xpresence
{
  x_object obj;
  struct ev_timer mytimer;
} x_presence;

/**
 * Global 'presence' entry in app's internal
 * proc directory
 */
static x_object *proc_presence_entry = NULL;

static void
__keepalive_cb(xevent_dom_t *loop, struct ev_timer *w, int revents)
{
  x_object *pres = X_OBJECT(w->data);
  struct x_bus *bus = (struct x_bus *) pres->bus;
  ENTER;
  x_bus_write(bus, " ", 1, 0);
  EXIT;
}

/* matching function */
static BOOL
presence_match(x_object *o, x_obj_attr_t *_o)
{
  /*const char *xmlns;*/
  ENTER;

  EXIT;
  return TRUE;
}

static int
presence_on_append(x_object *so, x_object *parent)
{
  x_presence *pres = (x_presence *) so;
  x_object *msg, *tmp;
  struct x_bus *bus = (struct x_bus *) parent->bus;

  ENTER;

  TRACE("MY PARENT IS '%s'\n",x_object_get_name(parent));

  pres->mytimer.data = (void *) so;
  ev_timer_init(&pres->mytimer, __keepalive_cb, 0., 5.);
  ev_timer_start(bus->obj.loop, &pres->mytimer);

  msg = _GNEW("$message",NULL);
  _SETNM(msg, "presence");

  /* @todo use presence status from __proc/presence
   * created by ___presence_on_x_path_event()
   */
  tmp = _GNEW("$message",NULL);
  _SETNM(tmp, "priority");

  x_object_set_content(tmp, "50", 2);
  x_object_append_child_no_cb(msg, tmp);

  tmp = _GNEW("$message",NULL);
  _SETNM(tmp, "c");

  _ASET(tmp, "xmlns", "http://jabber.org/protocol/caps");
  _ASET(tmp, "node", "http://xmppbot.gobee.org/caps");
  _ASET(tmp, "ver", "0.0001");
  _ASET(tmp, "ext",
      "ca cs cv e-time ep-notify-2 html last-act"
        "mr sxe whiteboard camera-v1 video-v1 voice-v1");
  x_object_append_child_no_cb(msg, tmp);

  x_object_send_down(parent, msg, NULL);
  _REFPUT(msg, NULL);
  
  EXIT;
  return 0;
}

static x_object *
presence_on_assign(x_object *this__, x_obj_attr_t *attrs)
{
  ENTER;
  x_object_default_assign_cb(this__, attrs);
  EXIT;
  return this__;
}

void
presence_on_child_append(x_object *this__, x_object *child)
{
    ENTER;
    EXIT;
}

void
presence_on_child_remove(x_object *this__, x_object *child)
{
  ENTER;
  EXIT;
}

static void
presence_exit(x_object *this__)
{
  x_object *msg;
  const char *from;
  ENTER;

  from = x_object_getattr(this__, "from");

  if (from)
    {
//      TRACE("Presence from '%s'\n",from);
      if (proc_presence_entry == x_object_from_path(proc_presence_entry, from))
        {
          x_object_add_path(proc_presence_entry, from, NULL);
          msg = _GNEW("$message",NULL);
          _ASET(msg, "subject", "presence-update");
          _ASET(msg, "jid", from);
          _MCAST(this__, msg);
          _REFPUT(msg, NULL);
        }
      else
        {
          TRACE("Presence '%s' already exists\n",from);
        }
    }

  _REFPUT(_CHLD(this__, "c"),NULL);
  _REFPUT(_CHLD(this__, "priority"),NULL);

  EXIT;
}

static struct xobjectclass presence_class;

static void
___presence_on_x_path_event(void *_obj)
{
  x_object *obj = _obj;

  ENTER;

  /**
   * @todo Fixme Use some x_object_get_root()
   * for example but not obj->bus.
   * This ugly hack should be removed when correct path
   * notification mechanism will be completed.
   */
  if (obj->bus)
    proc_presence_entry = x_object_add_path(obj->bus, "__proc/presence", NULL);

  EXIT;
}

static struct x_path_listener presence_bus_listener;

__x_plugin_visibility
__plugin_init void
presence_init(void)
{

  presence_class.cname = "presence";
  presence_class.psize = (sizeof(x_presence) - sizeof(x_object));
  presence_class.match = &presence_match;
  presence_class.on_assign = &presence_on_assign;
  presence_class.on_append = &presence_on_append;
  presence_class.on_child_append = &presence_on_child_append;
  presence_class.on_child_remove = &presence_on_child_remove;
  presence_class.commit = &presence_exit;

  x_class_register(presence_class.cname, &presence_class);

  presence_bus_listener.on_x_path_event = &___presence_on_x_path_event;

  /**
   * Listen for 'session' object from "urn:ietf:params:xml:ns:xmpp-session"
   * namespace. So presence will be activated just after session xmpp is
   * activated.
   */
  x_path_listener_add("session", &presence_bus_listener);
  return;
}

PLUGIN_INIT(presence_init)
;


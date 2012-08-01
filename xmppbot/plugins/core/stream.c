/*
 * stream.c
 *
 *  Created on: Aug 12, 2011
 *      Author: artemka
 */

#undef DEBUG_PRFX
#define DEBUG_PRFX "[stream] "
#include <xwlib/x_types.h>

#include <xmppagent.h>

struct xmpp_stream
{
  x_object super;
  struct
  {
    int state;
    u_int32_t cr0;
  } regs;
};

static void
__stream_publish(struct xmpp_stream *stm);

/* matching function */
static int
stream_match(x_object *o, x_obj_attr_t *_o)
{
  const char *a1, *a2;
  ENTER;

  a1 = x_object_getattr(o, "remote");
  a2 = getattr("from", _o);
  if (a2 && EQ(a1,a2))
    return TRUE;

  EXIT;
  return FALSE;
}

/* destructor */
static void
stream_create(x_object *o)
{
  struct xmpp_stream *stm = (struct xmpp_stream *) o;
  ENTER;
  stm->regs.cr0 = 0;
  EXIT;
  return;
}

/* handler of append event (appears when object is appended to its parent) */
static int
stream_append(x_object *so, x_object *parent)
{
  struct xmpp_stream *stm = (struct xmpp_stream *) so;
  ENTER;

  x_object_setattr(so, "xmlns", "jabber:client");
  x_object_setattr(so, "xmlns:stream", "http://etherx.jabber.org/streams");
  x_object_setattr(so, "version", "1.0");
  x_object_setattr(so, "xml:lang", "en");
  x_object_setattr(so, "xmlns:xml", "http://www.w3.org/XML/1998/namespace");

  __stream_publish(stm);

  EXIT;
  return 0;
}

/* handler of removing event (appears when object is removing from its parent) */
static void
stream_remove(x_object *o)
{
  ENTER;
  EXIT;
}

/* default constructor */
static void
stream_release(x_object *o)
{
  ENTER;
  EXIT;
}

static void
__stream_publish(struct xmpp_stream *stm)
{
  struct x_bus *bus = (struct x_bus *) X_OBJECT(stm)->bus;
  x_string xstr =
    { NULL, 0, 0 };
  x_obj_attr_t *at;

  x_string_write(&xstr, "<?xml version='1.0'?>", x_strlen(
      "<?xml version='1.0'?>"));

  x_string_write(&xstr, "<", 1);
  x_string_write(&xstr, x_object_get_name(X_OBJECT(stm)), x_strlen(
      x_object_get_name(X_OBJECT(stm))));
  x_string_write(&xstr, ":", 1);
  x_string_write(&xstr, x_object_get_name(X_OBJECT(stm)), x_strlen(
      x_object_get_name(X_OBJECT(stm))));

  // write attributes
  for (at = ((x_object *) stm)->attrs.next; at; at = at->next)
    {
      x_string_write(&xstr, " ", 1);
      if (!x_strncasecmp("remote", at->key, 5))
        {
          x_string_write(&xstr, "to", 2);
        }
      else if (!x_strncasecmp("local", at->key, 5))
        {
          x_string_write(&xstr, "from", 4);
        }
      else
        {
          x_string_write(&xstr, at->key, x_strlen(at->key));
        }
      x_string_write(&xstr, "='", 2);
      if (at->val)
        x_string_write(&xstr, at->val, x_strlen(at->val));
      x_string_write(&xstr, "'", 1);
    }
  x_string_write(&xstr, ">", 1);

  // copy to stdout
  x_bus_write(bus, xstr.cbuf, xstr.pos, 0);
  x_string_trunc(&xstr);

}

/* assignment handler */
static x_object *
stream_assign(x_object *o, x_obj_attr_t *attrs)
{
  ENTER;
  EXIT;
  return o;
}

static void
stream_exit(x_object *o)
{
  printf("%s:%s():%d\n",__FILE__,__FUNCTION__,__LINE__);
}

static struct xobjectclass stream_class;

__x_plugin_visibility
__plugin_init void
xmppstream_init(void)
{
  ENTER;
  stream_class.cname = "stream";
  stream_class.psize = (unsigned int) (sizeof(struct xmpp_stream)
      - sizeof(x_object));
  stream_class.match = &stream_match;
  stream_class.on_create = &stream_create;
  stream_class.on_append = &stream_append;
  stream_class.on_remove = &stream_remove;
  stream_class.on_release = &stream_release;
  stream_class.on_assign = &stream_assign;
  stream_class.finalize = stream_exit;
  x_class_register_ns(stream_class.cname, &stream_class, "http://etherx.jabber.org/streams");
  EXIT;
  return;
}
PLUGIN_INIT(xmppstream_init)
;

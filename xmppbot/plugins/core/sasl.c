/*
 * stream.c
 *
 *  Created on: Aug 12, 2011
 *      Author: artemka
 */

#undef DEBUG_PRFX
#define DEBUG_PRFX "[sasl] "
#include <xwlib/x_types.h>

#include <xmppagent.h>

/* matching function */
static int
sasl_match(x_object *o, x_obj_attr_t *_o)
{
  const char *a;
  ENTER;

  if (!_o || ((a = getattr("xmlns", _o))
      && !EQ(a,"urn:ietf:params:xml:ns:xmpp-sasl")))
    return FALSE;

  EXIT;
  return TRUE;
}

/* destructor */
static void
sasl_create(x_object *o)
{
  ENTER;
  EXIT;
  return;
}

/* handler of append event (appears when object is appended to its parent) */
static int
sasl_append(x_object *so, x_object *parent)
{
  ENTER;
  EXIT;
  return 0;
}

/* handler of removing event (appears when object is removing from its parent) */
static void
sasl_remove(x_object *o)
{
  ENTER;
  EXIT;
}

/* default constructor */
static void
sasl_release(x_object *o)
{
  ENTER;
  EXIT;
}

/* assignment handler */
static x_object *
sasl_assign(x_object *o, x_obj_attr_t *attrs)
{
  ENTER;
  x_object_default_assign_cb(o, attrs);
  EXIT;
  return o;
}

static void
sasl_exit(x_object *o)
{
  x_object *msg, *o1;
  const char *xmlns;
  ENTER;

  printf("%s:%s():%d\n",__FILE__,__FUNCTION__,__LINE__);

  xmlns = x_object_getattr(o, "xmlns");
  if (!xmlns)
    return;

  if (!EQ(xmlns,"urn:ietf:params:xml:ns:xmpp-sasl"))
    return;

  if (EQ(x_object_get_name(o), "success"))
    {
      x_bus_reset((struct x_bus *) o->bus);
    }
  else if (EQ(x_object_get_name(o), "failure"))
    {
#pragma message("Fixme! Filed auth mechanism")
      TRACE("Authentication ERROR!\n");
      x_bus_reset((struct x_bus *) o->bus);
    }
  else
    {
      for (o1 = x_object_get_child(o, "mechanism"); o1; o1 = x_object_get_next(
          o1))
        {
          TRACE("'%s' mechanism\n", o1->content.cbuf);
          if (EQ(o1->content.cbuf,"PLAIN"))
            {
              msg = x_object_new("auth-plain");
              x_object_append_child(x_object_get_parent(o), msg);
              break;
            }
#if 0
          else if (EQ(o1->content.cbuf,"DIGEST-MD5"))
            {
              msg = x_object_new("auth-digest-md5");
              x_object_append_child(x_object_get_parent(o), msg);
              break;
            }
#endif
        }
    }

  EXIT;
}

static int
sasl_class_match(x_object *to, x_obj_attr_t *attr)
{
  const char *a;
  ENTER;

  if (attr && (a = getattr("xmlns", attr))
      && EQ(a,"urn:ietf:params:xml:ns:xmpp-sasl"))
    return TRUE;

  EXIT;
  return FALSE;
}

static struct xobjectclass sasl_class;

__x_plugin_visibility
__plugin_init void
sasl_features_init(void)
{
  ENTER;

  sasl_class.cname = "mechanisms";
  sasl_class.psize = 0;
  sasl_class.match = &sasl_match;
  sasl_class.on_create = &sasl_create;
  sasl_class.on_append = &sasl_append;
  sasl_class.on_remove = &sasl_remove;
  sasl_class.on_release = &sasl_release;
  sasl_class.on_assign = &sasl_assign;
  sasl_class.finalize = &sasl_exit;
  sasl_class.classmatch = &sasl_class_match;

  x_class_register_ns(sasl_class.cname, &sasl_class,
      "urn:ietf:params:xml:ns:xmpp-sasl");
  x_class_register_ns("success", &sasl_class,
      "urn:ietf:params:xml:ns:xmpp-sasl");
  x_class_register_ns("failure", &sasl_class,
      "urn:ietf:params:xml:ns:xmpp-sasl");
  EXIT;
  return;
}
PLUGIN_INIT(sasl_features_init)
;

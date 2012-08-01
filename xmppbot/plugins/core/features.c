/*
 * stream.c
 *
 *  Created on: Aug 12, 2011
 *      Author: artemka
 */

#include <stdio.h>
#include <string.h>

/* #include <strings.h> */

#undef DEBUG_PRFX
#define DEBUG_PRFX "[features] "
#include <xwlib/x_types.h>
#include <xwlib/x_obj.h>
#include <xwlib/x_utils.h>

#include <xmppagent.h>

extern struct x_transport *
xmpp_start_tls(int sock);

struct xmpp_features
{
  x_object super;
  struct
  {
    int state;
    u_int32_t cr0;
  } regs;
  x_object *msgslot;
};

/* matching function */
static int
features_match(x_object *o, x_obj_attr_t *_o)
{
  ENTER;
  EXIT;
  return TRUE;
}

/* destructor */
static void
features_create(x_object *o)
{
  /*struct xmpp_features *ft = (struct xmpp_features *) o;*/
  ENTER;
  EXIT;
  return;
}

/* handler of append event (appears when object is appended to its parent) */
static int
features_append(x_object *so, x_object *parent)
{
  /*struct xmpp_stream *stm = (struct xmpp_stream *) so;*/
  ENTER;
  EXIT;
  return 0;
}

/* handler of removing event (appears when object is removing from its parent) */
static void
features_remove(x_object *o)
{
  ENTER;
  EXIT;
}

/* default constructor */
static void
features_release(x_object *o)
{
  ENTER;
  EXIT;
}

/* assignment handler */
static x_object *
features_assign(x_object *o, x_obj_attr_t *attrs)
{
  ENTER;

  EXIT;
  return o;
}

static void
features_exit(x_object *this__)
{
  struct xmpp_features *ft = (struct xmpp_features *) this__;

  ENTER;

  printf("%s:%s():%d\n",__FILE__,__FUNCTION__,__LINE__);

  if (ft->msgslot)
    {
      x_object_send_down(x_object_get_parent(this__), ft->msgslot, NULL);
    }

  ft->msgslot = NULL;

  EXIT;
}

static int
features_tx(x_object *o, x_object *msg, x_obj_attr_t *attrs)
{
  struct xmpp_features *ft = (struct xmpp_features *) o;

  ENTER;

  if (!ft->msgslot)
    {
      ft->msgslot = msg;
    }

  EXIT;
  return 0;
}

static struct xobjectclass features_class;

/*
 * STARTTLS CLASSES
 */
/* matching function */
static BOOL
tls_match(x_object *o, x_obj_attr_t *_o)
{
  const char *a;
  ENTER;

  if (_o && (a = getattr("xmlns", _o))
      && !EQ(a,"urn:ietf:params:xml:ns:xmpp-tls"))
    return FALSE;

  EXIT;
  /*by default will treat any 'starttls' or 'proceed' as matched*/
  return TRUE;
}

static int
tls_append(x_object *me, x_object *parent)
{
  x_object *msg;

  ENTER;

  if (EQ(x_object_get_name(me),"starttls"))
    {
      msg = x_object_new("starttls");
      x_object_setattr(msg, "xmlns", "urn:ietf:params:xml:ns:xmpp-tls");
      x_object_send_down(parent, msg, NULL);
    }

  EXIT;
  return 0;
}

static void
tls_exit(x_object *o)
{
  const char *oname;
  struct x_transport *t = NULL;
  ENTER;

  printf("%s:%s():%d\n",__FILE__,__FUNCTION__,__LINE__);

  oname = x_object_get_name(o);

  if (EQ(oname,"proceed"))
    {
      t = xmpp_start_tls(((struct x_bus *) o->bus)->sys_socket);
      if (t)
        {
          x_bus_set_transport((struct x_bus *) o->bus, t);
        }
      x_bus_reset((struct x_bus *) o->bus);
    }
  else if (EQ(oname,"failure"))
    {
    }
  else if (EQ(oname,"starttls"))
    {
    }
  else
    {
      TRACE("%s\n",oname);
      BUG();
    }

  EXIT;
}

static struct xobjectclass starttls_class;

__x_plugin_visibility
__plugin_init void
stream_features_init(void)
{
  ENTER;

  features_class.cname = "features";
  features_class.psize = (unsigned int) (sizeof(struct xmpp_features)
      - sizeof(x_object));
  features_class.match = &features_match;
  features_class.on_create = &features_create;
  features_class.on_append = &features_append;
  features_class.on_remove = &features_remove;
  features_class.on_release = &features_release;
  features_class.on_assign = &features_assign;
  features_class.finalize = &features_exit;
  features_class.tx = &features_tx;

  x_class_register_ns(features_class.cname, &features_class,
      "http://etherx.jabber.org/streams");

  starttls_class.cname = "starttls";
  starttls_class.psize = 0;
  starttls_class.match = &tls_match;
  starttls_class.on_append = &tls_append;
  starttls_class.finalize = &tls_exit;

  x_class_register_ns("starttls", &starttls_class,
      "urn:ietf:params:xml:ns:xmpp-tls");
  x_class_register_ns("proceed", &starttls_class,
      "urn:ietf:params:xml:ns:xmpp-tls");
  x_class_register_ns("failure", &starttls_class,
      "urn:ietf:params:xml:ns:xmpp-tls");
  EXIT;
  return;
}
PLUGIN_INIT(stream_features_init)
;

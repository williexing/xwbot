/*
 * stream.c
 *
 *  Created on: Aug 12, 2011
 *      Author: artemka
 */

#include <stdio.h>
#include <string.h>

#if defined(_WIN32) || defined(WIN32)
#else
#include <strings.h>
#endif

#undef DEBUG_PRFX

#include <xwlib/x_config.h>
#if TRACE_XIQ_ON
#define DEBUG_PRFX "[iq] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include <xmppagent.h>

struct iq_object
{
  x_object super;
  struct
  {
    int state;
    int id_counter;
    u_int32_t cr0;
  } regs;
#if 0
/* iq should contain only one child object */
x_object *msgslot;
/* input iq multiplexer */
struct ht_cell *mux[HTABLESIZE];
#endif
};

/* matching function */
static BOOL
iq_match(x_object *o, x_obj_attr_t *_o)
{
  const char *from1, *from2;

  from1 = getattr("from", _o);
  from2 = _AGET(o, "from");

  if (!from1 && !from2)
    return TRUE;
  else if (from1 && from2 && EQ(from1,from2))
    return TRUE;
  else
    return FALSE;
}

/* constructor */
static void
iq_on_create(x_object *o)
{
  return;
}

static int
iq_on_append(x_object *so, x_object *parent)
{
  return 0;
}

/* handler of removing event (appears when object is removing from its parent) */
static void
iq_on_remove(x_object *o)
{
}

/* default constructor */
static void
iq_on_release(x_object *o)
{
  ENTER;
  EXIT;
}

/* assignment handler */
static x_object *
iq_on_assign(x_object *o, x_obj_attr_t *attrs)
{
  x_object *recipient = NULL;
  const char *typ, *id;

  x_object_default_assign_cb(o, attrs);

  typ = getattr("type", attrs);
  TRACE("IQ(%s,id[%s])\n", typ, getattr("id", attrs));

  if (!typ)
    {
      TRACE("Invalid IQ\n");
      return NULL;
    }

  if (EQ(typ,"set"))
    {

    }
  else if (EQ(typ,"get"))
    {

    }
  else if (EQ(typ,"result"))
    {
      id = getattr("id", attrs);
      if (id)
        {
          if (recipient)
            {

            }
        }
    }
  else if (EQ(typ,"error"))
    {
    }
  else
    {
    }

  return o;
}

static void
iq_exit(x_object *this__)
{
  /*  struct iq_object *ft = (struct iq_object *) this__;*/
  printf("%s:%s():%d\n",__FILE__,__FUNCTION__,__LINE__);
}

static int
iq_tx(x_object *o, x_object *msg, x_obj_attr_t *ctx)
{
  struct iq_object *iqo;
  const char *id;

  ENTER;
  iqo = (struct iq_object *) _NEW("iq", NULL);

  if (iqo)
    {
      id = _AGET(o, "id");
      if (!id)
        {
          TRACE("NO ID IN IQ\n");
          x_object_print_path(o, 0);
        }

      _ASET(X_OBJECT(iqo), "id", _XS(id));
      if (ctx)
        {
          _ASET(X_OBJECT(iqo), "type", getattr("#type", ctx));
          if (getattr("#id", ctx))
            _ASET(X_OBJECT(iqo), "id", getattr("#id", ctx));
        }
      else
        {
          _ASET(X_OBJECT(iqo), "type", "get");
        }

      if (msg)
        _INS(X_OBJECT(iqo), msg);

      _ASET(X_OBJECT(iqo), "from", _AGET(o, "to"));
      _ASET(X_OBJECT(iqo), "to", _AGET(o, "from"));

//      x_object_print_path(iqo,0);
//      x_object_print_path(o,0);

      x_object_send_down(_PARNT(o), X_OBJECT(iqo), NULL);
    }

  EXIT;
  return 0;
}

static struct xobjectclass iq_class;

__x_plugin_visibility
__plugin_init void
iq_init(void)
{
  ENTER;

  iq_class.cname = "iq";
  iq_class.psize = (unsigned int) (sizeof(struct iq_object) - sizeof(x_object));
  iq_class.match = &iq_match;
  iq_class.on_create = &iq_on_create;
  iq_class.on_append = &iq_on_append;
  iq_class.on_remove = &iq_on_remove;
  iq_class.on_release = &iq_on_release;
  iq_class.on_assign = &iq_on_assign;
  iq_class.finalize = &iq_exit;
  iq_class.tx = &iq_tx;

  x_class_register_ns(iq_class.cname, &iq_class, "jabber:client");
  EXIT;
  return;
}
PLUGIN_INIT(iq_init)
;

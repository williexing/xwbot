/**
 * payload_ctl.c
 *
 *  Created on: Jan 12, 2012
 *      Author: artemka
 */

#undef DEBUG_PRFX
#define DEBUG_PRFX "[__payloadctl] "
#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

typedef struct media_ctl
{
  x_object xobj;
  x_object *encoders_directory;

  x_object *encoder;
  x_object *encoder_nexthop;

  uint32_t data_clockrate;
  uint32_t data_framerate;
  uint32_t target_clockrate;
  uint32_t target_framerate;

  x_object *decoder;
  x_object *decoder_nexthop;

} media_ctl_t;

static void
payloadctl_on_create(x_object *o)
{
  ENTER;
  EXIT;
}

static int
payloadctl_on_append(x_object *o, x_object *parent)
{
  media_ctl_t *mctl = (media_ctl_t *) o;
  ENTER;
  mctl->encoders_directory = x_object_from_path(o->bus, "__proc/audio/");

  EXIT;
  return 0;
}

/**
 * Encoder's frame ready callback
 */
static int
payloadctl_on_encoded_data_ready(void *o, void *frame, int framelen)
{
  x_obj_attr_t hints =
    { 0, 0, 0 };
  media_ctl_t *mctl = (media_ctl_t *) o;

  // send to next_hop;
  if (!mctl->encoder_nexthop)
    {
      /* drop frame ... */
    }

  // send data to next hop object
  x_object_try_write(mctl->encoder_nexthop, buf, len, &hints);

}

static int
payloadctl_try_write(x_object *o, void *buf, u_int32_t len, x_obj_attr_t *attr)
{
  int err;
  const char *idstr;
  x_object *codec;
  media_ctl_t *mctl = (media_ctl_t *) o;

  if (!attr)
    {
      ERROR;
      return (-1);
    }

  idstr = getattr("id", attr);

  if (!idstr)
    {
      ERROR;
      return (-1);
    }

  codec = _CHLD(o,idstr);

  if (!codec)
    {
      ERROR;
      return (-1);
    }

  err = x_object_try_write(codec, buf, len, attr);

  return err;
}

static int
payloadctl_on_assign(x_object *o, x_obj_attr_t *attrs)
{
  const char *str;
  ENTER;

  if ((str = getattr("rate", attrs)))
    {
      /* set current bitrate */
    }

  if ((str = getattr("clockrate", attrs)))
    {
      /* set current bitrate */
    }

  if ((str = getattr("framerate", attrs)))
    {
      /* set current bitrate */
    }

  EXIT;
  return 0;
}

static void UNUSED
payloadctl_remove_cb(x_object *o)
{
  ENTER;
  EXIT;
}

static void
payloadctl_on_child_append(x_object *o, x_object *child)
{
  int err;
  char buf[32];
  x_obj_attr_t hints =
    { 0, 0, 0 };
  x_object *msg;
  x_object *body;

  //  EXIT;
}

static void UNUSED
payloadctl_release_cb(x_object *o)
{
  ENTER;
  EXIT;
}

/* matching function */
static BOOL
payloadctl_equals(x_object *o, x_obj_attr_t *attrs)
{
  return TRUE;
}

static struct xobjectclass __payloadctl_objclass;

__x_plugin_visibility
__plugin_init void
__payloadctl_init(void)
{
  ENTER;
  __payloadctl_objclass.cname = X_STRING("__rtppldctl");
  __payloadctl_objclass.psize = (unsigned int) (sizeof(media_ctl_t)
      - sizeof(x_object));

  __payloadctl_objclass.match = &payloadctl_equals;
  __payloadctl_objclass.on_create = &payloadctl_on_create;
  __payloadctl_objclass.on_append = &payloadctl_on_append;
  __payloadctl_objclass.on_assign = &payloadctl_on_assign;
  __payloadctl_objclass.on_child_append = &payloadctl_on_child_append;
  __payloadctl_objclass.on_remove = &payloadctl_remove_cb;
  __payloadctl_objclass.on_release = &payloadctl_release_cb;
  __payloadctl_objclass.try_write = &payloadctl_try_write;

  x_class_register_ns(__payloadctl_objclass.cname,
      &__payloadctl_objclass,"jingle");
  EXIT;
  return;
}
PLUGIN_INIT(__payloadctl_init);


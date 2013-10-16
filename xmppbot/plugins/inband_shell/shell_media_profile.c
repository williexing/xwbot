/**
 * Copyrights (C) 2012, CrossLine Media, Ltd.
 * @file shell_media_profile.c
 *
 *  Created on: Aug 4, 2012
 *  @author: CrossLine Media, Ltd. http://www.clmedia.ru
 */

#undef DEBUG_PRFX
#define DEBUG_PRFX "[__inbandshell_m] "
#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include <plugins/sessions_api/sessions.h>

enum
{
  TYPE_UNSPECIFIED = 0, TYPE_ENCODER = 1, TYPE_DECODER,
};

typedef struct shell_ctl
{
  x_object xobj;
  int mode;
  struct
  {
    int state;
    volatile uint32_t cr0;
  } regs;

  x_object *shello;
} shell_ctl_t;

static void
inbandshell_on_create(x_object *o)
{
  ENTER;
  EXIT;
}

static int
inbandshell_on_append(x_object *o, x_object *parent)
{
  x_string_t name;
  x_object *shello = NULL;
  shell_ctl_t *thiz = (shell_ctl_t *) o;
  ENTER;

  name = _GETNM(o);

  if (name && EQ(name, MEDIA_IN_CLASS_STR))
    {
      thiz->mode = TYPE_DECODER;
    }
  else if (name && EQ(name,MEDIA_OUT_CLASS_STR))
    {
      thiz->mode = TYPE_ENCODER;
    }

  // insert shell object
  shello = _CHLD(_PARNT(o),"__ibshell");
  if (!shello)
  {
      shello = _GNEW("__ibshell","gobee:media");
      if(shello)
      {
          _INS(_PARNT(o),shello);
      }
  }

  if(shello)
  {
      if(thiz->mode == TYPE_ENCODER)
        x_object_set_write_handler(shello,o);
      else
        x_object_set_write_handler(o,shello);
  }

  EXIT;
  return 0;
}

static int
__ibs_shell_write_out(shell_ctl_t *thiz, void *buf, int len, int isOut)
{
  x_object *tmp;
  int res = -1;
  x_obj_attr_t hints =
    { 0, 0, 0 };

  if (!thiz->xobj.write_handler)
    {
      if (thiz->mode == TYPE_ENCODER)
          tmp = _CHLD(_PARNT(X_OBJECT(thiz)), "transport");
      else
          tmp = _CHLD(_PARNT(X_OBJECT(thiz)), "__ibshell");

      if (!tmp)
        {
          TRACE("No output channel... Decreasing camera status...\n");
          ERROR;
          return -1;
        }
      else
        {
            x_object_set_write_handler(X_OBJECT(&thiz->xobj),X_OBJECT(tmp));
        }
    }
  else
    {
      TRACE("Handler OK\n");
    }

  if (isOut)
  {
      setattr("iotype","out",&hints);
  }

  if(thiz->xobj.write_handler)
      res = _WRITE(thiz->xobj.write_handler, buf, len, &hints);

  attr_list_clear(&hints);

  return res;
}

static int
inbandshell_try_write(x_object *o, void *buf,
                      u_int32_t len, x_obj_attr_t *attr)
{
    int err;
    x_object *tmpo;
    shell_ctl_t *shio = (shell_ctl_t *) (void *) o;

    TRACE("received message to write %s\n", buf);

#if 0
    __ibs_shell_write_out(shio, buf, len, 1);
#else
    switch (shio->mode)
    {
    case TYPE_DECODER:
        TRACE("received message to write in:\n");
        //    TRACE("%s\n", buf);
        __ibs_shell_write_out(shio, buf, len, 0);
        break;

    case TYPE_ENCODER:
        TRACE("received message to write out:\n");
        TRACE("%s\n", buf);
        __ibs_shell_write_out(shio, buf, len, 1);
        break;
    };
#endif
    return err;
}

static x_object *
inbandshell_on_assign(x_object *thiz, x_obj_attr_t *attrs)
{
  shell_ctl_t *shio = (shell_ctl_t *) thiz;
  x_object_default_assign_cb(thiz, attrs);

  return thiz;
}

static void UNUSED
inbandshell_remove_cb(x_object *o)
{
  ENTER;
  EXIT;
}

static void UNUSED
inbandshell_release_cb(x_object *o)
{
  ENTER;
  EXIT;
}

/* matching function */
static BOOL
inbandshell_equals(x_object *o, x_obj_attr_t *attrs)
{
  return TRUE;
}

static struct xobjectclass __ibshell_media_class;

__x_plugin_visibility
__plugin_init void
__inbandshell_m_init(void)
{
  ENTER;
  __ibshell_media_class.cname = X_STRING("__ibshell_m_profile");
  __ibshell_media_class.psize = (unsigned int) (sizeof(shell_ctl_t)
      - sizeof(x_object));

  __ibshell_media_class.match = &inbandshell_equals;
  __ibshell_media_class.on_create = &inbandshell_on_create;
  __ibshell_media_class.on_append = &inbandshell_on_append;
  __ibshell_media_class.on_assign = &inbandshell_on_assign;
  __ibshell_media_class.on_remove = &inbandshell_remove_cb;
  __ibshell_media_class.on_release = &inbandshell_release_cb;
  __ibshell_media_class.try_write = &inbandshell_try_write;

  x_class_register_ns(__ibshell_media_class.cname, &__ibshell_media_class,
      _XS("gobee:media"));
  EXIT;
  return;
}
PLUGIN_INIT(__inbandshell_m_init);


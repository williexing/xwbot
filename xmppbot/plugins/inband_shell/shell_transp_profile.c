/**
 * Copyrights (C) 2012, CrossLine Media, Ltd.
 * @file shell_media_profile.c
 *
 *  Created on: Aug 4, 2012
 *  @author: CrossLine Media, Ltd. http://www.clmedia.ru
 */

#undef DEBUG_PRFX
#define DEBUG_PRFX "[__inbandshell_t] "
#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include <plugins/sessions_api/sessions.h>

typedef struct shell_transport
{
  x_object xobj;
} shell_transport_t;


static int
inbandshell_t_try_write(x_object *thiz_, void *buf, u_int32_t len,
                        x_obj_attr_t *attr)
{
    int err = -1;
    x_string_t xs;
    x_object *msg;
    x_object *tmp;
//    shell_transport_t *thiz = (shell_transport_t *) (void*) thiz_;

    if (attr)
        xs = getattr("iotype",attr);

    if(xs && EQ(xs,"out"))
    {
        TRACE("write in: %s\n", (char *)buf);
        msg = _GNEW("$message",NULL);
        x_object_set_content(msg, buf, len);
        _MCAST(thiz_,msg);
        _REFPUT(msg,NULL);
    }
    else
    {
        TRACE("write out: %s\n", (char *)buf);
        if (!thiz_->write_handler)
        {
            tmp = _CHLD(_PARNT(X_OBJECT(thiz_)), MEDIA_IN_CLASS_STR);
            thiz_->write_handler = tmp;
            if (!thiz_->write_handler)
            {
                TRACE("No valid media was found\n");
                ERROR;
                return -1;
            }
            else
            {
                _REFGET(thiz_->write_handler);
            }
        }
        err = _WRITE(thiz_->write_handler,buf,len,attr);
    }
    return err;
}

/* matching function */
static BOOL
inbandshell_t_equals(x_object *o, x_obj_attr_t *attrs)
{
  return TRUE;
}

static struct xobjectclass __ibshell_t_profile;

__x_plugin_visibility
__plugin_init void
__inbandshell_t_init(void)
{
  ENTER;
  __ibshell_t_profile.cname = X_STRING("__ibshell_t_profile");
  __ibshell_t_profile.psize = (unsigned int) (sizeof(shell_transport_t)
      - sizeof(x_object));

  __ibshell_t_profile.match = &inbandshell_t_equals;
  __ibshell_t_profile.try_write = &inbandshell_t_try_write;

  x_class_register_ns(__ibshell_t_profile.cname, &__ibshell_t_profile,
      _XS("gobee:media"));
  EXIT;
  return;
}
PLUGIN_INIT(__inbandshell_t_init);


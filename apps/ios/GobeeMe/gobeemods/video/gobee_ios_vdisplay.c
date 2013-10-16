//
//  gobee_ios_vdisplay.c
//  GobeeMe
//
//  Created by артемка on 08.03.13.
//
//

#include <stdio.h>

#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>

#undef DEBUG_PRFX
#include <x_config.h>
#if 1 //TRACE_VIRTUAL_DIPSLAY_ON
#define TRACE_LEVEL 2
#define DEBUG_PRFX "[ ios-vdisplay ] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include "gl_common.h"

#include "gobee_ios_vdisplay.h"

void display_new (x_object *xoid, img_plane_buffer *yuvbuf, void **pctx);

#define X_OUT_DEVDIR "__proc/out"
#define DISPLAY_VENDOR_ID "IOS_vDisplay_GLESv2"

static int
x_vdisplay_try_writev(x_object *o, const x_iobuf_t *iov,
                      u_int32_t iovlen, x_obj_attr_t *attr)
{
  int err;
//  img_plane_buffer yuv;
  virtual_display_t *thiz = (virtual_display_t *) o;

  int *stride = NULL;
  
  ENTER;
  
  if (iovlen>3)
  {
    stride = (int *)X_IOV_DATA(&iov[3]);
  }
  
  thiz->yuv[0].data = X_IOV_DATA(&iov[0]);
  thiz->yuv[0].width = thiz->frameWidth ? thiz->frameWidth : 320;
  thiz->yuv[0].height = thiz->frameHeight ? thiz->frameHeight : 240;
  if (stride && *stride > 0)
  {
    thiz->yuv[0].stride = *stride;
  }
  else
  {
    thiz->yuv[0].stride = 352;
  }
  
  thiz->yuv[1].width = thiz->yuv[0].width / 2;
  thiz->yuv[1].height = thiz->yuv[0].height / 2;
  thiz->yuv[1].stride = thiz->yuv[0].stride / 2;
  
  thiz->yuv[2].width = thiz->yuv[1].width;
  thiz->yuv[2].height = thiz->yuv[1].height;
  thiz->yuv[2].stride = thiz->yuv[1].stride;
  
  thiz->yuv[1].data = X_IOV_DATA(&iov[1]);
  thiz->yuv[2].data = X_IOV_DATA(&iov[2]);
  
  return 100;
}

static int
x_vdisplay_on_append(x_object *so, x_object *parent)
{
  virtual_display_t *thiz = (virtual_display_t *) so;
  ENTER;
  
  thiz->yuv[0].data = NULL;
  display_new(so, &thiz->yuv, &thiz->UI_ctx);
  
  EXIT;
  return 0;
}

static x_object *
x_vdisplay_on_assign(x_object *thiz, x_obj_attr_t *attrs)
{
  register x_obj_attr_t *attr;
  int update = 0;
  virtual_display_t *mctl = (virtual_display_t *) thiz;
  
  ENTER;
  
  for (attr = attrs->next; attr; attr = attr->next)
  {
    if (EQ("width", attr->key))
    {
      mctl->frameWidth = atoi(attr->val);
      TRACE("GobeeDisplay::assign(): width=%d\n",mctl->frameWidth);
      update = 1;
    }
    else if (EQ("stride", attr->key))
    {
      mctl->frameWidth = atoi(attr->val);
      TRACE("GobeeDisplay::assign(): stride=%d\n",mctl->frameWidth);
      update = 1;
    }
    else  if (EQ("height", attr->key))
    {
      mctl->frameHeight = atoi(attr->val);
      TRACE("GobeeDisplay::assign(): height=%d\n",mctl->frameHeight);
      update = 1;
    }
    
    _ASET(thiz, attr->key, attr->val);
  }
  
  if (update)
  {
    // this->framebuf update
  }
  
  EXIT;
  return thiz;
}

static void
x_vdisplay_exit(x_object *this__)
{
  virtual_display_t *mctl = (virtual_display_t *) this__;
  ENTER;
  TRACE("\n");
  EXIT;
}

static struct xobjectclass virt_display_classs;

static void
___vdisplay_on_x_path_event(void *_obj)
{
  x_object *obj = _obj;
  x_object *ob;
  
  ENTER;
  
  ob = x_object_add_path(obj, X_OUT_DEVDIR "/" DISPLAY_VENDOR_ID, NULL);
  
  EXIT;
}

static struct x_path_listener virtdisplay_bus_listener;

__x_plugin_visibility
__plugin_init void
x_virtdisplay_init(void)
{
  ENTER;
  virt_display_classs.cname = _XS("video_player");
  virt_display_classs.psize = (sizeof(virtual_display_t) - sizeof(x_object));
  virt_display_classs.on_assign = &x_vdisplay_on_assign;
  virt_display_classs.on_append = &x_vdisplay_on_append;
  virt_display_classs.finalize = &x_vdisplay_exit;
  virt_display_classs.try_writev = &x_vdisplay_try_writev;
  
  x_class_register_ns(virt_display_classs.cname, &virt_display_classs,
                      _XS("gobee:media"));
  
  virtdisplay_bus_listener.on_x_path_event = &___vdisplay_on_x_path_event;
  
  /**
   * Listen for 'session' object from "urn:ietf:params:xml:ns:xmpp-session"
   * namespace. So presence will be activated just after session xmpp is
   * activated.
   */
  x_path_listener_add("/", &virtdisplay_bus_listener);
  EXIT;
  return;
}

PLUGIN_INIT(x_virtdisplay_init)
;

//
//  gobee_ios_vcam_plugin.m
//  GobeeMe
//
//  Copyright (c) 2013 CrossLine Media, Ltd.. All rights reserved.
//

#undef DEBUG_PRFX
#include <x_config.h>
#if TRACE_VIRTUAL_DIPSLAY_ON
#define TRACE_LEVEL 2
#define DEBUG_PRFX "[IOS-CAMERA] "
#endif

#include <sys/mman.h>

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#define X_IN_DEVDIR "__proc/in"
#define CAMERA_VENDOR_ID "Ios_Camera"

void create_camera (const char *sid, x_object *xobj);

typedef struct ios_camera
{
  x_object xobj;
  struct ev_periodic cam_qwartz;
  int vfd;
  unsigned char *_framebuf;
  unsigned char *framebuf;
  int fbw;
  int fbstride;
  int fbh;
  int w;
  int h;
} ios_camera_t;

static void
__gobee_yuv_set_data(ios_camera_t *thiz,
                     gobee_img_frame_t yuv, void *data);

static int
__xqvfb_write_frame_to_encoder_hlp(
                                   ios_camera_t *thiz , gobee_img_frame_t yuv);

static int
__xqvfb_check_writehandler_hlp(ios_camera_t *thiz)
{
  x_object *tmp;
  if (!thiz->xobj.write_handler)
  {
    tmp = _CHLD(_PARNT(X_OBJECT(thiz)), "out$media");
    if (!tmp)
    {
      
      TRACE("No output channel... Decreasing camera status...\n");
      ERROR;
      return -1;
    }
    
    tmp = _CHLD(tmp,NULL);
    if (!tmp)
    {
      TRACE("No output channel... Decreasing camera status...\n");
      ERROR;
      return -1;
    }
    else
    {
      x_object_set_write_handler(&thiz->xobj,tmp);
    }
  }
  else
  {
    TRACE("Handler OK\n");
  }
  
  return 0;
}

static void
create_camera_fb(const char *sid, x_object *obj)
{
}

static void
nv21_to_i420p(unsigned char *chromaFROM,
              unsigned char *chromaTO, int w, int h)
{
  unsigned char *chromaU;
  unsigned char *chromaV;
  char c = 0;
  int i, j, k;
  int w2 = w >> 1;
  int h2 = h >> 1;
  
  chromaU = chromaTO;
  chromaV = (unsigned char *)&chromaTO[w2 * h2];
  
  for (j = 0; j < h/2; j++)
  {
    for (i = 0; i < w; i++)
    {
      chromaU[j * w/2 + i/2] = chromaFROM[j * w + i];
      chromaV[j * w/2 + i/2] = chromaFROM[j * w + i + 1];
    }
  }
}

static void
YUYV_to_i420p(unsigned char *from, unsigned char *to, int w, int h)
{
}

static void
__gobee_yuv_set_data(ios_camera_t *thiz,
                     gobee_img_frame_t yuv, void *data)
{
  yuv[0].data = data;
  yuv[1].data = &yuv[0].data[0]
  + thiz->fbstride * thiz->h;
  yuv[2].data = &yuv[1].data[0]
  + thiz->fbstride * thiz->h / 4;
  
  yuv[0].width = thiz->w;
  yuv[0].height = thiz->h;
  yuv[0].stride = thiz->fbstride;
  
  yuv[2].width = yuv[1].width = yuv[0].width / 2;
  yuv[2].height = yuv[1].height = yuv[0].height / 2;
  yuv[2].stride = yuv[1].stride = yuv[0].stride / 2;
  
}

static int
__xqvfb_write_frame_to_encoder_hlp(
                                   ios_camera_t *thiz , gobee_img_frame_t yuv)
{
  int len;
  char buf[16];
  x_iobuf_t planes[4];
  x_obj_attr_t hints =
  { 0, 0, 0 };
  
  /* pass to next hop (encoder) */
  
  X_IOV_DATA(&planes[0]) = yuv[0].data;
  X_IOV_LEN(&planes[0]) = yuv[0].height * yuv[0].stride;
  
  X_IOV_DATA(&planes[1]) = yuv[1].data;
  X_IOV_LEN(&planes[1]) = yuv[1].height * yuv[1].stride;
  
  X_IOV_DATA(&planes[2]) = yuv[2].data;
  X_IOV_LEN(&planes[2]) = yuv[2].height * yuv[2].stride;

  X_IOV_DATA(&planes[3]) = &yuv[0].stride;
  X_IOV_LEN(&planes[3]) = sizeof(yuv[0].stride);

  if (_WRITEV(thiz->xobj.write_handler, planes, 4, NULL) < 0)
  {
    len = yuv[0].height * yuv[0].stride * 3 / 2;
    _WRITE(thiz->xobj.write_handler, yuv[0].data, len, NULL);
  }
}

static void
x_ios_cam_on_write_frame(x_object *xobj,
                           void *framedata, size_t siz, x_obj_attr_t *attrs)
{
  int err;
  gobee_img_frame_t img;
  x_object *tmp;
  time_t stamp = time(NULL);
  ios_camera_t *thiz = (ios_camera_t *)(void *)xobj;
  
  if (!thiz->xobj.write_handler)
  {
    tmp = _CHLD(_PARNT(X_OBJECT(thiz)), "out$media");
    tmp = _CHLD(tmp,NULL);
    if (!tmp)
    {
      TRACE("No output channel... Decreasing camera status...\n");
      ERROR;
      return;
    }
    else
    {
      thiz->xobj.write_handler = tmp;
      _REFGET(thiz->xobj.write_handler);
    }
  }
  
#if 1
  x_memcpy(thiz->framebuf,framedata,thiz->fbstride * thiz->h);
  __gobee_yuv_set_data(thiz, img, thiz->framebuf);
  __xqvfb_write_frame_to_encoder_hlp(thiz, img);
#else
  x_memcpy(thiz->framebuf,framedata,thiz->w * thiz->h);
  nv21_to_i420p(
                &framedata[thiz->w * thiz->h],
                &thiz->framebuf[thiz->w * thiz->h],
                thiz->w,
                thiz->h);
  _WRITE(thiz->xobj.write_handler, thiz->framebuf /* framedata */, thiz->w*thiz->h*3/2, NULL);
#endif
  
}

static int
x_ios_cam_on_append(x_object *so, x_object *parent)
{
  int ofd;
  ios_camera_t *mctl = (ios_camera_t *) so;
  ENTER;
  
  mctl->w = 352;
  mctl->h = 288;
  mctl->fbstride = 352;
  
  mctl->framebuf = x_malloc(mctl->fbstride * mctl->h * 4 /*/ 2 + 1*/);
  
  /* init capture */
  create_camera(_ENV(so,"sid"), so);
  
  TRACE("Opened camera at %dx%d\n", mctl->w, mctl->h);
  
  EXIT;
  return 0;
}

static x_object *
x_ios_cam_on_assign(x_object *this__, x_obj_attr_t *attrs)
{
  ios_camera_t *mctl = (ios_camera_t *) this__;
  ENTER;
  
  x_object_default_assign_cb(this__, attrs);
  
  EXIT;
  return this__;
}

static void
x_ios_cam_exit(x_object *this__)
{
  ios_camera_t *mctl = (ios_camera_t *) this__;
  ENTER;
  TRACE("\n");
  
  EXIT;
}

static struct xobjectclass ios_cam_class;

static void
___ios_cam_on_x_path_event(void *_obj)
{
  x_object *obj = X_OBJECT(_obj);
  x_object *ob;
  
  ENTER;
  
  ob = x_object_add_path(obj, X_IN_DEVDIR "/" CAMERA_VENDOR_ID, NULL);
  
  EXIT;
}

static struct x_path_listener ios_cam_bus_listener;

#if 1
__x_plugin_visibility
__plugin_init void
x_ioscam_init(void)
{
  ENTER;
  ios_cam_class.cname = _XS("camera");
  ios_cam_class.psize = (sizeof(ios_camera_t) - sizeof(x_object));
  ios_cam_class.on_assign = &x_ios_cam_on_assign;
  ios_cam_class.on_append = &x_ios_cam_on_append;
  ios_cam_class.finalize = &x_ios_cam_exit;
  ios_cam_class.try_write = &x_ios_cam_on_write_frame;
  
  x_class_register_ns(ios_cam_class.cname, &ios_cam_class,
                      _XS("gobee:media"));
  
  ios_cam_bus_listener.on_x_path_event = &___ios_cam_on_x_path_event;
  
  /**
   * Listen for 'session' object from "urn:ietf:params:xml:ns:xmpp-session"
   * namespace. So presence will be activated just after session xmpp is
   * activated.
   */
  x_path_listener_add("/", &ios_cam_bus_listener);
  EXIT;
  return;
}

PLUGIN_INIT(x_ioscam_init)
;
#endif

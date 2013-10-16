/**
 * xvirtdisplay_view.c
 *
 *  Created on: Jun 6, 2012
 *      Author: artemka
 */
#include <jni.h>
#include <android/log.h>

#undef DEBUG_PRFX
#include <x_config.h>
#if TRACE_VIRTUAL_DIPSLAY_ON
#define TRACE_LEVEL 2
#define DEBUG_PRFX "[DROID-CAMERA] "
#endif

#include <sys/mman.h>

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include <fcntl.h>
#include <linux/fb.h>

#define X_IN_DEVDIR "__proc/in"
#define CAMERA_VENDOR_ID "Android_Camera"

typedef struct droid_camera
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

  struct fb_fix_screeninfo finfo;
  struct fb_var_screeninfo vinfo;

} droid_camera_t;

static void
__gobee_yuv_set_data(droid_camera_t *thiz,
    gobee_img_frame_t yuv, void *data);
static int
__xqvfb_write_frame_to_encoder_hlp(
    droid_camera_t *thiz , gobee_img_frame_t yuv);

static int
__xqvfb_check_writehandler_hlp(droid_camera_t *thiz)
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


static unsigned char
clamp(int d)
{
    if (d < 0)
        return 0;

    if (d > 255)
        return 255;

    return (unsigned char) d;
}

static void
rgb2yuv(unsigned char *from, unsigned char *to,
    int srcw, int srch, int srcstride, int bpp,
    int w, int h)
{
    int x;
    int y;
    int r;
    int g;
    int b;

    unsigned char *luma;
    unsigned char *chromaU;
    unsigned char *chromaV;

    int w2 = w >> 1;
    int h2 = h >> 1;

    luma = &to[0];
    chromaU = &to[w * h];
    chromaV = &chromaU[w2 * h2];

    union
    {
        unsigned int raw;
        unsigned char rgba[4];
    } color;

    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {

#if 0
            color.rgba[0] = from[(y * w + x) * 3 + 0];
            color.rgba[1] = from[(y * w + x) * 3 + 1];
            color.rgba[2] = from[(y * w + x) * 3 + 2];
            r = clamp(((color.raw >> 12) & 0x3f) << 2);
            g = clamp(((color.raw >> 6) & 0x3f) << 2);
            b = clamp(((color.raw) & 0x3f) << 2);
#else
            r = from[ y * srcstride + x * bpp + 1];
            g = from[ y * srcstride + x * bpp + 2];
            b = from[ y * srcstride + x * bpp + 3];
#endif

            luma[y * w + x] = clamp(
                        ((66 * r + 129 * g + 25 * b + 128) >> 8) + 16);

            chromaU[(x >> 1) + (y >> 1) * w2] = clamp(
                        ((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128);

            chromaV[(x >> 1) + (y >> 1) * w2] = clamp(
                        ((112 * r - 94 * g - 18 * b + 128) >> 8) + 128);

        }
    }
}


static __inline__ void
rgb565_to_yuv(unsigned char *from, unsigned char *to,
    int srcw, int srch, int srcstride, int bpp,
    int w, int h, int stride, int yoffset, int xoffset)
{
    int x,_x;
    int y,_y;
    int r;
    int g;
    int b;

    int idx;

    ////
    float yscale = 1.0;
    float xscale = 1.0;

    int hend = h;
    int wend = w;

    int p1, p2;

    float aspect;
//    float scale;

    unsigned char *luma;
    unsigned char *chromaU;
    unsigned char *chromaV;

    int w2 = w / 2;
    int h2 = h / 2;

    luma = &to[0];
    chromaU = &to[w * h];
    chromaV = &chromaU[w2 * h2];

    ////// calc bounds
    aspect = (float)w / (float)h;
    if ((float)srch * aspect < (float)srcw)
      {
        srcw = srch * aspect;
      }
    else
      {
        xscale = yscale = (float)srcw / (float)w;
      }
    idx = srcw;
    if ((float)idx / aspect < (float)srch)
      {
        srch = (int)((float)idx / aspect);
      }
    else
      {
        xscale = yscale = (float)srch / (float)h;
      }

//    xscale = yscale = (float)srch / (float)h;
//    xscale = (float)srcw / (float)w;

    TRACE("yoffset(%p), xoffset(%p)\n",
        yoffset, xoffset);

    for (y = 0, _y =0; y < h && _y < srch; ++y, _y=y*yscale)
    {
        for (x = 0, _x=0; x < w && _x < srcw; ++x, _x=x*xscale)
        {
            idx = (_y  /*+ yoffset*/) * srcstride
                + (_x /*+ xoffset*/) * bpp;

            idx &= ~(1UL);
            p2 = from[idx];
            p1 = from[idx + 1];

            r = p1 & 0xf8;
            g = ((p1 & 0x7) << 5 ) | ((p2 & 0xe0) >> 3 );
            b = (p2 & 0x1f) << 3;

            luma[y * w + x] = clamp(
                        ((66 * r + 129 * g + 25 * b + 128) >> 8) + 16);

            chromaU[(x >> 1) + (y >> 1) * w2] = clamp(
                        ((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128);

            chromaV[(x >> 1) + (y >> 1) * w2] = clamp(
                        ((112 * r - 94 * g - 18 * b + 128) >> 8) + 128);

        }
    }
}


static void
__xqvfb_periodic_cb(struct ev_loop *loop, struct ev_periodic *w, int revents)
{
   int err;
    gobee_img_frame_t img;
    droid_camera_t *thiz = w->data;

    TRACE("\n");

    if (__xqvfb_check_writehandler_hlp(thiz) != 0)
    {
        TRACE("ERROR State!\n");
        return;
    }

    // convert first
#if 0
    if (thiz->vfd > 0)
      {
        lseek(thiz->vfd,0, SEEK_SET);
        err = x_read(thiz->vfd,thiz->_framebuf,thiz->w * thiz->h);
        TRACE("Read frame len(%d)\n",err);
        rgb2yuv(thiz->_framebuf, thiz->framebuf, thiz->w, thiz->h);
      }
#endif

    ioctl(thiz->vfd, FBIOGET_VSCREENINFO, &thiz->vinfo);

    rgb565_to_yuv(thiz->_framebuf, thiz->framebuf,
        thiz->fbw, thiz->fbh,
        thiz->fbstride, thiz->vinfo.bits_per_pixel / 8,
        thiz->w, thiz->h, thiz->w,
        thiz->vinfo.yoffset,
        thiz->vinfo.xoffset);

//    TRACE("next frame: this(%p) buf(%p,%p), width(%d), height(%d)\n",
//          thiz, thiz->_framebuf, thiz->framebuf, thiz->w, thiz->h);

    img[0].width = thiz->w;
    img[0].stride = thiz->w;
    img[0].height = thiz->h;

    img[1].width = thiz->w / 2;
    img[1].stride = thiz->w / 2;
    img[1].height = thiz->h / 2;

    img[2].width = thiz->w / 2;
    img[2].stride = thiz->w / 2;
    img[2].height = thiz->h / 2;

    __gobee_yuv_set_data(thiz,img,thiz->framebuf);
    __xqvfb_write_frame_to_encoder_hlp(thiz,img);

    //    _WRITE(thiz->xobj.write_handler, thiz->framebuf, thiz->w*thiz->h*3/2,NULL);
}

static void
create_camera_fb(const char *sid, x_object *obj)
{
  droid_camera_t *thiz = (droid_camera_t *)(void *)obj;
  thiz->cam_qwartz.data = (void *) obj;

  ENTER;

  thiz->vfd = x_open("/dev/graphics/fb0",O_RDWR);

  if (thiz->vfd < 0)
    {
      TRACE("Open fb0 failed: %s\n", strerror(errno));
    }

  TRACE("Opened /dev/graphics/fb0... fd=%d\n",thiz->vfd);

  ioctl(thiz->vfd, FBIOGET_FSCREENINFO, &thiz->finfo );
  ioctl(thiz->vfd, FBIOGET_VSCREENINFO, &thiz->vinfo);

  thiz->fbw =  thiz->vinfo.xres;
  thiz->fbh = thiz->vinfo.yres;
  thiz->fbstride = thiz->finfo.line_length;

#if 1
  thiz->_framebuf =
      mmap(NULL,
          thiz->fbw * thiz->fbh * thiz->vinfo.bits_per_pixel / 8,
          PROT_READ | PROT_WRITE,
          MAP_SHARED,thiz->vfd,0);

  if (thiz->_framebuf == MAP_FAILED)
    {
      TRACE("Error mapping framebuffer: %s, "
          "%dx%d, stride(%d)\n",strerror(errno),
          thiz->fbw, thiz->fbh, thiz->fbstride
          );
      thiz->_framebuf =
          x_malloc(thiz->w * thiz->h * 4);
      thiz->fbw = thiz->w;
      thiz->fbh = thiz->h;
      thiz->vinfo.bits_per_pixel = 32;
      thiz->fbstride = thiz->w * 4;
    }
  else
    {
      TRACE("Mapped framebuffer OK: %s, "
          "%dx%d, stride(%d), bpp(%d)\n",strerror(errno),
          thiz->fbw, thiz->fbh, thiz->fbstride, thiz->vinfo.bits_per_pixel
          );
//     unsigned int *vram;
//      unsigned int *ip;
//      int i;
//      int j;
//      for (i = 0; i < thiz->w; i++)
//        {
//          ip = vram;
//          for (j = 0; j < thiz->h; j++)
//            {
//              *ip = 0x00FF0000 | ((i & 0xFF) << 8) | (j & 0xFF);
//              ip++;
//            }
//          vram += thiz->w;
//        }
    }

  TRACE("Opened FB0: this(%p) (f1=%p,f2=%p) at %dx%d, vfd(%d)\n",
        obj, thiz->_framebuf, thiz->framebuf, thiz->w, thiz->h, thiz->vfd);
#else
  thiz->_framebuf = x_malloc(thiz->w * thiz->h * 5);
#endif

  /** @todo This should be moved in x_object API */
  ev_periodic_init(&thiz->cam_qwartz, __xqvfb_periodic_cb, 0., 0.0667, 0);

  x_object_add_watcher(obj, &thiz->cam_qwartz, EVT_PERIODIC);

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
//          __android_log_print(ANDROID_LOG_ERROR, "[ xwjni ]",
//              "%s(): WxH(%dx%d),i(%d),j(%d): %p -> %p", __FUNCTION__, w, h, i,
//              j, chromaFROM, chromaTO);
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
__gobee_yuv_set_data(droid_camera_t *thiz,
    gobee_img_frame_t yuv, void *data)
{
#if 0
  x_memcpy(thiz->framebuf,data,thiz->w * thiz->h);

  nv21_to_i420p(
      &data[thiz->w * thiz->h],
      &thiz->framebuf[thiz->w * thiz->h],
      thiz->w,
      thiz->h);
#endif

  yuv[0].data = thiz->framebuf;
  yuv[1].data = &yuv[0].data[0]
          + yuv[0].width * yuv[0].height;
  yuv[2].data = &yuv[1].data[0]
          + yuv[1].width * yuv[1].height;

  yuv[0].width = thiz->w;
  yuv[0].height = thiz->h;
  yuv[0].stride = thiz->w;

  yuv[1].width = thiz->w >> 1;
  yuv[1].height = thiz->h >> 1;
  yuv[1].stride = thiz->w >> 1;

  yuv[2].width = thiz->w >> 1;
  yuv[2].height = thiz->h >> 1;
  yuv[2].stride = thiz->w >> 1;

}

static int
__xqvfb_write_frame_to_encoder_hlp(
    droid_camera_t *thiz , gobee_img_frame_t yuv)
{
    int len;
    char buf[16];
    x_iobuf_t planes[3];
    x_obj_attr_t hints =
    { 0, 0, 0 };

    /* pass to next hop (encoder) */

    X_IOV_DATA(&planes[0]) = yuv[0].data;
    X_IOV_LEN(&planes[0]) = yuv[0].height * yuv[0].stride;

    X_IOV_DATA(&planes[1]) = yuv[1].data;
    X_IOV_LEN(&planes[1]) = yuv[1].height * yuv[1].stride;

    X_IOV_DATA(&planes[2]) = yuv[2].data;
    X_IOV_LEN(&planes[2]) = yuv[2].height * yuv[2].stride;

    if (_WRITEV(thiz->xobj.write_handler, planes, 3, NULL) < 0)
    {
        len = yuv[0].height * yuv[0].stride * 3 / 2;
        _WRITE(thiz->xobj.write_handler, yuv[0].data, len, NULL);
    }
}

static void
x_droid_cam_on_write_frame(x_object *xobj,
    void *framedata, size_t siz, x_obj_attr_t *attrs)
{
  int err;
  gobee_img_frame_t img;
  x_object *tmp;
  time_t stamp = time(NULL);
  droid_camera_t *thiz = (droid_camera_t *)(void *)xobj;

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

#if 0
  __gobee_yuv_set_data(thiz, img, framedata);
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
x_droid_cam_on_append(x_object *so, x_object *parent)
{
  int ofd;
  droid_camera_t *mctl = (droid_camera_t *) so;
  ENTER;

  mctl->w = 320;
  mctl->h = 240;

  mctl->framebuf = x_malloc(mctl->w * mctl->h * 4 /*/ 2 + 1*/);

  /* init capture */
#ifndef DROID_FB_CAMERA
  create_camera(_ENV(so,"sid"), so);
#else // framebuffer source
  create_camera_fb(_ENV(so,"sid"), so);
#endif

  TRACE("Opened camera at %dx%d\n", mctl->w, mctl->h);

#if 0
#ifdef DEBUG
  ofd = open("yuv.out.dump", O_RDWR | O_TRUNC | O_CREAT,
      S_IRWXU | S_IRWXG | S_IRWXO);
  close(ofd);
#endif
#endif

  EXIT;
  return 0;
}

static x_object *
x_droid_cam_on_assign(x_object *this__, x_obj_attr_t *attrs)
{
  droid_camera_t *mctl = (droid_camera_t *) this__;
  ENTER;

  x_object_default_assign_cb(this__, attrs);

  EXIT;
  return this__;
}

static void
x_droid_cam_exit(x_object *this__)
{
  droid_camera_t *mctl = (droid_camera_t *) this__;
  ENTER;
  TRACE("\n");

  EXIT;
}

static struct xobjectclass droid_cam_class;

static void
___droid_cam_on_x_path_event(void *_obj)
{
  x_object *obj = X_OBJECT(_obj);
  x_object *ob;

  ENTER;

  ob = x_object_add_path(obj, X_IN_DEVDIR "/" CAMERA_VENDOR_ID, NULL);

  EXIT;
}

static struct x_path_listener droid_cam_bus_listener;

__x_plugin_visibility
__plugin_init void
x_droidcam_init(void)
{
  ENTER;
  droid_cam_class.cname = _XS("camera");
  droid_cam_class.psize = (sizeof(droid_camera_t) - sizeof(x_object));
  droid_cam_class.on_assign = &x_droid_cam_on_assign;
  droid_cam_class.on_append = &x_droid_cam_on_append;
  droid_cam_class.finalize = &x_droid_cam_exit;
  droid_cam_class.try_write = &x_droid_cam_on_write_frame;

  x_class_register_ns(droid_cam_class.cname, &droid_cam_class,
      _XS("gobee:media"));

  droid_cam_bus_listener.on_x_path_event = &___droid_cam_on_x_path_event;

  /**
   * Listen for 'session' object from "urn:ietf:params:xml:ns:xmpp-session"
   * namespace. So presence will be activated just after session xmpp is
   * activated.
   */
  x_path_listener_add("/", &droid_cam_bus_listener);
  EXIT;
  return;
}

PLUGIN_INIT(x_droidcam_init)
;


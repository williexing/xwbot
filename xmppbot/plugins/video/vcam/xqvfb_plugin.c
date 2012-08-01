/**
 * xvirtdisplay_view.c
 *
 *  Created on: Jun 6, 2012
 *      Author: artemka
 */

#undef DEBUG_PRFX
#include <xwlib/x_config.h>
#if TRACE_VIRTUAL_DIPSLAY_ON
#define TRACE_LEVEL 2
#define DEBUG_PRFX "[VIRT-QVFB] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#define X_IN_DEVDIR "__proc/in"
#define CAMERA_VENDOR_ID "qvfb_Camera"

#include "xqvfb.h"

typedef struct x_qvfb_cam
{
  x_object xobj;
  struct ev_periodic cam_qwartz;
  int displaypipe;
  unsigned char *displayBuf;
  unsigned char *_framebuf;
  unsigned char *framebuf;
  int w;
  int h;
} x_qvfb_cam_t;

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
rgb2yuv(unsigned char *from, unsigned char *to, int w, int h)
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
          r = from[(y * w + x) * 3 + 0];
          g = from[(y * w + x) * 3 + 1];
          b = from[(y * w + x) * 3 + 2];
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

static void
__xqvfb_periodic_cb(struct ev_loop *loop, struct ev_periodic *w, int revents)
{
  int ofd;
  int granule;
  x_object *tmp;
  x_qvfb_cam_t *_qvfb = w->data;

  TRACE("\n");

  if (!_qvfb->xobj.write_handler)
    {
      tmp = _CHLD(_PARNT(X_OBJECT(_qvfb)), "out$media");
      tmp = _CHLD(tmp,NULL);
      if (!tmp)
        {
          TRACE("No output channel... Decreasing camera status...\n");
          ERROR;
          return;
        }
      else
        {
          _qvfb->xobj.write_handler = tmp;
          _REFGET(_qvfb->xobj.write_handler);
        }
    }
  else
    {
      TRACE("Handler OK\n");
    }

  /**
   * @todo Make length correct here!!!!
   */
  // convert first
  rgb2yuv(_qvfb->_framebuf, _qvfb->framebuf, _qvfb->w, _qvfb->h);

  TRACE("next frame: this(%p) buf(%p,%p), width(%d), height(%d)\n",
      _qvfb, _qvfb->_framebuf, &_qvfb->framebuf, _qvfb->w, _qvfb->h);

  _WRITE(_qvfb->xobj.write_handler, _qvfb->framebuf, _qvfb->w*_qvfb->h*3/2,
      NULL);
}

static int
x_qvfb_on_append(x_object *so, x_object *parent)
{
  int ofd;
  x_qvfb_cam_t *mctl = (x_qvfb_cam_t *) so;
  ENTER;

  TRACE("\n");

  mctl->w = 320;
  mctl->h = 240;

  /**
   * @todo Display ID should be dynamically obtained
   */
  /* init capture */
  mctl->displayBuf = qvfb_create(0, mctl->w, mctl->h, 24);
  mctl->_framebuf = mctl->displayBuf + sizeof(struct QVFbHeader);
  mctl->framebuf = x_malloc(mctl->w * mctl->h * 3 / 2 + 1);

  mctl->cam_qwartz.data = (void *) so;

  TRACE("Opened qvfb: this(%p) (f1=%p,f2=%p) at %dx%d\n",
      so, mctl->_framebuf, mctl->framebuf, mctl->w, mctl->h);

  /** @todo This should be moved in x_object API */
  ev_periodic_init(&mctl->cam_qwartz, __xqvfb_periodic_cb, 0., 0.0667, 0);

  x_object_add_watcher(so, &mctl->cam_qwartz, EVT_PERIODIC);

  _ASET(so,_XS("Vendor"),_XS("Gobee Alliance, Inc."));
  _ASET(so,_XS("Descr"),_XS("Virtual Camera, Qvfb buffer interface"));

  EXIT;
  return 0;
}

static x_object *
x_qvfb_on_assign(x_object *this__, x_obj_attr_t *attrs)
{
  x_qvfb_cam_t *mctl = (x_qvfb_cam_t *) this__;
  ENTER;

  x_object_default_assign_cb(this__, attrs);

  EXIT;
  return this__;
}

static void
x_qvfb_exit(x_object *this__)
{
  x_qvfb_cam_t *mctl = (x_qvfb_cam_t *) this__;
  ENTER;
  printf("%s:%s():%d\n", __FILE__, __FUNCTION__, __LINE__);

  if (mctl->xobj.write_handler)
    _REFPUT(mctl->xobj.write_handler, NULL);

  EXIT;
}

static struct xobjectclass xqvfb_class;

static void
___qvfb_on_x_path_event(void *_obj)
{
  x_object *obj = _obj;
  x_object *ob;

  ENTER;

  ob = x_object_add_path(obj, X_IN_DEVDIR "/" CAMERA_VENDOR_ID, NULL);

  EXIT;
}

static struct x_path_listener qvfb_bus_listener;

__x_plugin_visibility
__plugin_init void
x_qvfb_init(void)
{
  ENTER;
  xqvfb_class.cname = _XS("camera");
  xqvfb_class.psize = (sizeof(x_qvfb_cam_t) - sizeof(x_object));
  xqvfb_class.on_assign = &x_qvfb_on_assign;
  xqvfb_class.on_append = &x_qvfb_on_append;
  xqvfb_class.finalize = &x_qvfb_exit;

  x_class_register_ns(xqvfb_class.cname, &xqvfb_class, _XS("gobee:media"));

  qvfb_bus_listener.on_x_path_event = &___qvfb_on_x_path_event;

  /**
   * Listen for 'session' object from "urn:ietf:params:xml:ns:xmpp-session"
   * namespace. So presence will be activated just after session xmpp is
   * activated.
   */
  x_path_listener_add("/", &qvfb_bus_listener);
  EXIT;
  return;
}

PLUGIN_INIT(x_qvfb_init)
;


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
#define DEBUG_PRFX "[VIRT-CAMERA] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#define X_IN_DEVDIR "__proc/in"
#define CAMERA_VENDOR_ID "v4l2_Camera"

int
v4l2_capture_init(unsigned char **, int *w, int *h);
int
v4l2_next_frame(int fd);
void
nv21_to_i420p(unsigned char *chromaFROM, unsigned char *chromaTO, int w, int h);
void
YUYV_to_i420p(unsigned char *from, unsigned char *to, int w, int h);

typedef struct virtual_camera
{
  x_object xobj;
  struct ev_periodic cam_qwartz;
  int vfd;
  unsigned char *_framebuf;
  unsigned char *framebuf;
  int w;
  int h;
} virtual_camera_t;

static void
__vcam_periodic_cb(struct ev_loop *loop, struct ev_periodic *w, int revents)
{
  int ofd;
  int granule;
  x_object *tmp;
  virtual_camera_t *_vcam = w->data;

  TRACE("\n");

  if (!(granule++ % 200))
    {
//      gobee_theora_resize(cdata->w, cdata->h, cdata->w, sock,
//          (struct sockaddr *) &cli, sizeof(cli));
    }
  if (v4l2_next_frame(_vcam->vfd) == 0)
    {
      if (!_vcam->xobj.write_handler)
        {
          tmp = _CHLD(_PARNT(X_OBJECT(_vcam)), "out$media");
          tmp = _CHLD(tmp,NULL);
          if (!tmp)
            {
              TRACE("No output channel... Decreasing camera status...\n");
              ERROR;
              return;
            }
          else
            {
              _vcam->xobj.write_handler = tmp;
              _REFGET(_vcam->xobj.write_handler);
            }
        }

      /**
       * @todo Make length correct here!!!!
       */
      // convert first
      YUYV_to_i420p(&_vcam->_framebuf[0], &_vcam->framebuf[0], _vcam->w,
          _vcam->h);

      _WRITE(_vcam->xobj.write_handler, _vcam->framebuf, _vcam->w*_vcam->h*3/2,
          NULL);

#ifdef DEBUG
      ofd = open("yuv.out.dump", O_WRONLY | O_APPEND);
      write(ofd,_vcam->_framebuf,_vcam->w*_vcam->h*2);
      close(ofd);
#endif

    }
}

static int
x_vcam_on_append(x_object *so, x_object *parent)
{
  int ofd;
  virtual_camera_t *mctl = (virtual_camera_t *) so;
  ENTER;

  mctl->w = 320;
  mctl->h = 240;

  /* init capture */
  mctl->vfd = v4l2_capture_init(&mctl->_framebuf, &mctl->w, &mctl->h);
  mctl->framebuf = x_malloc(mctl->w * mctl->h * 3 / 2 + 1);

  TRACE("Opend camera at %dx%d\n", mctl->w, mctl->h);

  mctl->cam_qwartz.data = (void *) so;

  /** @todo This should be moved in x_object API */
  ev_periodic_init(&mctl->cam_qwartz, __vcam_periodic_cb, 0., 0.06667, 0);

  x_object_add_watcher(so, &mctl->cam_qwartz, EVT_PERIODIC);

#ifdef DEBUG
  ofd = open("yuv.out.dump", O_RDWR | O_TRUNC | O_CREAT,
      S_IRWXU | S_IRWXG | S_IRWXO);
  close(ofd);
#endif

  EXIT;
  return 0;
}

static x_object *
x_vcam_on_assign(x_object *this__, x_obj_attr_t *attrs)
{
  virtual_camera_t *mctl = (virtual_camera_t *) this__;
  ENTER;

  x_object_default_assign_cb(this__, attrs);

  EXIT;
  return this__;
}

static void
x_vcam_exit(x_object *this__)
{
  virtual_camera_t *mctl = (virtual_camera_t *) this__;
  ENTER;
  printf("%s:%s():%d\n",__FILE__,__FUNCTION__,__LINE__);

  EXIT;
}

static struct xobjectclass virt_cam_class;

static void
___vcam_on_x_path_event(void *_obj)
{
  x_object *obj = _obj;
  x_object *ob;

  ENTER;

  ob = x_object_add_path(obj, X_IN_DEVDIR "/" CAMERA_VENDOR_ID, NULL);

  EXIT;
}

static struct x_path_listener vcam_bus_listener;

__x_plugin_visibility
__plugin_init void
x_virtcam_init(void)
{
  ENTER;
  virt_cam_class.cname = _XS("camera");
  virt_cam_class.psize = (sizeof(virtual_camera_t) - sizeof(x_object));
  virt_cam_class.on_assign = &x_vcam_on_assign;
  virt_cam_class.on_append = &x_vcam_on_append;
  virt_cam_class.finalize = &x_vcam_exit;

  x_class_register_ns(virt_cam_class.cname, &virt_cam_class,
      _XS("gobee:media"));

  vcam_bus_listener.on_x_path_event = &___vcam_on_x_path_event;

  /**
   * Listen for 'session' object from "urn:ietf:params:xml:ns:xmpp-session"
   * namespace. So presence will be activated just after session xmpp is
   * activated.
   */
  x_path_listener_add("/", &vcam_bus_listener);
  EXIT;
  return;
}

PLUGIN_INIT(x_virtcam_init)
;


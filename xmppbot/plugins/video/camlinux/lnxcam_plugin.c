/**
 * lnxcam_plugin.c
 *
 *  Created on: Jun 6, 2012
 *      Author: artemka
 */

#undef DEBUG_PRFX
#include <x_config.h>
#if TRACE_VIRTUAL_DIPSLAY_ON
//#define TRACE_LEVEL 2
#define DEBUG_PRFX "[VIRT-lnxcam] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include <sys/types.h>
#include <signal.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <linux/videodev2.h>

#define X_IN_DEVDIR "__proc/in"
#define CAMERA_VENDOR_ID "lnxcam_Camera"

//#include "lnxcam.h"

static __inline unsigned char
clamp(int d)
{
    if (d < 0)
        return 0;

    if (d > 255)
        return 255;

    return (unsigned char) d;
}

static __inline void
YUYV_to_i420p(unsigned char *__restrict from,
              unsigned char *__restrict to, int w, int h)
{
  unsigned char *luma;
  unsigned char *chromaU;
  unsigned char *chromaV;
  int i, j, k;
  int w2 = w >> 1;
  int h2 = h >> 1;
  int stride = w * 2; /* w * YUYV */

  luma = &to[0];
  chromaU = &to[w * h];
  chromaV = &chromaU[w2 * h2];

//  fprintf(stderr, "%s(): w=%d,h=%d\n", __FUNCTION__, w, h);

  for (j = 0; j < h; j++)
    {
      for (i = 0; i < w; i++)
        {
          luma[j * w + i] = from[j * stride + i * 2];

          if (!(j & 0x1)) /* skip odd rows of chroma */
            {
              if (!(i & 0x1)) /* switch chroma planes */
                chromaU[j >> 1 * w2 + i >> 1] = from[j * stride + i * 2 + 1];
              else
                chromaV[j >> 1 * w2 + i >> 1] = from[j * stride + i * 2 + 1];
            }

        }
    }
}


static __inline void
YUYV_to_RGB24(unsigned char *__restrict from,
              unsigned char *__restrict to, int w, int h)
{
  unsigned char *rgb;
  int i, j;
  int stride = w * 2; /* w * YUYV */
  double Y1,Y2;
  double u;
  double v;

  for (j = 0; j < h; j++)
  {
      for (i = 0; i < w; i+=2)
      {
          rgb = &to[ (j*w + i) * 3 ];

          Y1 = from[j * stride + i * 2]  - 16;
          u = from[j * stride + i * 2 + 1]  -128;

          Y2 = from[j * stride + i * 2 + 2] - 16;
          v = from[j * stride + i * 2 + 3] - 128;

#if 1
          // first pixel
          rgb[0] = clamp((int)(1.164*Y1 + 1.596 * v));
          rgb[1] = clamp((int)(1.164*Y1 - 0.813 * v - 0.391 * u));
          rgb[2] = clamp((int)(1.164*Y1 + 2.018 * u));
//          rgb[0] = 0xff;

          // second pixel
          rgb[3] = clamp((int)(1.164*Y2 + 1.140 * v));
          rgb[4] = clamp((int)(1.164*Y2 - 0.813 * v - 0.581 * u));
          rgb[5] = clamp((int)(1.164*Y2 + 2.018 * u));
//          rgb[4] = 0xff;
#else
          // first pixel
          rgb[0] = clamp(Y1);
          rgb[1] = clamp(Y1);
          rgb[2] = clamp(Y1);
          rgb[3] = 0xff;

          // second pixel
          rgb[4] = clamp(Y2);
          rgb[5] = clamp(Y2);
          rgb[6] = clamp(Y2);
          rgb[7] = 0xff;
#endif
      }
  }
}


static __inline void
nv21_to_i420p(unsigned char *__restrict chromaFROM,
              unsigned char *__restrict chromaTO, int w, int h)
{
  unsigned char *chromaU;
  unsigned char *chromaV;
  int i, j, k;
  int w2 = w >> 1;
  int h2 = h >> 1;

  chromaU = &chromaTO[0];
  chromaV = &chromaTO[w2 * h2];

  for (j = 0; j < h; j++)
    {
      for (i = 0; i < w; i++)
        {
          chromaU[j / 2 * w / 2 + i / 2] = chromaFROM[j / 2 * w + i];
          chromaV[j / 2 * w / 2 + i / 2] = chromaFROM[j / 2 * w + i + 1];
        }
    }
}

typedef struct x_lnx_cam
{
    x_object xobj;
    struct ev_periodic cam_qwartz;
    unsigned char *_framebuf;
    unsigned char *framebuf;
    int w;
    int h;
    int vfd;
} x_lnx_cam_t;


static __inline void *
__lnxcam_next_frame(x_lnx_cam_t *thiz)
{
    struct v4l2_buffer buf;

    memset(&(buf), 0, sizeof(struct v4l2_buffer));

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (-1 == ioctl(thiz->vfd, VIDIOC_DQBUF, &buf))
    {
        switch (errno)
        {
        case EAGAIN:
            TRACE("%s():%d\n", __FUNCTION__, __LINE__);
            return NULL;

        case EIO:
            TRACE("%s():%d\n", __FUNCTION__, __LINE__);
            return NULL;
        case EINVAL:
            TRACE("%s():%d\n", __FUNCTION__, __LINE__);
            return NULL;
        case ENOMEM:
            TRACE("%s():%d\n", __FUNCTION__, __LINE__);
            return NULL;
        default:
            perror("Getting video frame\n");
            TRACE("%d %s():%d\n", thiz->vfd, __FUNCTION__, __LINE__);
            return NULL;
        }
    }

    if (-1 == ioctl(thiz->vfd, VIDIOC_QBUF, &buf))
    {
        TRACE("%s():%d\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    return thiz->_framebuf;
}

static __inline int
__lnxcam_open(x_lnx_cam_t *thiz)
{
    int index;
    int vfd;
    struct v4l2_buffer vbuf;
    struct v4l2_format fmt;

    struct v4l2_buffer qbuf;
    struct v4l2_requestbuffers req;

    x_string_t devname = "/dev/video0";

    enum v4l2_buf_type type;

    if ((vfd = open(devname, O_RDONLY)) < 0)
    {
        perror("Can't open video device");
        return -1;
    }

    index = 0;
    if (-1 == ioctl(vfd, VIDIOC_S_INPUT, &index))
    {
        perror("Can't set input");
        return -1;
    }

    memset(&(fmt), 0, sizeof(struct v4l2_format));

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = thiz->w;
    fmt.fmt.pix.height = thiz->h;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    //  fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    if (-1 == ioctl(vfd, VIDIOC_S_FMT, &fmt))
    {
        perror("Can't set video format");
        //        fprintf(stderr, "%s():%d\n", __FUNCTION__, __LINE__);
        return -1;
    }

    memset(&(req), 0, sizeof(struct v4l2_requestbuffers));

    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == ioctl(vfd, VIDIOC_REQBUFS, &req))
    {
        if (EINVAL == errno)
        {
            perror("Memory mapping not supported");
            //            fprintf(stderr, "%s does not support "
            //                    "memory mapping\n", "/dev/video0");
            return -1;
        }
        else
        {
            perror("Can't set video VIDIOC_REQBUFS");
            //            fprintf(stderr, "%s():%d\n", __FUNCTION__, __LINE__);
            return -1;
        }
    }

    memset(&(vbuf), 0, sizeof(struct v4l2_buffer));

    vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vbuf.memory = V4L2_MEMORY_MMAP;
    vbuf.index = 0;

    if (-1 == ioctl(vfd, VIDIOC_QUERYBUF, &vbuf))
    {
        perror("Can't set video VIDIOC_QUERYBUF");
        //        fprintf(stderr, "%s():%d\n", __FUNCTION__, __LINE__);
        return -1;
    }

    thiz->_framebuf = mmap(NULL, vbuf.length, PROT_READ, MAP_SHARED, vfd, vbuf.m.offset);

        fprintf(stderr, "%s():%d MAPPED v4l AT ADDR=%p len=%d, offset=%p, [%dx%d]\n",
                __FUNCTION__, __LINE__, thiz->_framebuf, vbuf.length, vbuf.m.offset,
                fmt.fmt.pix.width, fmt.fmt.pix.height);

    memset(&(qbuf), 0, sizeof(struct v4l2_buffer));

    qbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    qbuf.memory = V4L2_MEMORY_MMAP;
    qbuf.index = 0;

    if (-1 == ioctl(vfd, VIDIOC_QBUF, &qbuf))
    {
        perror("Can't set video VIDIOC_QBUF");
        //        fprintf(stderr, "%s():%d\n", __FUNCTION__, __LINE__);
        return -1;
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(vfd, VIDIOC_STREAMON, &type))
    {
        perror("Can't set video VIDIOC_STREAMON");
        //        fprintf(stderr, "%s():%d\n", __FUNCTION__, __LINE__);
        return -1;
    }

    thiz->vfd = vfd;
    thiz->framebuf = x_malloc(thiz->w * thiz->h * 3 / 2 + 1);

    return 0;
}



static int
__xlnxcam_check_writehandler_hlp(x_lnx_cam_t *thiz)
{
    char buf[32];
    x_obj_attr_t attrs =
    { 0, 0, 0 };

    x_object *tmp;

#ifdef SYNAPTIC_API
    if (thiz->xobj.writeout_vector.next)
    {
        return 0;
    }
    else
#endif
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

            setattr("width", _AGET(thiz,"width"), &attrs);
            setattr("stride",_AGET(thiz,"width"), &attrs);
            setattr("height",_AGET(thiz,"height"),  &attrs);

            _ASGN(tmp,&attrs);

        }
    }
    else
    {
        TRACE("Handler OK\n");
    }

    return 0;
}

static int
__xlnxcam_write_frame_to_encoder_hlp(
        x_lnx_cam_t *thiz , gobee_img_frame_t yuv)
{
    int len;
    char buf[16];
    x_iobuf_t planes[3];
//    x_obj_attr_t hints =
//    { 0, 0, 0 };

    /* pass to next hop (display) */
    X_IOV_DATA(&planes[0]) = yuv[0].data;
    X_IOV_LEN(&planes[0]) = yuv[0].height * yuv[0].stride;

    X_IOV_DATA(&planes[1]) = yuv[1].data;
    X_IOV_LEN(&planes[1]) = yuv[1].height * yuv[1].stride;

    X_IOV_DATA(&planes[2]) = yuv[2].data;
    X_IOV_LEN(&planes[2]) = yuv[2].height * yuv[2].stride;

#if 0
    if(thiz->xobj.write_handler)
    if (_WRITEV(thiz->xobj.write_handler, planes, 3, NULL) < 0)
    {
        len = yuv[0].height * yuv[0].stride * 3 / 2;
        _WRITE(thiz->xobj.write_handler, yuv[0].data, len, NULL);
    }
#endif

#ifdef SYNAPTIC_API
    x_object_writev_out(thiz,planes,3,NULL);
#endif

}

static void
__gobee_yuv_set_data(gobee_img_frame_t yuv, void *data)
{
    yuv[0].data = data;
    yuv[1].data = &yuv[0].data[0]
            + yuv[0].width * yuv[0].height;
    yuv[2].data = &yuv[1].data[0]
            + yuv[1].width * yuv[1].height;
}

static void
__xlnxcam_periodic_cb(xevent_dom_t *loop, struct ev_periodic *w, int revents)
{
    gobee_img_frame_t img;
    x_lnx_cam_t *thiz = w->data;

    TRACE("\n");

    if (!thiz || !_CRGET(thiz) || !thiz->framebuf
            || __xlnxcam_check_writehandler_hlp(thiz) != 0)
    {
        TRACE("ERROR State! thiz(%p), thiz->framebuf(%d)\n",thiz,thiz->framebuf);
//        return;
    }

    if(!__lnxcam_next_frame(thiz))
    {
        TRACE("ERROR! Cannot get frame\n");
        return;
    }

    // convert first
    YUYV_to_i420p(thiz->_framebuf, thiz->framebuf, thiz->w, thiz->h);
//        TRACE("next frame: this(%p) buf(%p,%p), width(%d), height(%d)\n",
//              thiz, thiz->_framebuf, thiz->framebuf, thiz->w, thiz->h);
    img[0].width = thiz->w;
    img[0].stride = thiz->w;
    img[0].height = thiz->h;

    img[1].width = thiz->w / 2;
    img[1].stride = thiz->w / 2;
    img[1].height = thiz->h / 2;

    img[2].width = thiz->w / 2;
    img[2].stride = thiz->w / 2;
    img[2].height = thiz->h / 2;

    __gobee_yuv_set_data(img,thiz->framebuf);
    __xlnxcam_write_frame_to_encoder_hlp(thiz,img);

}

static int
x_lnxcam_on_append(x_object *_thiz, x_object *parent)
{
    int displayId = 0;
    x_lnx_cam_t *thiz = (x_lnx_cam_t *) _thiz;
    ENTER;

    TRACE("\n");

    thiz->w = 320;
    thiz->h = 240;

    if(__lnxcam_open(thiz) < 0)
    {
        EXIT;
        return -1;
    }

    TRACE("Opened lnxcam: DisplayID(%d), this(%p) (f1=%p,f2=%p) at %dx%d\n",
          displayId, _thiz, thiz->_framebuf, thiz->framebuf, thiz->w, thiz->h);

    thiz->cam_qwartz.data = (void *) _thiz;

    /** @todo This should be moved in x_object API */
    ev_periodic_init(&thiz->cam_qwartz, __xlnxcam_periodic_cb, 0., 0.0667, 0);

    x_object_add_watcher(_thiz, &thiz->cam_qwartz, EVT_PERIODIC);

    _ASET(_thiz,_XS("Vendor"),_XS("Gobee Alliance, Inc."));
    _ASET(_thiz,_XS("Descr"),_XS("Virtual Camera, lnxcam interface"));

    _ASET(_thiz,"width","320");
    _ASET(_thiz,"stride","320");
    _ASET(_thiz,"height","240");

    // caps tags setup
    _ASET(_thiz,"linux","yes");
    _ASET(_thiz,"os","linux");
    _ASET(_thiz,"camera","native");
    _ASET(_thiz,"v4l","yes");

    _CRSET(thiz,1);

    EXIT;
    return 0;
}


static void
x_lnxcam_on_remove(x_object *_thiz, x_object *parent)
{
    x_lnx_cam_t *thiz = (x_lnx_cam_t *) _thiz;
    ENTER;

    TRACE("\n");

    thiz->_framebuf = NULL;
    if(thiz->framebuf)
    {
        x_free(thiz->framebuf);
        thiz->framebuf = NULL;
    }

    thiz->cam_qwartz.data = NULL;

    /** @todo This should be moved in x_object API */
    //    ev_periodic_init(&thiz->cam_qwartz, __xlnxcam_periodic_cb, 0., 0.0667, 0);

    x_object_remove_watcher(_thiz, &thiz->cam_qwartz, EVT_PERIODIC);

    _REFPUT(thiz->xobj.write_handler, NULL);
    thiz->xobj.write_handler = NULL;

    EXIT;
    return 0;
}

static x_object *
x_lnxcam_on_assign(x_object *_thiz, x_obj_attr_t *attrs)
{
    x_lnx_cam_t *thiz = (x_lnx_cam_t *) _thiz;
    ENTER;

    x_object_default_assign_cb(_thiz, attrs);

    EXIT;
    return _thiz;
}

static void
x_lnxcam_exit(x_object *this__)
{
    x_lnx_cam_t *mctl = (x_lnx_cam_t *) this__;
    ENTER;
    EXIT;
}

static struct xobjectclass xlnxcam_class;

static void
___lnxcam_on_x_path_event(void *_obj)
{
    x_object *obj = _obj;
    x_object *ob;

    ENTER;

    ob = x_object_add_path(obj, X_IN_DEVDIR "/" CAMERA_VENDOR_ID, NULL);

    EXIT;
}

static struct x_path_listener lnxcam_bus_listener;

__x_plugin_visibility
__plugin_init void
x_lnxcam_init(void)
{
    ENTER;
    xlnxcam_class.cname = _XS("camera");
    xlnxcam_class.psize = (sizeof(x_lnx_cam_t) - sizeof(x_object));
    xlnxcam_class.on_assign = &x_lnxcam_on_assign;
    xlnxcam_class.on_append = &x_lnxcam_on_append;
    xlnxcam_class.on_remove = &x_lnxcam_on_remove;
    xlnxcam_class.commit = &x_lnxcam_exit;

    x_class_register_ns(xlnxcam_class.cname, &xlnxcam_class, _XS("gobee:media"));

    lnxcam_bus_listener.on_x_path_event = &___lnxcam_on_x_path_event;

    /**
       * Listen for 'session' object from "urn:ietf:params:xml:ns:xmpp-session"
       * namespace. So presence will be activated just after session xmpp is
       * activated.
       */
    x_path_listener_add("/", &lnxcam_bus_listener);
    EXIT;
    return;
}

PLUGIN_INIT(x_lnxcam_init)
;


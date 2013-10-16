/**
 * xvirtdisplay_view.c
 *
 *  Created on: Jun 6, 2012
 *      Author: artemka
 */

#undef DEBUG_PRFX
#include <x_config.h>
#if TRACE_VIRTUAL_DIPSLAY_ON
//#define TRACE_LEVEL 2
#define DEBUG_PRFX "[VIRT-QVFB] "
#endif

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib/x_obj.h>

#include <sys/types.h>
#include <signal.h>

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
    pid_t mypid;
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


static int
__xqvfb_check_writehandler_hlp(x_qvfb_cam_t *thiz)
{
    char buf[32];
    x_obj_attr_t attrs =
    { 0, 0, 0 };

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
__xqvfb_write_frame_to_encoder_hlp(
    x_qvfb_cam_t *thiz , gobee_img_frame_t yuv)
{
    int len;
    char buf[16];
    x_iobuf_t planes[3];
    x_obj_attr_t hints =
    { 0, 0, 0 };

//    TRACE("New frame ready (%p),0{%d,%d,%d,%p},"
//          "1{%d,%d,%d,%p},"
//          "2{%d,%d,%d,%p}... playing\n",
//          yuv, yuv[0].width, yuv[0].height,
//          yuv[0].stride, yuv[0].data, yuv[1].width,
//          yuv[1].height, yuv[1].stride, yuv[1].data,
//          yuv[2].width, yuv[2].height, yuv[2].stride,
//          yuv[2].data);

#if 0
    if (thiz->ywidth != ycbcr[0].width
            || thiz->yheight != ycbcr[0].height
            || thiz->stride != ycbcr[0].stride)
    {
        x_sprintf(buf,"%d",ycbcr[0].width);
        setattr("width",buf,&hints);

        x_sprintf(buf,"%d",ycbcr[0].height);
        setattr("height",buf,&hints);

        x_sprintf(buf,"%d",ycbcr[0].stride);
        setattr("stride",buf,&hints);
        _ASGN(thiz->obj.write_handler,&hints);

        // update local states
        thiz->ywidth = ycbcr[0].width;
        thiz->yheight = ycbcr[0].height;
        thiz->stride = ycbcr[0].stride;
    }
#endif

    /* pass to next hop (display) */

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
__gobee_yuv_set_data(gobee_img_frame_t yuv, void *data)
{
    yuv[0].data = data;
    yuv[1].data = &yuv[0].data[0]
            + yuv[0].width * yuv[0].height;
    yuv[2].data = &yuv[1].data[0]
            + yuv[1].width * yuv[1].height;
}

static void
__xqvfb_periodic_cb(xevent_dom_t *loop, struct ev_periodic *w, int revents)
{
    gobee_img_frame_t img;
    x_qvfb_cam_t *thiz = w->data;

//    TRACE("\n");

    if (!thiz || !thiz->framebuf
            || __xqvfb_check_writehandler_hlp(thiz) != 0)
    {
        TRACE("ERROR State! thiz(%p), thiz->framebuf(%d)\n",thiz,thiz->framebuf);
        return;
    }

    // convert first
    rgb2yuv(thiz->_framebuf, thiz->framebuf, thiz->w, thiz->h);

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

    __gobee_yuv_set_data(img,thiz->framebuf);
    __xqvfb_write_frame_to_encoder_hlp(thiz,img);

    //    _WRITE(thiz->xobj.write_handler, thiz->framebuf, thiz->w*thiz->h*3/2,NULL);
}

static void
x_qvfb_run_external_vfb(x_qvfb_cam_t *thiz, QVFbSession *qvfbsess)
{
    unsigned char args[128];
    pid_t mypid;
    char *execmd = "/DATA/qt_e/qte-4.7.3/bin/qtdemo";
//    char *execmd = "/DATA/projects/Remoting/GobeeCarDriver-build-desktop/GobeeCarDriver";
//    char *execmd = "/bin/true";

    mypid = fork();

    x_sprintf(args,"QVFB:%d",qvfbsess->dId);

    printf("Running command:\n"
           "%s %s\n", execmd,args);

    if (mypid == 0)
    {
        if (execl(execmd, execmd, "-qws","-display", args, NULL) < 0)
            exit(0);
    }
    else
    {
        thiz->mypid = mypid;
    }
}


static int
x_qvfb_on_append(x_object *so, x_object *parent)
{
    QVFbSession *qvfbsess;
    int displayId = 0;
    x_qvfb_cam_t *thiz = (x_qvfb_cam_t *) so;
    ENTER;

    TRACE("\n");

    thiz->w = 320;
    thiz->h = 240;

    qvfbsess = qvfb_create(_ENV(so,"sid"), thiz->w, thiz->h, 24);
    if (!qvfbsess)
    {
        TRACE("Cannot generate Display ID!!!!!\n");
        return -1;
    }

    thiz->displayBuf = qvfbsess->displayMem;
    thiz->_framebuf = thiz->displayBuf + sizeof(struct QVFbHeader);
    thiz->framebuf = x_malloc(thiz->w * thiz->h * 3 / 2 + 1);

    TRACE("Opened qvfb: DisplayID(%d), this(%p) (f1=%p,f2=%p) at %dx%d\n",
          displayId, so, thiz->_framebuf, thiz->framebuf, thiz->w, thiz->h);

    thiz->cam_qwartz.data = (void *) so;

    /** @todo This should be moved in x_object API */
    ev_periodic_init(&thiz->cam_qwartz, __xqvfb_periodic_cb, 0., 0.0667, 0);

    x_object_add_watcher(so, &thiz->cam_qwartz, EVT_PERIODIC);

    _ASET(so,_XS("Vendor"),_XS("Gobee Alliance, Inc."));
    _ASET(so,_XS("Descr"),_XS("Virtual Camera, Qvfb buffer interface"));

    _ASET(so,"width","320");
    _ASET(so,"stride","320");
    _ASET(so,"height","240");

    // run qvfb external program
    x_qvfb_run_external_vfb(thiz,qvfbsess);

    EXIT;
    return 0;
}


static void
x_qvfb_on_remove(x_object *so, x_object *parent)
{
//    QVFbSession *qvfbsess;
//    int displayId = 0;
    x_qvfb_cam_t *thiz = (x_qvfb_cam_t *) so;
    ENTER;

    TRACE("\n");

//    qvfbsess = qvfb_create(_ENV(so,"sid"), thiz->w, thiz->h, 24);

    thiz->displayBuf = NULL;
    thiz->_framebuf = NULL;
    if(thiz->framebuf)
    {
        x_free(thiz->framebuf);
        thiz->framebuf = NULL;
    }

    thiz->cam_qwartz.data = NULL;

    /** @todo This should be moved in x_object API */
//    ev_periodic_init(&thiz->cam_qwartz, __xqvfb_periodic_cb, 0., 0.0667, 0);

    x_object_remove_watcher(so, &thiz->cam_qwartz, EVT_PERIODIC);

    // stop qvfb external program
//    x_qvfb_run_external_vfb(thiz,qvfbsess);
    if (thiz->mypid)
    {
        kill(thiz->mypid,SIGTERM);
    }

    _REFPUT(thiz->xobj.write_handler, NULL);
    thiz->xobj.write_handler = NULL;

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
    xqvfb_class.on_remove = &x_qvfb_on_remove;
    xqvfb_class.commit = &x_qvfb_exit;

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


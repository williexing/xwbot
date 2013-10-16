#include "gobeedisplay.h"
#include "../displayframe.h"

#include <x_config.h>
#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib++/x_objxx.h>

#include <stdio.h>

/* native interface callbacks */
extern void pushWidgetToShow(gobee::__x_object *gbus,
                             const char *__rj,
                             const char *__sid, void *buf,
                             int w, int h, void **qwgt);

extern int pushWidgetInvalidate(gobee::__x_object *gbus,
                                 void *qwgt);

#define X_OUT_DEVDIR "__proc/out"
#define DISPLAY_VENDOR_ID "QtDisplay_NO_GL"

#if 0
typedef struct gobee_img_plane
{
    /**The width of this plane.*/
    int width;
    /**The height of this plane.*/
    int height;
    /**The offset in bytes between successive rows.*/
    int stride;
    /**A pointer to the beginning of the first row.*/
    unsigned char *data;
} gobee_img_plane_t;

typedef gobee_img_plane_t gobee_img_frame_t[3];
#endif

static __inline__ unsigned char
clamp(int d)
{
    if (d < 0)
        return 0;

    if (d > 255)
        return 255;

    return (unsigned char) d;
}

static /*__inline__*/ void
yuv2rgb32 (gobee_img_frame_t /*__restrict*/ rgb32,
           gobee_img_frame_t /*__restrict*/ yuv)
{
    int x;
    int y;
    float Y,_y;
    float u,_u;
    float v,_v;

    for (y = 0; (y < yuv[0].height
                 && y < rgb32[0].height); y++)
    {
        for (x = 0; (x < yuv[0].width
                     && x < rgb32[0].width); x++)
        {
            Y = yuv[0].data[y * yuv[0].stride + x] - 16;

//            if (!(x % 2) && !(y % 2))
//            {
                u = yuv[1].data[(y >> 1) * yuv[1].stride + (x >> 1)];
                v = yuv[2].data[(y >> 1) * yuv[2].stride + (x >> 1)];
                u-=128;
                v-=128;
//            }

            rgb32[0].data[(y * rgb32[0].width + x) * 3 + 0]
                    = clamp((int)(1.164*Y + 1.596 * v));
            rgb32[0].data[(y * rgb32[0].width + x) * 3 + 1]
                    = clamp((int)(1.164*Y - 0.813 * v - 0.391 * u));
            rgb32[0].data[(y * rgb32[0].width + x) * 3 + 2]
                    = clamp((int)(1.164*Y + 2.018 * u));
        }
    }
}


int
GobeeDisplay::try_write(void *buf, u_int32_t len, x_obj_attr_t *attr)
{
    gobee_img_frame_t rgb32;
    gobee_img_frame_t yuv;


    rgb32[0].data = (unsigned char *)framebuf;
    rgb32[0].width = framewidth;
    rgb32[0].stride = framewidth;
    rgb32[0].height = frameheight;

    yuv[0].data = (unsigned char *)buf;
    yuv[0].width = srcwidth;
    yuv[0].height = srcheight;
    yuv[0].stride = srcstride > srcwidth ? srcstride : srcwidth;

    yuv[1].width = yuv[0].width / 2;
    yuv[1].height = yuv[0].height / 2;
    yuv[1].stride = yuv[0].stride / 2;

    yuv[2].width = yuv[1].width;
    yuv[2].height = yuv[1].height;
    yuv[2].stride = yuv[1].stride;

    yuv[1].data = yuv[0].data + yuv[0].stride * yuv[0].height;
    yuv[2].data = yuv[1].data + yuv[1].stride * yuv[1].height;

    yuv2rgb32(rgb32,yuv);

    pushWidgetInvalidate(
                reinterpret_cast<gobee::__x_object *>(this->xobj.bus),
                nativedisplay);
    return len;
}

int
GobeeDisplay::on_append(gobee::__x_object *parent)
{
    ENTER;
    const char  *remote,*sid;
    framewidth = 320;
    frameheight = 240;
    framebuf = new uchar[framewidth*(frameheight+1)*4];

    remote = this->__getenv("from");
    if (!remote)
    {
        remote = this->__getenv("remote");
    }

    if (!remote)
    {
        remote = "nobody";
    }

    sid = this->__getenv("sid");
    if (!sid)
    {
        sid = "-1";
    }

    pushWidgetToShow(
                reinterpret_cast<gobee::__x_object *>(this->xobj.bus),
                 remote, sid,
                (void *)framebuf, framewidth, frameheight,
                &nativedisplay);
    EXIT;
    return 0;
}

gobee::__x_object *
GobeeDisplay::operator+=(x_obj_attr_t *attrs)
{
    register x_obj_attr_t *attr;
    bool update = false;

    ENTER;
    for (attr = attrs->next; attr; attr = attr->next)
    {
        if (EQ("width", attr->key))
        {
            srcwidth = atoi(attr->val);
            ::printf("GobeeDisplay::assign(): width=%d\n",framewidth);
            update = true;
        }
        else if (EQ("stride", attr->key))
        {
            srcstride = atoi(attr->val);
            ::printf("GobeeDisplay::assign(): stride=%d\n",framestride);
            update = true;
        }
        else  if (EQ("height", attr->key))
        {
            srcheight = atoi(attr->val);
            ::printf("GobeeDisplay::assign(): height=%d\n",frameheight);
            update = true;
        }

        __setattr(attr->key, attr->val);
    }

    if (update)
    {
        // this->framebuf update
    }

    EXIT;
    return static_cast<gobee::__x_object *>(this);
}

int
GobeeDisplay::try_writev(const struct iovec *iov, int iovcnt,
                         x_obj_attr_t *attr)
{
    int len = 0;
    gobee_img_frame_t rgb32;
    gobee_img_frame_t yuv;
    int *stride;

    if (iovcnt>3)
    {
        stride = (int *)X_IOV_DATA(&iov[3]);
        if (*stride)
            srcstride = *stride;
    }

    rgb32[0].data = (unsigned char *)framebuf;
    rgb32[0].width = framewidth;
    rgb32[0].stride = framewidth * 3; // rgb24
    rgb32[0].height = frameheight;

    yuv[0].data = (unsigned char *)X_IOV_DATA(&iov[0]);
    yuv[0].width = framewidth;
    yuv[0].height = frameheight;
    yuv[0].stride = srcstride > srcwidth ? srcstride : srcwidth;

    yuv[1].width = yuv[0].width / 2;
    yuv[1].height = yuv[0].height / 2;
    yuv[1].stride = yuv[0].stride / 2;

    yuv[2].width = yuv[1].width;
    yuv[2].height = yuv[1].height;
    yuv[2].stride = yuv[1].stride;

    yuv[1].data = (unsigned char *)X_IOV_DATA(&iov[1]);
    yuv[2].data = (unsigned char *)X_IOV_DATA(&iov[2]);

    yuv2rgb32(rgb32,yuv);

    if(pushWidgetInvalidate(
                reinterpret_cast<gobee::__x_object *>(this->xobj.bus),
                nativedisplay) != 0)
    {
        ::printf("GobeeDisplay::try_writev() error\n");
        return -1;
    }
    else
    {
        ::printf("GobeeDisplay::try_writev() OK\n");
    }

    return len;
}




static void
___on_x_path_event(gobee::__x_object *obj)
{
    x_object_add_path(X_OBJECT(obj), X_OUT_DEVDIR "/" DISPLAY_VENDOR_ID, NULL);
}

static gobee::__path_listener _bus_listener;

GobeeDisplay::GobeeDisplay()
{
    gobee::__x_class_register_xx(this, sizeof(GobeeDisplay),
                                 "gobee:media", "video_player");

    _bus_listener.lstnr.on_x_path_event = (void(*)(void*)) &___on_x_path_event;
    x_path_listener_add("/", (x_path_listener_t *) (void *) &_bus_listener);

    return;
}

static GobeeDisplay oproto;

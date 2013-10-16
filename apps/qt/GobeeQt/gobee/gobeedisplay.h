#ifndef GOBEEDISPLAY_H
#define GOBEEDISPLAY_H

#include <QWidget>

#include <xwlib++/x_objxx.h>

class GobeeDisplay : public gobee::__x_object
{
    volatile void *framebuf;
    int framewidth;
    int frameheight;
    int framestride;

    int srcwidth;
    int srcheight;
    int srcstride;

    void *nativedisplay;
public:
    GobeeDisplay();
    virtual int on_append(gobee::__x_object *parent);
    virtual int try_write(void *buf, u_int32_t len, x_obj_attr_t *attr);
    virtual __x_object *operator+=(x_obj_attr_t *); // on_assign
    virtual int try_writev (const struct iovec *iov, int iovcnt, x_obj_attr_t *attr);
};

#endif // GOBEEDISPLAY_H

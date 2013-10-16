#ifndef FTRACK_H
#define FTRACK_H

#include <iostream>
#include <new>

#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include <xwlib/x_utils.h>
#include <xwlib++/x_objxx.h>

class GbDataComparator : public gobee::__x_object
{
    circular_buffer_t fifo;

    struct Data
    {
        float weigth;
        char *buf;
        int bufsiz;
    };

    class Data *tData;

public:
    GbDataComparator();
    virtual int on_append(gobee::__x_object *parent);
    virtual int try_write(void *buf, u_int32_t len, x_obj_attr_t *attr);
    virtual __x_object *operator+=(x_obj_attr_t *); // on_assign
    virtual int try_writev (const struct iovec *iov, int iovcnt, x_obj_attr_t *attr);
    virtual void on_create();
};

#endif // FTRACK_H

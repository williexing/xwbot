/**
 * Calculates convolution
 * comparator.cpp
 *
 */

#undef DEBUG_PRFX

#include <iostream>
#include <new>

#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <opencv2/video/tracking.hpp>

#undef DEBUG_PRFX
#include <x_config.h>

#define DEBUG_PRFX "[cv-ftrack] "

#include "comparator.h"

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib++/x_objxx.h>

#include <ev.h>


int
GbDataComparator::on_append(gobee::__x_object *parent)
{
}

int
GbDataComparator::try_write(void *buf, u_int32_t len, x_obj_attr_t *attr)
{
}

gobee::__x_object *
GbDataComparator::operator+=(x_obj_attr_t *)
{
}

void
GbDataComparator::on_create()
{
    this->tData = new Data();
    TRACE("\n");
}

int
GbDataComparator::try_writev (const struct iovec *iov, int iovcnt, x_obj_attr_t *attr)
{
    circbuf_write(&this->fifo,(uint8_t *)iov[0].iov_base,(int)iov[0].iov_len);
}

static void
___on_x_path_event(gobee::__x_object *pathobj)
{
}

static gobee::__path_listener _bus_listener;

GbDataComparator::GbDataComparator()
{
    gobee::__x_class_register_xx(this, sizeof(GbDataComparator),
                                 "gobee:cv","comparator");

//    _bus_listener.lstnr.on_x_path_event = (void(*)(void*)) &___on_x_path_event;
//    x_path_listener_add("camera", (x_path_listener_t *) (void *) &_bus_listener);

    TRACE("Registered!\n");
    return;
}

static GbDataComparator oproto;

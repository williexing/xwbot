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

#include "filter.h"

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib++/x_objxx.h>

void
GbPointTrack::filter(cv:: Mat &gray,cv:: Mat &out)
{

}

int
GbPointTrack::on_append(gobee::__x_object *parent)
{
    circbuf_init(&this->fifo,800*600*4);
}

int
GbPointTrack::try_write(void *buf, u_int32_t len, x_obj_attr_t *attr)
{

}

gobee::__x_object *
GbPointTrack::operator+=(x_obj_attr_t *)
{

}

void
GbPointTrack::on_create()
{
    this->tData = new TrackData();
    tData->max_count = 1000;
    tData->qlevel = 0.01;
    tData->minDist = 10.;
    tData->points[0].clear();
    tData->points[1].clear();
    tData->initial.clear();
    tData->features.clear();
    tData->status.clear();
    tData->err.clear();

    TRACE("\n");
}

int
GbPointTrack::try_writev (const struct iovec *iov, int iovcnt, x_obj_attr_t *attr)
{
#if 0
    float *points1 = X_IOV_DATA(iov);
    int siz1 = X_IOV_LEN(iov)/(sizeof(float) * 2);

    iov++;
    float *points2 = X_IOV_DATA(iov);
    int siz2 = X_IOV_LEN(iov)/(sizeof(float) * 2);

    // calc inertia
    cv::Matx66f matrx;
#endif
}

static void
___on_x_path_event(gobee::__x_object *pathobj)
{
    gobee::__x_object *xobj = new("gobee:cv","pointrack")gobee::__x_object();
    pathobj->get_parent()->add_child(xobj);
    x_object_add_follower(X_OBJECT(pathobj),X_OBJECT(xobj),0);
}

static gobee::__path_listener _bus_listener;

GbPointTrack::GbPointTrack()
{
    gobee::__x_class_register_xx(this, sizeof(GbPointTrack),
                                 "gobee:cv","pointrack");

    _bus_listener.lstnr.on_x_path_event = (void(*)(void*)) &___on_x_path_event;
    x_path_listener_add("ftrack", (x_path_listener_t *) (void *) &_bus_listener);

    TRACE("Registered!\n");
    return;
}

static GbPointTrack oproto;

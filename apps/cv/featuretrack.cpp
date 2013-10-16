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

#include "featuretrack.h"

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib++/x_objxx.h>

#include <ev.h>


void
GbFeatureTrack::filter(cv:: Mat &gray,cv:: Mat &out)
{
    int k=0;

    if(tData->points[0].size() <= 10)
    {
        cv::goodFeaturesToTrack(gray,tData->features,
                                tData->max_count, tData->qlevel, tData->minDist);

        tData->points[0].insert(tData->points[0].end(),
                tData->features.begin(),tData->features.end());
        tData->initial.insert(tData->initial.end(),
                       tData->features.begin(),tData->features.end());
//        std::cout << tData->points[0];
//        TRACE("goodFeaturesToTrack passed %d features\n", tData->features.size());

    }

    if(!tData->features.size())
    {
        goto _finout_;
    }

    if(tData->gray_prev.empty())
    {
        gray.copyTo(tData->gray_prev);
        cv::imshow("previous",tData->gray_prev);
        cv::waitKey(20);
    }

    cv::calcOpticalFlowPyrLK(tData->gray_prev, gray,
                             tData->points[0],tData->points[1],
            tData->status,tData->err);

    for( int i= 0; i < tData->points[1].size(); i++ )
    {

        if (
                tData->status[i]
                &&
                (abs(tData->points[0][i].x-tData->points[1][i].x)+
                 (abs(tData->points[0][i].y-tData->points[1][i].y))>0)
                )
        {
            tData->initial[k]= tData->initial[i];
            tData->points[1][k++] = tData->points[1][i];
        }
    }

    tData->points[1].resize(k);
    tData->initial.resize(k);

_finout_:
    gray.copyTo(out);
    for(int i= 0; i < tData->points[1].size(); i++ )
    {
        cv::line(out,tData->initial[i],tData->points[1][i],cv::Scalar(255,255,255));
        cv::circle(out, tData->points[1][i], 3,cv::Scalar(255,255,255),-1);
    }

    // save
    std::swap(tData->points[1], tData->points[0]);
    cv::swap(tData->gray_prev, gray);
}

int
GbFeatureTrack::on_append(gobee::__x_object *parent)
{
    cv::Mat mymat(240,320,CV_32S);
    circbuf_init(&this->fifo,800*600*4);

    circbuf_write(&this->fifo,mymat.data,320*240*3);
    cv::namedWindow("camview");
}

int
GbFeatureTrack::try_write(void *buf, u_int32_t len, x_obj_attr_t *attr)
{

}

gobee::__x_object *
GbFeatureTrack::operator+=(x_obj_attr_t *)
{

}

void
GbFeatureTrack::on_create()
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
GbFeatureTrack::try_writev (const struct iovec *iov, int iovcnt, x_obj_attr_t *attr)
{
//    TRACE("NEW FRAME RECEIVED\n");
    circbuf_write(&this->fifo,(uint8_t *)iov[0].iov_base,(int)iov[0].iov_len);

    cv::Mat mymat(240,320,CV_8UC1);
    circbuf_read(&this->fifo,mymat.data,320*240);

    cv::Mat outmat;
    cv::Mat m_integral;

    filter(mymat,outmat);

    cv::integral(mymat,m_integral);

    cv::imshow("camview",outmat);
    cv::imshow("integrated image",m_integral);
    cv::waitKey(20);
}

static void
___on_x_path_event(gobee::__x_object *pathobj)
{
    // set data handler (subscribe) to camera data
    gobee::__x_object *xobj = new("gobee:cv","ftrack")gobee::__x_object();
    pathobj->get_parent()->add_child(xobj);
//    x_object_set_write_handler(X_OBJECT(pathobj),X_OBJECT(xobj));

    x_object_add_follower(X_OBJECT(pathobj),X_OBJECT(xobj),0);

}

static gobee::__path_listener _bus_listener;

GbFeatureTrack::GbFeatureTrack()
{
    gobee::__x_class_register_xx(this, sizeof(GbFeatureTrack),
                                 "gobee:cv","ftrack");

    _bus_listener.lstnr.on_x_path_event = (void(*)(void*)) &___on_x_path_event;
    x_path_listener_add("camera", (x_path_listener_t *) (void *) &_bus_listener);

    TRACE("Registered!\n");
    return;
}

static GbFeatureTrack oproto;

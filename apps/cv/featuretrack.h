#ifndef FTRACK_H
#define FTRACK_H

#include <iostream>
#include <new>

#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include <xwlib/x_utils.h>
#include <xwlib++/x_objxx.h>

class GbFeatureTrack : public gobee::__x_object
{
    circular_buffer_t fifo;

    class TrackData
    {
    public:
        cv::Mat gray_prev;
        std::vector<cv::Point2f> points[2];
        std::vector<cv::Point2f> initial;
        std::vector<cv::Point2f> features;
        std::vector<uchar> status;
        std::vector<float> err;
        int max_count; // maximum number of features to detect
        double qlevel;
        double minDist;
    };

    class TrackData *tData;
    void filter(cv:: Mat &gray, cv::Mat &out);

public:
    GbFeatureTrack();
    virtual int on_append(gobee::__x_object *parent);
    virtual int try_write(void *buf, u_int32_t len, x_obj_attr_t *attr);
    virtual __x_object *operator+=(x_obj_attr_t *); // on_assign
    virtual int try_writev (const struct iovec *iov, int iovcnt, x_obj_attr_t *attr);
    virtual void on_create();
};

#endif // FTRACK_H

#undef DEBUG_PRFX

#include <iostream>
#include <new>

#include <stdio.h>
#include <iostream>
#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/nonfree/features2d.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <opencv2/video/tracking.hpp>

#undef DEBUG_PRFX
#include <x_config.h>

#define DEBUG_PRFX "[cv-ftrack] "

#include "eyes.h"

#include <xwlib/x_types.h>
#include <xwlib/x_utils.h>
#include <xwlib++/x_objxx.h>

#include <ev.h>


#include <iostream>
#include <math.h>
#include <string.h>

using namespace cv;
using namespace std;

int thresh = 50, N = 11;
const char* wndname = "Square Detection Demo";

// helper function:
// finds a cosine of angle between vectors
// from pt0->pt1 and from pt0->pt2
double angle( Point pt1, Point pt2, Point pt0 )
{
    double dx1 = pt1.x - pt0.x;
    double dy1 = pt1.y - pt0.y;
    double dx2 = pt2.x - pt0.x;
    double dy2 = pt2.y - pt0.y;
    return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
}

// returns sequence of squares detected on the image.
// the sequence is stored in the specified memory storage
void findSquares( const Mat& image, vector<vector<Point> >& squares )
{
    squares.clear();

    Mat pyr, timg, gray0(image.size(), CV_8U), gray;

    // down-scale and upscale the image to filter out the noise
    pyrDown(image, pyr, Size(image.cols/2, image.rows/2));
    pyrUp(pyr, timg, image.size());
    vector<vector<Point> > contours;

    // find squares in every color plane of the image
    for( int c = 0; c < 3; c++ )
    {
        int ch[] = {c, 0};
//        mixChannels(&timg, 1, &gray0, 1, ch, 1);

        // try several threshold levels
        for( int l = 0; l < N; l++ )
        {
            // hack: use Canny instead of zero threshold level.
            // Canny helps to catch squares with gradient shading
            if( l == 0 )
            {
                // apply Canny. Take the upper threshold from slider
                // and set the lower to 0 (which forces edges merging)
                Canny(gray0, gray, 0, thresh, 5);
                // dilate canny output to remove potential
                // holes between edge segments
                dilate(gray, gray, Mat(), Point(-1,-1));
            }
            else
            {
                // apply threshold if l!=0:
                //     tgray(x,y) = gray(x,y) < (l+1)*255/N ? 255 : 0
                gray = gray0 >= (l+1)*255/N;
            }

            // find contours and store them all as a list
            findContours(gray, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);

            vector<Point> approx;

            // test each contour
            for( size_t i = 0; i < contours.size(); i++ )
            {
                // approximate contour with accuracy proportional
                // to the contour perimeter
                approxPolyDP(Mat(contours[i]), approx, arcLength(Mat(contours[i]), true)*0.02, true);

                // square contours should have 4 vertices after approximation
                // relatively large area (to filter out noisy contours)
                // and be convex.
                // Note: absolute value of an area is used because
                // area may be positive or negative - in accordance with the
                // contour orientation
                if( approx.size() == 4 &&
                    fabs(contourArea(Mat(approx))) > 1000 &&
                    isContourConvex(Mat(approx)) )
                {
                    double maxCosine = 0;

                    for( int j = 2; j < 5; j++ )
                    {
                        // find the maximum cosine of the angle between joint edges
                        double cosine = fabs(angle(approx[j%4], approx[j-2], approx[j-1]));
                        maxCosine = MAX(maxCosine, cosine);
                    }

                    // if cosines of all angles are small
                    // (all angles are ~90 degree) then write quandrange
                    // vertices to resultant sequence
                    if( maxCosine < 0.3 )
                        squares.push_back(approx);
                }
            }
        }
    }
}


// the function draws all the squares in the image
void drawSquares( Mat& image, const vector<vector<Point> >& squares )
{
    for( size_t i = 0; i < squares.size(); i++ )
    {
        const Point* p = &squares[i][0];
        int n = (int)squares[i].size();
        polylines(image, &p, &n, 1, true, Scalar(0,255,0), 3, CV_AA);
    }

    imshow(wndname, image);
}

void
GbEyesTrack::filter(cv:: Mat &gray,cv:: Mat &out)
{
    vector<vector<Point> > squares;
    findSquares(gray, squares);
    drawSquares(gray, squares);
}

int
GbEyesTrack::on_append(gobee::__x_object *parent)
{
    cv::Mat mymat(240,320,CV_32S);
    circbuf_init(&this->fifo,800*600*4);

    circbuf_write(&this->fifo, mymat.data, 320*240*3);
    cv::namedWindow("camview");

    int minHessian = 400;

    cv::SurfFeatureDetector detector( minHessian );
}

int
GbEyesTrack::try_write(void *buf, u_int32_t len, x_obj_attr_t *attr)
{

}

gobee::__x_object *
GbEyesTrack::operator+=(x_obj_attr_t *)
{

}

void
GbEyesTrack::on_create()
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
GbEyesTrack::try_writev (const struct iovec *iov, int iovcnt, x_obj_attr_t *hints)
{
//    TRACE("NEW FRAME RECEIVED\n");
    circbuf_write(&this->fifo,(uint8_t *)iov[0].iov_base,(int)iov[0].iov_len);

    cv::Mat mymat(240,320,CV_8UC1);
    circbuf_read(&this->fifo,mymat.data,320*240);

    cv::Mat outmat;
    filter(mymat,outmat);
//    cv::imshow("eyeview",outmat);
    cv::waitKey(20);
}

static void
___on_x_path_event(gobee::__x_object *pathobj)
{
    gobee::__x_object *xobj = new("gobee:cv","eyetrack")gobee::__x_object();
    pathobj->get_parent()->add_child(xobj);
    x_object_add_follower(X_OBJECT(pathobj),X_OBJECT(xobj),0);

}

static gobee::__path_listener _bus_listener;

GbEyesTrack::GbEyesTrack()
{
    gobee::__x_class_register_xx(this, sizeof(GbEyesTrack),
                                 "gobee:cv","eyetrack");

    _bus_listener.lstnr.on_x_path_event = (void(*)(void*)) &___on_x_path_event;
    x_path_listener_add("camera", (x_path_listener_t *) (void *) &_bus_listener);

    TRACE("Registered!\n");
    return;
}

static GbEyesTrack oproto;

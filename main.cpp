#include "CMT.h"
#include "gui.h"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>

#include "libuvc/libuvc.h"

#ifdef __GNUC__
#include <getopt.h>
#else
#include "getopt/getopt.h"
#endif

using cmt::CMT;
using cv::imread;
using cv::namedWindow;
using cv::Scalar;
using cv::waitKey;

static string WIN_NAME = "CMT";

// Set resolution (gets a bit slower if you make it too high)
#define W 640
#define H 480

#define FPS 30

#define CV_UPDATE_TIME  30 // ms

void *image_data = NULL;
bool got_first_frame = false;

int display(Mat im, CMT & cmt)
{
    //Visualize the output
    //It is ok to draw on im itself, as CMT only uses the grayscale image
    for(size_t i = 0; i < cmt.points_active.size(); i++)
    {
        circle(im, cmt.points_active[i], 2, Scalar(255,0,0));
    }

    Point2f vertices[4];
    cmt.bb_rot.points(vertices);
    for (int i = 0; i < 4; i++)
    {
        line(im, vertices[i], vertices[(i+1)%4], Scalar(255,0,0));
    }

    imshow(WIN_NAME, im);

    return waitKey(CV_UPDATE_TIME);
}

void uvc_callback(uvc_frame_t *frame, void *ptr) 
{
    uvc_frame_t *bgr;
    uvc_error_t ret;

    bgr = uvc_allocate_frame(frame->width * frame->height * 3);
    if (!bgr) 
    {
        printf("unable to allocate bgr frame!");
        return;
    }

    ret = uvc_any2bgr(frame, bgr);
    if (ret) 
    {
        uvc_perror(ret, "uvc_any2bgr");
        uvc_free_frame(bgr);
        return;
    }

    memcpy(image_data, bgr->data, bgr->width * bgr->height * 3);
    uvc_free_frame(bgr);
    got_first_frame = true;
}

void uvc_error(uvc_error_t res, char *m) 
{
    if(res < 0)
    {
        uvc_perror(res, m);
        exit(res);
    }
}


int main(int argc, char **argv)
{
    CMT cmt;
    Rect rect;
    
    FILELog::ReportingLevel() = logINFO; // = logDEBUG;
    Output2FILE::Stream() = stdout; //Log to stdout

    // openCV window
    namedWindow(WIN_NAME);

    // libuvc variables
    uvc_context *ctx = NULL;
    uvc_device_t *dev = NULL;
    uvc_device_handle_t *handle = NULL;
    uvc_stream_ctrl_t ctrl;
    uvc_error_t res;

    // create a UVC context
    res = uvc_init(&ctx, NULL);
    uvc_error(res, "init");

    // search for the Intel camera specifically
    res = uvc_find_device(ctx, &dev, 0x8086, 0x0a66, NULL);
    uvc_error(res, "find_device");

    // open the camera device
    res = uvc_open(dev, &handle);
    uvc_error(res, "open2");

    //uvc_print_diag(handle, stderr);

    // configure the stream format to use
    res = uvc_get_stream_ctrl_format_size(handle, &ctrl, UVC_FRAME_FORMAT_YUYV, W, H, FPS);
    uvc_perror(res, "get_stream");

    //uvc_print_stream_ctrl(&ctrl, stderr);

    // OpenCV matrix for storing the captured image data, CV_8UC3
    // means 8-bit Unsigned Char, 3 channels per pixel.
    Mat im;
    im.create( H, W, CV_8UC3);

    image_data = malloc(W*H*3);

    // start streaming data from the camera
    printf("Start streaming...\n");
    res = uvc_start_streaming(handle, &ctrl, uvc_callback, (void*)&cmt, 0);
    uvc_error(res, "start_streaming");
    
    // wait until the first frame has arrived so it can be used for previewing
    printf("Waiting for first frame...\n");
    while(!got_first_frame)
        usleep(10000);

    // grab the data for the captured frame
    memcpy(im.data, image_data, W*H*3);

    // display it in OpenCV and select a region to track
    rect = getRect(im, WIN_NAME);
    printf("Bounding box: %d,%d+%dx%d\n", rect.x, rect.y, rect.width, rect.height);

    // convert to suitable format for CMT if needed
    Mat im0_gray;
    if (im.channels() > 1) 
        cvtColor(im, im0_gray, CV_BGR2GRAY);
    else 
        im0_gray = im;

    cmt.initialize(im0_gray, rect);
    printf("Main loop starting\n");

    int frame = 0;
    while (true)
    {
        frame++;

        // grab the data for the captured frame
        memcpy(im.data, image_data, W*H*3);

        Mat im_gray;
        if (im.channels() > 1) 
            cvtColor(im, im_gray, CV_BGR2GRAY);
        else
            im_gray = im;

        cmt.processFrame(im_gray);
       
        printf("#%d, active: %lu\n", frame, cmt.points_active.size());
        char key = display(im, cmt);
        if(key == 'q')
            break;
    }

    uvc_stop_streaming(handle);

    free(image_data);

    uvc_close(handle);
    uvc_unref_device(dev);
    uvc_exit(ctx);

    return 0;
}

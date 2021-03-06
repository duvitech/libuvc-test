#include <iostream>
#include <cstdlib>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
#include "libuvc_test.h"
#include "libuvc/libuvc.h"

/* This callback function runs once per frame. Use it to perform any
 * quick processing you need, or have it put the frame into your application's
 * input queue. If this function takes too long, you'll start losing frames. */
void cb(uvc_frame_t *frame, void *ptr) {
  uvc_frame_t *bgr;
  uvc_error_t ret;
  /* We'll convert the image from YUV/JPEG to BGR, so allocate space */
  bgr = uvc_allocate_frame(frame->width * frame->height * 3);
  if (!bgr) {
    std::cout << "unable to allocate bgr frame!" << std::endl;
    return;
  }
  /* Do the BGR conversion */
  ret = uvc_any2bgr(frame, bgr);
  if (ret) {
    uvc_perror(ret, "uvc_any2bgr");
    uvc_free_frame(bgr);
    return;
  }

  std::cout << "We Have a frame!" << std::endl;

  /* Call a user function:
   *
   * my_type *my_obj = (*my_type) ptr;
   * my_user_function(ptr, bgr);
   * my_other_function(ptr, bgr->data, bgr->width, bgr->height);
   */
  /* Call a C++ method:
   *
   * my_type *my_obj = (*my_type) ptr;
   * my_obj->my_func(bgr);
   */
  /* Use opencv.highgui to display the image:
   * 
   * cvImg = cvCreateImageHeader(
   *     cvSize(bgr->width, bgr->height),
   *     IPL_DEPTH_8U,
   *     3);
   *
   * cvSetData(cvImg, bgr->data, bgr->width * 3); 
   *
   * cvNamedWindow("Test", CV_WINDOW_AUTOSIZE);
   * cvShowImage("Test", cvImg);
   * cvWaitKey(10);
   *
   * cvReleaseImageHeader(&cvImg);
   */
  uvc_free_frame(bgr);
}


int main (int argc, char *argv[])
{

    uvc_context_t *ctx;
    uvc_device_t *dev;
    uvc_device_handle_t *devh;
    uvc_stream_ctrl_t ctrl;
    uvc_error_t res;
    char *p;
    int vid = 0;
    int pid = 0;


    if (argc < 3) {
        // report version
        std::cout << argv[0] << " Version " << libuvctest_VERSION_MAJOR << "."
                << libuvctest_VERSION_MINOR << std::endl;
        std::cout << "Usage: " << argv[0] << " vid pid" << std::endl;
        return 1;
    }

    vid = strtol(argv[1], &p, 10);
    pid = strtol(argv[2], &p, 10);
    std::cout << "VID: " << vid << " PID: " << pid << std::endl;

    /* Initialize a UVC service context. Libuvc will set up its own libusb
    * context. Replace NULL with a libusb_context pointer to run libuvc
    * from an existing libusb context. */
    res = uvc_init(&ctx, NULL);
    if (res < 0) {
        uvc_perror(res, "uvc_init");
        return res;
    }

    std::cout << "UVC initialized" << std::endl;
    
    /* Locates the first attached UVC device, stores in dev */
    res = uvc_find_device(
        ctx, &dev,
        vid, pid, NULL); /* filter devices: vendor_id, product_id, "serial_num" */

    if (res < 0) {
        uvc_perror(res, "uvc_find_device"); /* no devices found */
    } else {
        std::cout << "Device found" << std::endl;
        /* Try to open the device: requires exclusive access */
        res = uvc_open(dev, &devh);
        if (res < 0) {
        uvc_perror(res, "uvc_open"); /* unable to open device */
        } else {
            
        std::cout << "Device opened" << std::endl;
        /* Print out a message containing all the information that libuvc
        * knows about the device */
        uvc_print_diag(devh, stderr);
        /* Try to negotiate a 640x480 30 fps YUYV stream profile */
        res = uvc_get_stream_ctrl_format_size(
            devh, &ctrl, /* result stored in ctrl */
            UVC_FRAME_FORMAT_YUYV, /* YUV 422, aka YUV 4:2:2. try _COMPRESSED */
            640, 480, 30 /* width, height, fps */
        );
        /* Print out the result */
        uvc_print_stream_ctrl(&ctrl, stderr);
        if (res < 0) {
            uvc_perror(res, "get_mode"); /* device doesn't provide a matching stream */
        } else {
            /* Start the video stream. The library will call user function cb:
            *   cb(frame, (void*) 12345)
            */
            res = uvc_start_streaming(devh, &ctrl, cb, NULL, 0);
            if (res < 0) {
            uvc_perror(res, "start_streaming"); /* unable to start stream */
            } else {
            
            std::cout << "Streaming ...." << std::endl;
            uvc_set_ae_mode(devh, 1); /* e.g., turn on auto exposure */
            sleep(10); /* stream for 10 seconds */
            /* End the stream. Blocks until last callback is serviced */
            uvc_stop_streaming(devh);
            std::cout << "Done streaming." << std::endl;
            }
        }
        /* Release our handle on the device */
        uvc_close(devh);
        std::cout << "Device closed" << std::endl;
        }
        /* Release the device descriptor */
        uvc_unref_device(dev);
    }

    /* Close the UVC context. This closes and cleans up any existing device handles,
    * and it closes the libusb context if one was not provided. */
    uvc_exit(ctx);
    std::cout << "UVC exited" << std::endl;

    return 0;
}

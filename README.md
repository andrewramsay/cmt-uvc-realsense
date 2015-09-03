## CMT with Realsense/libuvc

* only tested on OSX 10.10!
* requires OpenCV (tested with 3.0.0) and libuvc (install the modified version from [here](https://github.com/mcguire-steve/libuvc))

### Instructions:
1. cd cmt-uvc-realsense
2. mkdir build
3. cd build
4. cmake ..
5. make
6. ./cmt

If it works, you should see a still image appear in an OpenCV window. Draw a rect as instruction to set the region to track, and it should then start displaying new frames in the window. Hit q to exit (if you Ctrl-C it the camera may need unplugged+replugged before it works again). 

If it doesn't seem to be streaming any data, try editing libuvc/include/libuvc/libuvc_internals.h, change LIBUVC_NUM_TRANSFER_BUFS to 1, recompile & reinstall then try again. 

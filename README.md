# kinect-time
Using an Xbox One Kinect and Windows adapter to hook it up with my computer

## kinect/
Wrapper around libfreenect2 to configure it as I need

Tasklist:
- [x] Get running with print statement output
- [ ] Save frames to disk
    - started on impl
- [ ] Create stream of frames
- [ ] Use smart pointers instead of raw pointers around libfreenect2 objects

## frame-stream/
Pulls images from kinect as bytestream or some other generic way, unsure yet

## third_party/libfreenect2
Using my fork https://github.com/mik90/libfreenect2 to grab images from the device. Allows me to build with CUDA due to some include issues.
**Note:**May require GCC 10.x. If so, I can't easily include it as a CMake dependency, or can I?
Easy way out is to just use a custom_command in a libfreenect2-config.cmake file.

## third_party/gt
TODO
Using my fork https://github.com/mik90/grt to try to get gestures from those images
- Build artifacts are in /obj
- Not added yet
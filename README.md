# kinect-time
Using an Xbox One Kinect and Windows adapter to hook it up with my computer


## frame-stream/
Pulls images from kinect as bytestream or some other generic way, unsure yet

## kinect/
Wrapper around libfreenect2 to configure it as I need

Tasklist:
- [x] Get running with print statement output
- [x] Save frames to disk
    - started on impl
    - [x] figure out format to save as??
        - Save relative to bytes per pixel
    - [ ] don't save in a blocking way
- [ ] Create stream of frames for input to inference engine
- [ ] Use smart pointers instead of raw pointers around libfreenect2 objects

## third_party/libfreenect2
Using my fork https://github.com/mik90/libfreenect2 to grab images from the device. Allows me to build with CUDA due to some include issues.

**Note:** May require GCC 10.x. So far I've been able to build with GCC 11 though.

## third_party/nvidia-ngc
Pre-trained GestureNet model from nvidia's model registry.

Technically you're supposed to use DeepStream a Jetson or a workstation GPU and not the consumer one like i have (RTX 2060 Super).
We'll see how it works

Model: https://ngc.nvidia.com/catalog/models/nvidia:tao:gesturenet
- Input is RGB Images of 160 X 160 X 3 (Width X Height X Byte depth (R, G, B))
- Limited to frontal views, bland backgrounds, and poor lighting

Guide on usage: https://developer.nvidia.com/blog/fast-tracking-hand-gesture-recognition-ai-applications-with-pretrained-models-from-ngc/

## third_party/grt
**TODO**
Using my fork https://github.com/mik90/grt to try to get gestures from those images
- Build artifacts are in /obj
- Not added yet
- I'd probably have to train my own model

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
Pre-trained GestureNet model from nvidia's model registry. NGC == Nvidia GPU Cloud.

This is the guide I'm following: https://developer.nvidia.com/blog/fast-tracking-hand-gesture-recognition-ai-applications-with-pretrained-models-from-ngc/

**Note:** you will need the nvidia-container-toolkit package in order to run a docker container that uses your nvidia gpu.

I use gentoo and made use of guru's overlay from this search: http://gpo.zugaina.org/app-emulation/nvidia-container-toolkit

GestureNet - pre-trained model: https://ngc.nvidia.com/catalog/models/nvidia:tao:gesturenet
- Input is RGB Images of 160 X 160 X 3 (Width X Height X Byte depth (R, G, B))
- Limited to frontal views, bland backgrounds, and poor lighting

Triton Inference Server - inference: https://ngc.nvidia.com/catalog/containers/nvidia:tritonserver
- nvidia's "Triton Inference Server" allows the pre-trained model to be used
- Quickstart guide on dockerized triton server: https://github.com/triton-inference-server/server/blob/r21.08/docs/quickstart.md
- So I need to convert the model.etlt file to a model.plan since you need that to run in the inference server
    - My guide is wrong since tlt-converter isn't available in the image, I need tao-converter
    - [ ] tell the nvidia folk that their doc is outdated
    - [convert-model.sh](convert-model.sh)
- [run-triton.sh](run-triton.sh)


## third_party/grt
**TODO**
Using my fork https://github.com/mik90/grt to try to get gestures from those images
- Build artifacts are in /obj
- Not added yet
- I'd probably have to train my own model

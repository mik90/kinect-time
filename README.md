# kinect-time

Gesture recognition using with an Xbox One Kinect on Linux.

## Usage

- Ensure that you have an Xbox One Kinect with the extra Windows adapter that allows you to plug it in via USB.
- Build the frame-stream-cli with:

    ```bash
    mkdir build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build . --target frame-stream-cli
    ```

- Download the pre-trained GestureNet model with [get-gesture-net.sh](get-gesture-net.sh)
- Convert model.etlt to model.plan with [convert-model.sh](convert-model.sh)
  - an .etlt file is platform agnostic while a .plan file isn't
- Start the server with [run-triton.sh](run-triton.sh)

## Tasklist

### Kinect

- [x] Get running with print statement output
- [x] Kinect: Save frames to disk
  - started on impl
  - [x] figure out format to save as??
    - Save relative to bytes per pixel
  - [ ] don't save in a blocking way
- [ ] Create stream of frames for input to inference engine
  - [ ] resize frames to resolution desired by inference engine
- [ ] Use smart pointers instead of raw pointers around libfreenect2 objects

### nvidia / GestureNet

- [x] convert GestureNet model file from .etlt to .plan
- [x] get inference server up and running
- [ ] integrate triton inference server client SDK: <https://github.com/triton-inference-server/client>
- [ ] send frames to inference server

## frame-stream/

Pulls images from kinect as bytestream or some other generic way, unsure yet

## kinect/

Wrapper around libfreenect2 to configure it as I need

## kinect-triton-client/

grPC-based client to the triton inference server. Sends over images for classification.

Pulls in triton-client as well as gRPC so it'll take a while to build.

## third_party/libfreenect2

Using my fork <https://github.com/mik90/libfreenect2> to grab images from the device. Allows me to build with CUDA due to some include issues.

This is kept as a git submodule since it is meant to be commited to an updated.

**Note:** May require GCC 10.x. So far I've been able to build with GCC 11 though.

## third_party/nvidia-ngc

Pre-trained GestureNet model from nvidia's model registry. Pulled via script.

## third_party/grt

**TODO:** May not end up using this

Using my fork <https://github.com/mik90/grt> to try to get gestures from those images

- Build artifacts are in /obj
- Not added yet
- I'd probably have to train my own model

## trtis_model_repo/

Workspace for the pre-trained gesture classification model from nvidia

**Note:** you will need the nvidia-container-toolkit package in order to run a docker container that uses your nvidia gpu.

I use gentoo and made use of guru's overlay from this search: <http://gpo.zugaina.org/app-emulation/nvidia-container-toolkit>

This is the guide I'm following: <https://developer.nvidia.com/blog/fast-tracking-hand-gesture-recognition-ai-applications-with-pretrained-models-from-ngc/>
    - The GestureNet doc has fp16 in one place and FP_32 in another for the model input/output types
    - It also writes tlt-converter and trt-converter although only tao-converter was in the image, maybe it's older
    - [ ] tell the nvidia folk that their doc is outdated, make a PR if it's in github

GestureNet - pre-trained model: <https://ngc.nvidia.com/catalog/models/nvidia:tao:gesturenet>

- Input is RGB Images of 160 X 160 X 3 (Width X Height X Byte depth (R, G, B))
- Limited to frontal views, bland backgrounds, and good lighting

Triton Inference Server - inference: <https://ngc.nvidia.com/catalog/containers/nvidia:tritonserver>

- nvidia's dockerized "Triton Inference Server" allows the pre-trained model to be used.
- Quickstart guide: <https://github.com/triton-inference-server/server/blob/r21.08/docs/quickstart.md>

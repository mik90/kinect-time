# kinect-time

Gesture recognition using with an Xbox One Kinect on Linux.

Dataflow from Kinect to image classification:

Xbox One Kinect -> third_party/libfreenect2 (C++ driver) -> kinect/ (C++ kinect wrapper and de-resolution-er) -> pybind11 -> kinect-triton-client (python) -> nvidia py sdk (image classification with GestureNet)

The Kinect exports color images as 1920x1080 where each pixel is BGRX (4 bytes, last byte is empty space)

## Gesture recognition with GestureNet

GestureNet is a gesture recognition model from nvidia.

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
- Server quickstart guide: <https://github.com/triton-inference-server/server/blob/r21.08/docs/quickstart.md>
- Client repo: <https://github.com/triton-inference-server/client/tree/r21.08>

## Usage

- Ensure that you have an Xbox One Kinect with the extra Windows adapter that allows you to plug it in via USB.
- If you build libfreenect2 with CUDA support, Clang won't work (I tried Clang 12.0.1) so just use GCC (I used GCC 11.2)

- Build kinect-test-driver with:

    ```bash
    mkdir build && cd build
    cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
    cmake --build . --target test-driver
    ```

- Download the pre-trained GestureNet model with [get-gesture-net.sh](get-gesture-net.sh)
- Convert model.etlt to model.plan with [convert-model.sh](convert-model.sh)
  - an .etlt file is platform agnostic while a .plan file isn't
- Start the server with [run-triton-server.sh](run-triton-server.sh)

## Tasklist

### Kinect

- [x] Get running with print statement output
- [x] Kinect: Save frames to disk
  - started on impl
  - [x] figure out format to save as??
    - Save relative to bytes per pixel
  - [x] non-blocking `Kinect::save_frames()`
- [x] resize frames to resolution desired by inference engine (160x160x3)
  - started on this. Merging rows and then columns
  - [x] Merge the width and just disregard the bottom half of the image
- ~~[ ] Use smart pointers instead of raw pointers around libfreenect2 objects~~
  - Nope, this isn't worth it. Just causes segfaults when the test-driver is done running
- [ ] Create Python API with pybind11 so kinect-triton-client can use it
- [ ] create frame stream
- [ ] Actually scale down resolution from 1920x1080 to 160x160 properly
- [ ] Benchmark/Optimize frame conversion
- [ ] Benchmark/Optimize frame saving

### nvidia / GestureNet

- [x] convert GestureNet model file from .etlt to .plan
- [x] get inference server up and running

### kinect-triton-client

- [ ] integrate triton inference server client SDK: <https://github.com/triton-inference-server/client>
  - tried to build it from source via CMake FetchContent but it was a pain. Switching to Python
- [ ] send frames to inference server
  - just send frames that were saved to disk (for now)
- [ ] Use pybind11 in order to start/stop the recorder
  - <https://pybind11.readthedocs.io/en/latest/basics.html>
  - CMake example: <https://github.com/pybind/cmake_example>
  - Ensure that the compiled python module and py venv versions match
- [ ] handle frame stream

## kinect/

Wrapper around libfreenect2 to configure it as I need

## third_party/libfreenect2

Using my fork <https://github.com/mik90/libfreenect2> to grab images from the device. Allows me to build with CUDA due to some include issues.

This is kept as a git submodule since it is meant to be commited to an updated.

**Note:** May require GCC 10.x. So far I've been able to build with GCC 11 though.

## kinect-triton-client/

gRPC-based python client to the triton inference server. Sends over images for classification.

Enter venv and download the packages with

```bash
cd kinect-triton-client
mkdir env
python3.10 -m venv env # This should match the version genereted by pybind11. Unsure what happens if it doesn't
source env/bin/activate
pip install -r requirements.txt
```

In the venv, the requirements.txt file is generated with `pip freeze > requirements.txt`

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

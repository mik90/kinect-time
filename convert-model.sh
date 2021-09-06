#!/bin/bash

# Run the converter utility to convert .etlt -> .plan
# The "Converting the models to TensorRT" part of this guide is wrong: https://developer.nvidia.com/blog/fast-tracking-hand-gesture-recognition-ai-applications-with-pretrained-models-from-ngc/ 
# Use tao-converter instead: https://github.com/NVIDIA-AI-IOT/tao-toolkit-triton-apps/blob/fc7e222c036354498e53a8ed11b5cf7c0a3e5239/scripts/download_and_convert.sh

docker run --gpus=1 --rm \
         -v$PWD/trtis_model_repo/hcgesture_tlt/1:/models \
         nvcr.io/nvidia/tao/tao-cv-inference-pipeline:v0.3-ga-server-utilities \
        tao-converter \
            -k nvidia_tlt  \
            -t fp16  \
            -p input_1,1x3x160x160,1x3x160x160,2x3x160x160 \
            -e /models/model.plan  \
            /models/model.etlt

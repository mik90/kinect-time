#!/bin/bash

# Runs the nvidia triton inferance server
# https://github.com/triton-inference-server/server/blob/r21.08/docs/quickstart.md

docker run --gpus=1 --rm \
         -p8000:8000 -p8001:8001 -p8002:8002 \
         -v/full/path/to/docs/examples/model_repository:/models \
         nvcr.io/nvidia/tritonserver:21.08-py3 \
         tritonserver --model-repository=/models

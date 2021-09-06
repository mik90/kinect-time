#!/bin/bash

readonly DOWNLOAD_DIR=third_party/nvidia-ngc
readonly DOWNLOAD_FILE=$DOWNLOAD_DIR/tlt_gesturenet_deployable_v1.0.zip
readonly OUT_DIR=trtis_model_repo/hcgesture_tlt/1

mkdir -p $DOWNLOAD_DIR

# Grab it off nvidia NGC
wget -v --content-disposition https://api.ngc.nvidia.com/v2/models/nvidia/tlt_gesturenet/versions/deployable_v1.0/zip \
     -O $DOWNLOAD_FILE

unzip -v $DOWNLOAD_FILE -d $OUT_DIR

# rm -v $DOWNLOAD_FILE

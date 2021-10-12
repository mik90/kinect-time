# Based off of https://github.com/triton-inference-server/client/blob/main/src/python/examples/simple_grpc_infer_client.py

from pathlib import Path
import tritonclient.grpc as grpcclient
import numpy as np
import argparse
import time
# fmt: off
import sys
kinectpy_outdir = str((Path() / ".." / "build" / "lib").resolve())
sys.path.insert(0, kinectpy_outdir)
print(f"kinectpy_outdir: {kinectpy_outdir}")
import kinectpy
# fmt: on

# output from kinectpy

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-u',
                        '--url',
                        type=str,
                        required=False,
                        default='localhost:8001',
                        help='triton-server URL. Default is localhost:8001.')
    args = parser.parse_args()
    try:
        triton_client = grpcclient.InferenceServerClient(
            url=args.url)
    except Exception as e:
        print(f"gRPC channel creation failed: {str(e)}")
        sys.exit()

    model_name = "GestureNet"
    kinect = kinectpy.Kinect()
    print("Created Kinect obj")
    print("Starting recording")
    kinect.start_recording()
    time.sleep(5)  # seconds
    kinect.stop_recording()
    print("Stopped recording")

    # Infer
    inputs = []
    outputs = []
    # GestureNet expects images as 160x160 with each pixel as RGB (3 bytes)
    # Based off of ${workspaceRepo}/trtis_model_repo/hcgesture_tlt/config.pbtxt
    inputs.append(grpcclient.InferInput('input_1', [3, 160, 160], "TYPE_FP32"))

    # TODO: Make this correct for my usage
    # Create the data for the two input tensors. Initialize the first
    # to unique integers and the second to all ones.
    input0_data = np.arange(start=0, stop=16, dtype=np.int32)
    input0_data = np.expand_dims(input0_data, axis=0)
    input1_data = np.ones(shape=(1, 16), dtype=np.int32)

    # Initialize the data
    inputs[0].set_data_from_numpy(input0_data)
    inputs[1].set_data_from_numpy(input1_data)

    outputs.append(grpcclient.InferRequestedOutput('activation_18'))

    # Test with outputs
    results = triton_client.infer(
        model_name=model_name,
        inputs=inputs,
        outputs=outputs,
        headers={'test': '1'})

    statistics = triton_client.get_inference_statistics(model_name=model_name)
    print(statistics)
    if len(statistics.model_stats) != 1:
        print("FAILED: Inference Statistics")
        sys.exit(1)

    # Get the output arrays from the results
    output0_data = results.as_numpy('activation_18')

    for i in range(16):
        print(
            str(input0_data[0][i]) + " + " + str(input1_data[0][i]) + " = " +
            str(output0_data[0][i]))
        print(
            str(input0_data[0][i]) + " - " + str(input1_data[0][i]) + " = " +
            str(output1_data[0][i]))
        if (input0_data[0][i] + input1_data[0][i]) != output0_data[0][i]:
            print("sync infer error: incorrect sum")
            sys.exit(1)
        if (input0_data[0][i] - input1_data[0][i]) != output1_data[0][i]:
            print("sync infer error: incorrect difference")
            sys.exit(1)

    # Test with no outputs
    results = triton_client.infer(
        model_name=model_name,
        inputs=inputs,
        outputs=None)

    # Get the output arrays from the results
    output0_data = results.as_numpy('activation_18')
    output1_data = results.as_numpy('OUTPUT1')

    for i in range(16):
        print(
            str(input0_data[0][i]) + " + " + str(input1_data[0][i]) + " = " +
            str(output0_data[0][i]))
        print(
            str(input0_data[0][i]) + " - " + str(input1_data[0][i]) + " = " +
            str(output1_data[0][i]))
        if (input0_data[0][i] + input1_data[0][i]) != output0_data[0][i]:
            print("sync infer error: incorrect sum")
            sys.exit(1)
        if (input0_data[0][i] - input1_data[0][i]) != output1_data[0][i]:
            print("sync infer error: incorrect difference")
            sys.exit(1)

    print('PASS: infer')

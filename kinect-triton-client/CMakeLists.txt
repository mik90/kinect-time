# Based off of https://github.com/triton-inference-server/client/blob/r21.08/src/c%2B%2B/examples/CMakeLists.txt
# Specifically, simple_grpc_infer_client

add_executable(kinect-triton-client
    main.cpp
)
target_link_libraries(kinect-triton-client PRIVATE
    grpcclient_static
)
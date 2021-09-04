#include "kinect.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iterator>

#include <iostream>
#include <string_view>

namespace mik {

// I'd use std::source_location but clang doesn't have it yet so clangd will complain
#define exit_with_error(err_msg) exit_with_error_impl(__FILE__, __LINE__, err_msg)
void exit_with_error_impl(const char* file, std::uint32_t line, std::string_view err_msg) {
    std::cerr << file << ":" << line << " - " << err_msg << "\n";
    std::cerr << "Exiting with return code 1\n";
    std::exit(1);
}

Kinect::Kinect(KinectConfig config) {
    std::cout << "Writing images to " << config.image_output_dir << "\n";

    // Log to console
    libfreenect2::setGlobalLogger(libfreenect2::createConsoleLogger(libfreenect2::Logger::Debug));

    switch (config.framework) {
    case InputPipeline::CUDA:
        pipeline_ = new libfreenect2::CudaPacketPipeline(config.gpu_device_id);
        break;
    case InputPipeline::CPU:
        pipeline_ = new libfreenect2::CpuPacketPipeline();
        break;
    case InputPipeline::OPENGL:
        pipeline_ = new libfreenect2::OpenGLPacketPipeline();
        break;
    case InputPipeline::OTHER:
        exit_with_error("InputFramework is not supported!");
        std::exit(1);
        break;
    }

    if (freenect2_.enumerateDevices() == 0) {
        exit_with_error("no device connected!");
    }
    const std::string serial_device_desc = freenect2_.getDefaultDeviceSerialNumber();

    if (!pipeline_) {
        exit_with_error("Could not open input pipeline");
    }
    std::cout << "Serial device: " << serial_device_desc << "\n";

    device_ptr_ = freenect2_.openDevice(serial_device_desc, pipeline_);

    if (!device_ptr_->start()) {
        exit_with_error("Could not start Freenect2Device!");
    }

    std::cout << "Device started\n";
    std::cout << "  Serial number: " << device_ptr_->getSerialNumber() << "\n";
    std::cout << "  Firmware version: " << device_ptr_->getFirmwareVersion() << "\n";

    registration_ = new libfreenect2::Registration(device_ptr_->getIrCameraParams(),
                                                   device_ptr_->getColorCameraParams());
    undistorted_ptr_ = new libfreenect2::Frame(512, 424, 4);
    registered_ptr_ = new libfreenect2::Frame(512, 424, 4);
}

void Kinect::save_frames(std::uint32_t n_frames_to_save) {

    constexpr int frame_timeout_ms = 10 * 1'000;
    for (std::size_t frames_pulled = 0; frames_pulled < n_frames_to_save; ++frames_pulled) {

        if (!listener_ptr_->waitForNewFrame(frame_map_, frame_timeout_ms)) {
            exit_with_error("Waited too long to get a frame!");
        }

        libfreenect2::Frame* rgb_frame = frame_map_[libfreenect2::Frame::Color];
        [[maybe_unused]] libfreenect2::Frame* ir_frame = frame_map_[libfreenect2::Frame::Ir];
        libfreenect2::Frame* depth_frame = frame_map_[libfreenect2::Frame::Depth];

        registration_->apply(rgb_frame, depth_frame, undistorted_ptr_, registered_ptr_);
    }
}

Kinect::~Kinect() {
    listener_ptr_->release(frame_map_);
    device_ptr_->stop();
    device_ptr_->close();
    delete registration_;
}
} // namespace mik
#include "kinect.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <iterator>

#include <fstream>
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

Kinect::Kinect(KinectConfig config) : config_(config) {

    if (!std::filesystem::exists(config_.image_output_dir)) {
        if (!std::filesystem::create_directory(config_.image_output_dir)) {
            exit_with_error("Could not create directory: " + std::string(config_.image_output_dir));
        }
    }

    std::cout << "Writing images to " << config_.image_output_dir << "\n";

    // Log to console
    libfreenect2::setGlobalLogger(libfreenect2::createConsoleLogger(libfreenect2::Logger::Debug));

    switch (config.framework) {
    case InputPipeline::CUDA:
        pipeline_ = new libfreenect2::CudaPacketPipeline(config_.gpu_device_id);
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

    // Set up listener
    listener_ptr_ = new libfreenect2::SyncMultiFrameListener(
        libfreenect2::Frame::Color | libfreenect2::Frame::Ir | libfreenect2::Frame::Depth);
    device_ptr_->setColorFrameListener(listener_ptr_);
    device_ptr_->setIrAndDepthFrameListener(listener_ptr_);

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
    std::uint32_t n_frames_saved = 0;
    while (n_frames_saved < n_frames_to_save) {

        if (!listener_ptr_->waitForNewFrame(frame_map_, frame_timeout_ms)) {
            exit_with_error("Waited too long to get a frame!");
        }

        libfreenect2::Frame* rgb_frame = frame_map_[libfreenect2::Frame::Color];
        [[maybe_unused]] libfreenect2::Frame* ir_frame = frame_map_[libfreenect2::Frame::Ir];
        libfreenect2::Frame* depth_frame = frame_map_[libfreenect2::Frame::Depth];

        registration_->apply(rgb_frame, depth_frame, undistorted_ptr_, registered_ptr_);

        ++n_frames_saved;
        std::cout << "Saving frame " << n_frames_saved << "\n";
        // TODO do this non-blocking
        save_frame(libfreenect2::Frame::Ir, rgb_frame);
        save_frame(libfreenect2::Frame::Ir, ir_frame);
        save_frame(libfreenect2::Frame::Depth, depth_frame);
    }
}
std::string frame_type_to_string(libfreenect2::Frame::Type type) {
    switch (type) {
    case libfreenect2::Frame::Color:
        return "Color";
    case libfreenect2::Frame::Ir:
        return "Ir";
    case libfreenect2::Frame::Depth:
        return "Infrared";
    default:
        return "Unknown";
    }
}

void Kinect::save_frame(libfreenect2::Frame::Type frame_type, libfreenect2::Frame* frame) const {
    const auto file_name =
        frame_type_to_string(frame_type) + "-" + std::to_string(frame->timestamp) + ".dat";
    std::fstream output_stream(config_.image_output_dir / file_name, output_stream.out);
    if (!output_stream.is_open()) {
        exit_with_error("Could not open output stream: " +
                        std::string(config_.image_output_dir / file_name));
    } else {
        output_stream << frame->data;
    }
}

Kinect::~Kinect() {
    listener_ptr_->release(frame_map_);
    device_ptr_->stop();
    device_ptr_->close();
    delete registration_;
}
} // namespace mik
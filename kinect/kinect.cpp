#include "kinect.hpp"
#include "conversion.hpp"
#include "util.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <system_error>

namespace mik {

using std::size_t;
using std::uint32_t;
using std::uint8_t;

Kinect::Kinect(KinectConfig config) : config_(config) {

    if (!std::filesystem::exists(config_.image_output_dir)) {
        std::cout << config_.image_output_dir.c_str() << " does not exist, creating it...\n";
        std::error_code error;
        if (!std::filesystem::create_directory(config_.image_output_dir, error)) {
            exit_with_error(HERE, "Could not create directory \'" +
                                      std::string(config_.image_output_dir) +
                                      "\': " + error.message());
        }
        std::cout << "Created " << config_.image_output_dir.c_str() << "\n";
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
        exit_with_error(HERE, "InputFramework is not supported!");
        std::exit(1);
        break;
    }

    if (freenect2_.enumerateDevices() == 0) {
        exit_with_error(HERE, "no device connected!");
    }
    const std::string serial_device_desc = freenect2_.getDefaultDeviceSerialNumber();

    if (!pipeline_) {
        exit_with_error(HERE, "Could not open input pipeline");
    }
    std::cout << "Serial device: " << serial_device_desc << "\n";

    device_ptr_ = freenect2_.openDevice(serial_device_desc, pipeline_);

    if (!device_ptr_->start()) {
        exit_with_error(HERE, "Could not start Freenect2Device!");
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

void Kinect::start_recording() {
    std::cout << "starting to record\n";
    should_record_ = true;
    recorder_ = std::thread([this]() {
        constexpr int frame_timeout_ms = 10 * 1'000;
        while (should_record_) {
            if (!listener_ptr_->waitForNewFrame(frame_map_, frame_timeout_ms)) {
                exit_with_error(HERE, "Waited too long to get a frame!");
            }

            libfreenect2::Frame* rgb_frame = frame_map_[libfreenect2::Frame::Color];
            [[maybe_unused]] libfreenect2::Frame* ir_frame = frame_map_[libfreenect2::Frame::Ir];
            libfreenect2::Frame* depth_frame = frame_map_[libfreenect2::Frame::Depth];

            registration_->apply(rgb_frame, depth_frame, undistorted_ptr_, registered_ptr_);

            save_frame_async(libfreenect2::Frame::Color, rgb_frame);
            save_gnet_frame_async(libfreenect2::Frame::Color, rgb_frame);
            save_frame_async(libfreenect2::Frame::Ir, ir_frame);
            save_frame_async(libfreenect2::Frame::Depth, depth_frame);
        }
    });
}
void Kinect::stop_recording() {
    should_record_ = false;
    std::cout << "Told recorder to stop\n";
}

void Kinect::save_frames(std::uint32_t n_frames_to_save) {
    constexpr int frame_timeout_ms = 10 * 1'000;
    std::uint32_t n_frames_saved = 0;
    std::cout << "Writing frames to " << std::filesystem::absolute(config_.image_output_dir)
              << "\n";
    while (n_frames_saved < n_frames_to_save) {

        if (!listener_ptr_->waitForNewFrame(frame_map_, frame_timeout_ms)) {
            exit_with_error(HERE, "Waited too long to get a frame!");
        }

        libfreenect2::Frame* rgb_frame = frame_map_[libfreenect2::Frame::Color];
        libfreenect2::Frame* ir_frame = frame_map_[libfreenect2::Frame::Ir];
        libfreenect2::Frame* depth_frame = frame_map_[libfreenect2::Frame::Depth];

        registration_->apply(rgb_frame, depth_frame, undistorted_ptr_, registered_ptr_);

        save_frame_async(libfreenect2::Frame::Color, rgb_frame);
        save_gnet_frame_async(libfreenect2::Frame::Color, rgb_frame);
        save_frame_async(libfreenect2::Frame::Ir, ir_frame);
        save_frame_async(libfreenect2::Frame::Depth, depth_frame);
    }
}

std::string Kinect::frame_type_to_string(libfreenect2::Frame::Type type) {
    switch (type) {
    case libfreenect2::Frame::Color:
        return "Color";
    case libfreenect2::Frame::Ir:
        return "Infrared";
    case libfreenect2::Frame::Depth:
        return "Depth";
    default:
        return "Unknown";
    }
}

std::string Kinect::frame_format_to_string(libfreenect2::Frame::Format format) {
    switch (format) {
    case libfreenect2::Frame::Format::Invalid:
        return "Invalid";
    case libfreenect2::Frame::Format::Raw:
        return "Raw";
    case libfreenect2::Frame::Format::Float:
        return "Float";
    case libfreenect2::Frame::Format::BGRX:
        return "BGRX";
    case libfreenect2::Frame::Format::RGBX:
        return "RGBX";
    case libfreenect2::Frame::Format::Gray:
        return "Gray";
    default:
        return "Unknown";
    }
}

void Kinect::save_frame_impl(std::filesystem::path image_output_dir,
                             libfreenect2::Frame::Type frame_type,
                             const libfreenect2::Frame* frame) {
    const auto frame_dimensions =
        std::to_string(frame->width) + "x" + std::to_string(frame->height);
    const auto file_name = frame_type_to_string(frame_type) + "-" + frame_dimensions + "-" +
                           frame_format_to_string(frame->format) + "-seq" +
                           std::to_string(frame->sequence) + ".bin";

    const auto file_path = image_output_dir / file_name;

    // We're writing out unsigned char instead of sgned char
    std::basic_ofstream<unsigned char, std::char_traits<unsigned char>> output_stream(
        file_path, output_stream.out | output_stream.binary);

    if (!output_stream.is_open()) {
        exit_with_error(HERE, "Could not open output stream: " + file_path.string());
    } else {
        std::cout << "Saving frame to " << file_path << "\n";

        const std::size_t n_pixels = frame->width * frame->height;
        output_stream.write(frame->data,
                            static_cast<std::streamsize>(n_pixels * frame->bytes_per_pixel));
        std::cout << "Frame saved.\n ";
    }
}

// Synchronous
void Kinect::save_frame(libfreenect2::Frame::Type frame_type,
                        const libfreenect2::Frame* frame) const {
    save_frame_impl(config_.image_output_dir, frame_type, frame);
}

// Maybe this should just be running on a single separate thread
void Kinect::save_frame_async(libfreenect2::Frame::Type frame_type,
                              const libfreenect2::Frame* frame) {
    auto task = std::async([image_output_dir = config_.image_output_dir, frame_type, frame]() {
        save_frame_impl(image_output_dir / "kinect", frame_type, frame);
    });
    saveTasks_.push(std::move(task));
}
void Kinect::save_gnet_frame_async(libfreenect2::Frame::Type frame_type,
                                   const libfreenect2::Frame* frame) {
    if (frame_type == libfreenect2::Frame::Type::Color) {
        const auto gn_frame = Conversion::from_kinect_frame(libfreenect2::Frame::Color, frame);
        gn_frame->save_frame(config_.image_output_dir / "gesturenet", frame->sequence);
    } else {
        std::cerr << "Not saving frame " << std::to_string(frame->sequence)
                  << " since it's not RGB\n";
    }
}

Kinect::~Kinect() {
    while (!saveTasks_.empty()) {
        std::cout << "Waiting for saveTasks_. " << std::to_string(saveTasks_.size())
                  << " task(s) are left.\n";
        if (saveTasks_.front().valid()) {
            try {
                saveTasks_.front().get();
            } catch (const std::exception& e) {
                std::cerr << "Could not finish save task: " << e.what() << '\n';
            }
        };
        saveTasks_.pop();
    }

    listener_ptr_->release(frame_map_);
    device_ptr_->stop();
    device_ptr_->close();
    delete registration_;

    if (recorder_.joinable()) {
        recorder_.join();
    }
}

} // namespace mik

#include "kinect.hpp"

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

namespace mik {

using std::size_t;
using std::uint32_t;
using std::uint8_t;

// I'd use std::source_location but clang doesn't have it yet so clangd will complain
#define HERE __FILE__, __LINE__
void exit_with_error(const char* file, std::uint32_t line, std::string_view err_msg) {
    std::cerr << file << ":" << line << " - " << err_msg << "\n";
    std::cerr << "Exiting with return code 1\n";
    std::exit(1);
}

Kinect::Kinect(KinectConfig config) : config_(config) {

    if (!std::filesystem::exists(config_.image_output_dir)) {
        if (!std::filesystem::create_directory(config_.image_output_dir)) {
            exit_with_error(HERE,
                            "Could not create directory: " + std::string(config_.image_output_dir));
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

        ++n_frames_saved;
        std::cout << "Saving frame " << n_frames_saved << "\n";
        save_frame_async(libfreenect2::Frame::Color, rgb_frame);
        save_frame_async(libfreenect2::Frame::Ir, ir_frame);
        save_frame_async(libfreenect2::Frame::Depth, depth_frame);
    }
}

std::string frame_type_to_string(libfreenect2::Frame::Type type) {
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

std::string frame_format_to_string(libfreenect2::Frame::Format format) {
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

void save_frame_impl(std::filesystem::path image_output_dir, libfreenect2::Frame::Type frame_type,
                     libfreenect2::Frame* frame) {
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
void Kinect::save_frame(libfreenect2::Frame::Type frame_type, libfreenect2::Frame* frame) const {
    save_frame_impl(config_.image_output_dir, frame_type, frame);
}

// Maybe this should just be running on a single separate thread
void Kinect::save_frame_async(libfreenect2::Frame::Type frame_type, libfreenect2::Frame* frame) {
    auto task = std::async([image_output_dir = config_.image_output_dir, frame_type, frame]() {
        save_frame_impl(image_output_dir / "kinect", frame_type, frame);
        const auto gn_frame = GestureNetFrame::from_kinect_frame(frame_type, frame);
        gn_frame->save_frame(image_output_dir / "gesturenet", frame->sequence);
    });
    saveTasks_.push(std::move(task));
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
}

/// @brief Pixel to pixel conversion
std::optional<GestureNetPixel> GestureNetPixel::from_kinect_bgrx_pixel(const unsigned char* data,
                                                                       std::size_t len) {
    if (len != 4) {
        std::cerr << "Cannot convert chunk of length " << std::to_string(len)
                  << " to a GestureNetPixel\n";
        return {};
    }
    // BGRX -> RGB
    GestureNetPixel g_net_pixel;
    g_net_pixel.blue = data[0];
    g_net_pixel.green = data[1];
    g_net_pixel.red = data[2];
    // Disregard the last byte, but still require a length of 4 since a kinect pixel is technically
    // 4 bytes
    return g_net_pixel;
}

/**
 * @brief For width:  1920 / 160 = 12   so the color values across 12 pixels should be averaged
 */
GestureNetFrame::PixelRow GestureNetFrame::from_kinect_row(const unsigned char* data) {
    // Convert to GestureNet PixelRow format
    GestureNetFrame::PixelRow g_n_row;
    g_n_row.reserve(bytes_per_pixel() * width_in_pixels());
    constexpr size_t kinect_row_pixel_count = 1920;
    const auto pixels_to_merge = kinect_row_pixel_count / width_in_pixels(); // 12

    PixelRow gesturenet_row;

    // Keep merging 12 red, green, and blue pixels with each other until the end of the row is hit
    size_t pixels_converted = 0;
    while (pixels_converted < kinect_row_pixel_count) {
        // running averages of colors
        uint32_t blue_sum = 0;
        uint32_t green_sum = 0;
        uint32_t red_sum = 0;
        for (size_t i = 0; i < pixels_to_merge; ++i) {
            blue_sum += static_cast<uint32_t>(data[pixels_converted++]);
            green_sum += static_cast<uint32_t>(data[pixels_converted++]);
            red_sum += static_cast<uint32_t>(data[pixels_converted++]);
            pixels_converted++; // This is the X of BGRX that can be ignored
        }
        GestureNetPixel g_n_pixel;
        g_n_pixel.blue = static_cast<uint8_t>(blue_sum / pixels_to_merge);
        g_n_pixel.green = static_cast<uint8_t>(green_sum / pixels_to_merge);
        g_n_pixel.red = static_cast<uint8_t>(red_sum / pixels_to_merge);
        gesturenet_row.emplace_back(std::move(g_n_pixel));
    }

    return gesturenet_row;
}

/**
 * @brief Convert from a Kinect color frame to a GestureNet input frame
 *
 * @details Convert from 1920x1080xBGRX to 160x160xRGB
 * Average out the values of a given color across multiple pixels in order to downscale.
 * Do this for R, G, and B.
 * For width:  1920 / 160 = 12   so the color values across 12 pixels should be averaged
 * For height: 1080 / 160 = 6.75 so this won't be as simple. How to figure this out?
 */
std::optional<GestureNetFrame>
GestureNetFrame::from_kinect_frame(libfreenect2::Frame::Type frame_type,
                                   const libfreenect2::Frame* frame) {
    if (frame_type != libfreenect2::Frame::Type::Color) {
        std::cerr << "Cannot convert frame from " << frame_type_to_string(frame_type)
                  << " to GestureNestFrame\n";
        return {};
    }

    std::vector<PixelRow> pixel_rows;
    constexpr std::ptrdiff_t bytes_per_row = 4 * 1920;
    /// @todo Only grabbing 160 pixels of height
    unsigned char* frame_data = frame->data;
    for (size_t cur_row = 0; cur_row < height_in_pixels(); ++cur_row) {
        pixel_rows.emplace_back(from_kinect_row(frame_data));
        frame_data += bytes_per_row;
    }
    GestureNetFrame g_n_frame(std::move(pixel_rows));
    return g_n_frame;
}

void GestureNetFrame::save_frame(const std::filesystem::path& output_dir, size_t sequence) const {

    const auto file_name = "160x160-BGRX-seq" + std::to_string(sequence) + ".bin";

    const auto file_path = output_dir / file_name;

    // We're writing out unsigned char instead of sgned char
    std::basic_ofstream<unsigned char, std::char_traits<unsigned char>> output_stream(
        file_path, output_stream.out | output_stream.binary);

    if (!output_stream.is_open()) {
        exit_with_error(HERE, "Could not open output stream: " + file_path.string());
    } else {
        std::cout << "Saving GestureNet frame to " << file_path << "\n";
        for (const auto& row : pixel_rows_) {
            for (const auto& pixel : row) {
                auto pixel_bytes = pixel.to_bytes();
                output_stream.write(pixel_bytes.data(), pixel_bytes.size());
            }
        }
        std::cout << "Frame saved.\n ";
    }
}

} // namespace mik

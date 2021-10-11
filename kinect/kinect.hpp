#pragma once

/*
 * Wrapper around libfreenect2 to use a kinect and write images out as files
 * Basing this impl off of libfreenect2's Protonect.cpp
 */
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <future>
#include <optional>
#include <queue>
#include <thread>

#include <libfreenect2/frame_listener.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/logger.h>
#include <libfreenect2/packet_pipeline.h>
#include <libfreenect2/registration.h>

namespace mik {
/// @TODO A to_string method
enum class InputPipeline { CUDA, OPENGL, CPU, OTHER };
struct KinectConfig {
    InputPipeline framework = InputPipeline::CPU;
    int32_t gpu_device_id = 0; // TODO Figure out what this should be, also make optional
    std::filesystem::path image_output_dir = "~/Development/kinect-time/kinect-frames";
    // Use rgb and depth by default
};

class Kinect {
  public:
    Kinect(KinectConfig config);
    ~Kinect();

    static std::string frame_type_to_string(libfreenect2::Frame::Type type);
    static std::string frame_format_to_string(libfreenect2::Frame::Format format);

    // Save frame to disk, best for checking functionality
    void save_frames(std::uint32_t n_frames_to_save);

    void start_recording();
    void stop_recording();

  private:
    /// @todo void lower_resolution(libfreenect2::Frame* frame);
    /// @todo void convert_to_gesture_net(const libfreenect2::Frame* frame);
    void save_frame(libfreenect2::Frame::Type frame_type, const libfreenect2::Frame* frame) const;
    void save_frame_async(libfreenect2::Frame::Type frame_type, const libfreenect2::Frame* frame);
    void save_gnet_frame_async(libfreenect2::Frame::Type frame_type,
                               const libfreenect2::Frame* frame);
    static void save_frame_impl(std::filesystem::path image_output_dir,
                                libfreenect2::Frame::Type frame_type,
                                const libfreenect2::Frame* frame);

    KinectConfig config_;
    std::queue<std::future<void>> saveTasks_;
    std::atomic<bool> should_record_ = false;
    std::thread recorder_;

    // libfreenect2 members
    // Don't use smart pointers as it messes with how libfreenect2 manages these objects.
    libfreenect2::Freenect2 freenect2_;
    libfreenect2::PacketPipeline* pipeline_ = nullptr;
    libfreenect2::Freenect2Device* device_ptr_ = nullptr;
    libfreenect2::Registration* registration_ = nullptr;
    libfreenect2::Frame* undistorted_ptr_ = nullptr;
    libfreenect2::Frame* registered_ptr_ = nullptr;
    libfreenect2::FrameMap frame_map_;
    libfreenect2::SyncMultiFrameListener* listener_ptr_ = nullptr;
};

} // namespace mik
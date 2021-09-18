/*
 * Wrapper around libfreenect2 to use a kinect and write images out as files
 * Basing this impl off of libfreenect2's Protonect.cpp
 */
#include <cstdint>
#include <filesystem>

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
    std::filesystem::path image_output_dir = "./kinect-frames"; // Running dir
    // Use rgb and depth by default
};

class Kinect {
  public:
    Kinect(KinectConfig config);
    ~Kinect();
    // Save frame to disk
    void save_frames(std::uint32_t n_frames_to_save);

  private:
    /// @todo void lower_resolution(libfreenect2::Frame* frame);
    /// @todo void convert_to_gesture_net(const libfreenect2::Frame* frame);
    void save_frame(libfreenect2::Frame::Type frame_type, libfreenect2::Frame* frame) const;

    KinectConfig config_;

    // libfreenect2 members
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
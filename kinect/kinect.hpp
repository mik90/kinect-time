/*
 * Wrapper around libfreenect2 to use a kinect and write images out as files
 * Basing this impl off of libfreenect2's Protonect.cpp
 */
#include "libfreenect2/frame_listener.hpp"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <future>
#include <optional>
#include <queue>

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
    // Save frame to disk
    void save_frames(std::uint32_t n_frames_to_save);

  private:
    /// @todo void lower_resolution(libfreenect2::Frame* frame);
    /// @todo void convert_to_gesture_net(const libfreenect2::Frame* frame);
    void save_frame(libfreenect2::Frame::Type frame_type, libfreenect2::Frame* frame) const;
    void save_frame_async(libfreenect2::Frame::Type frame_type, libfreenect2::Frame* frame);

    KinectConfig config_;
    std::queue<std::future<void>> saveTasks_;

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

struct GestureNetPixel {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
    /// @brief Should be a length of 4 (BGRX)
    static std::optional<GestureNetPixel> from_kinect_bgrx_pixel(const unsigned char* data,
                                                                 std::size_t len);
};
/**
 * @brief Input frame for nvidia GestureNet. Create from a kinect frame
 */
class GestureNetFrame {
  public:
    using PixelRow = std::vector<GestureNetPixel>;
    std::optional<GestureNetFrame> from_kinect_frame(libfreenect2::Frame::Type frame_type,
                                                     const libfreenect2::Frame* frame);
    void save_frame(const std::filesystem::path& output) const;

    static std::size_t bytes_per_pixel() noexcept { return 3; };
    static std::size_t width_in_pixels() noexcept { return 160; };
    static std::size_t height_in_pixels() noexcept { return 160; };
    static GestureNetFrame::PixelRow from_kinect_row(const unsigned char* data,
                                                     std::size_t kinect_row_pixel_count = 1920);
    // void downscale_height(); Convert 1920 to 160 pixels

  private:
    GestureNetFrame();

    std::vector<PixelRow> pixels_;
};

} // namespace mik
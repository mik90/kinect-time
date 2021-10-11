#pragma once
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <future>
#include <optional>
#include <queue>
#include <thread>

namespace mik {

struct GestureNetPixel {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
    std::array<std::uint8_t, 3> to_bytes() const { return {red, green, blue}; }
};
/**
 * @brief Input frame for nvidia GestureNet. Create from a kinect frame
 */
class GestureNetFrame {
  public:
    using PixelRow = std::vector<GestureNetPixel>;
    void save_frame(const std::filesystem::path& output_dir, size_t sequence) const;

    static std::size_t bytes_per_pixel() noexcept { return 3; };
    static std::size_t width_in_pixels() noexcept { return 160; };
    static std::size_t height_in_pixels() noexcept { return 160; };
    // void downscale_height(); Convert 1920 to 160 pixels
    GestureNetFrame(std::vector<PixelRow> rows) : pixel_rows_(std::move(rows)){};

  private:
    std::vector<PixelRow> pixel_rows_;
};
} // namespace mik
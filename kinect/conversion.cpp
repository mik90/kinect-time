#include "conversion.hpp"
#include "util.hpp"
#include <iostream>
namespace mik {

/// @brief Pixel to pixel conversion
std::optional<GestureNetPixel> Conversion::from_kinect_bgrx_pixel(const unsigned char* data,
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
GestureNetFrame::PixelRow Conversion::from_kinect_row(const unsigned char* data) {
    // Convert to GestureNet PixelRow format
    GestureNetFrame::PixelRow g_n_row;
    g_n_row.reserve(GestureNetFrame::bytes_per_pixel() * GestureNetFrame::width_in_pixels());
    constexpr size_t kinect_row_pixel_count = 1920;
    const auto pixels_to_merge = kinect_row_pixel_count / GestureNetFrame::width_in_pixels(); // 12

    GestureNetFrame::PixelRow gesturenet_row;

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
std::optional<GestureNetFrame> Conversion::from_kinect_frame(libfreenect2::Frame::Type frame_type,
                                                             const libfreenect2::Frame* frame) {
    if (frame_type != libfreenect2::Frame::Type::Color) {
        std::cerr << "Cannot convert frame from " << Kinect::frame_type_to_string(frame_type)
                  << " to GestureNestFrame\n";
        return {};
    }

    std::vector<GestureNetFrame::PixelRow> pixel_rows;
    constexpr std::ptrdiff_t bytes_per_row = 4 * 1920;
    /// @todo Only grabbing 160 pixels of height
    unsigned char* frame_data = frame->data;
    for (size_t cur_row = 0; cur_row < GestureNetFrame::height_in_pixels(); ++cur_row) {
        pixel_rows.emplace_back(from_kinect_row(frame_data));
        frame_data += bytes_per_row;
    }
    GestureNetFrame g_n_frame(std::move(pixel_rows));
    return g_n_frame;
}
} // namespace mik
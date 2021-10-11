#pragma once

#include "gesturenet.hpp"
#include "kinect.hpp"

namespace mik {

class Conversion {
  public:
    /// @brief Unused as of yet
    static GestureNetFrame::PixelRow from_kinect_row(const unsigned char* data);
    static std::optional<GestureNetFrame> from_kinect_frame(libfreenect2::Frame::Type frame_type,
                                                            const libfreenect2::Frame* frame);
    /// @brief Should be a length of 4 (BGRX)
    static std::optional<GestureNetPixel> from_kinect_bgrx_pixel(const unsigned char* data,
                                                                 std::size_t len);
};
} // namespace mik
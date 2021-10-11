#include "gesturenet.hpp"
#include "util.hpp"

#include <fstream>
#include <iostream>

namespace mik {

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
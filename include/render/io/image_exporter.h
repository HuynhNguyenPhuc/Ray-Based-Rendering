// Image export (PNG / JPG / BMP)

#pragma once

#include "../core/film.h"
#include <string>
#include <vector>

namespace render {

/**
 * @class ImageExporter
 * @brief Writes 8-bit RGB images to disk. The format is chosen from the file extension.
 */
class ImageExporter {
public:
    // Write tightly-packed 8-bit RGB pixels. Creates parent directories as needed.
    // Returns false if the file could not be written.
    static bool writeRGB8(const std::string& path, int width, int height,
                          const std::vector<unsigned char>& pixels);

    // Convert a film (optionally tone mapped) and write it.
    static bool writeFilm(const std::string& path, const Film& film,
                          bool toneMap = true, float exposure = 1.0f, float gamma = 2.2f);
};

} // namespace render

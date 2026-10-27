#include "render/io/image_exporter.h"

// Single translation unit that provides the stb_image_write implementation.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/stb_image_write.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace render {

bool ImageExporter::writeRGB8(const std::string& path, int width, int height,
                              const std::vector<unsigned char>& pixels) {
    if (width <= 0 || height <= 0 ||
        pixels.size() < static_cast<size_t>(width) * height * 3) {
        return false;
    }

    std::filesystem::path fsPath(path);
    if (fsPath.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(fsPath.parent_path(), ec);
    }

    std::string ext = fsPath.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    int ok = 0;
    if (ext == ".jpg" || ext == ".jpeg") {
        ok = stbi_write_jpg(path.c_str(), width, height, 3, pixels.data(), 95);
    } else if (ext == ".bmp") {
        ok = stbi_write_bmp(path.c_str(), width, height, 3, pixels.data());
    } else {
        // PNG is the default
        ok = stbi_write_png(path.c_str(), width, height, 3, pixels.data(), width * 3);
    }
    return ok != 0;
}

bool ImageExporter::writeFilm(const std::string& path, const Film& film,
                              bool toneMap, float exposure, float gamma) {
    std::vector<unsigned char> pixels = toneMap
        ? film.toRGB8ToneMapped(exposure, gamma)
        : film.toRGB8();
    return writeRGB8(path, film.width, film.height, pixels);
}

} // namespace render

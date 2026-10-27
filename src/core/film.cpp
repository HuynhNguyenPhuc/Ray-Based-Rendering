#include "render/core/film.h"

namespace render {

Film::Film(int width, int height)
    : width(width), height(height), pixels_(width * height) {}

void Film::addSample(int x, int y, const Color& color, float weight) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    pixels_[y * width + x].add(color, weight);
}

Color Film::getPixel(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) return Color(0.0f);
    return pixels_[y * width + x].getColor();
}

void Film::clear() {
    for (auto& pixel : pixels_) {
        pixel.reset();
    }
}

namespace {

// Gamma-correct one channel and quantise it to 8 bits (rounded, not truncated).
unsigned char encode(float value, float invGamma) {
    return static_cast<unsigned char>(clamp(std::pow(value, invGamma), 0.0f, 1.0f) * 255.0f + 0.5f);
}

} // namespace

std::vector<unsigned char> Film::toRGB8(float gamma) const {
    std::vector<unsigned char> data(static_cast<size_t>(width) * height * 3);
    const float invGamma = 1.0f / gamma;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const Color c = getPixel(x, y);
            const size_t idx = (static_cast<size_t>(y) * width + x) * 3;
            data[idx + 0] = encode(c.x, invGamma);
            data[idx + 1] = encode(c.y, invGamma);
            data[idx + 2] = encode(c.z, invGamma);
        }
    }
    return data;
}

std::vector<unsigned char> Film::toRGB8ToneMapped(float exposure, float gamma) const {
    std::vector<unsigned char> data(static_cast<size_t>(width) * height * 3);
    const float invGamma = 1.0f / gamma;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Color c = getPixel(x, y) * exposure;
            c = c / (c + Color(1.0f));  // Reinhard tone mapping
            const size_t idx = (static_cast<size_t>(y) * width + x) * 3;
            data[idx + 0] = encode(c.x, invGamma);
            data[idx + 1] = encode(c.y, invGamma);
            data[idx + 2] = encode(c.z, invGamma);
        }
    }
    return data;
}

} // namespace render

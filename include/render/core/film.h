// Film/image buffer for accumulating samples

#pragma once

#include "vector.h"
#include "common.h"
#include <vector>

namespace render {

/**
 * @struct Pixel
 * @brief Accumulated pixel data.
 */
struct Pixel {
    Color color;
    float weight;

    Pixel() : color(0.0f), weight(0.0f) {}

    void add(const Color& c, float w = 1.0f) {
        color += c * w;
        weight += w;
    }

    Color getColor() const {
        if (weight > 0.0f) {
            return color / weight;
        }
        return Color(0.0f);
    }

    void reset() {
        color = Color(0.0f);
        weight = 0.0f;
    }
};

/**
 * @class Film
 * @brief Image buffer that accumulates samples and outputs final image.
 */
class Film {
public:
    int width;
    int height;

    Film(int width, int height);

    // Add sample contribution
    void addSample(int x, int y, const Color& color, float weight = 1.0f);

    // Get pixel color
    Color getPixel(int x, int y) const;

    // Clear all pixels
    void clear();

    // Apply gamma correction and get 8-bit image data
    std::vector<unsigned char> toRGB8(float gamma = 2.2f) const;

    // Apply tone mapping (Reinhard) and get 8-bit image data
    std::vector<unsigned char> toRGB8ToneMapped(float exposure = 1.0f, float gamma = 2.2f) const;

    // Get raw pixel data
    const std::vector<Pixel>& pixels() const { return pixels_; }
    std::vector<Pixel>& pixels() { return pixels_; }

private:
    std::vector<Pixel> pixels_;
};

} // namespace render

// Intrinsics of the orthographic projection model

#pragma once

#include "camera_intrinsics.h"

namespace render {

/**
 * @class OrthographicIntrinsics
 * @brief Parallel projection: a pixel covers `sx` x `sy` world units on the view plane.
 *
 *     x = (px - cx) * sx        y = (cy - py) * sy
 */
class OrthographicIntrinsics : public CameraIntrinsics {
public:
    float sx = 1.0f;   // world units per pixel (horizontal)
    float sy = 1.0f;   // world units per pixel (vertical)
    float cx = 0.0f;   // principal point x in pixels
    float cy = 0.0f;   // principal point y in pixels

    OrthographicIntrinsics() = default;
    OrthographicIntrinsics(float sx, float sy, float cx, float cy) : sx(sx), sy(sy), cx(cx), cy(cy) {}

    // Square pixels, principal point at the image centre; the view is `halfHeight` world units
    // above and below the axis.
    static OrthographicIntrinsics fromHalfHeight(int width, int height, float halfHeight) {
        const float s = 2.0f * halfHeight / static_cast<float>(height);
        return OrthographicIntrinsics(s, s, 0.5f * static_cast<float>(width), 0.5f * static_cast<float>(height));
    }

    // Half the height of the view volume in world units for an image `height` pixels tall.
    float halfHeight(int height) const { return 0.5f * static_cast<float>(height) * sy; }

    CameraSpaceRay generateRay(float px, float py) const override {
        return {Vec3((px - cx) * sx, (cy - py) * sy, 0.0f), Vec3(0.0f, 0.0f, 1.0f)};
    }

    bool project(const Vec3& p, float& px, float& py) const override {
        px = p.x / sx + cx;
        py = cy - p.y / sy;
        return true;
    }
};

} // namespace render

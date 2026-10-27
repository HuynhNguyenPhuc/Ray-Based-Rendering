// Intrinsics of the perspective (pinhole) projection model

#pragma once

#include "camera_intrinsics.h"

namespace render {

/**
 * @class PerspectiveIntrinsics
 * @brief Pinhole model: the usual K matrix without skew, all values in pixels.
 *
 *     | fx  0  cx |
 * K = |  0 fy  cy |
 *     |  0  0   1 |
 *
 * K is normally applied to a camera frame with y pointing down. Camera space here has y
 * pointing up, so the y row is negated:
 *     px = fx * x / z + cx
 *     py = cy - fy * y / z
 */
class PerspectiveIntrinsics : public CameraIntrinsics {
public:
    float fx = 1.0f;   // focal length in pixels (horizontal)
    float fy = 1.0f;   // focal length in pixels (vertical)
    float cx = 0.0f;   // principal point x in pixels
    float cy = 0.0f;   // principal point y in pixels

    PerspectiveIntrinsics() = default;
    PerspectiveIntrinsics(float fx, float fy, float cx, float cy) : fx(fx), fy(fy), cx(cx), cy(cy) {}

    // Square pixels, principal point at the image centre, `fovY` is the vertical field of view in degrees.
    static PerspectiveIntrinsics fromVerticalFov(int width, int height, float fovY) {
        const float f = 0.5f * static_cast<float>(height) / std::tan(radians(fovY) * 0.5f);
        return PerspectiveIntrinsics(f, f, 0.5f * static_cast<float>(width), 0.5f * static_cast<float>(height));
    }

    // Vertical field of view in degrees for an image `height` pixels tall.
    float verticalFov(int height) const {
        return degrees(2.0f * std::atan(0.5f * static_cast<float>(height) / fy));
    }

    CameraSpaceRay generateRay(float px, float py) const override {
        return {Vec3(0.0f), Vec3((px - cx) / fx, (cy - py) / fy, 1.0f)};
    }

    bool project(const Vec3& p, float& px, float& py) const override {
        if (p.z <= 0.0f) return false;
        px = fx * p.x / p.z + cx;
        py = cy - fy * p.y / p.z;
        return true;
    }
};

} // namespace render

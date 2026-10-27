// Intrinsics of the equirectangular (latitude/longitude) panorama model

#pragma once

#include "camera_intrinsics.h"

namespace render {

/**
 * @class EquirectangularIntrinsics
 * @brief 360-degree panorama: x spans the azimuth [0, 2*pi), y the polar angle [0, pi].
 *
 * The poles lie along camera-space +y. The mapping depends on the image size, which is stored here.
 *
 *     phi = 2*pi * px / width        theta = pi * py / height
 *     direction = (sin(theta) cos(phi), cos(theta), sin(theta) sin(phi))
 */
class EquirectangularIntrinsics : public CameraIntrinsics {
public:
    int width = 1;
    int height = 1;

    EquirectangularIntrinsics() = default;
    EquirectangularIntrinsics(int width, int height) : width(width), height(height) {}

    CameraSpaceRay generateRay(float px, float py) const override {
        const float theta = PI * py / height;      // polar angle
        const float phi = TWO_PI * px / width;     // azimuth
        const float sinTheta = std::sin(theta);
        return {Vec3(0.0f), Vec3(sinTheta * std::cos(phi), std::cos(theta), sinTheta * std::sin(phi))};
    }

    bool project(const Vec3& p, float& px, float& py) const override {
        const float r = p.length();
        if (r <= 0.0f) return false;
        const float theta = std::acos(std::clamp(p.y / r, -1.0f, 1.0f));
        float phi = std::atan2(p.z, p.x);
        if (phi < 0.0f) phi += TWO_PI;
        px = phi * width / TWO_PI;
        py = theta * height / PI;
        return true;
    }
};

} // namespace render

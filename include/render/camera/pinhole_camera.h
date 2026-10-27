// Ideal pinhole camera (everything in focus)

#pragma once

#include "perspective_camera.h"

namespace render {

/**
 * @class PinholeCamera
 * @brief Perspective camera whose rays all start at the camera position.
 */
class PinholeCamera : public PerspectiveCamera {
public:
    using PerspectiveCamera::PerspectiveCamera;

    PinholeCamera(int width, int height,
                  const Vec3& position,
                  const Vec3& target = Vec3(0.0f, 0.0f, 0.0f),
                  const Vec3& up = Vec3(0.0f, 1.0f, 0.0f),
                  float fov = 60.0f)
        : PerspectiveCamera(width, height, position, target, up, fov) {}

    Ray generateRay(const CameraSample& sample) const override;
};

} // namespace render

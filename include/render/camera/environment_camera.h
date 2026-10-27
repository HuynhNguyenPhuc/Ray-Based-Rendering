// 360-degree spherical camera

#pragma once

#include "camera.h"
#include "equirectangular_intrinsics.h"

namespace render {

/**
 * @class EnvironmentCamera
 * @brief 360-degree spherical (equirectangular) camera for environment mapping.
 *
 * The sphere's poles lie along the extrinsics' up axis. The position-only constructor uses a
 * world-aligned basis (poles along world +Y).
 */
class EnvironmentCamera : public Camera {
public:
    EnvironmentCamera(int width, int height, const Vec3& position = Vec3(0.0f))
        : EnvironmentCamera(width, height, CameraExtrinsics::worldAligned(position)) {}

    EnvironmentCamera(int width, int height, const CameraExtrinsics& extrinsics)
        : Camera(width, height, extrinsics, std::make_shared<EquirectangularIntrinsics>(width, height)) {}

    Ray generateRay(const CameraSample& sample) const override { return generateIntrinsicRay(sample); }
};

} // namespace render

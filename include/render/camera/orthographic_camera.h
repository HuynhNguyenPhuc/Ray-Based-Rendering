// Orthographic projection camera

#pragma once

#include "camera.h"
#include "orthographic_intrinsics.h"

namespace render {

/**
 * @class OrthographicCamera
 * @brief Orthographic projection camera: parallel rays, no perspective foreshortening.
 */
class OrthographicCamera : public Camera {
public:
    OrthographicCamera(int width, int height,
                       const CameraExtrinsics& extrinsics,
                       const OrthographicIntrinsics& intrinsics)
        : Camera(width, height, extrinsics, std::make_shared<OrthographicIntrinsics>(intrinsics)) {}

    // Convenience: look-at placement + half the height of the view volume in world units.
    OrthographicCamera(int width, int height,
                       const Vec3& position,
                       const Vec3& target = Vec3(0.0f, 0.0f, 0.0f),
                       const Vec3& up = Vec3(0.0f, 1.0f, 0.0f),
                       float scale = 1.0f)
        : OrthographicCamera(width, height,
                             CameraExtrinsics::lookAt(position, target, up),
                             OrthographicIntrinsics::fromHalfHeight(width, height, scale)) {}

    Ray generateRay(const CameraSample& sample) const override { return generateIntrinsicRay(sample); }

    // Half the height of the view volume in world units.
    float scale() const {
        return static_cast<const OrthographicIntrinsics&>(intrinsics()).halfHeight(height);
    }
};

} // namespace render

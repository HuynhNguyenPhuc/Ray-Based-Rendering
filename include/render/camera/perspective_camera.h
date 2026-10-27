// Base class for perspective (pinhole-projection) cameras

#pragma once

#include "camera.h"
#include "perspective_intrinsics.h"

namespace render {

/**
 * @class PerspectiveCamera
 * @brief Cameras that use the perspective (pinhole) projection model.
 *
 * Fixes the intrinsics to `PerspectiveIntrinsics`; subclasses decide where the ray starts:
 * `PinholeCamera` uses a single point, `ThinLensCamera` samples a lens for depth of field.
 */
class PerspectiveCamera : public Camera {
public:
    PerspectiveCamera(int width, int height,
                      const CameraExtrinsics& extrinsics,
                      const PerspectiveIntrinsics& intrinsics)
        : Camera(width, height, extrinsics, std::make_shared<PerspectiveIntrinsics>(intrinsics)) {}

    // Convenience: look-at placement + vertical field of view in degrees.
    PerspectiveCamera(int width, int height,
                      const Vec3& position,
                      const Vec3& target,
                      const Vec3& up,
                      float fov)
        : PerspectiveCamera(width, height,
                            CameraExtrinsics::lookAt(position, target, up),
                            PerspectiveIntrinsics::fromVerticalFov(width, height, fov)) {}

    const PerspectiveIntrinsics& perspectiveIntrinsics() const {
        return static_cast<const PerspectiveIntrinsics&>(intrinsics());
    }

    // Vertical field of view in degrees.
    float fov() const { return perspectiveIntrinsics().verticalFov(height); }
};

} // namespace render

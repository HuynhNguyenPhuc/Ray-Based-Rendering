// Thin-lens perspective camera with depth of field

#pragma once

#include "perspective_camera.h"

namespace render {

/**
 * @class ThinLensCamera
 * @brief Perspective camera with a finite aperture: rays start on the lens and converge on the focal plane.
 */
class ThinLensCamera : public PerspectiveCamera {
public:
    float lensRadius;     // Aperture radius in world units (0 behaves like a pinhole)
    float focalDistance;  // Distance from the camera to the plane in perfect focus

    ThinLensCamera(int width, int height,
                   const CameraExtrinsics& extrinsics,
                   const PerspectiveIntrinsics& intrinsics,
                   float lensRadius,
                   float focalDistance)
        : PerspectiveCamera(width, height, extrinsics, intrinsics)
        , lensRadius(lensRadius), focalDistance(focalDistance) {}

    ThinLensCamera(int width, int height,
                   const Vec3& position,
                   const Vec3& target = Vec3(0.0f, 0.0f, 0.0f),
                   const Vec3& up = Vec3(0.0f, 1.0f, 0.0f),
                   float fov = 60.0f,
                   float lensRadius = 0.0f,
                   float focalDistance = 1.0f)
        : PerspectiveCamera(width, height, position, target, up, fov)
        , lensRadius(lensRadius), focalDistance(focalDistance) {}

    Ray generateRay(const CameraSample& sample) const override;
};

} // namespace render

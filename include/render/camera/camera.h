// Abstract camera interface

#pragma once

#include "../core/common.h"
#include "../core/ray.h"
#include "../core/vector.h"
#include "camera_extrinsics.h"
#include "camera_intrinsics.h"
#include <memory>

namespace render {

/**
 * @struct CameraSample
 * @brief Sample point for generating a camera ray.
 */
struct CameraSample {
    float filmX, filmY;    // Continuous raster position in pixels (pixel (i, j) covers [i, i+1) x [j, j+1))
    float lensU, lensV;    // Position on lens for DOF
    float time;            // Time for motion blur
};

/**
 * @class Camera
 * @brief Abstract base class for cameras.
 *
 * Every camera is described by three things: its resolution, its `CameraExtrinsics`
 * (position + orientation in the world) and its `CameraIntrinsics` (the projection model that
 * maps raster positions to rays in camera space). Subclasses only decide how a film sample
 * becomes a world-space ray, typically via `generateIntrinsicRay()`.
 */
class Camera {
public:
    int width;
    int height;
    CameraExtrinsics extrinsics;

    Camera(int width, int height,
           const CameraExtrinsics& extrinsics,
           std::shared_ptr<const CameraIntrinsics> intrinsics)
        : width(width), height(height), extrinsics(extrinsics), intrinsics_(std::move(intrinsics)) {}
    virtual ~Camera() = default;

    // Generate ray for a given sample
    virtual Ray generateRay(const CameraSample& sample) const = 0;

    // Generate ray through the centre of pixel (x, y) given as integer pixel indices (no lens sampling).
    // Named differently from generateRay() so subclass overrides do not hide it.
    Ray generatePixelRay(int x, int y) const;

    // Project a world-space point to raster coordinates. Returns false if the projection model cannot see it.
    bool project(const Vec3& worldPoint, float& px, float& py) const;

    // Re-aim the camera.
    void lookAt(const Vec3& position, const Vec3& target, const Vec3& up = Vec3(0.0f, 1.0f, 0.0f)) {
        extrinsics = CameraExtrinsics::lookAt(position, target, up);
    }

    const CameraIntrinsics& intrinsics() const { return *intrinsics_; }
    const Vec3& position() const { return extrinsics.position; }

    float aspectRatio() const {
        return static_cast<float>(width) / static_cast<float>(height);
    }

protected:
    // World-space ray through the sample's film position, from the intrinsics and extrinsics.
    // Direction is normalized; lens, time and other effects are left to subclasses.
    Ray generateIntrinsicRay(const CameraSample& sample) const;

private:
    std::shared_ptr<const CameraIntrinsics> intrinsics_;
};

} // namespace render

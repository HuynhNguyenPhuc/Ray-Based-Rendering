// Camera intrinsics: the projection model that relates raster positions to camera-space rays

#pragma once

#include "../core/common.h"
#include "../core/vector.h"

namespace render {

/**
 * @struct CameraSpaceRay
 * @brief Ray in camera space (x right, y up, z forward). `direction` is not necessarily normalized.
 */
struct CameraSpaceRay {
    Vec3 origin;
    Vec3 direction;
};

/**
 * @class CameraIntrinsics
 * @brief Interface of a projection model: how raster positions map to rays in camera space.
 *
 * Every camera has intrinsics; what they contain depends on the model (focal lengths and a
 * principal point for perspective, a pixel size for orthographic, the angular layout for an
 * equirectangular panorama). The extrinsics place the resulting rays in the world.
 *
 * Raster coordinates are continuous: pixel (i, j) covers [i, i+1) x [j, j+1) with its centre
 * at (i + 0.5, j + 0.5); +x goes right and +y goes down the image. Camera space has +x right,
 * +y up (so the raster y axis is flipped) and +z forward.
 */
class CameraIntrinsics {
public:
    virtual ~CameraIntrinsics() = default;

    // Camera-space ray through raster position (px, py).
    virtual CameraSpaceRay generateRay(float px, float py) const = 0;

    // Raster position of a camera-space point. Returns false if the model cannot see it
    // (e.g. behind a perspective camera).
    virtual bool project(const Vec3& p, float& px, float& py) const = 0;
};

} // namespace render

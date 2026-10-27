// Camera extrinsics: where the camera is and how it is oriented in the world

#pragma once

#include "../core/vector.h"

namespace render {

/**
 * @struct CameraExtrinsics
 * @brief Rigid placement of a camera in world space (camera-to-world).
 *
 * Camera space uses +X = right, +Y = up, +Z = forward (the viewing direction).
 * That triple is left-handed (right x up = -forward), as in pbrt/DirectX; the
 * world it is placed in is right-handed with +Y up. The basis vectors are
 * expressed in world coordinates and are orthonormal when built by `lookAt`.
 */
struct CameraExtrinsics {
    Vec3 position{0.0f, 0.0f, 0.0f};
    Vec3 forward{0.0f, 0.0f, -1.0f};
    Vec3 right{1.0f, 0.0f, 0.0f};
    Vec3 up{0.0f, 1.0f, 0.0f};

    CameraExtrinsics() = default;

    // Place a camera at `position` looking at `target`. If `up` is (nearly) parallel to
    // the viewing direction an alternative up axis is chosen so the basis never degenerates.
    static CameraExtrinsics lookAt(const Vec3& position,
                                   const Vec3& target,
                                   const Vec3& up = Vec3(0.0f, 1.0f, 0.0f)) {
        CameraExtrinsics e;
        e.position = position;
        e.forward = (target - position).normalized();

        Vec3 r = cross(e.forward, up);
        if (r.lengthSquared() < 1e-12f) {
            // up is parallel to forward: fall back to the world axis least aligned with it
            const Vec3 alt = std::abs(e.forward.y) < 0.99f ? Vec3(0.0f, 1.0f, 0.0f)
                                                           : Vec3(0.0f, 0.0f, 1.0f);
            r = cross(e.forward, alt);
        }
        e.right = r.normalized();
        e.up = cross(e.right, e.forward);
        return e;
    }

    // Basis aligned with the world axes (right = +X, up = +Y, forward = +Z).
    static CameraExtrinsics worldAligned(const Vec3& position) {
        CameraExtrinsics e;
        e.position = position;
        e.forward = Vec3(0.0f, 0.0f, 1.0f);
        return e;
    }

    // Rotate a camera-space direction (x right, y up, z forward) into world space.
    Vec3 cameraToWorldDir(const Vec3& d) const {
        return right * d.x + up * d.y + forward * d.z;
    }

    // Express a world-space point in camera space (x right, y up, z forward).
    Vec3 worldToCamera(const Vec3& p) const {
        const Vec3 d = p - position;
        return Vec3(dot(d, right), dot(d, up), dot(d, forward));
    }
};

} // namespace render

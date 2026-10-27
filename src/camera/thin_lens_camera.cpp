#include "render/camera/thin_lens_camera.h"
#include "render/core/sampler.h"

namespace render {

Ray ThinLensCamera::generateRay(const CameraSample& sample) const {
    const Vec3 rayDir = generateIntrinsicRay(sample).direction;

    if (lensRadius > 0.0f) {
        // Sample a point on the lens
        auto [lensX, lensY] = sampling::concentricDisk(sample.lensU, sample.lensV);
        const Vec3 lensPoint = extrinsics.position
                             + (extrinsics.right * lensX + extrinsics.up * lensY) * lensRadius;

        // Point where this pinhole ray crosses the plane of focus
        const float ft = focalDistance / std::abs(dot(rayDir, extrinsics.forward));
        const Vec3 focusPoint = extrinsics.position + rayDir * ft;

        return Ray(lensPoint, (focusPoint - lensPoint).normalized());
    }

    return Ray(extrinsics.position, rayDir);
}

} // namespace render

#include "render/camera/camera.h"

namespace render {

Ray Camera::generateIntrinsicRay(const CameraSample& sample) const {
    const CameraSpaceRay local = intrinsics_->generateRay(sample.filmX, sample.filmY);
    return Ray(extrinsics.position + extrinsics.cameraToWorldDir(local.origin),
               extrinsics.cameraToWorldDir(local.direction).normalized());
}

Ray Camera::generatePixelRay(int x, int y) const {
    CameraSample sample{x + 0.5f, y + 0.5f, 0.5f, 0.5f, 0.0f};
    return generateRay(sample);
}

bool Camera::project(const Vec3& worldPoint, float& px, float& py) const {
    return intrinsics_->project(extrinsics.worldToCamera(worldPoint), px, py);
}

} // namespace render

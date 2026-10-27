#include "render/camera/pinhole_camera.h"

namespace render {

Ray PinholeCamera::generateRay(const CameraSample& sample) const {
    return generateIntrinsicRay(sample);
}

} // namespace render

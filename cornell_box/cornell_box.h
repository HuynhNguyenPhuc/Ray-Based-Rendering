// The Cornell box test scene shared by the CLI renderer and the technique comparison

#pragma once

#include "render/camera/camera.h"
#include "render/scene/scene.h"
#include <memory>

namespace render {
namespace cornell_box {

// Cornell box with a red/green wall pair, glass and mirror spheres,
// an area light on the ceiling and a point light. Optionally filled with fog.
Scene createScene(bool withFog = false);

// Camera looking into the open front of the box.
std::unique_ptr<Camera> createCamera(int width, int height);

} // namespace cornell_box
} // namespace render

// Build cameras and integrators from a RenderConfig

#pragma once

#include "render_config.h"
#include "../camera/camera.h"
#include "../renderers/integrator.h"
#include <memory>

namespace render {

// Lives in config/ because it depends on both camera/ and renderers/.
std::unique_ptr<Camera> createCamera(const RenderConfig& config);
std::unique_ptr<Integrator> createIntegrator(const RenderConfig& config);

} // namespace render

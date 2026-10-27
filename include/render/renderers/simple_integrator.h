// Direct lighting with Blinn-Phong shading

#pragma once

#include "integrator.h"

namespace render {

/**
 * @class SimpleIntegrator
 * @brief Basic direct lighting with Blinn-Phong shading.
 */
class SimpleIntegrator : public Integrator {
public:
    SimpleIntegrator() = default;
    explicit SimpleIntegrator(const IntegratorConfig& config) : Integrator(config) {}

    std::string name() const override { return "Simple (Direct Lighting)"; }

    Color Li(const Ray& ray, const Scene& scene, Sampler& sampler) const override;
};

} // namespace render

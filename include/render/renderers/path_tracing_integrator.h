// Monte Carlo path tracing with NEE, MIS and Russian roulette

#pragma once

#include "integrator.h"

namespace render {

/**
 * @class PathTracingIntegrator
 * @brief Monte Carlo path tracer with global illumination.
 */
class PathTracingIntegrator : public Integrator {
public:
    PathTracingIntegrator() = default;
    explicit PathTracingIntegrator(const IntegratorConfig& config) : Integrator(config) {}

    std::string name() const override { return "Path Tracing"; }

    Color Li(const Ray& ray, const Scene& scene, Sampler& sampler) const override;

private:
    Color estimateDirectLighting(const SurfaceInteraction& isect, const BSDF* bsdf,
        const Scene& scene, Sampler& sampler) const;

    Color sampleLightDirect(const SurfaceInteraction& isect, const BSDF* bsdf,
        const Light* light, const Scene& scene, Sampler& sampler) const;
};

} // namespace render

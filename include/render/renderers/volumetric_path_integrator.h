// Path tracing through participating media

#pragma once

#include "volume_integrator.h"

namespace render {

/**
 * @class VolumetricPathIntegrator
 * @brief Path tracing with volumetric support for multiple scattering.
 */
class VolumetricPathIntegrator : public Integrator {
public:
    VolumeConfig volumeConfig;

    VolumetricPathIntegrator() = default;
    
    VolumetricPathIntegrator(const IntegratorConfig& config, const VolumeConfig& volConfig)
        : Integrator(config), volumeConfig(volConfig) {}

    std::string name() const override { return "Volumetric Path Tracing"; }

    Color Li(const Ray& ray, const Scene& scene, Sampler& sampler) const override;

private:
    Color directLightingVolume(const Vec3& point, const Vec3& wo,
        const Medium* medium, const Scene& scene,
        Sampler& sampler) const;
    
    Color directLightingSurface(const SurfaceInteraction& isect, const BSDF* bsdf,
        const Scene& scene, Sampler& sampler) const;
};

} // namespace render

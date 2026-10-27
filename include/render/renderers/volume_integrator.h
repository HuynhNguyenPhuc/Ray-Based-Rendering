// Volume rendering with ray marching

#pragma once

#include "integrator.h"
#include "../materials/medium.h"

namespace render {

/**
 * @struct VolumeConfig
 * @brief Additional configuration for volume rendering.
 */
struct VolumeConfig {
    int numSteps = 64;              // Number of ray marching steps
    bool enableScattering = true;   // Enable in-scattering
    bool multipleScattering = false; // Enable multiple scattering
    float stepSizeMultiplier = 1.0f; // Adjust step size
};

/**
 * @class VolumeIntegrator
 * @brief Physically-based volume rendering integrator.
 * 
 * Implements the volume rendering equation:
 *   L(x,ω) = T(x,x_s) * L_s + ∫ T(x,x') * σ_s(x') * L_i(x',ω) dx'
 * 
 * where:
 *   T(a,b) = exp(-∫σ_t(x)dx) is the transmittance
 *   σ_t = σ_a + σ_s is the extinction coefficient
 *   L_i is the in-scattered radiance
 */
class VolumeIntegrator : public Integrator {
public:
    VolumeConfig volumeConfig;

    VolumeIntegrator() = default;
    
    VolumeIntegrator(const IntegratorConfig& config, const VolumeConfig& volConfig)
        : Integrator(config), volumeConfig(volConfig) {}

    std::string name() const override { return "Volume Rendering"; }

    Color Li(const Ray& ray, const Scene& scene, Sampler& sampler) const override;

private:
    // Volume ray marching
    Color traceVolume(const Ray& ray, const Scene& scene, Sampler& sampler, int depth) const;

    // Compute in-scattering at a point in the medium
    Color computeInScattering(const Vec3& point, const Vec3& wo,
        const Medium* medium, const Scene& scene,
        Sampler& sampler) const;

    // Compute transmittance along a ray segment through the medium
    Color computeTransmittance(const Vec3& start, const Vec3& dir, float distance,
        const Medium* medium) const;

    // Shade a surface intersection (fallback to simple direct lighting)
    Color shadeSurface(const SurfaceInteraction& isect, const Scene& scene,
        Sampler& sampler, int depth) const;

    // Trace surface only (when no medium present)
    Color traceSurface(const Ray& ray, const Scene& scene, Sampler& sampler, int depth) const;
};

} // namespace render

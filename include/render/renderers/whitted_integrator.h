// Whitted-style recursive ray tracing

#pragma once

#include "integrator.h"

namespace render {

/**
 * @class WhittedIntegrator
 * @brief Classic Whitted-style recursive ray tracing.
 */
class WhittedIntegrator : public Integrator {
public:
    WhittedIntegrator() = default;
    explicit WhittedIntegrator(const IntegratorConfig& config) : Integrator(config) {}

    std::string name() const override { return "Whitted Ray Tracing"; }

    Color Li(const Ray& ray, const Scene& scene, Sampler& sampler) const override;

private:
    Color trace(const Ray& ray, const Scene& scene, Sampler& sampler, int depth) const;

    // Perfect specular reflection
    Color traceReflection(const Ray& ray, const SurfaceInteraction& isect,
        const Scene& scene, Sampler& sampler, int depth) const;

    // Specular refraction with Fresnel
    Color traceRefraction(const Ray& ray, const SurfaceInteraction& isect,
        const Scene& scene, Sampler& sampler, int depth) const;

    // Refract ray using Snell's law
    static bool refract(const Vec3& incident, const Vec3& normal, float eta, Vec3& refracted);
};

} // namespace render

// Debug visualisation integrators (normals, depth, ambient occlusion)

#pragma once

#include "integrator.h"

namespace render {

/**
 * @class NormalIntegrator
 * @brief Visualize surface normals (debug integrator).
 */
class NormalIntegrator : public Integrator {
public:
    std::string name() const override { return "Normal Visualization"; }

    Color Li(const Ray& ray, const Scene& scene, Sampler& sampler) const override;
};

/**
 * @class DepthIntegrator
 * @brief Visualize depth (debug integrator).
 */
class DepthIntegrator : public Integrator {
public:
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

    DepthIntegrator() = default;
    DepthIntegrator(float near, float far) : nearPlane(near), farPlane(far) {}

    std::string name() const override { return "Depth Visualization"; }

    Color Li(const Ray& ray, const Scene& scene, Sampler& sampler) const override;
};

/**
 * @class AOIntegrator
 * @brief Ambient Occlusion integrator.
 */
class AOIntegrator : public Integrator {
public:
    int numSamples = 16;
    float maxDistance = 2.0f;

    AOIntegrator() = default;
    AOIntegrator(int samples, float maxDist) : numSamples(samples), maxDistance(maxDist) {}

    std::string name() const override { return "Ambient Occlusion"; }

    Color Li(const Ray& ray, const Scene& scene, Sampler& sampler) const override;
};

} // namespace render

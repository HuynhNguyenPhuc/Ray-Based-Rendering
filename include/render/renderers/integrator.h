// Base class for all rendering techniques

#pragma once

#include "../core/common.h"
#include "../core/vector.h"
#include "../core/ray.h"
#include "../core/film.h"
#include "../core/sampler.h"
#include "../core/interaction.h"
#include "../camera/camera.h"
#include "../scene/scene.h"
#include <functional>
#include <string>

namespace render {

/**
 * @struct IntegratorConfig
 * @brief Configuration parameters for integrators.
 */
struct IntegratorConfig {
    int maxDepth = 8;           // Maximum recursion depth
    int samplesPerPixel = 1;    // Number of samples per pixel
    float russianRoulette = 0.95f;  // Russian roulette survival probability
    bool useNEE = true;         // Use Next Event Estimation
    bool useMIS = true;         // Use Multiple Importance Sampling
};

/**
 * @class Integrator
 * @brief Abstract base class for rendering integrators.
 * 
 * An integrator computes the radiance along a ray by solving the
 * rendering equation using various algorithms.
 */
class Integrator {
public:
    IntegratorConfig config;

    Integrator() = default;
    explicit Integrator(const IntegratorConfig& config) : config(config) {}
    virtual ~Integrator() = default;

    /**
     * Compute incoming radiance along a ray.
     * @param ray The ray to trace
     * @param scene The scene to render
     * @param sampler Random number generator
     * @return Computed radiance
     */
    virtual Color Li(const Ray& ray, const Scene& scene, Sampler& sampler) const = 0;

    /**
     * Render the scene to a film.
     * @param scene The scene to render
     * @param camera The camera to use
     * @param film Output film buffer
     * @param progress Optional progress callback
     */
    virtual void render(const Scene& scene, const Camera& camera, Film& film,
        std::function<void(float)> progress = nullptr) const;

    /**
     * Get integrator name for logging.
     */
    virtual std::string name() const = 0;

protected:
    // Utility: Get direct lighting at a surface point
    Color directLighting(const SurfaceInteraction& isect, const Scene& scene,
        Sampler& sampler) const;
};

} // namespace render

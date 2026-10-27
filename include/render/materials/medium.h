// Participating media

#pragma once

#include "../core/common.h"
#include "../core/ray.h"
#include "../core/vector.h"
#include "phase_function.h"
#include <memory>

namespace render {

/**
 * @class Medium
 * @brief Abstract base for participating media.
 */
class Medium {
public:
    virtual ~Medium() = default;

    /**
     * Sample a distance along the ray in the medium.
     * @param ray Ray through the medium
     * @param maxDist Maximum distance to sample
     * @param u Random sample
     * @param[out] t Sampled distance
     * @param[out] weight Transmittance weight
     * @return True if scattering occurs before maxDist
     */
    virtual bool sampleDistance(const Ray& ray, float maxDist, float u,
                                float& t, Color& weight) const = 0;

    /**
     * Compute transmittance along a ray segment.
     */
    virtual Color transmittance(const Ray& ray, float distance) const = 0;

    /**
     * Get the phase function for this medium.
     */
    virtual const PhaseFunction* getPhaseFunction() const = 0;

    /**
     * Get absorption coefficient at a point.
     */
    virtual Color getSigmaA(const Vec3& p) const = 0;

    /**
     * Get scattering coefficient at a point.
     */
    virtual Color getSigmaS(const Vec3& p) const = 0;

    /**
     * Get extinction coefficient (sigma_a + sigma_s).
     */
    virtual Color getSigmaT(const Vec3& p) const;
};

/**
 * @class HomogeneousMedium
 * @brief Uniform density participating medium.
 */
class HomogeneousMedium : public Medium {
public:
    Color sigma_a;  // Absorption coefficient
    Color sigma_s;  // Scattering coefficient
    std::unique_ptr<PhaseFunction> phase;

    HomogeneousMedium(const Color& sigma_a, const Color& sigma_s, float g = 0.0f)
        : sigma_a(sigma_a), sigma_s(sigma_s)
        , phase(std::make_unique<HenyeyGreensteinPhase>(g)) {}

    Color getSigmaA(const Vec3& p) const override { return sigma_a; }
    Color getSigmaS(const Vec3& p) const override { return sigma_s; }

    bool sampleDistance(const Ray& ray, float maxDist, float u,
        float& t, Color& weight) const override;

    Color transmittance(const Ray& ray, float distance) const override;

    const PhaseFunction* getPhaseFunction() const override;
};

/**
 * @class ExponentialMedium
 * @brief Medium with exponentially decaying density (e.g., atmosphere).
 */
class ExponentialMedium : public Medium {
public:
    Color sigma_a;      // Base absorption at y=0
    Color sigma_s;      // Base scattering at y=0
    float falloff;      // Exponential falloff rate
    float baseHeight;   // Reference height
    std::unique_ptr<PhaseFunction> phase;

    ExponentialMedium(const Color& sigma_a, const Color& sigma_s,
                      float falloff, float baseHeight = 0.0f, float g = 0.0f)
        : sigma_a(sigma_a), sigma_s(sigma_s)
        , falloff(falloff), baseHeight(baseHeight)
        , phase(std::make_unique<HenyeyGreensteinPhase>(g)) {}

    float densityScale(const Vec3& p) const;

    Color getSigmaA(const Vec3& p) const override;

    Color getSigmaS(const Vec3& p) const override;

    bool sampleDistance(const Ray& ray, float maxDist, float u,
        float& t, Color& weight) const override;

    Color transmittance(const Ray& ray, float distance) const override;

    const PhaseFunction* getPhaseFunction() const override;
};

} // namespace render

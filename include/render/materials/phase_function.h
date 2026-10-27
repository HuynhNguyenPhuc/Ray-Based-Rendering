// Phase functions for participating media

#pragma once

#include "../core/common.h"
#include "../core/vector.h"
#include "../core/sampler.h"

namespace render {

/**
 * @struct PhaseFunction
 * @brief Describes how light scatters within a medium.
 */
class PhaseFunction {
public:
    virtual ~PhaseFunction() = default;

    /**
     * Evaluate phase function for given directions.
     * @param wo Outgoing direction
     * @param wi Incoming direction
     * @return Phase function value
     */
    virtual float evaluate(const Vec3& wo, const Vec3& wi) const = 0;

    /**
     * Sample a scattering direction.
     * @param wo Outgoing direction
     * @param u1, u2 Random samples
     * @return Sampled incoming direction
     */
    virtual Vec3 sample(const Vec3& wo, float u1, float u2) const = 0;

    /**
     * PDF for sampling a direction.
     */
    virtual float pdf(const Vec3& wo, const Vec3& wi) const = 0;
};

/**
 * @class IsotropicPhase
 * @brief Uniform scattering in all directions.
 */
class IsotropicPhase : public PhaseFunction {
public:
    float evaluate(const Vec3& wo, const Vec3& wi) const override;

    Vec3 sample(const Vec3& wo, float u1, float u2) const override;

    float pdf(const Vec3& wo, const Vec3& wi) const override;
};

/**
 * @class HenyeyGreensteinPhase
 * @brief Anisotropic phase function commonly used for clouds, fog, etc.
 * 
 * Parameter g controls anisotropy:
 *   g = 0: isotropic
 *   g > 0: forward scattering
 *   g < 0: backward scattering
 */
class HenyeyGreensteinPhase : public PhaseFunction {
public:
    float g;  // Asymmetry parameter [-1, 1]

    explicit HenyeyGreensteinPhase(float g = 0.0f) : g(g) {}

    float evaluate(const Vec3& wo, const Vec3& wi) const override;

    Vec3 sample(const Vec3& wo, float u1, float u2) const override;

    float pdf(const Vec3& wo, const Vec3& wi) const override;
};

} // namespace render

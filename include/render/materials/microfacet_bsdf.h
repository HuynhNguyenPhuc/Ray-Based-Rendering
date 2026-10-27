// Rough (GGX / Trowbridge-Reitz microfacet) reflection and transmission

#pragma once

#include "bsdf.h"
#include "fresnel.h"

namespace render {

/**
 * @class GGXDistribution
 * @brief Isotropic GGX normal distribution with Smith masking, sampled by visible normals.
 *
 * `alpha` is the squared perceptual roughness.
 */
class GGXDistribution {
public:
    explicit GGXDistribution(float alpha) : alpha(std::max(alpha, 1e-3f)) {}

    float D(const Vec3& wh) const;
    float G1(const Vec3& w) const;
    float G(const Vec3& wo, const Vec3& wi) const;

    /** Density of sampleWh(wo) with respect to the half vector's solid angle. */
    float pdf(const Vec3& wo, const Vec3& wh) const;
    Vec3 sampleWh(const Vec3& wo, float u1, float u2) const;

    float alpha;

private:
    float lambda(const Vec3& w) const;
};

/**
 * @class MicrofacetReflectionBSDF
 * @brief Glossy reflection off a rough interface (metal or dielectric, depending on `fresnel`).
 */
class MicrofacetReflectionBSDF : public BSDF {
public:
    Color reflectance;
    Fresnel fresnel;
    GGXDistribution distribution;

    MicrofacetReflectionBSDF(const Color& reflectance, const Fresnel& fresnel, float alpha)
        : reflectance(reflectance), fresnel(fresnel), distribution(alpha) {}

    Color evaluate(const Vec3& wo, const Vec3& wi) const override;
    BSDFSample sample(const Vec3& wo, float u1, float u2) const override;
    float pdf(const Vec3& wo, const Vec3& wi) const override;

    BSDFType type() const override { return BSDFType::GlossyReflection; }
    Color rho(const Vec3& wo) const override { return reflectance * fresnel(cosTheta(wo)); }
};

/**
 * @class MicrofacetTransmissionBSDF
 * @brief Refraction through a rough dielectric interface (the (1 - Fresnel) share).
 *
 * `etaOutside` is the index on the side the normal points to, `etaInside` on the other side.
 */
class MicrofacetTransmissionBSDF : public BSDF {
public:
    Color transmittance;
    float etaOutside, etaInside;
    GGXDistribution distribution;

    MicrofacetTransmissionBSDF(const Color& transmittance, float etaOutside, float etaInside,
                               float alpha)
        : transmittance(transmittance), etaOutside(etaOutside), etaInside(etaInside)
        , distribution(alpha) {}

    Color evaluate(const Vec3& wo, const Vec3& wi) const override;
    BSDFSample sample(const Vec3& wo, float u1, float u2) const override;
    float pdf(const Vec3& wo, const Vec3& wi) const override;

    BSDFType type() const override { return BSDFType::Glossy | BSDFType::Transmission; }
    Color rho(const Vec3& wo) const override {
        return transmittance * (1.0f - fresnelDielectric(cosTheta(wo), etaOutside, etaInside));
    }

private:
    // Half vector of a refracted pair, oriented towards wo; false if the pair is invalid.
    bool halfVector(const Vec3& wo, const Vec3& wi, Vec3& wh, float& eta) const;
};

} // namespace render

// Perfectly specular (delta) reflection and transmission

#pragma once

#include "bsdf.h"
#include "fresnel.h"

namespace render {

/**
 * @class SpecularReflectionBSDF
 * @brief Perfect mirror reflection, scaled by a Fresnel term.
 */
class SpecularReflectionBSDF : public BSDF {
public:
    Color reflectance;
    Fresnel fresnel;

    explicit SpecularReflectionBSDF(const Color& reflectance = Color(1.0f),
                                    const Fresnel& fresnel = Fresnel::none())
        : reflectance(reflectance), fresnel(fresnel) {}

    Color evaluate(const Vec3& wo, const Vec3& wi) const override;
    BSDFSample sample(const Vec3& wo, float u1, float u2) const override;
    float pdf(const Vec3& wo, const Vec3& wi) const override;

    BSDFType type() const override { return BSDFType::SpecularReflection; }
    bool isDelta() const override { return true; }
    Color rho(const Vec3& wo) const override { return reflectance * fresnel(cosTheta(wo)); }
};

/**
 * @class SpecularTransmissionBSDF
 * @brief Perfect refraction: the (1 - Fresnel) share of the light that crosses the interface.
 *
 * `etaOutside` is the index on the side the normal points to, `etaInside` on the other side.
 * The reflected share is a separate lobe (SpecularReflectionBSDF with a dielectric Fresnel).
 */
class SpecularTransmissionBSDF : public BSDF {
public:
    Color transmittance;
    float etaOutside, etaInside;

    SpecularTransmissionBSDF(const Color& transmittance, float etaOutside, float etaInside)
        : transmittance(transmittance), etaOutside(etaOutside), etaInside(etaInside) {}

    Color evaluate(const Vec3& wo, const Vec3& wi) const override;
    BSDFSample sample(const Vec3& wo, float u1, float u2) const override;
    float pdf(const Vec3& wo, const Vec3& wi) const override;

    BSDFType type() const override { return BSDFType::SpecularTransmission; }
    bool isDelta() const override { return true; }
    Color rho(const Vec3& wo) const override {
        return transmittance * (1.0f - fresnelDielectric(cosTheta(wo), etaOutside, etaInside));
    }
};

} // namespace render

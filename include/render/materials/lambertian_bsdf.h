// Lambertian BSDF

#pragma once

#include "bsdf.h"

namespace render {

/**
 * @class LambertianBSDF
 * @brief Ideal diffuse (Lambertian) reflection.
 */
class LambertianBSDF : public BSDF {
public:
    Color albedo;

    explicit LambertianBSDF(const Color& albedo) : albedo(albedo) {}

    Color evaluate(const Vec3& wo, const Vec3& wi) const override;

    BSDFSample sample(const Vec3& wo, float u1, float u2) const override;

    float pdf(const Vec3& wo, const Vec3& wi) const override;

    BSDFType type() const override { return BSDFType::DiffuseReflection; }
    Color rho(const Vec3&) const override { return albedo; }
};

} // namespace render

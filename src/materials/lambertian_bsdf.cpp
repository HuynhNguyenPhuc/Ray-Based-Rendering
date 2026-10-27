#include "render/materials/lambertian_bsdf.h"

namespace render {

Color LambertianBSDF::evaluate(const Vec3& wo, const Vec3& wi) const {
    if (!sameHemisphere(wo, wi)) return Color(0.0f);
    return albedo * INV_PI;
}

BSDFSample LambertianBSDF::sample(const Vec3& wo, float u1, float u2) const {
    BSDFSample result;
    
    // Cosine-weighted hemisphere sampling
    result.wi = sampling::cosineHemisphere(u1, u2);
    if (wo.z < 0.0f) result.wi.z = -result.wi.z;
    
    result.f = albedo * INV_PI;
    result.pdf = absCosTheta(result.wi) * INV_PI;
    result.type = BSDFType::DiffuseReflection;
    result.isDelta = false;
    
    return result;
}

float LambertianBSDF::pdf(const Vec3& wo, const Vec3& wi) const {
    if (!sameHemisphere(wo, wi)) return 0.0f;
    return absCosTheta(wi) * INV_PI;
}

} // namespace render

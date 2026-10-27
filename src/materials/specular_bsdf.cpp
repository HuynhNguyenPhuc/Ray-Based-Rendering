#include "render/materials/specular_bsdf.h"

namespace render {

Color SpecularReflectionBSDF::evaluate(const Vec3&, const Vec3&) const {
    return Color(0.0f);  // delta distribution
}

BSDFSample SpecularReflectionBSDF::sample(const Vec3& wo, float, float) const {
    BSDFSample result;
    const float cosO = absCosTheta(wo);
    if (cosO == 0.0f) return result;

    result.wi = Vec3(-wo.x, -wo.y, wo.z);
    result.f = reflectance * fresnel(cosTheta(wo)) / cosO;  // R * F / |cos|
    result.pdf = 1.0f;
    result.type = BSDFType::SpecularReflection;
    result.isDelta = true;
    return result;
}

float SpecularReflectionBSDF::pdf(const Vec3&, const Vec3&) const {
    return 0.0f;
}

Color SpecularTransmissionBSDF::evaluate(const Vec3&, const Vec3&) const {
    return Color(0.0f);  // delta distribution
}

BSDFSample SpecularTransmissionBSDF::sample(const Vec3& wo, float, float) const {
    BSDFSample result;
    if (wo.z == 0.0f) return result;

    const bool entering = wo.z > 0.0f;
    const float etaI = entering ? etaOutside : etaInside;
    const float etaT = entering ? etaInside : etaOutside;
    const float eta = etaI / etaT;

    // Snell's law; no transmitted ray under total internal reflection.
    const float cosI = absCosTheta(wo);
    const float sin2T = eta * eta * std::max(0.0f, 1.0f - cosI * cosI);
    if (sin2T >= 1.0f) return result;
    const float cosT = std::sqrt(1.0f - sin2T);

    result.wi = Vec3(-eta * wo.x, -eta * wo.y, entering ? -cosT : cosT);
    const float fr = fresnelDielectric(wo.z, etaOutside, etaInside);
    // Radiance is compressed/expanded by (etaI / etaT)^2 when it crosses the interface.
    result.f = transmittance * ((1.0f - fr) * eta * eta / cosT);
    result.pdf = 1.0f;
    result.type = BSDFType::SpecularTransmission;
    result.isDelta = true;
    return result;
}

float SpecularTransmissionBSDF::pdf(const Vec3&, const Vec3&) const {
    return 0.0f;
}

} // namespace render

#include "render/materials/microfacet_bsdf.h"

namespace render {

// ---------------------------------------------------------------------------------------------
// GGX distribution
// ---------------------------------------------------------------------------------------------

float GGXDistribution::D(const Vec3& wh) const {
    const float cos2 = wh.z * wh.z;
    if (cos2 <= 0.0f) return 0.0f;
    const float a2 = alpha * alpha;
    const float d = cos2 * (a2 - 1.0f) + 1.0f;
    return a2 / (PI * d * d);
}

float GGXDistribution::lambda(const Vec3& w) const {
    const float cos2 = w.z * w.z;
    if (cos2 <= 0.0f) return 0.0f;
    const float tan2 = std::max(0.0f, 1.0f - cos2) / cos2;
    return 0.5f * (std::sqrt(1.0f + alpha * alpha * tan2) - 1.0f);
}

float GGXDistribution::G1(const Vec3& w) const {
    return 1.0f / (1.0f + lambda(w));
}

float GGXDistribution::G(const Vec3& wo, const Vec3& wi) const {
    return 1.0f / (1.0f + lambda(wo) + lambda(wi));
}

float GGXDistribution::pdf(const Vec3& wo, const Vec3& wh) const {
    const float cosO = std::abs(wo.z);
    if (cosO == 0.0f) return 0.0f;
    return D(wh) * G1(wo) * std::abs(dot(wo, wh)) / cosO;
}

// Heitz, "Sampling the GGX Distribution of Visible Normals" (2018).
Vec3 GGXDistribution::sampleWh(const Vec3& wo, float u1, float u2) const {
    const bool flip = wo.z < 0.0f;
    const Vec3 v = flip ? -wo : wo;

    const Vec3 vh = Vec3(alpha * v.x, alpha * v.y, v.z).normalized();
    const Vec3 t1 = vh.z < 0.9999f ? cross(Vec3(0.0f, 0.0f, 1.0f), vh).normalized()
                                   : Vec3(1.0f, 0.0f, 0.0f);
    const Vec3 t2 = cross(vh, t1);

    const float r = std::sqrt(u1);
    const float phi = TWO_PI * u2;
    const float p1 = r * std::cos(phi);
    float p2 = r * std::sin(phi);
    const float s = 0.5f * (1.0f + vh.z);
    p2 = (1.0f - s) * std::sqrt(std::max(0.0f, 1.0f - p1 * p1)) + s * p2;

    const Vec3 nh = t1 * p1 + t2 * p2 + vh * std::sqrt(std::max(0.0f, 1.0f - p1 * p1 - p2 * p2));
    const Vec3 wh = Vec3(alpha * nh.x, alpha * nh.y, std::max(1e-6f, nh.z)).normalized();
    return flip ? -wh : wh;
}

// ---------------------------------------------------------------------------------------------
// Reflection
// ---------------------------------------------------------------------------------------------

Color MicrofacetReflectionBSDF::evaluate(const Vec3& wo, const Vec3& wi) const {
    if (!sameHemisphere(wo, wi)) return Color(0.0f);
    const float cosO = absCosTheta(wo), cosI = absCosTheta(wi);
    if (cosO == 0.0f || cosI == 0.0f) return Color(0.0f);

    Vec3 wh = wo + wi;
    if (wh.x == 0.0f && wh.y == 0.0f && wh.z == 0.0f) return Color(0.0f);
    wh = wh.normalized();
    if (wh.z < 0.0f) wh = -wh;

    const Color F = fresnel(dot(wo, wh));
    return reflectance * F * (distribution.D(wh) * distribution.G(wo, wi) / (4.0f * cosO * cosI));
}

BSDFSample MicrofacetReflectionBSDF::sample(const Vec3& wo, float u1, float u2) const {
    BSDFSample result;
    if (wo.z == 0.0f) return result;

    const Vec3 wh = distribution.sampleWh(wo, u1, u2);
    const Vec3 wi = wh * (2.0f * dot(wo, wh)) - wo;
    if (!sameHemisphere(wo, wi)) return result;

    const float p = pdf(wo, wi);
    if (p <= 0.0f) return result;
    result.wi = wi;
    result.pdf = p;
    result.f = evaluate(wo, wi);
    result.type = BSDFType::GlossyReflection;
    return result;
}

float MicrofacetReflectionBSDF::pdf(const Vec3& wo, const Vec3& wi) const {
    if (!sameHemisphere(wo, wi)) return 0.0f;
    Vec3 wh = wo + wi;
    if (wh.x == 0.0f && wh.y == 0.0f && wh.z == 0.0f) return 0.0f;
    wh = wh.normalized();
    if (wh.z < 0.0f) wh = -wh;
    const float dotOH = dot(wo, wh);
    if (dotOH == 0.0f) return 0.0f;
    return distribution.pdf(wo, wh) / (4.0f * std::abs(dotOH));
}

// ---------------------------------------------------------------------------------------------
// Transmission
// ---------------------------------------------------------------------------------------------

bool MicrofacetTransmissionBSDF::halfVector(const Vec3& wo, const Vec3& wi, Vec3& wh, float& eta) const {
    if (sameHemisphere(wo, wi)) return false;
    if (wo.z == 0.0f || wi.z == 0.0f) return false;

    // eta = index on the wi side over the index on the wo side.
    eta = wo.z > 0.0f ? etaInside / etaOutside : etaOutside / etaInside;
    wh = wo + wi * eta;
    if (wh.x == 0.0f && wh.y == 0.0f && wh.z == 0.0f) return false;
    wh = wh.normalized();
    if (dot(wo, wh) < 0.0f) wh = -wh;

    // Only microfacets that wo can see count (the same ones sampleWh produces), and the
    // microsurface normal must separate wo from wi.
    return wh.z * wo.z > 0.0f && dot(wi, wh) <= 0.0f;
}

Color MicrofacetTransmissionBSDF::evaluate(const Vec3& wo, const Vec3& wi) const {
    Vec3 wh;
    float eta;
    if (!halfVector(wo, wi, wh, eta)) return Color(0.0f);

    const float cosO = cosTheta(wo), cosI = cosTheta(wi);
    const float dotOH = dot(wo, wh), dotIH = dot(wi, wh);
    const float F = fresnelDielectric(wo.z > 0.0f ? dotOH : -dotOH, etaOutside, etaInside);
    const float denom = dotOH + eta * dotIH;
    if (denom == 0.0f) return Color(0.0f);

    // The radiance scaling 1/eta^2 cancels the eta^2 of the half-vector Jacobian.
    const float value = distribution.D(wh) * distribution.G(wo, wi) * std::abs(dotIH * dotOH) /
                        std::abs(cosI * cosO * denom * denom);
    return transmittance * ((1.0f - F) * value);
}

float MicrofacetTransmissionBSDF::pdf(const Vec3& wo, const Vec3& wi) const {
    Vec3 wh;
    float eta;
    if (!halfVector(wo, wi, wh, eta)) return 0.0f;

    const float dotOH = dot(wo, wh), dotIH = dot(wi, wh);
    const float denom = dotOH + eta * dotIH;
    if (denom == 0.0f) return 0.0f;
    const float dwhDwi = std::abs(eta * eta * dotIH) / (denom * denom);
    return distribution.pdf(wo, wh) * dwhDwi;
}

BSDFSample MicrofacetTransmissionBSDF::sample(const Vec3& wo, float u1, float u2) const {
    BSDFSample result;
    if (wo.z == 0.0f) return result;

    const Vec3 wh = distribution.sampleWh(wo, u1, u2);
    const float dotOH = dot(wo, wh);
    if (dotOH <= 0.0f) return result;

    // Refract wo about wh (wh faces the wo side); etaRatio = wo side over wi side.
    const bool entering = wo.z > 0.0f;
    const float etaRatio = entering ? etaOutside / etaInside : etaInside / etaOutside;
    const float sin2T = etaRatio * etaRatio * std::max(0.0f, 1.0f - dotOH * dotOH);
    if (sin2T >= 1.0f) return result;  // total internal reflection: the reflection lobe handles it
    const float cosT = std::sqrt(1.0f - sin2T);
    const Vec3 wi = wh * (etaRatio * dotOH - cosT) - wo * etaRatio;
    if (sameHemisphere(wo, wi) || wi.z == 0.0f) return result;

    const float p = pdf(wo, wi);
    if (p <= 0.0f) return result;
    result.wi = wi;
    result.pdf = p;
    result.f = evaluate(wo, wi);
    result.type = BSDFType::Glossy | BSDFType::Transmission;
    return result;
}

} // namespace render

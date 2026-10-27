#include "render/materials/phase_function.h"

namespace render {

float IsotropicPhase::evaluate(const Vec3& wo, const Vec3& wi) const {
    return 1.0f / (4.0f * PI);
}

Vec3 IsotropicPhase::sample(const Vec3& wo, float u1, float u2) const {
    return sampling::uniformSphere(u1, u2);
}

float IsotropicPhase::pdf(const Vec3& wo, const Vec3& wi) const {
    return 1.0f / (4.0f * PI);
}

float HenyeyGreensteinPhase::evaluate(const Vec3& wo, const Vec3& wi) const {
    float cosTheta = dot(wo, wi);
    float denom = 1.0f + g * g + 2.0f * g * cosTheta;
    return (1.0f - g * g) / (4.0f * PI * denom * std::sqrt(denom));
}

Vec3 HenyeyGreensteinPhase::sample(const Vec3& wo, float u1, float u2) const {
    float cosTheta;
    if (std::abs(g) < 1e-3f) {
        // Nearly isotropic
        cosTheta = 1.0f - 2.0f * u1;
    } else {
        float sqrTerm = (1.0f - g * g) / (1.0f + g - 2.0f * g * u1);
        cosTheta = -(1.0f + g * g - sqrTerm * sqrTerm) / (2.0f * g);
    }

    float sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));
    float phi = TWO_PI * u2;

    // Create local coordinate system around wo
    Vec3 tangent, bitangent;
    coordinateSystem(wo, tangent, bitangent);

    return tangent * (sinTheta * std::cos(phi)) +
           bitangent * (sinTheta * std::sin(phi)) +
           wo * cosTheta;
}

float HenyeyGreensteinPhase::pdf(const Vec3& wo, const Vec3& wi) const {
    return evaluate(wo, wi);
}

} // namespace render

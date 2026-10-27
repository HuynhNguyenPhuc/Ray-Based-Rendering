#include "render/materials/bsdf.h"

namespace render {

bool BSDF::sameHemisphere(const Vec3& wo, const Vec3& wi) {
    return wo.z * wi.z > 0.0f;
}

float BSDF::sinTheta(const Vec3& w) { 
    return std::sqrt(std::max(0.0f, 1.0f - cos2Theta(w))); 
}

float BSDF::sin2Theta(const Vec3& w) { 
    return std::max(0.0f, 1.0f - cos2Theta(w)); 
}

float BSDF::cosPhi(const Vec3& w) {
    float st = sinTheta(w);
    return (st == 0.0f) ? 1.0f : clamp(w.x / st, -1.0f, 1.0f);
}

float BSDF::sinPhi(const Vec3& w) {
    float st = sinTheta(w);
    return (st == 0.0f) ? 0.0f : clamp(w.y / st, -1.0f, 1.0f);
}

} // namespace render

// Fresnel reflectance for dielectrics and conductors

#pragma once

#include "../core/common.h"
#include "../core/vector.h"

namespace render {

/**
 * Unpolarised Fresnel reflectance of a dielectric interface.
 * @param cosThetaI Cosine between the incident direction and the normal. A negative value means
 *                  the ray arrives from the etaT side, so the two indices are swapped.
 * @return Reflected fraction in [0, 1]; 1 under total internal reflection.
 */
inline float fresnelDielectric(float cosThetaI, float etaI, float etaT) {
    cosThetaI = clamp(cosThetaI, -1.0f, 1.0f);
    if (cosThetaI < 0.0f) {
        std::swap(etaI, etaT);
        cosThetaI = -cosThetaI;
    }

    const float sinThetaI = std::sqrt(std::max(0.0f, 1.0f - cosThetaI * cosThetaI));
    const float sinThetaT = etaI / etaT * sinThetaI;
    if (sinThetaT >= 1.0f) return 1.0f;

    const float cosThetaT = std::sqrt(std::max(0.0f, 1.0f - sinThetaT * sinThetaT));
    const float rParl = (etaT * cosThetaI - etaI * cosThetaT) / (etaT * cosThetaI + etaI * cosThetaT);
    const float rPerp = (etaI * cosThetaI - etaT * cosThetaT) / (etaI * cosThetaI + etaT * cosThetaT);
    return 0.5f * (rParl * rParl + rPerp * rPerp);
}

/** Reflectance at normal incidence of a dielectric/air interface with index `ior`. */
inline float dielectricF0(float ior) {
    const float r = (ior - 1.0f) / (ior + 1.0f);
    return r * r;
}

/** Schlick's approximation, given the reflectance `f0` at normal incidence. */
inline Color fresnelSchlick(float cosTheta, const Color& f0) {
    const float m = 1.0f - clamp(std::abs(cosTheta), 0.0f, 1.0f);
    const float m2 = m * m;
    return f0 + (Color(1.0f) - f0) * (m2 * m2 * m);
}

/**
 * @struct Fresnel
 * @brief Value-type selection of the Fresnel term used by a reflection lobe.
 */
struct Fresnel {
    enum class Kind { None, Dielectric, Schlick };

    Kind kind = Kind::None;
    float etaI = 1.0f, etaT = 1.0f;  // Dielectric: index outside / inside
    Color f0{1.0f};                  // Schlick: reflectance at normal incidence

    static Fresnel none() { return Fresnel(); }
    static Fresnel dielectric(float etaI, float etaT) {
        Fresnel f;
        f.kind = Kind::Dielectric;
        f.etaI = etaI;
        f.etaT = etaT;
        return f;
    }
    static Fresnel schlick(const Color& f0) {
        Fresnel f;
        f.kind = Kind::Schlick;
        f.f0 = f0;
        return f;
    }

    /** Reflectance for a direction with the given (signed) cosine to the normal. */
    Color operator()(float cosThetaI) const {
        switch (kind) {
            case Kind::Dielectric: return Color(fresnelDielectric(cosThetaI, etaI, etaT));
            case Kind::Schlick:    return fresnelSchlick(cosThetaI, f0);
            default:               return Color(1.0f);
        }
    }
};

} // namespace render

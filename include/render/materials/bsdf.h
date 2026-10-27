// BSDF interface and sample types

#pragma once

#include "../core/common.h"
#include "../core/vector.h"
#include "../core/interaction.h"
#include "../core/sampler.h"
#include <cstdint>

namespace render {

enum class BSDFType : uint32_t {
    None        = 0,
    Reflection  = 1 << 0,
    Transmission = 1 << 1,
    Diffuse     = 1 << 2,
    Glossy      = 1 << 3,
    Specular    = 1 << 4,
    
    DiffuseReflection = Diffuse | Reflection,
    GlossyReflection = Glossy | Reflection,
    SpecularReflection = Specular | Reflection,
    SpecularTransmission = Specular | Transmission,
    All = Diffuse | Glossy | Specular | Reflection | Transmission
};

inline BSDFType operator|(BSDFType a, BSDFType b) {
    return static_cast<BSDFType>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline BSDFType operator&(BSDFType a, BSDFType b) {
    return static_cast<BSDFType>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool hasType(BSDFType type, BSDFType query) {
    return (static_cast<uint32_t>(type) & static_cast<uint32_t>(query)) == static_cast<uint32_t>(query);
}

// BSDF sample result

//
// Convention shared by every BSDF: `f` is the plain BSDF value (no cosine). A sampled path
// is weighted by f * |cos(theta_i)| / pdf. A delta lobe returns R / |cos(theta_i)| (R is its
// reflectance/transmittance) and a pdf equal to the probability of having chosen the lobe,
// so the same formula applies without a special case.

struct BSDFSample {
    Vec3 wi;           // Incoming light direction (sampled)
    Color f;           // BSDF value (without the cosine term)
    float pdf;         // Probability (density) of this sample
    BSDFType type;     // Type of interaction sampled
    bool isDelta;      // True if this is a delta distribution (specular)

    BSDFSample() : wi(0.0f), f(0.0f), pdf(0.0f), type(BSDFType::None), isDelta(false) {}
};

/**
 * @class BSDF
 * @brief Bidirectional Scattering Distribution Function interface.
 * 
 * All vectors are in local coordinate system where normal = (0, 0, 1).
 */
class BSDF {
public:
    virtual ~BSDF() = default;

    /**
     * Evaluate the BSDF for given directions.
     * @param wo Outgoing direction (towards camera)
     * @param wi Incoming direction (towards light)
     * @return BSDF value (reflectance)
     */
    virtual Color evaluate(const Vec3& wo, const Vec3& wi) const = 0;

    /**
     * Sample an incoming direction given outgoing direction.
     * @param wo Outgoing direction
     * @param u1, u2 Random samples in [0, 1)
     * @return Sampled direction, BSDF value (no cosine), and PDF; pdf == 0 means no sample
     */
    virtual BSDFSample sample(const Vec3& wo, float u1, float u2) const = 0;

    /**
     * Compute PDF for a given direction pair.
     */
    virtual float pdf(const Vec3& wo, const Vec3& wi) const = 0;

    /**
     * Get the type of this BSDF.
     */
    virtual BSDFType type() const = 0;

    /**
     * Check if BSDF is a delta distribution (specular).
     */
    virtual bool isDelta() const { return false; }

    /**
     * Rough directional albedo for `wo`, used by composite BSDFs to pick a lobe.
     * Only needs to be positive where the lobe can scatter; it does not have to be exact.
     */
    virtual Color rho(const Vec3& /*wo*/) const { return Color(1.0f); }

protected:
    // Helper: Check if direction is above surface (positive z)
    static bool sameHemisphere(const Vec3& wo, const Vec3& wi);

    // Helper: Cosine of angle with normal
    static float cosTheta(const Vec3& w) { return w.z; }
    static float cos2Theta(const Vec3& w) { return w.z * w.z; }
    static float absCosTheta(const Vec3& w) { return std::abs(w.z); }
    
    static float sinTheta(const Vec3& w);
    static float sin2Theta(const Vec3& w);
    static float tanTheta(const Vec3& w) { return sinTheta(w) / cosTheta(w); }
    static float tan2Theta(const Vec3& w) { return sin2Theta(w) / cos2Theta(w); }

    static float cosPhi(const Vec3& w);
    static float sinPhi(const Vec3& w);
};

} // namespace render

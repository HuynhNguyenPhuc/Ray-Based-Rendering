// Sampling utilities for Monte Carlo rendering

#pragma once

#include "vector.h"
#include "common.h"
#include <cstdint>
#include <memory>
#include <random>
#include <utility>

namespace render {

/**
 * @class Sampler
 * @brief Base class for generating random samples.
 */
class Sampler {
public:
    virtual ~Sampler() = default;

    // Get 1D random sample in [0, 1)
    virtual float get1D() = 0;

    // Get 2D random sample in [0, 1) x [0, 1)
    virtual std::pair<float, float> get2D() = 0;

    // Set seed
    virtual void setSeed(uint64_t seed) = 0;
};

/**
 * @class UniformSampler
 * @brief Simple uniform random sampler.
 */
class UniformSampler : public Sampler {
public:
    explicit UniformSampler(uint64_t seed = 0);

    float get1D() override;
    std::pair<float, float> get2D() override;
    void setSeed(uint64_t seed) override;

private:
    std::mt19937_64 rng_;
    std::uniform_real_distribution<float> dist_;
};

// =============================================================================
// Sampling Functions
// =============================================================================

namespace sampling {

/**
 * Sample point uniformly on unit sphere.
 */
inline Vec3 uniformSphere(float u1, float u2) {
    float z = 1.0f - 2.0f * u1;
    float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
    float phi = TWO_PI * u2;
    return Vec3(r * std::cos(phi), r * std::sin(phi), z);
}

/**
 * Cosine-weighted hemisphere sampling (for Lambertian BRDF).
 */
inline Vec3 cosineHemisphere(float u1, float u2) {
    float r = std::sqrt(u1);
    float theta = TWO_PI * u2;
    float x = r * std::cos(theta);
    float y = r * std::sin(theta);
    float z = std::sqrt(std::max(0.0f, 1.0f - u1));
    return Vec3(x, y, z);
}

/**
 * Concentric disk sampling (better distribution).
 */
inline std::pair<float, float> concentricDisk(float u1, float u2) {
    // Map uniform random numbers to [-1, 1]
    float sx = 2.0f * u1 - 1.0f;
    float sy = 2.0f * u2 - 1.0f;

    if (sx == 0.0f && sy == 0.0f) {
        return {0.0f, 0.0f};
    }

    float r, theta;
    if (std::abs(sx) > std::abs(sy)) {
        r = sx;
        theta = (PI / 4.0f) * (sy / sx);
    } else {
        r = sy;
        theta = (PI / 2.0f) - (PI / 4.0f) * (sx / sy);
    }

    return {r * std::cos(theta), r * std::sin(theta)};
}

/**
 * Orthonormal frame around a normal (local z axis), built once and reused for conversions.
 */
struct Frame {
    Vec3 tangent, bitangent, normal;

    explicit Frame(const Vec3& n) : normal(n) {
        coordinateSystem(n, tangent, bitangent);
    }

    Vec3 toWorld(const Vec3& local) const {
        return tangent * local.x + bitangent * local.y + normal * local.z;
    }

    Vec3 toLocal(const Vec3& world) const {
        return Vec3(dot(world, tangent), dot(world, bitangent), dot(world, normal));
    }
};

/**
 * Transform local direction to world space given normal.
 */
inline Vec3 localToWorld(const Vec3& local, const Vec3& normal) {
    return Frame(normal).toWorld(local);
}

/**
 * Transform world direction to local space given normal.
 */
inline Vec3 worldToLocal(const Vec3& world, const Vec3& normal) {
    return Frame(normal).toLocal(world);
}

/**
 * Power heuristic for MIS (Multiple Importance Sampling).
 */
inline float powerHeuristic(int nf, float fPdf, int ng, float gPdf) {
    float f = nf * fPdf;
    float g = ng * gPdf;
    // Avoid inf/inf and 0/0 for extreme densities (e.g. delta-like lobes).
    if (std::isinf(f)) return 1.0f;
    if (std::isinf(g)) return 0.0f;
    // Scale by the larger density so that squaring cannot overflow.
    const float m = std::max(f, g);
    if (m <= 0.0f) return 0.0f;
    f /= m;
    g /= m;
    return (f * f) / (f * f + g * g);
}

} // namespace sampling

} // namespace render



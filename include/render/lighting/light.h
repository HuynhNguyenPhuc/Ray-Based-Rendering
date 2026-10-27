// Light source interface

#pragma once

#include "../core/common.h"
#include "../core/ray.h"
#include "../core/vector.h"
#include "../core/interaction.h"

namespace render {

/**
 * @enum LightType
 * @brief Types of light sources.
 */
enum class LightType {
    Point,
    Directional,
    Area,
    Environment
};

/**
 * @struct LightSample
 * @brief Result of sampling a light source.
 */
struct LightSample {
    Vec3 wi;           // Direction towards light
    Color Li;          // Incident radiance
    float pdf;         // Probability density
    float distance;    // Distance to light (INFINITY for directional)
    bool isDelta;      // True if light is a delta distribution

    LightSample() : wi(0.0f), Li(0.0f), pdf(0.0f), distance(0.0f), isDelta(false) {}
};

/**
 * @class Light
 * @brief Abstract base class for light sources.
 */
class Light {
public:
    LightType type;
    Color color;
    float intensity;

    Light(LightType type, const Color& color, float intensity)
        : type(type), color(color), intensity(intensity) {}

    virtual ~Light() = default;

    /**
     * Sample incoming radiance at a surface point.
     * @param isect Surface interaction point
     * @param u1, u2 Random samples
     * @return Light sample with direction, radiance, and PDF
     */
    virtual LightSample sample(const SurfaceInteraction& isect, float u1, float u2) const = 0;

    /**
     * Evaluate radiance for a given direction (for area lights).
     */
    virtual Color Le(const Ray& ray) const { return Color(0.0f); }

    /**
     * Total power emitted by the light.
     */
    virtual Color power() const = 0;

    /**
     * Check if light is a delta distribution (point, directional).
     */
    virtual bool isDelta() const = 0;
};

} // namespace render

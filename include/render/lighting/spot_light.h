// Spot light

#pragma once

#include "light.h"

namespace render {

/**
 * @class SpotLight
 * @brief Spotlight with cone falloff.
 */
class SpotLight : public Light {
public:
    Vec3 position;
    Vec3 direction;
    float cosTotalWidth;   // Cosine of outer cone angle
    float cosFalloffStart; // Cosine of inner cone angle

    SpotLight(const Vec3& position, const Vec3& direction,
              float totalWidth, float falloffStart,  // Angles in degrees
              const Color& color, float intensity)
        : Light(LightType::Point, color, intensity)
        , position(position)
        , direction(direction.normalized())
        , cosTotalWidth(std::cos(radians(totalWidth)))
        , cosFalloffStart(std::cos(radians(falloffStart))) {}

    LightSample sample(const SurfaceInteraction& isect, float u1, float u2) const override;

    Color power() const override;

    bool isDelta() const override { return true; }
};

} // namespace render

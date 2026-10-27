// Point light

#pragma once

#include "light.h"

namespace render {

/**
 * @class PointLight
 * @brief Point light source (delta distribution).
 */
class PointLight : public Light {
public:
    Vec3 position;

    PointLight(const Vec3& position, const Color& color, float intensity)
        : Light(LightType::Point, color, intensity), position(position) {}

    LightSample sample(const SurfaceInteraction& isect, float u1, float u2) const override;

    Color power() const override;

    bool isDelta() const override { return true; }
};

} // namespace render

// Directional light

#pragma once

#include "light.h"

namespace render {

/**
 * @class DirectionalLight
 * @brief Distant directional light (like the sun).
 */
class DirectionalLight : public Light {
public:
    Vec3 direction;  // Direction FROM light (towards scene)

    DirectionalLight(const Vec3& direction, const Color& color, float intensity)
        : Light(LightType::Directional, color, intensity)
        , direction(direction.normalized()) {}

    LightSample sample(const SurfaceInteraction& isect, float u1, float u2) const override;

    Color power() const override;

    bool isDelta() const override { return true; }
};

} // namespace render

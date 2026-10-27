// Area light

#pragma once

#include "light.h"

namespace render {

/**
 * @class AreaLight
 * @brief Area light source (for soft shadows).
 */
class AreaLight : public Light {
public:
    Vec3 position;    // Center position
    Vec3 u, v;        // Edge vectors (defines a quad)
    Vec3 normal;      // Normal direction

    AreaLight(const Vec3& position, const Vec3& u, const Vec3& v,
        const Color& color, float intensity);

    LightSample sample(const SurfaceInteraction& isect, float u1, float u2) const override;

    Color Le(const Ray& ray) const override;

    Color power() const override;

    bool isDelta() const override { return false; }

    float area() const;
};

} // namespace render

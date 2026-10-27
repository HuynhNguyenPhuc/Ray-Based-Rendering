// Sphere primitive

#pragma once

#include "primitive.h"

namespace render {

/**
 * @class Sphere
 * @brief Sphere primitive.
 */
class Sphere : public Primitive {
public:
    Vec3 center;
    float radius;

    Sphere(const Vec3& center, float radius, const Material* mat = nullptr)
        : Primitive(mat), center(center), radius(radius) {}

    bool intersect(const Ray& ray, SurfaceInteraction& isect) const override;

    BoundingBox getBounds() const override;

    bool overlapsBox(const BoundingBox& box) const override;

    float area() const override;

    SurfaceInteraction sample(float u1, float u2, float& pdf) const override;

    // Volume intersection (for volume rendering)
    bool intersectVolume(const Ray& ray, float& t0, float& t1) const;
};

} // namespace render

// Infinite plane primitive

#pragma once

#include "primitive.h"

namespace render {

/**
 * @class Plane
 * @brief Infinite plane primitive.
 */
class Plane : public Primitive {
public:
    Vec3 normal;
    float d;  // Distance from origin (plane equation: n·p + d = 0)

    Plane(const Vec3& normal, float d, const Material* mat = nullptr)
        : Primitive(mat), normal(normal.normalized()), d(d) {}

    Plane(const Vec3& point, const Vec3& normal, const Material* mat = nullptr);

    bool intersect(const Ray& ray, SurfaceInteraction& isect) const override;

    BoundingBox getBounds() const override;
};

} // namespace render

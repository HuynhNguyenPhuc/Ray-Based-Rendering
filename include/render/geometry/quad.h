// Quad primitive

#pragma once

#include "primitive.h"

namespace render {

/**
 * @class Quad
 * @brief Quadrilateral (rectangle) primitive.
 */
class Quad : public Primitive {
public:
    Vec3 origin;    // Corner point
    Vec3 u, v;      // Edge vectors
    Vec3 normal;

    Quad(const Vec3& origin, const Vec3& u, const Vec3& v, const Material* mat = nullptr);

    bool intersect(const Ray& ray, SurfaceInteraction& isect) const override;

    BoundingBox getBounds() const override;

    float area() const override;
};

} // namespace render

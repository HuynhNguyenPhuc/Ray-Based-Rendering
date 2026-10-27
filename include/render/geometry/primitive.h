// Geometric primitive interface

#pragma once

#include "../core/vector.h"
#include "../core/ray.h"
#include "../core/bbox.h"
#include "../core/interaction.h"
#include "../materials/material.h"

namespace render {

/**
 * @class Primitive
 * @brief Abstract base class for all geometric primitives.
 */
class Primitive {
public:
    const Material* material = nullptr;

    Primitive() = default;
    explicit Primitive(const Material* mat) : material(mat) {}
    virtual ~Primitive() = default;

    /**
     * Test ray intersection.
     * @param ray The ray to test
     * @param[out] isect Surface interaction data (if hit)
     * @return True if ray intersects the primitive
     */
    virtual bool intersect(const Ray& ray, SurfaceInteraction& isect) const = 0;

    /**
     * Quick intersection test (shadow rays).
     */
    virtual bool intersectP(const Ray& ray) const;

    /**
     * Get axis-aligned bounding box.
     */
    virtual BoundingBox getBounds() const = 0;

    /**
     * Exact-ish test whether the primitive touches an axis-aligned box. Spatial
     * subdivision uses it to avoid listing a primitive in cells that only its
     * bounding box reaches. The default (bounding box only) is conservative.
     */
    virtual bool overlapsBox(const BoundingBox& box) const;

    /**
     * Get surface area (for importance sampling).
     */
    virtual float area() const { return 0.0f; }

    /**
     * Sample a point on the surface.
     */
    virtual SurfaceInteraction sample(float u1, float u2, float& pdf) const;
};

} // namespace render

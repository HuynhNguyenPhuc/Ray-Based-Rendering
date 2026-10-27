// Interface shared by all spatial acceleration structures

#pragma once

#include "../core/bbox.h"
#include "../core/interaction.h"
#include "../core/ray.h"
#include "../geometry/primitive.h"
#include <vector>

namespace render {

/**
 * @class Accelerator
 * @brief Spatial index over primitives that speeds up ray queries.
 *
 * Primitives are not owned; they must outlive the accelerator.
 */
class Accelerator {
public:
    virtual ~Accelerator() = default;

    // (Re)build the structure from the given primitives.
    virtual void build(std::vector<Primitive*> primitives) = 0;

    // Closest-hit query. Updates ray.tMax and fills `isect` on a hit.
    virtual bool intersect(const Ray& ray, SurfaceInteraction& isect) const = 0;

    // Any-hit query (shadow rays).
    virtual bool intersectP(const Ray& ray) const = 0;

    // Bounds of everything in the structure (empty box if nothing was built).
    virtual BoundingBox worldBounds() const = 0;
};

} // namespace render

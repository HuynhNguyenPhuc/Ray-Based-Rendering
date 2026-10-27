#include "render/geometry/primitive.h"

namespace render {

bool Primitive::intersectP(const Ray& ray) const {
    SurfaceInteraction isect;
    return intersect(ray, isect);
}

bool Primitive::overlapsBox(const BoundingBox& box) const {
    const BoundingBox b = getBounds();
    return b.pMax.x >= box.pMin.x && b.pMin.x <= box.pMax.x &&
           b.pMax.y >= box.pMin.y && b.pMin.y <= box.pMax.y &&
           b.pMax.z >= box.pMin.z && b.pMin.z <= box.pMax.z;
}

SurfaceInteraction Primitive::sample(float u1, float u2, float& pdf) const {
    pdf = 0.0f;
    return SurfaceInteraction();
}

} // namespace render

#include "render/geometry/plane.h"
#include "render/core/sampler.h"

namespace render {

Plane::Plane(const Vec3& point, const Vec3& normal, const Material* mat)
    : Primitive(mat), normal(normal.normalized())
{
    d = -dot(this->normal, point);
}

bool Plane::intersect(const Ray& ray, SurfaceInteraction& isect) const {
    float denom = dot(normal, ray.direction);
    if (std::abs(denom) < EPSILON) return false;

    float t = -(dot(normal, ray.origin) + d) / denom;
    if (t < EPSILON || t > ray.tMax) return false;

    Vec3 hitPoint = ray.at(t);
    
    isect.point = hitPoint;
    isect.t = t;
    isect.wo = -ray.direction;
    isect.primitive = this;
    isect.material = material;
    isect.setFaceNormal(ray, normal);

    // Simple planar UV mapping
    Vec3 tangent, bitangent;
    coordinateSystem(normal, tangent, bitangent);
    isect.uv = Vec2(dot(hitPoint, tangent), dot(hitPoint, bitangent));

    ray.tMax = t;
    return true;
}

BoundingBox Plane::getBounds() const {
    // Infinite bounds for infinite plane
    return BoundingBox(Vec3(-MAX_FLOAT), Vec3(MAX_FLOAT));
}

} // namespace render

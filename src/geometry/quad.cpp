#include "render/geometry/quad.h"
#include "render/core/sampler.h"

namespace render {

Quad::Quad(const Vec3& origin, const Vec3& u, const Vec3& v, const Material* mat)
    : Primitive(mat), origin(origin), u(u), v(v)
{
    normal = cross(u, v).normalized();
}

bool Quad::intersect(const Ray& ray, SurfaceInteraction& isect) const {
    float denom = dot(normal, ray.direction);
    if (std::abs(denom) < EPSILON) return false;

    float t = dot(origin - ray.origin, normal) / denom;
    if (t < EPSILON || t > ray.tMax) return false;

    Vec3 hitPoint = ray.at(t);
    Vec3 localP = hitPoint - origin;

    // Project onto quad
    float uLen2 = dot(u, u);
    float vLen2 = dot(v, v);
    float uCoord = dot(localP, u) / uLen2;
    float vCoord = dot(localP, v) / vLen2;

    if (uCoord < 0.0f || uCoord > 1.0f || vCoord < 0.0f || vCoord > 1.0f) {
        return false;
    }

    isect.point = hitPoint;
    isect.t = t;
    isect.wo = -ray.direction;
    isect.primitive = this;
    isect.material = material;
    isect.setFaceNormal(ray, normal);
    isect.uv = Vec2(uCoord, vCoord);

    ray.tMax = t;
    return true;
}

BoundingBox Quad::getBounds() const {
    BoundingBox box;
    box.expand(origin);
    box.expand(origin + u);
    box.expand(origin + v);
    box.expand(origin + u + v);
    return box;
}

float Quad::area() const {
    return cross(u, v).length();
}

} // namespace render

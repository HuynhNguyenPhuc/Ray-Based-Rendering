#include "render/geometry/sphere.h"
#include "render/core/sampler.h"

namespace render {

bool Sphere::intersect(const Ray& ray, SurfaceInteraction& isect) const {
    Vec3 oc = ray.origin - center;
    float a = dot(ray.direction, ray.direction);
    float halfB = dot(oc, ray.direction);
    float c = dot(oc, oc) - radius * radius;
    float discriminant = halfB * halfB - a * c;

    if (discriminant < 0.0f) return false;

    float sqrtD = std::sqrt(discriminant);
    float t = (-halfB - sqrtD) / a;
    
    if (t < EPSILON || t > ray.tMax) {
        t = (-halfB + sqrtD) / a;
        if (t < EPSILON || t > ray.tMax) {
            return false;
        }
    }

    // Fill in surface interaction
    Vec3 hitPoint = ray.at(t);
    Vec3 normal = (hitPoint - center) / radius;
    
    isect.point = hitPoint;
    isect.t = t;
    isect.wo = -ray.direction;
    isect.primitive = this;
    isect.material = material;
    isect.setFaceNormal(ray, normal);

    // Compute UV coordinates (spherical)
    float theta = std::acos(-normal.y);
    float phi = std::atan2(-normal.z, normal.x) + PI;
    isect.uv = Vec2(phi * INV_TWO_PI, theta * INV_PI);

    ray.tMax = t;
    return true;
}

BoundingBox Sphere::getBounds() const {
    return BoundingBox(
        center - Vec3(radius),
        center + Vec3(radius)
    );
}

float Sphere::area() const {
    return 4.0f * PI * radius * radius;
}

SurfaceInteraction Sphere::sample(float u1, float u2, float& pdf) const {
    Vec3 dir = sampling::uniformSphere(u1, u2);
    Vec3 point = center + dir * radius;

    SurfaceInteraction isect;
    isect.point = point;
    isect.normal = dir;
    isect.geometricNormal = dir;
    isect.primitive = this;
    isect.material = material;

    pdf = 1.0f / area();
    return isect;
}

bool Sphere::intersectVolume(const Ray& ray, float& t0, float& t1) const {
    Vec3 oc = ray.origin - center;
    float a = dot(ray.direction, ray.direction);
    float halfB = dot(oc, ray.direction);
    float c = dot(oc, oc) - radius * radius;
    float discriminant = halfB * halfB - a * c;

    if (discriminant < 0.0f) return false;

    float sqrtD = std::sqrt(discriminant);
    t0 = (-halfB - sqrtD) / a;
    t1 = (-halfB + sqrtD) / a;

    if (t0 > t1) std::swap(t0, t1);
    if (t1 < 0.0f) return false;
    if (t0 < 0.0f) t0 = 0.0f;

    return true;
}

} // namespace render

namespace render {

bool Sphere::overlapsBox(const BoundingBox& box) const {
    float d2 = 0.0f;
    for (int a = 0; a < 3; ++a) {
        const float c = center[a];
        if (c < box.pMin[a]) d2 += (box.pMin[a] - c) * (box.pMin[a] - c);
        else if (c > box.pMax[a]) d2 += (c - box.pMax[a]) * (c - box.pMax[a]);
    }
    return d2 <= radius * radius;
}

} // namespace render

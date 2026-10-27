#include "render/geometry/triangle.h"
#include "render/core/sampler.h"

namespace render {

bool Triangle::intersect(const Ray& ray, SurfaceInteraction& isect) const {
    Vec3 edge1 = v1 - v0;
    Vec3 edge2 = v2 - v0;
    Vec3 h = cross(ray.direction, edge2);
    float a = dot(edge1, h);

    if (std::abs(a) < EPSILON) return false;

    float f = 1.0f / a;
    Vec3 s = ray.origin - v0;
    float u = f * dot(s, h);
    if (u < 0.0f || u > 1.0f) return false;

    Vec3 q = cross(s, edge1);
    float v = f * dot(ray.direction, q);
    if (v < 0.0f || u + v > 1.0f) return false;

    float t = f * dot(edge2, q);
    if (t < EPSILON || t > ray.tMax) return false;

    // Compute barycentric coordinates
    float w = 1.0f - u - v;

    // Fill in surface interaction
    isect.point = ray.at(t);
    isect.t = t;
    isect.wo = -ray.direction;
    isect.primitive = this;
    isect.material = material;

    // Compute geometric normal
    Vec3 geoNormal = cross(edge1, edge2).normalized();
    isect.geometricNormal = geoNormal;

    // Shading normal (interpolated if available)
    if (hasNormals) {
        isect.normal = (n0 * w + n1 * u + n2 * v).normalized();
        // Ensure shading normal is in same hemisphere as geometric normal
        if (dot(isect.normal, geoNormal) < 0.0f) {
            isect.normal = -isect.normal;
        }
    } else {
        isect.normal = geoNormal;
    }

    isect.setFaceNormal(ray, isect.normal);

    // Texture coordinates
    if (hasUVs) {
        isect.uv = Vec2(
            uv0.u * w + uv1.u * u + uv2.u * v,
            uv0.v * w + uv1.v * u + uv2.v * v
        );
    } else {
        isect.uv = Vec2(u, v);
    }

    ray.tMax = t;
    return true;
}

BoundingBox Triangle::getBounds() const {
    return BoundingBox(
        Vec3::min(Vec3::min(v0, v1), v2),
        Vec3::max(Vec3::max(v0, v1), v2)
    );
}

Vec3 Triangle::faceNormal() const {
    return cross(v1 - v0, v2 - v0).normalized();
}

float Triangle::area() const {
    return 0.5f * cross(v1 - v0, v2 - v0).length();
}

SurfaceInteraction Triangle::sample(float u1, float u2, float& pdf) const {
    // Uniform sampling on triangle
    float su0 = std::sqrt(u1);
    float b0 = 1.0f - su0;
    float b1 = u2 * su0;
    float b2 = 1.0f - b0 - b1;

    SurfaceInteraction isect;
    isect.point = v0 * b0 + v1 * b1 + v2 * b2;
    isect.geometricNormal = faceNormal();
    
    if (hasNormals) {
        isect.normal = (n0 * b0 + n1 * b1 + n2 * b2).normalized();
    } else {
        isect.normal = isect.geometricNormal;
    }

    if (hasUVs) {
        isect.uv = Vec2(
            uv0.u * b0 + uv1.u * b1 + uv2.u * b2,
            uv0.v * b0 + uv1.v * b1 + uv2.v * b2
        );
    }

    isect.primitive = this;
    isect.material = material;
    
    pdf = 1.0f / area();
    return isect;
}

} // namespace render

namespace render {

bool Triangle::overlapsBox(const BoundingBox& box) const {
    // Work in the box's frame; a tiny margin keeps touching cases conservative.
    const Vec3 c = box.center();
    const Vec3 e = box.diagonal() * 0.5f + Vec3(1e-5f) * (1.0f + std::fabs(c.x) + std::fabs(c.y) + std::fabs(c.z));
    const Vec3 v[3] = {v0 - c, v1 - c, v2 - c};
    const Vec3 edge[3] = {v[1] - v[0], v[2] - v[1], v[0] - v[2]};

    // 1) box face normals (the triangle's bounding box against the box)
    for (int a = 0; a < 3; ++a) {
        const float lo = std::min({v[0][a], v[1][a], v[2][a]});
        const float hi = std::max({v[0][a], v[1][a], v[2][a]});
        if (lo > e[a] || hi < -e[a]) return false;
    }

    // 2) triangle plane
    const Vec3 n = cross(edge[0], edge[1]);
    const float r = e.x * std::fabs(n.x) + e.y * std::fabs(n.y) + e.z * std::fabs(n.z);
    if (std::fabs(dot(n, v[0])) > r) return false;

    // 3) the nine edge x axis cross products
    for (int i = 0; i < 3; ++i) {
        for (int a = 0; a < 3; ++a) {
            Vec3 axis(0.0f);
            axis[a] = 1.0f;
            const Vec3 ax = cross(edge[i], axis);
            const float p0 = dot(ax, v[0]), p1 = dot(ax, v[1]), p2 = dot(ax, v[2]);
            const float rad = e.x * std::fabs(ax.x) + e.y * std::fabs(ax.y) + e.z * std::fabs(ax.z);
            if (std::min({p0, p1, p2}) > rad || std::max({p0, p1, p2}) < -rad) return false;
        }
    }
    return true;
}

} // namespace render

// Helpers shared by the spatial accelerators (private to src/acceleration)

#pragma once

#include "render/core/bbox.h"
#include "render/core/ray.h"
#include "render/geometry/primitive.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace render {
namespace accel {

// Coordinates beyond this are treated as "unbounded": infinite primitives report
// their extent as +-FLT_MAX (e.g. Plane::getBounds()), which is finite but unusable.
constexpr float kUnboundedLimit = 1e18f;

inline bool isFinite(const Vec3& v) {
    return std::fabs(v.x) < kUnboundedLimit && std::fabs(v.y) < kUnboundedLimit &&
           std::fabs(v.z) < kUnboundedLimit;
}

inline bool isFinite(const BoundingBox& b) {
    return b.isValid() && isFinite(b.pMin) && isFinite(b.pMax);
}

/**
 * Split primitives into those with finite bounds (stored in the spatial structure)
 * and unbounded ones such as infinite planes (tested linearly on every query).
 * `bounds[i]` belongs to `bounded[i]`; `world` covers the bounded primitives only.
 */
inline void splitBounded(const std::vector<Primitive*>& prims, std::vector<Primitive*>& bounded,
                         std::vector<BoundingBox>& bounds, std::vector<Primitive*>& unbounded,
                         BoundingBox& world) {
    bounded.clear();
    bounds.clear();
    unbounded.clear();
    world = BoundingBox();
    for (Primitive* p : prims) {
        BoundingBox b = p->getBounds();
        if (isFinite(b)) {
            bounded.push_back(p);
            bounds.push_back(b);
            world.expand(b);
        } else {
            unbounded.push_back(p);
        }
    }
}

inline bool intersectAll(const std::vector<Primitive*>& prims, const Ray& ray,
                         SurfaceInteraction& isect) {
    bool hit = false;
    for (const Primitive* p : prims) {
        if (p->intersect(ray, isect)) hit = true;
    }
    return hit;
}

inline bool intersectAnyP(const std::vector<Primitive*>& prims, const Ray& ray) {
    for (const Primitive* p : prims) {
        if (p->intersectP(ray)) return true;
    }
    return false;
}

// Grow a box by a small relative margin so rays grazing the boundary are not lost.
inline BoundingBox padded(const BoundingBox& b) {
    Vec3 d = b.diagonal();
    float m = std::max({d.x, d.y, d.z}) * 1e-4f + 1e-6f;
    return BoundingBox(b.pMin - Vec3(m), b.pMax + Vec3(m));
}

/**
 * Robust ray/box slab test over [0, ray.tMax]. Handles zero direction components
 * (no NaN from 0 * inf). Returns false if the ray misses.
 */
inline bool clipRay(const BoundingBox& box, const Ray& ray, const Vec3& invDir,
                    float& tNear, float& tFar) {
    float t0 = 0.0f;
    float t1 = ray.tMax;
    for (int a = 0; a < 3; ++a) {
        if (ray.direction[a] == 0.0f) {
            if (ray.origin[a] < box.pMin[a] || ray.origin[a] > box.pMax[a]) return false;
            continue;
        }
        float tn = (box.pMin[a] - ray.origin[a]) * invDir[a];
        float tf = (box.pMax[a] - ray.origin[a]) * invDir[a];
        if (tn > tf) std::swap(tn, tf);
        t0 = std::max(t0, tn);
        t1 = std::min(t1, tf);
        if (t0 > t1) return false;
    }
    tNear = t0;
    tFar = t1;
    return true;
}

inline Vec3 inverse(const Vec3& d) {
    const float inf = std::numeric_limits<float>::infinity();
    return Vec3(d.x != 0.0f ? 1.0f / d.x : inf,
                d.y != 0.0f ? 1.0f / d.y : inf,
                d.z != 0.0f ? 1.0f / d.z : inf);
}

} // namespace accel
} // namespace render

// Axis-aligned bounding box

#pragma once

#include "vector.h"
#include "ray.h"
#include "common.h"

namespace render {

    /**
     * @struct BoundingBox
     * @brief Axis-aligned bounding box class.
     */
    class BoundingBox {
    public:
        Vec3 pMin;
        Vec3 pMax;

        // Default constructor creates an empty (invalid) box
        BoundingBox() 
            : pMin(MAX_FLOAT), pMax(-MAX_FLOAT) {}

        // Single point box
        explicit BoundingBox(const Vec3& p) 
            : pMin(p), pMax(p) {}

        // Two corner box
        BoundingBox(const Vec3& p1, const Vec3& p2)
            : pMin(Vec3::min(p1, p2)), pMax(Vec3::max(p1, p2)) {}

        // Access corners
        const Vec3& operator[](int i) const {
            return (i == 0) ? pMin : pMax;
        }

        Vec3& operator[](int i) {
            return (i == 0) ? pMin : pMax;
        }

        // Properties
        Vec3 center() const {
            return (pMin + pMax) * 0.5f;
        }

        Vec3 diagonal() const {
            return pMax - pMin;
        }

        float surfaceArea() const {
            Vec3 d = diagonal();
            return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
        }

        int maximumExtent() const {
            Vec3 d = diagonal();
            if (d.x > d.y && d.x > d.z) return 0;
            if (d.y > d.z) return 1;
            return 2;
        }

        // Check if empty/valid
        bool isEmpty() const {
            return pMin.x > pMax.x || pMin.y > pMax.y || pMin.z > pMax.z;
        }

        bool isValid() const {
            return !isEmpty();
        }

        // Point containment
        bool contains(const Vec3& p) const {
            return p.x >= pMin.x && p.x <= pMax.x &&
                p.y >= pMin.y && p.y <= pMax.y &&
                p.z >= pMin.z && p.z <= pMax.z;
        }

        // Expand to include point
        void expand(const Vec3& p) {
            pMin = Vec3::min(pMin, p);
            pMax = Vec3::max(pMax, p);
        }

        // Expand to include another box
        void expand(const BoundingBox& box) {
            pMin = Vec3::min(pMin, box.pMin);
            pMax = Vec3::max(pMax, box.pMax);
        }

        // Slab test against [0, ray.tMax]; returns the entry/exit parameters.
        // Rays parallel to a slab are handled explicitly (no 0 * inf NaNs).
        bool intersect(const Ray& ray, float& tMin, float& tMax) const {
            float t0 = 0.0f;
            float t1 = ray.tMax;

            for (int i = 0; i < 3; ++i) {
                const float dir = ray.direction[i];
                if (dir == 0.0f) {
                    if (ray.origin[i] < pMin[i] || ray.origin[i] > pMax[i]) return false;
                    continue;
                }

                const float invRayDir = 1.0f / dir;
                float tNear = (pMin[i] - ray.origin[i]) * invRayDir;
                float tFar = (pMax[i] - ray.origin[i]) * invRayDir;
                if (tNear > tFar) std::swap(tNear, tFar);

                // Update tFar for robustness
                tFar *= 1.0f + 2.0f * std::numeric_limits<float>::epsilon();

                t0 = std::max(t0, tNear);
                t1 = std::min(t1, tFar);

                if (t0 > t1) return false;
            }

            tMin = t0;
            tMax = t1;
            return true;
        }
    };

}



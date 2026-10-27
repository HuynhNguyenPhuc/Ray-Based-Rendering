// Ray class for ray tracing

#pragma once

#include "vector.h"
#include "common.h"

namespace render {

class Ray {
public:
    Vec3 origin;
    Vec3 direction;
    mutable float tMax;  // Primitives shrink it as they find closer hits

    Ray() 
        : origin(0.0f), direction(0.0f, 0.0f, -1.0f), tMax(INFINITY_F) {}

    Ray(const Vec3& origin, const Vec3& direction)
        : origin(origin), direction(direction.normalized()), tMax(INFINITY_F) {}

    Ray(const Vec3& origin, const Vec3& direction, float tMax)
        : origin(origin), direction(direction.normalized()), tMax(tMax) {}

    // Get point at parameter t
    Vec3 at(float t) const {
        return origin + direction * t;
    }

    // Alias for at()
    Vec3 operator()(float t) const {
        return at(t);
    }

};

} // namespace render



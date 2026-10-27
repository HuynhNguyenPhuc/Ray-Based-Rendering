// Surface and medium interaction structures

#pragma once

#include "vector.h"
#include "ray.h"
#include "common.h"

namespace render {

// Forward declarations
class Primitive;
class Material;

/**
 * @struct Interaction
 * @brief Base struct for interaction at a point.
 */
struct Interaction {
    Vec3 point;        // Intersection point
    Vec3 normal;       // Surface normal (shading)
    Vec3 wo;           // Outgoing direction (towards camera)
    float t = 0.0f;    // Ray parameter at intersection
    bool frontFace = true;  // Did we hit front face?

    Interaction() = default;

    Interaction(const Vec3& p, const Vec3& n, const Vec3& wo, float t)
        : point(p), normal(n), wo(wo), t(t), frontFace(true) {}

    // Spawn ray from this interaction point
    Ray spawnRay(const Vec3& direction) const {
        Vec3 offset = normal * EPSILON;
        if (dot(direction, normal) < 0.0f) {
            offset = -offset;
        }
        return Ray(point + offset, direction);
    }

    // Spawn ray towards a target point
    Ray spawnRayTo(const Vec3& target) const {
        Vec3 direction = target - point;
        float dist = direction.length();
        direction = direction / dist;
        
        Vec3 offset = normal * EPSILON;
        if (dot(direction, normal) < 0.0f) {
            offset = -offset;
        }
        return Ray(point + offset, direction, dist - 2.0f * EPSILON);
    }
};

/**
 * @struct SurfaceInteraction
 * @brief Complete information about a ray-surface intersection.
 */
struct SurfaceInteraction : public Interaction {
    Vec3 geometricNormal;  // True geometric normal
    Vec2 uv;               // Texture coordinates
    
    const Primitive* primitive = nullptr;  // Hit primitive
    const Material* material = nullptr;    // Surface material

    SurfaceInteraction() = default;

    SurfaceInteraction(const Vec3& p, const Vec3& n, const Vec3& gn, 
                       const Vec3& wo, float t, const Vec2& uv)
        : Interaction(p, n, wo, t), geometricNormal(gn), uv(uv) {}

    // Surface normal pointing out of the object, regardless of which side the ray hit.
    // BSDFs are defined in a frame built from this normal: the sign of wo.z then tells whether the
    // ray is outside (> 0) or inside (< 0) the surface, which refraction needs.
    Vec3 outwardNormal() const {
        return frontFace ? normal : -normal;
    }

    // Set face orientation based on ray direction
    void setFaceNormal(const Ray& ray, const Vec3& outward) {
        frontFace = dot(ray.direction, outward) < 0.0f;
        normal = frontFace ? outward : -outward;
        geometricNormal = normal;
    }
};

} // namespace render



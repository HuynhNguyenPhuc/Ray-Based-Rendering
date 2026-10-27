#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

namespace render {
    // ****************************************************************
    // Namespace for the rendering engine
    //
    // NOTE: 
    // - All core components are encapsulated within this namespace.
    // - To use, prefix with 'render::' or use 'using namespace render;'
    // ****************************************************************

    /**** Constants ****/
    // NOTE: 
    // - Use constexpr for compile-time constants.
    // - Use const for runtime constants.
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = 2.0f * PI;
    constexpr float INV_PI = 1.0f / PI;
    constexpr float INV_TWO_PI = 1.0f / TWO_PI;
    constexpr float EPSILON = 1e-4f;
    constexpr float INFINITY_F = std::numeric_limits<float>::infinity();
    constexpr float MAX_FLOAT = std::numeric_limits<float>::max();

    /**** Forward Declarations ****/
    // NOTE: Forward declare classes to reduce compilation dependencies
    // Math
    class Vec3;
    class Vec2;
    class Ray;

    // Core
    class BSDF;
    class Material;
    class Sampler;

    // Geometry
    class Primitive;
    class Sphere;
    class Plane;
    class Triangle;
    class BoundingBox;

    // Scene
    class Scene;
    class Light;
    class PointLight;
    class AreaLight;
    class Camera;

    // Acceleration
    class Accelerator;
    class BVH;

    // Integrators
    class Integrator;
    class WhittedIntegrator;
    class PathTracingIntegrator;
    class VolumeIntegrator;

    // Rendering
    class Film;

    // Configuration
    struct RenderConfig;
    struct CameraConfig;

    /**** Utility Functions ****/
    // NOTE: Use inline functions for small utilities and avoid multiple definitions
    inline float clamp(float value, float min_val, float max_val) {
        return std::max(min_val, std::min(max_val, value));
    }

    inline float radians(float degrees) {
        return degrees * PI / 180.0f;
    }

    inline float degrees(float radians) {
        return radians * 180.0f / PI;
    }

}


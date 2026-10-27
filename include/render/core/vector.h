#pragma once

#include <cmath>
#include <cassert>
#include <algorithm>
#include <iostream>

namespace render {

    /**
     * @class Vec2
     * @brief 2D vector class
     */
    class Vec2 {
        public:
            float u, v;

            // Constructors
            Vec2(): u(0.0f), v(0.0f) {}
            Vec2(float val): u(val), v(val) {}
            Vec2(float u, float v): u(u), v(v) {}

            // Accessors
            /**
             * @brief Read-only accessor for vector components
             * 
             * @param i Index of the component (0 for u, 1 for v)
             * @return Value of the component
             * 
             * @note Usage:
             * ```cpp
             * Vec2 vec(1.0f, 2.0f);
             * float x = vec[0]; // x = 1.0f
             * float y = vec[1]; // y = 2.0f
             * ```
             */
            float operator[](int i) const {
                assert(i >= 0 && i < 2);
                return i == 0 ? u : v;
            }
            
            /**
             * @brief Write accessor for vector components
             * 
             * @param i Index of the component (0 for u, 1 for v)
             * @return float& Reference to the component
             * 
             * @note Usage:
             * ```cpp
             * Vec2 vec;
             * vec[0] = 1.0f; // sets u to 1.0f
             * vec[1] = 2.0f; // sets v to 2.0f
             * ```
             */
            float& operator[](int i) {
                assert(i >= 0 && i < 2);
                return i == 0 ? u : v;
            }

            // Arithmetic operators
            Vec2 operator+(const Vec2& other) const {
                return Vec2(u + other.u, v + other.v);
            }

            Vec2 operator-(const Vec2& other) const {
                return Vec2(u - other.u, v - other.v);
            }

            Vec2 operator*(float s) const {
                return Vec2(u * s, v * s);
            }

            Vec2 operator/(float s) const {
                float inv = 1.0f / s;
                return Vec2(u * inv, v * inv);
            }

            // Compound assignment
            Vec2& operator+=(const Vec2& other) {
                u += other.u; v += other.v;
                return *this;
            }

            Vec2& operator*=(float s) {
                u *= s; v *= s;
                return *this;
            }

            // Vector operations
            float lengthSquared() const {
                return u * u + v * v;
            }

            float length() const {
                return std::sqrt(lengthSquared());
            }

            Vec2 normalized() const {
                float len = length();
                return len > 0.0f ? *this / len : Vec2(0.0f);
            }

            float dot(const Vec2& other) const {
                return u * other.u + v * other.v;
            }
    };

    /**
     * @class Vec3
     * @brief 3D vector class
     */
    class Vec3 {
        public:
            float x, y, z;

            // Constructors
            Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
            Vec3(float v) : x(v), y(v), z(v) {}
            Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

            // Accessors
            float operator[](int i) const {
                assert(i >= 0 && i < 3);
                return i == 0 ? x : (i == 1 ? y : z);
            }
            
            float& operator[](int i) {
                assert(i >= 0 && i < 3);
                return i == 0 ? x : (i == 1 ? y : z);
            }

            // Arithmetic operators
            Vec3 operator-() const {
                return Vec3(-x, -y, -z);
            }

            Vec3 operator+(const Vec3& v) const {
                return Vec3(x + v.x, y + v.y, z + v.z);
            }

            Vec3 operator-(const Vec3& v) const {
                return Vec3(x - v.x, y - v.y, z - v.z);
            }

            Vec3 operator*(float s) const {
                return Vec3(x * s, y * s, z * s);
            }

            Vec3 operator*(const Vec3& v) const {
                return Vec3(x * v.x, y * v.y, z * v.z);
            }

            Vec3 operator/(float s) const {
                float inv = 1.0f / s;
                return Vec3(x * inv, y * inv, z * inv);
            }

            Vec3 operator/(const Vec3& v) const {
                return Vec3(x / v.x, y / v.y, z / v.z);
            }

            // Compound assignment
            Vec3& operator+=(const Vec3& v) {
                x += v.x; y += v.y; z += v.z;
                return *this;
            }

            Vec3& operator-=(const Vec3& v) {
                x -= v.x; y -= v.y; z -= v.z;
                return *this;
            }

            Vec3& operator*=(float s) {
                x *= s; y *= s; z *= s;
                return *this;
            }

            Vec3& operator*=(const Vec3& v) {
                x *= v.x; y *= v.y; z *= v.z;
                return *this;
            }

            Vec3& operator/=(float s) {
                float inv = 1.0f / s;
                x *= inv; y *= inv; z *= inv;
                return *this;
            }

            // Comparison
            bool operator==(const Vec3& v) const {
                return std::abs(x - v.x) < 1e-6f && 
                    std::abs(y - v.y) < 1e-6f && 
                    std::abs(z - v.z) < 1e-6f;
            }

            bool operator!=(const Vec3& v) const {
                return !(*this == v);
            }

            // Vector operations
            float lengthSquared() const {
                return x * x + y * y + z * z;
            }

            float length() const {
                return std::sqrt(lengthSquared());
            }

            Vec3 normalized() const {
                float len = length();
                if (len > 0.0f) {
                    return *this / len;
                }
                return Vec3(0.0f);
            }

            void normalize() {
                float len = length();
                if (len > 0.0f) {
                    *this /= len;
                }
            }

            float dot(const Vec3& v) const {
                return x * v.x + y * v.y + z * v.z;
            }

            Vec3 cross(const Vec3& v) const {
                return Vec3(
                    y * v.z - z * v.y,
                    z * v.x - x * v.z,
                    x * v.y - y * v.x
                );
            }

            // Color operations
            float luminance() const {
                return 0.2126f * x + 0.7152f * y + 0.0722f * z;
            }

            bool isBlack() const {
                return x <= 0.0f && y <= 0.0f && z <= 0.0f;
            }

            // Min/Max components
            float maxComponent() const {
                return std::max({x, y, z});
            }

            // Static factory methods
            static Vec3 min(const Vec3& a, const Vec3& b) {
                return Vec3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
            }

            static Vec3 max(const Vec3& a, const Vec3& b) {
                return Vec3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
            }
    };

    // Free functions
    inline Vec3 operator*(float s, const Vec3& v) {
        return v * s;
    }

    inline float dot(const Vec3& a, const Vec3& b) {
        return a.dot(b);
    }

    inline Vec3 cross(const Vec3& a, const Vec3& b) {
        return a.cross(b);
    }

    inline Vec3 normalize(const Vec3& v) {
        return v.normalized();
    }

    inline Vec3 reflect(const Vec3& incident, const Vec3& normal) {
        return incident - 2.0f * dot(incident, normal) * normal;
    }

    // Create orthonormal basis from normal
    inline void coordinateSystem(const Vec3& n, Vec3& tangent, Vec3& bitangent) {
        if (std::abs(n.x) > std::abs(n.y)) {
            tangent = Vec3(-n.z, 0.0f, n.x).normalized();
        } else {
            tangent = Vec3(0.0f, n.z, -n.y).normalized();
        }
        bitangent = cross(n, tangent);
    }

    // Output stream
    inline std::ostream& operator<<(std::ostream& os, const Vec3& v) {
        return os << "Vec3(" << v.x << ", " << v.y << ", " << v.z << ")";
    }

    // Type alias for color
    using Color = Vec3;

}



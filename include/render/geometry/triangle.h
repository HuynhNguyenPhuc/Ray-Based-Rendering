// Triangle primitive

#pragma once

#include "primitive.h"

namespace render {

/**
 * @class Triangle
 * @brief Triangle primitive with vertex normals and texture coordinates.
 */
class Triangle : public Primitive {
public:
    // Vertices
    Vec3 v0, v1, v2;
    // Vertex normals (optional, for smooth shading)
    Vec3 n0, n1, n2;
    // Texture coordinates (optional)
    Vec2 uv0, uv1, uv2;
    bool hasNormals = false;
    bool hasUVs = false;

    Triangle(const Vec3& v0, const Vec3& v1, const Vec3& v2, 
             const Material* mat = nullptr)
        : Primitive(mat), v0(v0), v1(v1), v2(v2) {}

    Triangle(const Vec3& v0, const Vec3& v1, const Vec3& v2,
             const Vec3& n0, const Vec3& n1, const Vec3& n2,
             const Material* mat = nullptr)
        : Primitive(mat), v0(v0), v1(v1), v2(v2)
        , n0(n0.normalized()), n1(n1.normalized()), n2(n2.normalized())
        , hasNormals(true) {}

    Triangle(const Vec3& v0, const Vec3& v1, const Vec3& v2,
             const Vec3& n0, const Vec3& n1, const Vec3& n2,
             const Vec2& uv0, const Vec2& uv1, const Vec2& uv2,
             const Material* mat = nullptr)
        : Primitive(mat), v0(v0), v1(v1), v2(v2)
        , n0(n0.normalized()), n1(n1.normalized()), n2(n2.normalized())
        , uv0(uv0), uv1(uv1), uv2(uv2)
        , hasNormals(true), hasUVs(true) {}

    // Möller–Trumbore intersection algorithm
    bool intersect(const Ray& ray, SurfaceInteraction& isect) const override;

    // Separating-axis triangle/box test (Akenine-Moller).
    bool overlapsBox(const BoundingBox& box) const override;

    BoundingBox getBounds() const override;

    Vec3 faceNormal() const;

    float area() const override;

    SurfaceInteraction sample(float u1, float u2, float& pdf) const override;
};

} // namespace render

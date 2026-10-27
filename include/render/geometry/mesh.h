// Indexed triangle mesh

#pragma once

#include "../core/bbox.h"
#include "../core/vector.h"
#include "primitive.h"
#include <memory>
#include <vector>

namespace render {

/**
 * @struct TriangleMesh
 * @brief Plain-data triangle mesh (typically produced by an importer).
 *
 * `normals` and `uvs` are either empty or have one entry per position.
 * `indices` holds three entries per triangle.
 */
struct TriangleMesh {
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;
    std::vector<int> indices;

    size_t triangleCount() const { return indices.size() / 3; }
    bool empty() const { return indices.empty(); }

    // Axis-aligned bounds of all vertices.
    BoundingBox bounds() const;

    // Create one Triangle primitive per face using the given material.
    std::vector<std::unique_ptr<Primitive>> toPrimitives(const Material* material) const;
};

} // namespace render

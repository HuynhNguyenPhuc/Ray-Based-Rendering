#include "render/geometry/mesh.h"
#include "render/geometry/triangle.h"

namespace render {

BoundingBox TriangleMesh::bounds() const {
    BoundingBox box;
    for (const auto& p : positions) {
        box.expand(p);
    }
    return box;
}

std::vector<std::unique_ptr<Primitive>> TriangleMesh::toPrimitives(const Material* material) const {
    std::vector<std::unique_ptr<Primitive>> result;
    result.reserve(triangleCount());

    const bool hasNormals = normals.size() == positions.size();
    const bool hasUVs = uvs.size() == positions.size();

    for (size_t f = 0; f + 2 < indices.size(); f += 3) {
        const int i0 = indices[f], i1 = indices[f + 1], i2 = indices[f + 2];

        if (hasNormals && hasUVs) {
            result.push_back(std::make_unique<Triangle>(
                positions[i0], positions[i1], positions[i2],
                normals[i0], normals[i1], normals[i2],
                uvs[i0], uvs[i1], uvs[i2], material));
        } else if (hasNormals) {
            result.push_back(std::make_unique<Triangle>(
                positions[i0], positions[i1], positions[i2],
                normals[i0], normals[i1], normals[i2], material));
        } else {
            result.push_back(std::make_unique<Triangle>(
                positions[i0], positions[i1], positions[i2], material));
        }
    }

    return result;
}

} // namespace render

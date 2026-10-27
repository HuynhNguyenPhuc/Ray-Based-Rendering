// Wavefront OBJ importer

#pragma once

#include "../geometry/mesh.h"
#include <string>

namespace render {

/**
 * @class ObjImporter
 * @brief Loads triangle meshes from Wavefront .obj files.
 *
 * Supports `v`, `vt`, `vn` and `f` records (with negative indices and
 * fan triangulation of polygons). Materials (`mtllib`/`usemtl`) are ignored.
 */
class ObjImporter {
public:
    // Load a mesh from disk. Returns false (and leaves `mesh` empty) on failure.
    static bool load(const std::string& path, TriangleMesh& mesh, std::string* error = nullptr);

    // Parse OBJ text already in memory.
    static bool parse(const std::string& content, TriangleMesh& mesh, std::string* error = nullptr);
};

} // namespace render

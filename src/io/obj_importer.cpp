#include "render/io/obj_importer.h"

#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <tuple>

namespace render {

namespace {

// Resolve a (possibly negative, 1-based) OBJ index into a 0-based index.
// Returns -1 when the index is absent or out of range.
int resolveIndex(int idx, size_t count) {
    if (idx > 0) {
        return idx <= static_cast<int>(count) ? idx - 1 : -1;
    }
    if (idx < 0) {
        int resolved = static_cast<int>(count) + idx;
        return resolved >= 0 ? resolved : -1;
    }
    return -1;
}

// Parse one face corner: "v", "v/vt", "v//vn" or "v/vt/vn".
void parseCorner(const std::string& token, int& v, int& vt, int& vn) {
    v = vt = vn = 0;
    size_t first = token.find('/');
    v = std::atoi(token.substr(0, first).c_str());
    if (first == std::string::npos) return;

    size_t second = token.find('/', first + 1);
    std::string vtStr = token.substr(first + 1,
        second == std::string::npos ? std::string::npos : second - first - 1);
    if (!vtStr.empty()) vt = std::atoi(vtStr.c_str());
    if (second != std::string::npos) {
        std::string vnStr = token.substr(second + 1);
        if (!vnStr.empty()) vn = std::atoi(vnStr.c_str());
    }
}

} // namespace

bool ObjImporter::load(const std::string& path, TriangleMesh& mesh, std::string* error) {
    std::ifstream file(path);
    if (!file.is_open()) {
        if (error) *error = "Could not open OBJ file: " + path;
        mesh = TriangleMesh();
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return parse(buffer.str(), mesh, error);
}

bool ObjImporter::parse(const std::string& content, TriangleMesh& mesh, std::string* error) {
    mesh = TriangleMesh();

    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;

    // Unique (position, uv, normal) corners become mesh vertices.
    std::map<std::tuple<int, int, int>, int> vertexCache;
    bool anyNormals = false;
    bool anyUVs = false;

    std::vector<Vec3> outPositions;
    std::vector<Vec3> outNormals;
    std::vector<Vec2> outUVs;
    std::vector<int> outIndices;

    auto emitCorner = [&](int v, int vt, int vn) -> int {
        auto key = std::make_tuple(v, vt, vn);
        auto it = vertexCache.find(key);
        if (it != vertexCache.end()) return it->second;

        int index = static_cast<int>(outPositions.size());
        outPositions.push_back(positions[v]);
        outNormals.push_back(vn >= 0 ? normals[vn] : Vec3(0.0f));
        outUVs.push_back(vt >= 0 ? uvs[vt] : Vec2(0.0f));
        if (vn >= 0) anyNormals = true;
        if (vt >= 0) anyUVs = true;
        vertexCache[key] = index;
        return index;
    };

    std::istringstream stream(content);
    std::string line;
    int lineNumber = 0;

    while (std::getline(stream, line)) {
        ++lineNumber;
        std::istringstream ls(line);
        std::string tag;
        if (!(ls >> tag) || tag[0] == '#') continue;

        if (tag == "v") {
            float x, y, z;
            if (ls >> x >> y >> z) positions.emplace_back(x, y, z);
        } else if (tag == "vn") {
            float x, y, z;
            if (ls >> x >> y >> z) normals.emplace_back(x, y, z);
        } else if (tag == "vt") {
            float u, v = 0.0f;
            if (ls >> u) {
                ls >> v;
                uvs.emplace_back(u, v);
            }
        } else if (tag == "f") {
            std::vector<int> corners;
            std::string token;
            while (ls >> token) {
                int v, vt, vn;
                parseCorner(token, v, vt, vn);
                int pi = resolveIndex(v, positions.size());
                if (pi < 0) {
                    if (error) *error = "Invalid vertex index on line " + std::to_string(lineNumber);
                    mesh = TriangleMesh();
                    return false;
                }
                corners.push_back(emitCorner(pi,
                                             resolveIndex(vt, uvs.size()),
                                             resolveIndex(vn, normals.size())));
            }

            // Fan triangulation
            for (size_t i = 1; i + 1 < corners.size(); ++i) {
                outIndices.push_back(corners[0]);
                outIndices.push_back(corners[i]);
                outIndices.push_back(corners[i + 1]);
            }
        }
    }

    mesh.positions = std::move(outPositions);
    mesh.indices = std::move(outIndices);
    // Only keep attributes when every vertex has them (arrays must match positions).
    if (anyNormals) {
        bool complete = true;
        for (const auto& n : outNormals) {
            if (n.lengthSquared() == 0.0f) { complete = false; break; }
        }
        if (complete) mesh.normals = std::move(outNormals);
    }
    if (anyUVs) mesh.uvs = std::move(outUVs);

    return !mesh.empty();
}

} // namespace render

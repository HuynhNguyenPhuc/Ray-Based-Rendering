#include "render/acceleration/accelerator_factory.h"

#include "render/acceleration/bvh.h"
#include "render/acceleration/kd_tree.h"
#include "render/acceleration/octree.h"
#include "render/acceleration/uniform_grid.h"

#include <algorithm>
#include <cctype>

namespace render {

std::unique_ptr<Accelerator> createAccelerator(AcceleratorType type) {
    switch (type) {
        case AcceleratorType::Grid:
            return std::make_unique<UniformGrid>();
        case AcceleratorType::KDTree:
            return std::make_unique<KDTree>();
        case AcceleratorType::Octree:
            return std::make_unique<Octree>();
        case AcceleratorType::BVH:
        default:
            return std::make_unique<BVH>();
    }
}

const char* acceleratorName(AcceleratorType type) {
    switch (type) {
        case AcceleratorType::Grid: return "grid";
        case AcceleratorType::KDTree: return "kdtree";
        case AcceleratorType::Octree: return "octree";
        case AcceleratorType::BVH:
        default: return "bvh";
    }
}

bool parseAcceleratorType(const std::string& name, AcceleratorType& type) {
    std::string key;
    for (char c : name) {
        if (c != '-' && c != '_') key.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    if (key == "bvh") type = AcceleratorType::BVH;
    else if (key == "grid" || key == "uniformgrid") type = AcceleratorType::Grid;
    else if (key == "kdtree" || key == "kd") type = AcceleratorType::KDTree;
    else if (key == "octree") type = AcceleratorType::Octree;
    else return false;
    return true;
}

} // namespace render

// Create accelerators by type

#pragma once

#include "accelerator.h"
#include <memory>
#include <string>

namespace render {

/**
 * @enum AcceleratorType
 * @brief Available acceleration structures.
 */
enum class AcceleratorType {
    BVH,
    Grid,
    KDTree,
    Octree
};

std::unique_ptr<Accelerator> createAccelerator(AcceleratorType type);

// Short lowercase name ("bvh", "grid", "kdtree", "octree").
const char* acceleratorName(AcceleratorType type);

// Parse a name (case-insensitive; also accepts "kd-tree", "uniformgrid").
// Returns false and leaves `type` untouched if the name is unknown.
bool parseAcceleratorType(const std::string& name, AcceleratorType& type);

// All types, in a stable order (for comparisons).
inline constexpr AcceleratorType kAllAcceleratorTypes[] = {
    AcceleratorType::BVH, AcceleratorType::Grid, AcceleratorType::KDTree, AcceleratorType::Octree};

} // namespace render

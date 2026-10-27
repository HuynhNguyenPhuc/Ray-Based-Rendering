// Uniform grid accelerator

#pragma once

#include "accelerator.h"
#include <cstdint>
#include <vector>

namespace render {

/**
 * @class UniformGrid
 * @brief Regular voxel grid traversed with a 3D DDA (Amanatides & Woo).
 *
 * Resolution follows the usual heuristic of ~3 voxels per primitive along the
 * cube root of the primitive count. Primitives are listed in every voxel their
 * bounding box overlaps. Unbounded primitives (planes) are tested separately.
 */
class UniformGrid : public Accelerator {
public:
    static constexpr int MAX_RESOLUTION = 128;

    void build(std::vector<Primitive*> primitives) override;
    bool intersect(const Ray& ray, SurfaceInteraction& isect) const override;
    bool intersectP(const Ray& ray) const override;
    BoundingBox worldBounds() const override;

    int resolution(int axis) const { return res_[axis]; }

private:
    std::vector<Primitive*> prims_;      // bounded primitives
    std::vector<Primitive*> unbounded_;  // tested on every query
    std::vector<uint32_t> cellStart_;    // CSR layout: cell c owns cellPrims_[cellStart_[c] .. cellStart_[c+1])
    std::vector<uint32_t> cellPrims_;    // indices into prims_
    BoundingBox bounds_;
    Vec3 cellSize_{1.0f};
    int res_[3] = {1, 1, 1};

    int cellIndex(int x, int y, int z) const { return (z * res_[1] + y) * res_[0] + x; }
};

} // namespace render

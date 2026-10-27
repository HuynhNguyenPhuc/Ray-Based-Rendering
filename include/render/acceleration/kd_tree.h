// kd-tree accelerator

#pragma once

#include "accelerator.h"
#include <cstdint>
#include <vector>

namespace render {

/**
 * @class KDTree
 * @brief Axis-aligned binary space partition built with the surface area heuristic.
 *
 * Interior nodes split space at a plane; primitives straddling the plane are
 * referenced from both children. Traversal is front to back with an explicit
 * stack. Unbounded primitives (planes) are tested separately.
 */
class KDTree : public Accelerator {
public:
    static constexpr int MAX_STACK = 64;

    void build(std::vector<Primitive*> primitives) override;
    bool intersect(const Ray& ray, SurfaceInteraction& isect) const override;
    bool intersectP(const Ray& ray) const override;
    BoundingBox worldBounds() const override;

    size_t nodeCount() const { return nodes_.size(); }
    int maxDepth() const { return maxDepth_; }

private:
    struct Node {
        // Interior: axis in [0,2], `split` is the plane, `index` the above child
        // (the below child is the next node). Leaf: axis == 3, primitives are
        // leafPrims_[index .. index + count).
        float split = 0.0f;
        int32_t axis = 3;
        uint32_t index = 0;
        uint32_t count = 0;
    };

    struct BuildContext;

    std::vector<Primitive*> prims_;      // bounded primitives
    std::vector<Primitive*> unbounded_;  // tested on every query
    std::vector<Node> nodes_;
    std::vector<uint32_t> leafPrims_;    // indices into prims_
    BoundingBox bounds_;
    int maxDepth_ = 0;

    void buildNode(BuildContext& ctx, const BoundingBox& nodeBounds,
                   const std::vector<uint32_t>& indices, int depthLeft);
};

} // namespace render

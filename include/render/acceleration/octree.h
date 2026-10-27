// Octree accelerator

#pragma once

#include "accelerator.h"
#include <cstdint>
#include <vector>

namespace render {

/**
 * @class Octree
 * @brief Adaptive 8-way spatial subdivision.
 *
 * A node is split at its centre until it holds few primitives or the depth
 * limit is reached. Primitives overlapping several octants are referenced from
 * each of them. Traversal visits the octants a ray touches in front-to-back
 * order. Unbounded primitives (planes) are tested separately.
 */
class Octree : public Accelerator {
public:
    static constexpr int MAX_PRIMS_IN_NODE = 6;
    static constexpr int MAX_DEPTH = 10;
    // A node is not split if its primitives would be referenced more than this many
    // times on average across the eight octants.
    static constexpr size_t MAX_DUPLICATION = 3;

    void build(std::vector<Primitive*> primitives) override;
    bool intersect(const Ray& ray, SurfaceInteraction& isect) const override;
    bool intersectP(const Ray& ray) const override;
    BoundingBox worldBounds() const override;

    size_t nodeCount() const { return nodes_.size(); }
    int maxDepth() const { return maxDepth_; }

private:
    struct Node {
        // Interior: the eight children are nodes_[firstChild .. firstChild + 8),
        // indexed by bit 0 = +x, bit 1 = +y, bit 2 = +z. Leaf: firstChild == 0 and the
        // primitives are leafPrims_[primStart .. primStart + primCount).
        uint32_t firstChild = 0;
        uint32_t primStart = 0;
        uint32_t primCount = 0;
        uint32_t emptyMask = 0;  // interior: bit o set if child o is an empty leaf

        bool isLeaf() const { return firstChild == 0; }
    };

    std::vector<Primitive*> prims_;      // bounded primitives
    std::vector<BoundingBox> primBounds_;
    std::vector<Primitive*> unbounded_;  // tested on every query
    std::vector<Node> nodes_;
    std::vector<Primitive*> leafPrims_;  // leaf contents (one contiguous run per leaf)
    BoundingBox bounds_;
    int maxDepth_ = 0;

    void buildNode(uint32_t nodeIndex, const BoundingBox& box,
                   const std::vector<uint32_t>& indices, int depth);

    static BoundingBox childBox(const BoundingBox& box, int octant);

    // `slabs` holds the ray's entry (3) and exit (3) parameters for the node's three slabs.
    bool intersectNode(uint32_t nodeIndex, const BoundingBox& box, const float* slabs,
                       const Ray& ray, const Vec3& invDir, SurfaceInteraction& isect) const;
    bool intersectNodeP(uint32_t nodeIndex, const BoundingBox& box, const float* slabs,
                        const Ray& ray, const Vec3& invDir) const;
};

} // namespace render

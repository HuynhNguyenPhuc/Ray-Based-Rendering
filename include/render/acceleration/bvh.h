// Bounding volume hierarchy accelerator

#pragma once

#include "accelerator.h"
#include <memory>
#include <vector>

namespace render {

/**
 * @struct BVHNode
 * @brief Node in the BVH tree.
 */
struct BVHNode {
    BoundingBox bounds;
    std::unique_ptr<BVHNode> left;
    std::unique_ptr<BVHNode> right;
    size_t primStart = 0;
    size_t primCount = 0;

    bool isLeaf() const { return left == nullptr && right == nullptr; }
};

/**
 * @enum SplitMethod
 * @brief Methods for choosing BVH split.
 */
enum class SplitMethod {
    Middle,     // Split at midpoint of longest axis
    EqualCounts, // Split to have equal primitives on each side
    SAH         // Surface Area Heuristic (best quality)
};

/**
 * @class BVH
 * @brief Bounding Volume Hierarchy for fast ray intersection.
 */
class BVH : public Accelerator {
public:
    static constexpr int MAX_PRIMS_IN_NODE = 4;
    static constexpr int MAX_DEPTH = 32;

    BVH() = default;

    // Build with the default (SAH) split method.
    void build(std::vector<Primitive*> primitives) override;

    // Build with an explicit split method.
    void build(std::vector<Primitive*> primitives, SplitMethod method);

    bool intersect(const Ray& ray, SurfaceInteraction& isect) const override;
    bool intersectP(const Ray& ray) const override;
    BoundingBox worldBounds() const override;

    /**
     * Get primitives.
     */
    const std::vector<Primitive*>& primitives() const { return primitives_; }

private:
    struct PrimInfo {
        size_t index;
        BoundingBox bounds;

        Vec3 centroid() const { return bounds.center(); }
    };

    std::unique_ptr<BVHNode> root_;
    std::vector<Primitive*> primitives_;
    SplitMethod splitMethod_ = SplitMethod::SAH;

    std::unique_ptr<BVHNode> buildRecursive(std::vector<PrimInfo>& primInfo,
        size_t start, size_t end,
        std::vector<Primitive*>& orderedPrims,
        int depth);

    size_t findBestSplit(std::vector<PrimInfo>& primInfo, size_t start, size_t end,
        const BoundingBox& bounds, const BoundingBox& centroidBounds, int dim);

    bool intersectNode(const BVHNode* node, const Ray& ray, SurfaceInteraction& isect) const;

    bool intersectNodeP(const BVHNode* node, const Ray& ray) const;
};

} // namespace render

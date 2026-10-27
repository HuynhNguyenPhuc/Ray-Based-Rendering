#include "render/acceleration/bvh.h"
#include <algorithm>

namespace render {

void BVH::build(std::vector<Primitive*> primitives) {
    build(std::move(primitives), SplitMethod::SAH);
}

void BVH::build(std::vector<Primitive*> primitives, SplitMethod method) {
    primitives_ = std::move(primitives);
    splitMethod_ = method;

    if (primitives_.empty()) {
        root_ = nullptr;
        return;
    }

    // Build primitive info
    std::vector<PrimInfo> primInfo(primitives_.size());
    for (size_t i = 0; i < primitives_.size(); ++i) {
        primInfo[i] = { i, primitives_[i]->getBounds() };
    }

    // Build BVH recursively
    std::vector<Primitive*> orderedPrims;
    orderedPrims.reserve(primitives_.size());

    root_ = buildRecursive(primInfo, 0, primitives_.size(), orderedPrims, 0);
    primitives_ = std::move(orderedPrims);
}

bool BVH::intersect(const Ray& ray, SurfaceInteraction& isect) const {
    if (!root_) return false;
    return intersectNode(root_.get(), ray, isect);
}

bool BVH::intersectP(const Ray& ray) const {
    if (!root_) return false;
    return intersectNodeP(root_.get(), ray);
}

BoundingBox BVH::worldBounds() const {
    return root_ ? root_->bounds : BoundingBox();
}

std::unique_ptr<BVHNode> BVH::buildRecursive(std::vector<PrimInfo>& primInfo,
        size_t start, size_t end,
        std::vector<Primitive*>& orderedPrims,
        int depth) {
    auto node = std::make_unique<BVHNode>();

    // Compute bounds of all primitives
    BoundingBox bounds;
    for (size_t i = start; i < end; ++i) {
        bounds.expand(primInfo[i].bounds);
    }
    node->bounds = bounds;

    size_t nPrims = end - start;

    // Create leaf if few primitives or max depth reached
    if (nPrims <= MAX_PRIMS_IN_NODE || depth >= MAX_DEPTH) {
        node->primStart = orderedPrims.size();
        node->primCount = nPrims;
        for (size_t i = start; i < end; ++i) {
            orderedPrims.push_back(primitives_[primInfo[i].index]);
        }
        return node;
    }

    // Compute centroid bounds
    BoundingBox centroidBounds;
    for (size_t i = start; i < end; ++i) {
        centroidBounds.expand(primInfo[i].centroid());
    }

    int dim = centroidBounds.maximumExtent();

    // If centroids are at same position, create leaf
    if (centroidBounds.pMax[dim] == centroidBounds.pMin[dim]) {
        node->primStart = orderedPrims.size();
        node->primCount = nPrims;
        for (size_t i = start; i < end; ++i) {
            orderedPrims.push_back(primitives_[primInfo[i].index]);
        }
        return node;
    }

    // Choose split position
    size_t mid = (start + end) / 2;

    switch (splitMethod_) {
        case SplitMethod::Middle: {
            float pmid = (centroidBounds.pMin[dim] + centroidBounds.pMax[dim]) * 0.5f;
            auto midIter = std::partition(
                primInfo.begin() + start, primInfo.begin() + end,
                [dim, pmid](const PrimInfo& pi) {
                    return pi.centroid()[dim] < pmid;
                });
            mid = midIter - primInfo.begin();
            if (mid == start || mid == end) {
                mid = (start + end) / 2;
                std::nth_element(primInfo.begin() + start, primInfo.begin() + mid,
                                primInfo.begin() + end,
                                [dim](const PrimInfo& a, const PrimInfo& b) {
                                    return a.centroid()[dim] < b.centroid()[dim];
                                });
            }
            break;
        }

        case SplitMethod::EqualCounts: {
            mid = (start + end) / 2;
            std::nth_element(primInfo.begin() + start, primInfo.begin() + mid,
                            primInfo.begin() + end,
                            [dim](const PrimInfo& a, const PrimInfo& b) {
                                return a.centroid()[dim] < b.centroid()[dim];
                            });
            break;
        }

        case SplitMethod::SAH: {
            if (nPrims <= 2) {
                mid = (start + end) / 2;
                std::nth_element(primInfo.begin() + start, primInfo.begin() + mid,
                                primInfo.begin() + end,
                                [dim](const PrimInfo& a, const PrimInfo& b) {
                                    return a.centroid()[dim] < b.centroid()[dim];
                                });
            } else {
                mid = findBestSplit(primInfo, start, end, bounds, centroidBounds, dim);
            }
            break;
        }
    }

    // Build children
    node->left = buildRecursive(primInfo, start, mid, orderedPrims, depth + 1);
    node->right = buildRecursive(primInfo, mid, end, orderedPrims, depth + 1);

    return node;
}

size_t BVH::findBestSplit(std::vector<PrimInfo>& primInfo, size_t start, size_t end,
        const BoundingBox& bounds, const BoundingBox& centroidBounds, int dim) {
    constexpr int nBuckets = 12;

    struct Bucket {
        int count = 0;
        BoundingBox bounds;
    };
    Bucket buckets[nBuckets];

    // Initialize buckets
    for (size_t i = start; i < end; ++i) {
        int b = nBuckets * ((primInfo[i].centroid()[dim] - centroidBounds.pMin[dim]) /
                            (centroidBounds.pMax[dim] - centroidBounds.pMin[dim]));
        if (b == nBuckets) b = nBuckets - 1;
        buckets[b].count++;
        buckets[b].bounds.expand(primInfo[i].bounds);
    }

    // Compute costs for splitting after each bucket
    float cost[nBuckets - 1];
    for (int i = 0; i < nBuckets - 1; ++i) {
        BoundingBox b0, b1;
        int count0 = 0, count1 = 0;

        for (int j = 0; j <= i; ++j) {
            b0.expand(buckets[j].bounds);
            count0 += buckets[j].count;
        }
        for (int j = i + 1; j < nBuckets; ++j) {
            b1.expand(buckets[j].bounds);
            count1 += buckets[j].count;
        }

        cost[i] = 1.0f + (count0 * b0.surfaceArea() + count1 * b1.surfaceArea()) / bounds.surfaceArea();
    }

    // Find best split
    float minCost = cost[0];
    int minCostBucket = 0;
    for (int i = 1; i < nBuckets - 1; ++i) {
        if (cost[i] < minCost) {
            minCost = cost[i];
            minCostBucket = i;
        }
    }

    // Create leaf if no good split found
    float leafCost = static_cast<float>(end - start);
    if (minCost >= leafCost) {
        return (start + end) / 2;
    }

    // Partition primitives
    auto midIter = std::partition(
        primInfo.begin() + start, primInfo.begin() + end,
        [=](const PrimInfo& pi) {
            int b = nBuckets * ((pi.centroid()[dim] - centroidBounds.pMin[dim]) /
                                (centroidBounds.pMax[dim] - centroidBounds.pMin[dim]));
            if (b == nBuckets) b = nBuckets - 1;
            return b <= minCostBucket;
        });

    return midIter - primInfo.begin();
}

bool BVH::intersectNode(const BVHNode* node, const Ray& ray, SurfaceInteraction& isect) const {
    float tMin, tMax;
    if (!node->bounds.intersect(ray, tMin, tMax)) {
        return false;
    }

    if (node->isLeaf()) {
        bool hit = false;
        for (size_t i = 0; i < node->primCount; ++i) {
            if (primitives_[node->primStart + i]->intersect(ray, isect)) {
                hit = true;
            }
        }
        return hit;
    }

    // Traverse children (front to back)
    float tLeft, tRight;
    bool hitLeft = node->left->bounds.intersect(ray, tLeft, tMax);
    bool hitRight = node->right->bounds.intersect(ray, tRight, tMax);

    BVHNode* first = node->left.get();
    BVHNode* second = node->right.get();

    if (hitLeft && hitRight && tLeft > tRight) {
        std::swap(first, second);
    }

    bool hit = false;
    if (hitLeft || hitRight) {
        hit |= intersectNode(first, ray, isect);
        if (hitRight || hitLeft) {
            hit |= intersectNode(second, ray, isect);
        }
    }

    return hit;
}

bool BVH::intersectNodeP(const BVHNode* node, const Ray& ray) const {
    float tMin, tMax;
    if (!node->bounds.intersect(ray, tMin, tMax)) {
        return false;
    }

    if (node->isLeaf()) {
        for (size_t i = 0; i < node->primCount; ++i) {
            if (primitives_[node->primStart + i]->intersectP(ray)) {
                return true;
            }
        }
        return false;
    }

    return intersectNodeP(node->left.get(), ray) || 
        intersectNodeP(node->right.get(), ray);
}

} // namespace render

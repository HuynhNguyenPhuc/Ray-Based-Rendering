#include "render/acceleration/kd_tree.h"

#include "accel_common.h"

namespace render {

namespace {

constexpr float kTraversalCost = 1.0f;
constexpr float kIntersectCost = 20.0f;
constexpr float kEmptyBonus = 0.2f;
constexpr int kMaxLeafPrims = 2;
constexpr int kBins = 32;

} // namespace

struct KDTree::BuildContext {
    const std::vector<BoundingBox>& primBounds;
    int depthLimit;
    int deepest = 0;
};

void KDTree::build(std::vector<Primitive*> primitives) {
    std::vector<BoundingBox> primBounds;
    accel::splitBounded(primitives, prims_, primBounds, unbounded_, bounds_);
    nodes_.clear();
    leafPrims_.clear();
    maxDepth_ = 0;

    if (prims_.empty()) return;

    bounds_ = accel::padded(bounds_);
    const int depthLimit = std::min(MAX_STACK - 1,
        static_cast<int>(std::lround(8.0 + 1.3 * std::log2(static_cast<double>(prims_.size())))));

    std::vector<uint32_t> all(prims_.size());
    for (uint32_t i = 0; i < all.size(); ++i) all[i] = i;

    BuildContext ctx{primBounds, depthLimit, 0};
    nodes_.reserve(2 * prims_.size());
    buildNode(ctx, bounds_, all, depthLimit);
    maxDepth_ = ctx.deepest;
}

void KDTree::buildNode(BuildContext& ctx, const BoundingBox& nodeBounds,
                       const std::vector<uint32_t>& indices, int depthLeft) {
    const uint32_t nodeIndex = static_cast<uint32_t>(nodes_.size());
    nodes_.emplace_back();

    auto makeLeaf = [&]() {
        Node& n = nodes_[nodeIndex];
        n.axis = 3;
        n.index = static_cast<uint32_t>(leafPrims_.size());
        n.count = static_cast<uint32_t>(indices.size());
        ctx.deepest = std::max(ctx.deepest, ctx.depthLimit - depthLeft);
        leafPrims_.insert(leafPrims_.end(), indices.begin(), indices.end());
    };

    const int n = static_cast<int>(indices.size());
    if (n <= kMaxLeafPrims || depthLeft <= 0) {
        makeLeaf();
        return;
    }

    const Vec3 d = nodeBounds.diagonal();
    const float invTotalSA = 1.0f / nodeBounds.surfaceArea();
    const float leafCost = kIntersectCost * n;

    float bestCost = std::numeric_limits<float>::infinity();
    int bestAxis = -1;
    float bestSplit = 0.0f;

    // Binned SAH: histogram primitive start/end positions per axis (O(n)) and
    // evaluate a plane at every bin boundary.
    for (int axis = 0; axis < 3; ++axis) {
        if (d[axis] <= 0.0f) continue;
        const float lo = nodeBounds.pMin[axis];
        const float binWidth = d[axis] / kBins;
        const float invWidth = 1.0f / binWidth;
        int starts[kBins] = {}, ends[kBins] = {};
        for (uint32_t i : indices) {
            const BoundingBox& b = ctx.primBounds[i];
            const int bs = std::clamp(static_cast<int>((b.pMin[axis] - lo) * invWidth), 0, kBins - 1);
            const int be = std::clamp(static_cast<int>((b.pMax[axis] - lo) * invWidth), 0, kBins - 1);
            ++starts[bs];
            ++ends[be];
        }

        const int axis1 = (axis + 1) % 3, axis2 = (axis + 2) % 3;
        int nBelow = 0, endedBelow = 0;
        for (int k = 1; k < kBins; ++k) {
            nBelow += starts[k - 1];
            endedBelow += ends[k - 1];
            const int nAbove = n - endedBelow;

            const float below = k * binWidth;
            const float above = d[axis] - below;
            const float saBelow = 2.0f * (d[axis1] * d[axis2] + below * (d[axis1] + d[axis2]));
            const float saAbove = 2.0f * (d[axis1] * d[axis2] + above * (d[axis1] + d[axis2]));
            const float bonus = (nBelow == 0 || nAbove == 0) ? kEmptyBonus : 0.0f;
            const float cost = kTraversalCost +
                kIntersectCost * (1.0f - bonus) * (saBelow * nBelow + saAbove * nAbove) * invTotalSA;
            if (cost < bestCost) {
                bestCost = cost;
                bestAxis = axis;
                bestSplit = lo + below;
            }
        }
    }

    if (bestAxis < 0 || bestCost >= leafCost) {
        makeLeaf();
        return;
    }

    // Distribute primitives; those touching the plane go to both sides.
    std::vector<uint32_t> below, above;
    for (uint32_t i : indices) {
        const BoundingBox& b = ctx.primBounds[i];
        if (b.pMin[bestAxis] <= bestSplit) below.push_back(i);
        if (b.pMax[bestAxis] >= bestSplit) above.push_back(i);
    }
    // No progress (everything straddles the plane): stop splitting.
    if (below.size() == indices.size() && above.size() == indices.size()) {
        makeLeaf();
        return;
    }

    BoundingBox belowBounds = nodeBounds, aboveBounds = nodeBounds;
    belowBounds.pMax[bestAxis] = bestSplit;
    aboveBounds.pMin[bestAxis] = bestSplit;

    nodes_[nodeIndex].axis = bestAxis;
    nodes_[nodeIndex].split = bestSplit;

    buildNode(ctx, belowBounds, below, depthLeft - 1);
    nodes_[nodeIndex].index = static_cast<uint32_t>(nodes_.size());
    buildNode(ctx, aboveBounds, above, depthLeft - 1);
}

bool KDTree::intersect(const Ray& ray, SurfaceInteraction& isect) const {
    bool hit = accel::intersectAll(unbounded_, ray, isect);
    if (nodes_.empty()) return hit;

    const Vec3 invDir = accel::inverse(ray.direction);
    float tMin, tMax;
    if (!accel::clipRay(bounds_, ray, invDir, tMin, tMax)) return hit;

    struct Todo { uint32_t node; float tMin, tMax; };
    Todo stack[MAX_STACK];
    int top = 0;
    uint32_t node = 0;

    while (true) {
        // A closer hit has already been found: nothing in this segment can beat it.
        if (ray.tMax < tMin) {
            if (top == 0) break;
            const Todo& t = stack[--top];
            node = t.node; tMin = t.tMin; tMax = t.tMax;
            continue;
        }

        const Node& n = nodes_[node];
        if (n.axis == 3) {
            for (uint32_t k = 0; k < n.count; ++k) {
                if (prims_[leafPrims_[n.index + k]]->intersect(ray, isect)) hit = true;
            }
            if (top == 0) break;
            const Todo& t = stack[--top];
            node = t.node; tMin = t.tMin; tMax = t.tMax;
            continue;
        }

        const int a = n.axis;
        uint32_t first, second;
        float tPlane;
        if (ray.direction[a] == 0.0f) {
            // Parallel to the plane: stays on the side it starts on.
            first = (ray.origin[a] <= n.split) ? node + 1 : n.index;
            node = first;
            continue;
        }
        tPlane = (n.split - ray.origin[a]) * invDir[a];
        const bool belowFirst = (ray.origin[a] < n.split) ||
                                (ray.origin[a] == n.split && ray.direction[a] <= 0.0f);
        if (belowFirst) { first = node + 1; second = n.index; }
        else            { first = n.index;  second = node + 1; }

        if (tPlane > tMax || tPlane <= 0.0f) {
            node = first;
        } else if (tPlane < tMin) {
            node = second;
        } else {
            stack[top++] = {second, tPlane, tMax};
            node = first;
            tMax = tPlane;
        }
    }
    return hit;
}

bool KDTree::intersectP(const Ray& ray) const {
    if (accel::intersectAnyP(unbounded_, ray)) return true;
    if (nodes_.empty()) return false;

    const Vec3 invDir = accel::inverse(ray.direction);
    float tMin, tMax;
    if (!accel::clipRay(bounds_, ray, invDir, tMin, tMax)) return false;

    struct Todo { uint32_t node; float tMin, tMax; };
    Todo stack[MAX_STACK];
    int top = 0;
    uint32_t node = 0;

    while (true) {
        const Node& n = nodes_[node];
        if (n.axis == 3) {
            for (uint32_t k = 0; k < n.count; ++k) {
                if (prims_[leafPrims_[n.index + k]]->intersectP(ray)) return true;
            }
            if (top == 0) break;
            const Todo& t = stack[--top];
            node = t.node; tMin = t.tMin; tMax = t.tMax;
            continue;
        }

        const int a = n.axis;
        if (ray.direction[a] == 0.0f) {
            node = (ray.origin[a] <= n.split) ? node + 1 : n.index;
            continue;
        }
        const float tPlane = (n.split - ray.origin[a]) * invDir[a];
        const bool belowFirst = (ray.origin[a] < n.split) ||
                                (ray.origin[a] == n.split && ray.direction[a] <= 0.0f);
        const uint32_t first = belowFirst ? node + 1 : n.index;
        const uint32_t second = belowFirst ? n.index : node + 1;

        if (tPlane > tMax || tPlane <= 0.0f) {
            node = first;
        } else if (tPlane < tMin) {
            node = second;
        } else {
            stack[top++] = {second, tPlane, tMax};
            node = first;
            tMax = tPlane;
        }
    }
    return false;
}

BoundingBox KDTree::worldBounds() const {
    return nodes_.empty() ? BoundingBox() : bounds_;
}

} // namespace render

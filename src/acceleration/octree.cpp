#include "render/acceleration/octree.h"

#include "accel_common.h"

namespace render {

void Octree::build(std::vector<Primitive*> primitives) {
    accel::splitBounded(primitives, prims_, primBounds_, unbounded_, bounds_);
    nodes_.clear();
    leafPrims_.clear();
    maxDepth_ = 0;

    if (prims_.empty()) return;

    bounds_ = accel::padded(bounds_);

    std::vector<uint32_t> all(prims_.size());
    for (uint32_t i = 0; i < all.size(); ++i) all[i] = i;

    nodes_.emplace_back();
    buildNode(0, bounds_, all, 0);
}

BoundingBox Octree::childBox(const BoundingBox& box, int octant) {
    const Vec3 c = box.center();
    Vec3 lo = box.pMin, hi = box.pMax;
    for (int a = 0; a < 3; ++a) {
        if (octant & (1 << a)) lo[a] = c[a];
        else hi[a] = c[a];
    }
    return BoundingBox(lo, hi);
}

void Octree::buildNode(uint32_t nodeIndex, const BoundingBox& box,
                       const std::vector<uint32_t>& indices, int depth) {
    maxDepth_ = std::max(maxDepth_, depth);

    auto makeLeaf = [&]() {
        Node& n = nodes_[nodeIndex];
        n.firstChild = 0;
        n.primStart = static_cast<uint32_t>(leafPrims_.size());
        n.primCount = static_cast<uint32_t>(indices.size());
        for (uint32_t i : indices) leafPrims_.push_back(prims_[i]);
    };

    if (indices.size() <= static_cast<size_t>(MAX_PRIMS_IN_NODE) || depth >= MAX_DEPTH) {
        makeLeaf();
        return;
    }

    // Distribute the primitives over the octants their boxes overlap.
    std::vector<uint32_t> childIndices[8];
    BoundingBox childBoxes[8];
    for (int o = 0; o < 8; ++o) {
        childBoxes[o] = childBox(box, o);
        for (uint32_t i : indices) {
            const BoundingBox& b = primBounds_[i];
            const BoundingBox& c = childBoxes[o];
            if (b.pMax.x >= c.pMin.x && b.pMin.x <= c.pMax.x &&
                b.pMax.y >= c.pMin.y && b.pMin.y <= c.pMax.y &&
                b.pMax.z >= c.pMin.z && b.pMin.z <= c.pMax.z &&
                prims_[i]->overlapsBox(c)) {
                childIndices[o].push_back(i);
            }
        }
    }

    // Splitting is pointless when primitives are mostly duplicated into many octants
    // (large primitives spanning the node) or when a single child inherits everything.
    size_t references = 0, largest = 0;
    for (int o = 0; o < 8; ++o) {
        references += childIndices[o].size();
        largest = std::max(largest, childIndices[o].size());
    }
    if (largest == indices.size() || references > MAX_DUPLICATION * indices.size()) {
        makeLeaf();
        return;
    }

    const uint32_t first = static_cast<uint32_t>(nodes_.size());
    nodes_.resize(nodes_.size() + 8);
    nodes_[nodeIndex].firstChild = first;
    uint32_t emptyMask = 0;
    for (int o = 0; o < 8; ++o) {
        buildNode(first + o, childBoxes[o], childIndices[o], depth + 1);
        if (nodes_[first + o].isLeaf() && nodes_[first + o].primCount == 0) emptyMask |= 1u << o;
    }
    nodes_[nodeIndex].emptyMask = emptyMask;
}

namespace {

struct Slabs {
    float tn[3], tf[3];  // ray parameter where the ray enters / leaves each slab of the node
};

struct ChildVisit {
    float tEntry;
    int octant;
    Slabs slabs;
};

constexpr float kInf = std::numeric_limits<float>::infinity();

} // namespace

// Parametric traversal (Revelles et al.): the entry/exit parameters of the eight children
// follow from the node's slab values and the three mid-plane crossings, so no per-child
// box test or division is needed. Returns the overlapping children sorted by entry.
static int overlappingChildren(const Slabs& s, const Vec3& center, const Ray& ray,
                               const Vec3& invDir, uint32_t emptyMask, ChildVisit out[8]) {
    float lo[3][2], hi[3][2];  // [axis][0 = lower half, 1 = upper half]
    for (int a = 0; a < 3; ++a) {
        const float dir = ray.direction[a];
        if (dir == 0.0f) {
            const bool lower = ray.origin[a] < center[a];
            lo[a][0] = lower ? -kInf : kInf;  hi[a][0] = lower ? kInf : -kInf;
            lo[a][1] = lower ? kInf : -kInf;  hi[a][1] = lower ? -kInf : kInf;
        } else {
            const float tm = (center[a] - ray.origin[a]) * invDir[a];
            if (dir > 0.0f) {
                lo[a][0] = s.tn[a]; hi[a][0] = tm;
                lo[a][1] = tm;      hi[a][1] = s.tf[a];
            } else {
                lo[a][1] = s.tn[a]; hi[a][1] = tm;
                lo[a][0] = tm;      hi[a][0] = s.tf[a];
            }
        }
    }

    int count = 0;
    for (int o = 0; o < 8; ++o) {
        if (emptyMask & (1u << o)) continue;
        const int hx = o & 1, hy = (o >> 1) & 1, hz = (o >> 2) & 1;
        const float tEntry = std::max({lo[0][hx], lo[1][hy], lo[2][hz]});
        const float tExit = std::min({hi[0][hx], hi[1][hy], hi[2][hz]});
        if (tEntry > tExit || tExit < 0.0f || tEntry > ray.tMax) continue;

        int i = count++;
        while (i > 0 && out[i - 1].tEntry > tEntry) {
            out[i] = out[i - 1];
            --i;
        }
        out[i].tEntry = tEntry;
        out[i].octant = o;
        out[i].slabs = {{lo[0][hx], lo[1][hy], lo[2][hz]}, {hi[0][hx], hi[1][hy], hi[2][hz]}};
    }
    return count;
}

bool Octree::intersectNode(uint32_t nodeIndex, const BoundingBox& box, const float* slabs,
                           const Ray& ray, const Vec3& invDir, SurfaceInteraction& isect) const {
    const Node& n = nodes_[nodeIndex];
    if (n.isLeaf()) {
        bool hit = false;
        for (uint32_t k = 0; k < n.primCount; ++k) {
            if (leafPrims_[n.primStart + k]->intersect(ray, isect)) hit = true;
        }
        return hit;
    }

    Slabs s;
    std::copy(slabs, slabs + 3, s.tn);
    std::copy(slabs + 3, slabs + 6, s.tf);
    ChildVisit visits[8];
    const int count = overlappingChildren(s, box.center(), ray, invDir, n.emptyMask, visits);

    bool hit = false;
    for (int i = 0; i < count; ++i) {
        // ray.tMax shrinks as hits are found; later octants may now be too far.
        if (visits[i].tEntry > ray.tMax) break;
        float cs[6] = {visits[i].slabs.tn[0], visits[i].slabs.tn[1], visits[i].slabs.tn[2],
                       visits[i].slabs.tf[0], visits[i].slabs.tf[1], visits[i].slabs.tf[2]};
        if (intersectNode(n.firstChild + visits[i].octant, childBox(box, visits[i].octant), cs,
                          ray, invDir, isect)) {
            hit = true;
        }
    }
    return hit;
}

bool Octree::intersectNodeP(uint32_t nodeIndex, const BoundingBox& box, const float* slabs,
                            const Ray& ray, const Vec3& invDir) const {
    const Node& n = nodes_[nodeIndex];
    if (n.isLeaf()) {
        for (uint32_t k = 0; k < n.primCount; ++k) {
            if (leafPrims_[n.primStart + k]->intersectP(ray)) return true;
        }
        return false;
    }

    Slabs s;
    std::copy(slabs, slabs + 3, s.tn);
    std::copy(slabs + 3, slabs + 6, s.tf);
    ChildVisit visits[8];
    const int count = overlappingChildren(s, box.center(), ray, invDir, n.emptyMask, visits);
    for (int i = 0; i < count; ++i) {
        float cs[6] = {visits[i].slabs.tn[0], visits[i].slabs.tn[1], visits[i].slabs.tn[2],
                       visits[i].slabs.tf[0], visits[i].slabs.tf[1], visits[i].slabs.tf[2]};
        if (intersectNodeP(n.firstChild + visits[i].octant, childBox(box, visits[i].octant), cs,
                           ray, invDir)) {
            return true;
        }
    }
    return false;
}

// Slab entry/exit values of the root box (infinite for axes the ray runs parallel to).
static void rootSlabs(const BoundingBox& b, const Ray& ray, const Vec3& invDir, float out[6]) {
    for (int a = 0; a < 3; ++a) {
        if (ray.direction[a] == 0.0f) {
            out[a] = -kInf;
            out[3 + a] = kInf;
        } else {
            float t1 = (b.pMin[a] - ray.origin[a]) * invDir[a];
            float t2 = (b.pMax[a] - ray.origin[a]) * invDir[a];
            out[a] = std::min(t1, t2);
            out[3 + a] = std::max(t1, t2);
        }
    }
}

bool Octree::intersect(const Ray& ray, SurfaceInteraction& isect) const {
    bool hit = accel::intersectAll(unbounded_, ray, isect);
    if (nodes_.empty()) return hit;

    const Vec3 invDir = accel::inverse(ray.direction);
    float t0, t1;
    if (!accel::clipRay(bounds_, ray, invDir, t0, t1)) return hit;
    float slabs[6];
    rootSlabs(bounds_, ray, invDir, slabs);
    return intersectNode(0, bounds_, slabs, ray, invDir, isect) || hit;
}

bool Octree::intersectP(const Ray& ray) const {
    if (accel::intersectAnyP(unbounded_, ray)) return true;
    if (nodes_.empty()) return false;

    const Vec3 invDir = accel::inverse(ray.direction);
    float t0, t1;
    if (!accel::clipRay(bounds_, ray, invDir, t0, t1)) return false;
    float slabs[6];
    rootSlabs(bounds_, ray, invDir, slabs);
    return intersectNodeP(0, bounds_, slabs, ray, invDir);
}

BoundingBox Octree::worldBounds() const {
    return nodes_.empty() ? BoundingBox() : bounds_;
}

} // namespace render

#include "render/acceleration/uniform_grid.h"

#include "accel_common.h"

namespace render {

void UniformGrid::build(std::vector<Primitive*> primitives) {
    std::vector<BoundingBox> primBounds;
    accel::splitBounded(primitives, prims_, primBounds, unbounded_, bounds_);
    cellStart_.clear();
    cellPrims_.clear();
    res_[0] = res_[1] = res_[2] = 1;

    if (prims_.empty()) return;

    bounds_ = accel::padded(bounds_);
    const Vec3 d = bounds_.diagonal();
    const float maxExtent = std::max({d.x, d.y, d.z});

    // ~3 voxels per primitive overall: voxelsPerUnit = 3 * cbrt(N) / maxExtent.
    const float voxelsPerUnit = 3.0f * std::cbrt(static_cast<float>(prims_.size())) / maxExtent;
    for (int a = 0; a < 3; ++a) {
        res_[a] = std::clamp(static_cast<int>(std::ceil(d[a] * voxelsPerUnit)), 1, MAX_RESOLUTION);
        cellSize_[a] = d[a] / static_cast<float>(res_[a]);
    }

    auto cellRange = [&](const BoundingBox& b, int lo[3], int hi[3]) {
        for (int a = 0; a < 3; ++a) {
            lo[a] = std::clamp(static_cast<int>((b.pMin[a] - bounds_.pMin[a]) / cellSize_[a]), 0, res_[a] - 1);
            hi[a] = std::clamp(static_cast<int>((b.pMax[a] - bounds_.pMin[a]) / cellSize_[a]), 0, res_[a] - 1);
        }
    };

    // Does primitive i really touch cell (x,y,z)? Only checked when its box spans several cells.
    auto touches = [&](size_t i, const int lo[3], const int hi[3], int x, int y, int z) {
        if (lo[0] == hi[0] && lo[1] == hi[1] && lo[2] == hi[2]) return true;
        const Vec3 cmin(bounds_.pMin.x + x * cellSize_.x, bounds_.pMin.y + y * cellSize_.y,
                        bounds_.pMin.z + z * cellSize_.z);
        return prims_[i]->overlapsBox(BoundingBox(cmin, cmin + cellSize_));
    };

    // Two passes: count entries per cell, then fill (compressed row layout).
    const size_t nCells = static_cast<size_t>(res_[0]) * res_[1] * res_[2];
    cellStart_.assign(nCells + 1, 0);
    for (size_t i = 0; i < primBounds.size(); ++i) {
        int lo[3], hi[3];
        cellRange(primBounds[i], lo, hi);
        for (int z = lo[2]; z <= hi[2]; ++z)
            for (int y = lo[1]; y <= hi[1]; ++y)
                for (int x = lo[0]; x <= hi[0]; ++x)
                    if (touches(i, lo, hi, x, y, z)) ++cellStart_[cellIndex(x, y, z) + 1];
    }
    for (size_t c = 0; c < nCells; ++c) cellStart_[c + 1] += cellStart_[c];

    cellPrims_.resize(cellStart_[nCells]);
    std::vector<uint32_t> cursor(cellStart_.begin(), cellStart_.end() - 1);
    for (size_t i = 0; i < primBounds.size(); ++i) {
        int lo[3], hi[3];
        cellRange(primBounds[i], lo, hi);
        for (int z = lo[2]; z <= hi[2]; ++z)
            for (int y = lo[1]; y <= hi[1]; ++y)
                for (int x = lo[0]; x <= hi[0]; ++x)
                    if (touches(i, lo, hi, x, y, z)) cellPrims_[cursor[cellIndex(x, y, z)]++] = static_cast<uint32_t>(i);
    }
}

bool UniformGrid::intersect(const Ray& ray, SurfaceInteraction& isect) const {
    bool hit = accel::intersectAll(unbounded_, ray, isect);
    if (prims_.empty()) return hit;

    const Vec3 invDir = accel::inverse(ray.direction);
    float tEnter, tExit;
    if (!accel::clipRay(bounds_, ray, invDir, tEnter, tExit)) return hit;

    // Starting voxel and DDA setup.
    const Vec3 start = ray.origin + ray.direction * tEnter;
    int cell[3], step[3], stop[3];
    float tNext[3], tDelta[3];
    for (int a = 0; a < 3; ++a) {
        cell[a] = std::clamp(static_cast<int>((start[a] - bounds_.pMin[a]) / cellSize_[a]), 0, res_[a] - 1);
        if (ray.direction[a] > 0.0f) {
            step[a] = 1;
            stop[a] = res_[a];
            tNext[a] = tEnter + (bounds_.pMin[a] + (cell[a] + 1) * cellSize_[a] - start[a]) * invDir[a];
            tDelta[a] = cellSize_[a] * invDir[a];
        } else if (ray.direction[a] < 0.0f) {
            step[a] = -1;
            stop[a] = -1;
            tNext[a] = tEnter + (bounds_.pMin[a] + cell[a] * cellSize_[a] - start[a]) * invDir[a];
            tDelta[a] = -cellSize_[a] * invDir[a];
        } else {
            step[a] = 0;
            stop[a] = -2;
            tNext[a] = std::numeric_limits<float>::infinity();
            tDelta[a] = std::numeric_limits<float>::infinity();
        }
    }

    while (true) {
        const int c = cellIndex(cell[0], cell[1], cell[2]);
        for (uint32_t k = cellStart_[c]; k < cellStart_[c + 1]; ++k) {
            if (prims_[cellPrims_[k]]->intersect(ray, isect)) hit = true;
        }

        // Axis whose voxel boundary is crossed first.
        const int axis = (tNext[0] < tNext[1]) ? ((tNext[0] < tNext[2]) ? 0 : 2)
                                               : ((tNext[1] < tNext[2]) ? 1 : 2);

        // A hit inside the current voxel cannot be beaten by later voxels.
        if (ray.tMax <= tNext[axis]) break;

        cell[axis] += step[axis];
        if (cell[axis] == stop[axis]) break;
        tNext[axis] += tDelta[axis];
    }
    return hit;
}

bool UniformGrid::intersectP(const Ray& ray) const {
    if (accel::intersectAnyP(unbounded_, ray)) return true;
    if (prims_.empty()) return false;

    const Vec3 invDir = accel::inverse(ray.direction);
    float tEnter, tExit;
    if (!accel::clipRay(bounds_, ray, invDir, tEnter, tExit)) return false;

    const Vec3 start = ray.origin + ray.direction * tEnter;
    int cell[3], step[3], stop[3];
    float tNext[3], tDelta[3];
    for (int a = 0; a < 3; ++a) {
        cell[a] = std::clamp(static_cast<int>((start[a] - bounds_.pMin[a]) / cellSize_[a]), 0, res_[a] - 1);
        if (ray.direction[a] > 0.0f) {
            step[a] = 1;
            stop[a] = res_[a];
            tNext[a] = tEnter + (bounds_.pMin[a] + (cell[a] + 1) * cellSize_[a] - start[a]) * invDir[a];
            tDelta[a] = cellSize_[a] * invDir[a];
        } else if (ray.direction[a] < 0.0f) {
            step[a] = -1;
            stop[a] = -1;
            tNext[a] = tEnter + (bounds_.pMin[a] + cell[a] * cellSize_[a] - start[a]) * invDir[a];
            tDelta[a] = -cellSize_[a] * invDir[a];
        } else {
            step[a] = 0;
            stop[a] = -2;
            tNext[a] = std::numeric_limits<float>::infinity();
            tDelta[a] = std::numeric_limits<float>::infinity();
        }
    }

    while (true) {
        const int c = cellIndex(cell[0], cell[1], cell[2]);
        for (uint32_t k = cellStart_[c]; k < cellStart_[c + 1]; ++k) {
            if (prims_[cellPrims_[k]]->intersectP(ray)) return true;
        }

        const int axis = (tNext[0] < tNext[1]) ? ((tNext[0] < tNext[2]) ? 0 : 2)
                                               : ((tNext[1] < tNext[2]) ? 1 : 2);
        if (ray.tMax <= tNext[axis]) break;
        cell[axis] += step[axis];
        if (cell[axis] == stop[axis]) break;
        tNext[axis] += tDelta[axis];
    }
    return false;
}

BoundingBox UniformGrid::worldBounds() const {
    return prims_.empty() ? BoundingBox() : bounds_;
}

} // namespace render

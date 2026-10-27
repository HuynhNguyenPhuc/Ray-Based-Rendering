// Every accelerator must return exactly what a brute-force loop over the primitives returns.

#include "render/acceleration/accelerator_factory.h"
#include "render/geometry/plane.h"
#include "render/geometry/sphere.h"
#include "render/geometry/triangle.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

using namespace render;

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        std::cerr << "FAIL: " << what << "\n";
        ++failures;
    }
}

struct World {
    std::vector<std::unique_ptr<Primitive>> owned;
    std::vector<Primitive*> prims;

    void add(std::unique_ptr<Primitive> p) {
        prims.push_back(p.get());
        owned.push_back(std::move(p));
    }
};

World makeWorld(std::mt19937& rng, int nSpheres, int nTriangles, bool withPlane) {
    std::uniform_real_distribution<float> pos(-10.0f, 10.0f), size(0.1f, 1.5f);
    World w;
    for (int i = 0; i < nSpheres; ++i) {
        w.add(std::make_unique<Sphere>(Vec3(pos(rng), pos(rng), pos(rng)), size(rng)));
    }
    for (int i = 0; i < nTriangles; ++i) {
        Vec3 c(pos(rng), pos(rng), pos(rng));
        auto jitter = [&]() { return Vec3(size(rng), size(rng), size(rng)) - Vec3(0.8f); };
        w.add(std::make_unique<Triangle>(c + jitter(), c + jitter(), c + jitter()));
    }
    // Axis-aligned (flat) quads-as-triangles stress zero-thickness bounds.
    for (int i = 0; i < 20; ++i) {
        float z = pos(rng), x = pos(rng), y = pos(rng);
        w.add(std::make_unique<Triangle>(Vec3(x, y, z), Vec3(x + 2, y, z), Vec3(x, y + 2, z)));
    }
    if (withPlane) w.add(std::make_unique<Plane>(Vec3(0, 1, 0), 12.0f));
    return w;
}

// Reference: closest hit by testing everything.
bool bruteForce(const std::vector<Primitive*>& prims, const Ray& ray, float& tHit) {
    Ray r = ray;
    SurfaceInteraction isect;
    bool hit = false;
    for (const Primitive* p : prims) {
        if (p->intersect(r, isect)) hit = true;
    }
    tHit = hit ? isect.t : 0.0f;
    return hit;
}

bool bruteForceP(const std::vector<Primitive*>& prims, const Ray& ray) {
    for (const Primitive* p : prims) {
        if (p->intersectP(ray)) return true;
    }
    return false;
}

void compare(AcceleratorType type, const World& world, std::mt19937& rng, int nRays,
             const char* label) {
    auto accel = createAccelerator(type);
    accel->build(world.prims);

    std::uniform_real_distribution<float> pos(-14.0f, 14.0f), dir(-1.0f, 1.0f), len(0.5f, 40.0f);
    int mismatches = 0, mismatchesP = 0, hits = 0;

    for (int i = 0; i < nRays; ++i) {
        Vec3 o(pos(rng), pos(rng), pos(rng));
        Vec3 d(dir(rng), dir(rng), dir(rng));
        // A third of the rays are axis-aligned (zero direction components).
        if (i % 3 == 0) d = Vec3(0.0f, 0.0f, (i % 2) ? 1.0f : -1.0f);
        else if (i % 7 == 0) d = Vec3(dir(rng), 0.0f, dir(rng));
        if (d.length() < 1e-3f) continue;

        Ray ray(o, d);
        if (i % 4 == 0) ray.tMax = len(rng);  // finite shadow-style segments

        float tRef = 0.0f;
        const bool refHit = bruteForce(world.prims, ray, tRef);
        hits += refHit;

        Ray r1 = ray;
        SurfaceInteraction isect;
        const bool hit = accel->intersect(r1, isect);
        if (hit != refHit || (hit && std::fabs(isect.t - tRef) > 1e-3f * std::max(1.0f, tRef))) {
            ++mismatches;
        }
        if (accel->intersectP(ray) != bruteForceP(world.prims, ray)) ++mismatchesP;
    }
    check(mismatches == 0, std::string(acceleratorName(type)) + " closest hit (" + label + "): " +
                               std::to_string(mismatches) + " mismatches");
    check(mismatchesP == 0, std::string(acceleratorName(type)) + " any hit (" + label + "): " +
                                std::to_string(mismatchesP) + " mismatches");
    check(hits > nRays / 20, std::string("test rays hit nothing (") + label + ")");
}

} // namespace

int main() {
    std::mt19937 rng(12345);

    for (AcceleratorType type : kAllAcceleratorTypes) {
        // empty and single-primitive structures
        {
            auto empty = createAccelerator(type);
            empty->build({});
            Ray ray(Vec3(0, 0, 5), Vec3(0, 0, -1));
            SurfaceInteraction isect;
            check(!empty->intersect(ray, isect), std::string(acceleratorName(type)) + " empty hit");
            check(!empty->intersectP(ray), std::string(acceleratorName(type)) + " empty hitP");
            check(empty->worldBounds().isEmpty(), std::string(acceleratorName(type)) + " empty bounds");

            World one;
            one.add(std::make_unique<Sphere>(Vec3(0, 0, 0), 1.0f));
            auto single = createAccelerator(type);
            single->build(one.prims);
            Ray r2(Vec3(0, 0, 5), Vec3(0, 0, -1));
            check(single->intersect(r2, isect) && std::fabs(isect.t - 4.0f) < 1e-3f,
                  std::string(acceleratorName(type)) + " single sphere");
        }

        World small = makeWorld(rng, 30, 30, false);
        compare(type, small, rng, 3000, "small");

        World big = makeWorld(rng, 400, 1500, false);
        compare(type, big, rng, 6000, "large");

        World withPlane = makeWorld(rng, 60, 200, true);
        compare(type, withPlane, rng, 3000, "with infinite plane");

        // A rebuild must replace the previous contents.
        auto accel = createAccelerator(type);
        accel->build(big.prims);
        accel->build(small.prims);
        Ray probe(Vec3(0, 0, 50), Vec3(0, 0, -1));
        SurfaceInteraction isect;
        float tRef = 0.0f;
        const bool refHit = bruteForce(small.prims, probe, tRef);
        const bool hit = accel->intersect(probe, isect);
        check(hit == refHit && (!hit || std::fabs(isect.t - tRef) < 1e-3f),
              std::string(acceleratorName(type)) + " rebuild");
    }

    if (failures == 0) std::cout << "All accelerator tests passed\n";
    return failures == 0 ? 0 : 1;
}

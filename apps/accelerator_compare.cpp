// Compares the spatial acceleration structures (BVH, uniform grid, kd-tree, octree) on a mesh.
//
// Usage: render_accel_compare [mesh.obj] [-r RAYS] [--plane]
//
// For each structure it prints the build time, the time to trace RAYS primary rays
// (closest hit) and RAYS shadow rays (any hit), and checks the answers against the BVH.

#include "render/acceleration/accelerator_factory.h"
#include "render/geometry/mesh.h"
#include "render/geometry/plane.h"
#include "render/io/obj_importer.h"

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

using namespace render;

namespace {

using Clock = std::chrono::steady_clock;

double secondsSince(Clock::time_point t0) {
    return std::chrono::duration<double>(Clock::now() - t0).count();
}

} // namespace

int main(int argc, char** argv) {
    std::string path = "assets/motocompo.obj";
    int nRays = 200000;
    bool withPlane = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "-r" || arg == "--rays") && i + 1 < argc) nRays = std::atoi(argv[++i]);
        else if (arg == "--plane") withPlane = true;
        else if (arg == "--help") {
            std::cout << "Usage: render_accel_compare [mesh.obj] [-r RAYS] [--plane]\n"
                         "  --plane  add an infinite ground plane (unbounded primitive)\n";
            return 0;
        }
        else path = arg;
    }

    TriangleMesh mesh;
    std::string error;
    if (!ObjImporter::load(path, mesh, &error)) {
        std::cerr << "Failed to load " << path << ": " << error << "\n";
        return 1;
    }
    auto owned = mesh.toPrimitives(nullptr);
    std::vector<Primitive*> prims;
    for (auto& p : owned) prims.push_back(p.get());

    const BoundingBox box = mesh.bounds();
    if (withPlane) {
        owned.push_back(std::make_unique<Plane>(Vec3(0, 1, 0), -box.pMin.y));
        prims.push_back(owned.back().get());
    }
    const Vec3 center = box.center();
    const float radius = 0.5f * box.diagonal().length();

    std::cout << "Mesh: " << path << " (" << mesh.triangleCount() << " triangles"
              << (withPlane ? " + ground plane" : "") << ")\n";
    std::cout << "Rays: " << nRays << " primary + " << nRays << " shadow\n\n";

    // Primary rays: from random points on a sphere around the mesh towards random
    // points inside its box. Shadow rays: random points inside the box towards a light.
    std::mt19937 rng(7);
    std::uniform_real_distribution<float> u01(0.0f, 1.0f), n11(-1.0f, 1.0f);
    std::vector<Ray> primary, shadow;
    primary.reserve(nRays);
    shadow.reserve(nRays);
    const Vec3 light = center + Vec3(0.3f, 1.0f, 0.2f) * (3.0f * radius);
    for (int i = 0; i < nRays; ++i) {
        Vec3 d(n11(rng), n11(rng), n11(rng));
        if (d.length() < 1e-3f) d = Vec3(0, 0, 1);
        Vec3 from = center + d.normalized() * (2.0f * radius);
        Vec3 to(box.pMin.x + u01(rng) * (box.pMax.x - box.pMin.x),
                box.pMin.y + u01(rng) * (box.pMax.y - box.pMin.y),
                box.pMin.z + u01(rng) * (box.pMax.z - box.pMin.z));
        primary.emplace_back(from, to - from);

        Vec3 p(box.pMin.x + u01(rng) * (box.pMax.x - box.pMin.x),
               box.pMin.y + u01(rng) * (box.pMax.y - box.pMin.y),
               box.pMin.z + u01(rng) * (box.pMax.z - box.pMin.z));
        Vec3 toLight = light - p;
        Ray s(p, toLight);
        s.tMax = toLight.length() * 0.999f;
        shadow.push_back(s);
    }

    std::vector<float> reference(nRays);  // BVH closest-hit distance, -1 for a miss
    std::vector<char> referenceP(nRays);

    std::cout << std::left << std::setw(8) << "name" << std::right << std::setw(12) << "build [ms]"
              << std::setw(14) << "closest [s]" << std::setw(12) << "Mrays/s" << std::setw(12)
              << "shadow [s]" << std::setw(12) << "Mrays/s" << std::setw(12) << "mismatch" << "\n";

    for (AcceleratorType type : kAllAcceleratorTypes) {
        auto accel = createAccelerator(type);

        auto t0 = Clock::now();
        accel->build(prims);
        const double build = secondsSince(t0);

        std::vector<float> result(nRays);
        t0 = Clock::now();
        for (int i = 0; i < nRays; ++i) {
            Ray r = primary[i];
            SurfaceInteraction isect;
            result[i] = accel->intersect(r, isect) ? isect.t : -1.0f;
        }
        const double closest = secondsSince(t0);

        std::vector<char> resultP(nRays);
        t0 = Clock::now();
        for (int i = 0; i < nRays; ++i) resultP[i] = accel->intersectP(shadow[i]) ? 1 : 0;
        const double any = secondsSince(t0);

        int mismatches = 0;
        if (type == AcceleratorType::BVH) {
            reference = result;
            referenceP = resultP;
        } else {
            for (int i = 0; i < nRays; ++i) {
                const bool sameHit = (result[i] < 0.0f) == (reference[i] < 0.0f);
                if (!sameHit || (result[i] >= 0.0f &&
                                 std::fabs(result[i] - reference[i]) > 1e-3f * std::max(1.0f, reference[i]))) {
                    ++mismatches;
                }
                if (resultP[i] != referenceP[i]) ++mismatches;
            }
        }

        std::cout << std::left << std::setw(8) << acceleratorName(type) << std::right << std::fixed
                  << std::setprecision(1) << std::setw(12) << build * 1000.0 << std::setprecision(3)
                  << std::setw(14) << closest << std::setprecision(2) << std::setw(12)
                  << nRays / closest / 1e6 << std::setprecision(3) << std::setw(12) << any
                  << std::setprecision(2) << std::setw(12) << nRays / any / 1e6 << std::setw(12)
                  << mismatches << "\n";
    }
    return 0;
}

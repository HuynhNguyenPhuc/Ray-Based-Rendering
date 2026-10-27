// Checks for the core module: interactions, bounding boxes, sampling helpers and the film.

#include "render/core/bbox.h"
#include "render/core/film.h"
#include "render/core/interaction.h"
#include "render/core/sampler.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <string>

using namespace render;

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        std::cerr << "FAIL: " << what << "\n";
        ++failures;
    }
}

bool near(float a, float b, float tol = 1e-5f) { return std::abs(a - b) <= tol; }

void testOutwardNormal() {
    // A ray hitting a surface from the front and from the back (e.g. from inside a sphere).
    SurfaceInteraction front, back;
    const Vec3 outward(0.0f, 1.0f, 0.0f);
    front.setFaceNormal(Ray(Vec3(0, 1, 0), Vec3(0, -1, 0)), outward);
    back.setFaceNormal(Ray(Vec3(0, -1, 0), Vec3(0, 1, 0)), outward);

    check(front.frontFace && !back.frontFace, "frontFace flag");
    check(near(front.normal.y, 1.0f) && near(back.normal.y, -1.0f), "normal faces the ray");
    check(near(front.outwardNormal().y, 1.0f) && near(back.outwardNormal().y, 1.0f),
          "outwardNormal() is the same from both sides");

    // In the frame of the outward normal, wo.z tells which side the ray is on.
    const sampling::Frame f(back.outwardNormal());
    check(f.toLocal(Vec3(0, -1, 0)).z < 0.0f, "wo.z < 0 for a ray inside the object");
}

void testFrame() {
    const Vec3 normals[] = {Vec3(0, 0, 1), Vec3(0, 1, 0), Vec3(1, 0, 0), Vec3(0, 0, -1),
                            Vec3(1, 1, 1).normalized(), Vec3(-0.3f, 0.2f, -0.9f).normalized()};
    for (const Vec3& n : normals) {
        const sampling::Frame f(n);
        check(near(f.tangent.length(), 1.0f) && near(f.bitangent.length(), 1.0f), "frame axes are unit");
        check(near(dot(f.tangent, f.bitangent), 0.0f) && near(dot(f.tangent, n), 0.0f), "frame is orthogonal");
        const Vec3 v = Vec3(0.3f, -0.5f, 0.8f).normalized();
        const Vec3 back = f.toWorld(f.toLocal(v));
        check((back - v).length() < 1e-5f, "toWorld(toLocal(v)) == v");
        check(near(f.toLocal(n).z, 1.0f), "the normal maps to +z");
    }
}

void testBoundingBox() {
    const BoundingBox box(Vec3(-1.0f), Vec3(1.0f));
    float t0, t1;

    check(box.intersect(Ray(Vec3(0, 0, 5), Vec3(0, 0, -1)), t0, t1) && near(t0, 4.0f) && near(t1, 6.0f),
          "ray along -z");
    check(!box.intersect(Ray(Vec3(2, 0, 5), Vec3(0, 0, -1)), t0, t1), "parallel ray outside the slab misses");
    // Origin exactly on the slab boundary and parallel to it: must not turn into NaN and miss.
    check(box.intersect(Ray(Vec3(1, 0, 5), Vec3(0, 0, -1)), t0, t1), "parallel ray on the slab boundary hits");
    check(box.intersect(Ray(Vec3(0, 0, 0), Vec3(0, 1, 0)), t0, t1) && near(t0, 0.0f) && near(t1, 1.0f),
          "ray starting inside");

    Ray shortRay(Vec3(0, 0, 5), Vec3(0, 0, -1), 3.0f);
    check(!box.intersect(shortRay, t0, t1), "ray shorter than the distance to the box");

    BoundingBox empty;
    check(empty.isEmpty(), "default box is empty");
    empty.expand(Vec3(1, 2, 3));
    check(empty.isValid() && near(empty.pMin.y, 2.0f), "expanding an empty box");
}

void testSampling() {
    // MIS weights stay finite for extreme densities.
    const float inf = std::numeric_limits<float>::infinity();
    check(near(sampling::powerHeuristic(1, 1.0f, 1, 1.0f), 0.5f), "power heuristic of equal pdfs");
    check(sampling::powerHeuristic(1, inf, 1, 1.0f) == 1.0f, "power heuristic, infinite f pdf");
    check(sampling::powerHeuristic(1, 1.0f, 1, inf) == 0.0f, "power heuristic, infinite g pdf");
    check(sampling::powerHeuristic(1, 0.0f, 1, 0.0f) == 0.0f, "power heuristic, both pdfs zero");
    check(!std::isnan(sampling::powerHeuristic(1, 1e30f, 1, 1e30f)), "power heuristic, huge pdfs");

    UniformSampler a(42), b(42);
    check(a.get1D() == b.get1D(), "samplers with the same seed agree");
    const Vec3 d = sampling::cosineHemisphere(0.3f, 0.7f);
    check(near(d.length(), 1.0f) && d.z >= 0.0f, "cosine hemisphere direction");
}

void testFilm() {
    Film film(2, 1);
    film.addSample(0, 0, Color(0.5f, 1.0f, 0.0f));
    film.addSample(5, 5, Color(1.0f));  // outside: ignored
    const auto data = film.toRGB8(1.0f);  // gamma 1: values map straight to 0..255
    check(data.size() == 6, "pixel buffer size");
    check(data[0] == 128, "0.5 rounds to 128, not truncates to 127");
    check(data[1] == 255 && data[2] == 0, "full and zero channels");
    check(data[3] == 0 && data[4] == 0 && data[5] == 0, "untouched pixel is black");
}

} // namespace

int main() {
    testOutwardNormal();
    testFrame();
    testBoundingBox();
    testSampling();
    testFilm();

    if (failures == 0) std::cout << "All core tests passed\n";
    else std::cerr << failures << " core check(s) failed\n";
    return failures == 0 ? 0 : 1;
}

// Sanity checks for the camera module (extrinsics, intrinsics, projection, pixel convention).

#include "render/camera/environment_camera.h"
#include "render/camera/orthographic_camera.h"
#include "render/camera/pinhole_camera.h"
#include "render/camera/thin_lens_camera.h"

#include <cmath>
#include <iostream>

using namespace render;

namespace {

int failures = 0;

void check(bool ok, const char* what) {
    if (!ok) {
        std::cerr << "FAIL: " << what << "\n";
        ++failures;
    }
}

bool near(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) <= eps; }
bool near(const Vec3& a, const Vec3& b, float eps = 1e-4f) {
    return near(a.x, b.x, eps) && near(a.y, b.y, eps) && near(a.z, b.z, eps);
}

} // namespace

int main() {
    const int W = 200, H = 100;

    // Pinhole: the centre of the image looks at the target.
    PinholeCamera cam(W, H, Vec3(0, 1, 5), Vec3(0, 1, 0), Vec3(0, 1, 0), 40.0f);
    check(near(cam.generatePixelRay(W / 2, H / 2).direction, Vec3(0, 0, -1), 0.01f),
          "pixel next to the image centre looks (almost) along forward");

    // Pixel convention: continuous raster coordinates, pixel centre = index + 0.5.
    CameraSample centre{W / 2.0f, H / 2.0f, 0.5f, 0.5f, 0.0f};
    check(near(cam.generateRay(centre).direction, Vec3(0, 0, -1)), "film centre (W/2, H/2) is the principal ray");

    // Vertical field of view: the top edge of the film makes fov/2 with the viewing axis.
    const Vec3 top = cam.generateRay(CameraSample{W / 2.0f, 0.0f, 0.5f, 0.5f, 0.0f}).direction;
    check(near(std::acos(dot(top, Vec3(0, 0, -1))) * 180.0f / PI, 20.0f, 1e-2f), "vertical fov is respected");
    check(near(cam.fov(), 40.0f, 1e-2f), "fov() round-trips through the intrinsics");

    // project() inverts generateRay().
    for (float px : {10.5f, 100.0f, 187.25f}) {
        for (float py : {3.5f, 50.0f, 95.75f}) {
            const Ray r = cam.generateRay(CameraSample{px, py, 0.5f, 0.5f, 0.0f});
            float qx = 0, qy = 0;
            check(cam.project(r.origin + r.direction * 3.0f, qx, qy) && near(qx, px, 1e-2f) && near(qy, py, 1e-2f),
                  "project(generateRay) round-trips");
        }
    }
    float dx, dy;
    check(!cam.project(Vec3(0, 1, 10), dx, dy), "points behind the camera are not projected");

    // Extrinsics: lookAt basis is orthonormal, also when up is parallel to the view direction.
    for (const Vec3& up : {Vec3(0, 1, 0), Vec3(0, 0, 1)}) {
        CameraExtrinsics e = CameraExtrinsics::lookAt(Vec3(0, 5, 0), Vec3(0, 0, 0), up);
        check(near(e.forward.length(), 1.0f) && near(e.right.length(), 1.0f) && near(e.up.length(), 1.0f),
              "lookAt basis is normalized (no NaN)");
        check(near(dot(e.forward, e.right), 0.0f) && near(dot(e.forward, e.up), 0.0f) && near(dot(e.right, e.up), 0.0f),
              "lookAt basis is orthogonal");
    }

    // Thin lens: all rays through one pixel meet on the focal plane; lensRadius 0 equals the pinhole.
    ThinLensCamera lens(W, H, Vec3(0, 1, 5), Vec3(0, 1, 0), Vec3(0, 1, 0), 40.0f, 0.3f, 4.0f);
    const CameraSample s0{150.5f, 20.5f, 0.1f, 0.2f, 0.0f}, s1{150.5f, 20.5f, 0.9f, 0.7f, 0.0f};
    const Ray a = lens.generateRay(s0), b = lens.generateRay(s1);
    check(!near(a.origin, b.origin), "lens samples start at different points");
    const Vec3 fa = a.origin + a.direction * (4.0f / std::fabs(a.direction.z));
    const Vec3 fb = b.origin + b.direction * (4.0f / std::fabs(b.direction.z));
    check(near(fa, fb, 1e-3f), "lens rays converge on the focal plane");
    ThinLensCamera noLens(W, H, Vec3(0, 1, 5), Vec3(0, 1, 0), Vec3(0, 1, 0), 40.0f, 0.0f, 4.0f);
    check(near(noLens.generateRay(s0).direction, cam.generateRay(s0).direction), "lensRadius 0 equals pinhole");

    // Orthographic: parallel rays, film centre on the axis, half-height = scale.
    OrthographicCamera ortho(W, H, Vec3(0, 1, 5), Vec3(0, 1, 0), Vec3(0, 1, 0), 2.0f);
    const Ray o = ortho.generateRay(centre);
    check(near(o.origin, Vec3(0, 1, 5)) && near(o.direction, Vec3(0, 0, -1)), "orthographic centre ray");
    const Ray oTop = ortho.generateRay(CameraSample{W / 2.0f, 0.0f, 0.5f, 0.5f, 0.0f});
    check(near(oTop.origin, Vec3(0, 3, 5)) && near(oTop.direction, Vec3(0, 0, -1)),
          "orthographic top edge is `scale` above the axis");
    check(near(ortho.scale(), 2.0f), "scale() round-trips through the intrinsics");
    for (float px : {10.5f, 100.0f}) {
        const Ray r = ortho.generateRay(CameraSample{px, 40.25f, 0.5f, 0.5f, 0.0f});
        float qx = 0, qy = 0;
        check(ortho.project(r.origin + r.direction * 2.0f, qx, qy) && near(qx, px, 1e-2f) && near(qy, 40.25f, 1e-2f),
              "orthographic project(generateRay) round-trips");
    }

    // Environment: honours orientation (poles follow the extrinsics' up axis).
    EnvironmentCamera env(W, H, CameraExtrinsics::lookAt(Vec3(0, 0, 0), Vec3(1, 0, 0), Vec3(0, 0, 1)));
    const Ray up = env.generateRay(CameraSample{0.0f, 0.0f, 0.5f, 0.5f, 0.0f});
    check(up.direction.z > 0.99f, "environment camera pole follows camera up");
    for (float px : {5.5f, 60.0f, 150.5f}) {
        for (float py : {10.5f, 50.0f, 90.0f}) {
            const Ray r = env.generateRay(CameraSample{px, py, 0.5f, 0.5f, 0.0f});
            float qx = 0, qy = 0;
            check(env.project(r.origin + r.direction * 2.0f, qx, qy) && near(qx, px, 5e-2f) && near(qy, py, 5e-2f),
                  "environment project(generateRay) round-trips");
        }
    }

    // Every camera exposes its intrinsics through the base class.
    const Camera* cameras[] = {&cam, &lens, &ortho, &env};
    for (const Camera* c : cameras) {
        const CameraSpaceRay r = c->intrinsics().generateRay(W / 2.0f, H / 2.0f);
        check(r.direction.length() > 0.0f, "intrinsics().generateRay returns a direction");
    }

    if (failures == 0) std::cout << "camera tests passed\n";
    return failures == 0 ? 0 : 1;
}

// Example: render one sphere with a given set of material parameters in a small studio scene.
// Used by docs/figures/materials/make_material_figures.py to produce the figures of docs/MATERIAL.md.
//
// usage: render_material_cell out.png r g b metallic roughness ior transmission er eg eb [size] [spp]
#include "render/camera/pinhole_camera.h"
#include "render/core/film.h"
#include "render/io/image_exporter.h"
#include "render/renderers/path_tracing_integrator.h"
#include "render/scene/scene_builder.h"
#include <cstdlib>
#include <string>
using namespace render;

int main(int argc, char** argv) {
    if (argc < 12) return 1;
    auto f = [&](int i) { return (float)std::atof(argv[i]); };
    const int size = argc > 12 ? std::atoi(argv[12]) : 200;
    const int spp = argc > 13 ? std::atoi(argv[13]) : 96;

    SceneBuilder b;
    auto mat = std::make_unique<Material>();
    mat->baseColor = Color(f(2), f(3), f(4));
    mat->metallic = f(5);
    mat->roughness = f(6);
    mat->ior = f(7);
    mat->transmission = f(8);
    mat->emission = Color(f(9), f(10), f(11));
    Material* sphereMat = b.material(std::move(mat));

    // Floor and a striped backdrop give the reflections and refractions something to show.
    b.plane(Vec3(0, 0, 0), Vec3(0, 1, 0), b.diffuse(Color(0.55f)));
    Material* white = b.diffuse(Color(0.85f));
    Material* blue = b.diffuse(Color(0.12f, 0.25f, 0.65f));
    Material* orange = b.diffuse(Color(0.85f, 0.4f, 0.1f));
    for (int i = -6; i < 6; ++i) {
        Material* m = (i % 3 == 0) ? blue : (i % 3 == 1 || i % 3 == -2) ? orange : white;
        b.quad(Vec3(i * 0.7f, 0.0f, -2.5f), Vec3(0.7f, 0, 0), Vec3(0, 4.0f, 0), m);
    }
    b.sphere(Vec3(0, 1.0f, 0), 1.0f, sphereMat);

    b.areaLight(Vec3(-2.0f, 5.0f, 2.0f), Vec3(2.0f, 0, 0), Vec3(0, 0, 2.0f), Color(1.0f, 0.97f, 0.92f), 14.0f);
    b.pointLight(Vec3(3.0f, 3.0f, 4.0f), Color(0.9f, 0.95f, 1.0f), 25.0f);
    b.background(Color(0.55f, 0.65f, 0.8f));

    auto scenePtr = b.build();
    Scene& scene = *scenePtr;
    PinholeCamera camera(size, size, Vec3(0, 1.45f, 5.2f), Vec3(0, 0.95f, 0), Vec3(0, 1, 0), 36.0f);
    Film film(size, size);

    IntegratorConfig cfg;
    cfg.maxDepth = 10;
    cfg.samplesPerPixel = spp;
    cfg.useNEE = true;
    cfg.useMIS = true;
    PathTracingIntegrator integrator(cfg);
    integrator.render(scene, camera, film, nullptr);

    ImageExporter::writeRGB8(argv[1], size, size, film.toRGB8ToneMapped(1.2f, 2.2f));
    return 0;
}

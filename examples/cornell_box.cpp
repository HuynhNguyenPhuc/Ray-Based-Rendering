// Example: Classic Cornell Box scene

#include "render/core/common.h"
#include "render/core/vector.h"
#include "render/camera/pinhole_camera.h"
#include "render/core/film.h"
#include "render/core/sampler.h"

#include "render/materials/material.h"
#include "render/geometry/plane.h"
#include "render/geometry/quad.h"
#include "render/geometry/sphere.h"
#include "render/lighting/area_light.h"
#include "render/lighting/directional_light.h"
#include "render/lighting/point_light.h"
#include "render/scene/scene.h"
#include "render/scene/scene_builder.h"
#include "render/io/image_exporter.h"

#include "render/renderers/path_tracing_integrator.h"


#include <iostream>
#include <chrono>

using namespace render;

int main() {
    // Configuration
    const int width = 600;
    const int height = 600;
    const int samples = 256;
    
    std::cout << "=== Cornell Box Example ===" << std::endl;
    std::cout << "Resolution: " << width << "x" << height << std::endl;
    std::cout << "Samples: " << samples << std::endl;
    
    // Build Cornell Box
    SceneBuilder builder;
    
    // Materials
    auto white = builder.diffuse(Color(0.73f, 0.73f, 0.73f));
    auto red = builder.diffuse(Color(0.65f, 0.05f, 0.05f));
    auto green = builder.diffuse(Color(0.12f, 0.45f, 0.15f));
    auto light = builder.emissive(Color(17.0f, 12.0f, 4.0f));
    
    auto glass = builder.glass(1.5f, Color(1.0f));
    auto mirror = builder.mirror(Color(0.95f));
    
    // Box dimensions
    const float boxSize = 1.0f;
    
    // Floor
    builder.quad(
        Vec3(-boxSize, 0.0f, -boxSize),
        Vec3(2.0f * boxSize, 0.0f, 0.0f),
        Vec3(0.0f, 0.0f, 2.0f * boxSize),
        white
    );
    
    // Ceiling
    builder.quad(
        Vec3(-boxSize, 2.0f * boxSize, -boxSize),
        Vec3(2.0f * boxSize, 0.0f, 0.0f),
        Vec3(0.0f, 0.0f, 2.0f * boxSize),
        white
    );
    
    // Back wall
    builder.quad(
        Vec3(-boxSize, 0.0f, -boxSize),
        Vec3(2.0f * boxSize, 0.0f, 0.0f),
        Vec3(0.0f, 2.0f * boxSize, 0.0f),
        white
    );
    
    // Left wall (red)
    builder.quad(
        Vec3(-boxSize, 0.0f, -boxSize),
        Vec3(0.0f, 0.0f, 2.0f * boxSize),
        Vec3(0.0f, 2.0f * boxSize, 0.0f),
        red
    );
    
    // Right wall (green)
    builder.quad(
        Vec3(boxSize, 0.0f, -boxSize),
        Vec3(0.0f, 0.0f, 2.0f * boxSize),
        Vec3(0.0f, 2.0f * boxSize, 0.0f),
        green
    );
    
    // Light on ceiling
    builder.areaLight(
        Vec3(-0.235f, 1.98f, -0.235f),
        Vec3(0.47f, 0.0f, 0.0f),
        Vec3(0.0f, 0.0f, 0.47f),
        Color(1.0f, 1.0f, 1.0f),
        17.0f
    );
    
    // Spheres
    builder.sphere(Vec3(-0.4f, 0.35f, 0.1f), 0.35f, glass);
    builder.sphere(Vec3(0.35f, 0.25f, 0.35f), 0.25f, mirror);
    
    auto scene_ptr = builder.build();
    Scene scene = std::move(*scene_ptr);
    scene.ambientLight = Color(0.0f);
    scene.backgroundColor = Color(0.0f);
    
    // Camera
    PinholeCamera camera(
        width, height,
        Vec3(0.0f, 1.0f, 3.5f),   // position
        Vec3(0.0f, 0.8f, 0.0f),   // lookAt
        Vec3(0.0f, 1.0f, 0.0f),   // up
        40.0f                    // fov
    );
    
    // Film
    Film film(width, height);
    
    // Integrator
    IntegratorConfig intConfig;
    intConfig.maxDepth = 10;
    intConfig.samplesPerPixel = samples;
    intConfig.russianRoulette = true;
    intConfig.useNEE = true;
    intConfig.useMIS = true;
    
    PathTracingIntegrator integrator(intConfig);
    
    // Render
    std::cout << "\nRendering..." << std::endl;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    integrator.render(scene, camera, film, [](float p) {
        int bar = static_cast<int>(p * 40);
        std::cout << "\r[";
        for (int i = 0; i < 40; ++i) std::cout << (i < bar ? '#' : ' ');
        std::cout << "] " << static_cast<int>(p * 100) << "%" << std::flush;
    });
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "\n\nRender time: " << duration.count() / 1000.0f << " seconds" << std::endl;
    
    // Save
    auto pixels = film.toRGB8ToneMapped(1.5f, 2.2f);
    ImageExporter::writeRGB8("results/cornell_box.png", width, height, pixels);
    std::cout << "Saved to: results/cornell_box.png" << std::endl;
    
    return 0;
}

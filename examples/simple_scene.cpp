// Example: Simple scene with multiple spheres

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
    const int width = 800;
    const int height = 600;
    const int samples = 64;
    
    std::cout << "=== Simple Scene Example ===" << std::endl;
    std::cout << "Resolution: " << width << "x" << height << std::endl;
    std::cout << "Samples: " << samples << std::endl;
    
    // Build scene
    SceneBuilder builder;
    
    // Ground plane
    auto groundMat = builder.diffuse(Color(0.5f, 0.5f, 0.5f));
    builder.plane(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), groundMat);
    
    // Materials
    auto redMat = builder.diffuse(Color(0.8f, 0.1f, 0.1f));
    auto greenMat = builder.diffuse(Color(0.1f, 0.8f, 0.1f));
    auto blueMat = builder.diffuse(Color(0.1f, 0.1f, 0.8f));
    auto mirrorMat = builder.mirror(Color(0.95f));
    auto glassMat = builder.glass(1.5f, Color(1.0f));
    auto goldMat = builder.diffuse(Color(0.8f, 0.6f, 0.2f));
    
    // Arrange spheres in a line
    builder.sphere(Vec3(-3.0f, 0.5f, 0.0f), 0.5f, redMat);
    builder.sphere(Vec3(-1.5f, 0.5f, 0.0f), 0.5f, greenMat);
    builder.sphere(Vec3(0.0f, 0.5f, 0.0f), 0.5f, mirrorMat);
    builder.sphere(Vec3(1.5f, 0.5f, 0.0f), 0.5f, glassMat);
    builder.sphere(Vec3(3.0f, 0.5f, 0.0f), 0.5f, blueMat);
    
    // Large sphere in the back
    builder.sphere(Vec3(0.0f, 1.5f, -3.0f), 1.5f, goldMat);
    
    // Lights
    builder.pointLight(Vec3(-5.0f, 10.0f, 5.0f), Color(1.0f, 0.95f, 0.9f), 150.0f);
    builder.pointLight(Vec3(5.0f, 8.0f, 5.0f), Color(0.9f, 0.95f, 1.0f), 100.0f);
    builder.directionalLight(Vec3(0.0f, -1.0f, 0.0f).normalized(), Color(0.3f, 0.35f, 0.4f), 0.3f);
    
    auto scene_ptr = builder.build();
    Scene scene = std::move(*scene_ptr);
    scene.ambientLight = Color(0.05f);
    scene.backgroundColor = Color(0.6f, 0.75f, 0.95f);
    
    // Camera
    PinholeCamera camera(
        width, height,
        Vec3(0.0f, 2.5f, 8.0f),  // position
        Vec3(0.0f, 0.5f, 0.0f),  // lookAt
        Vec3(0.0f, 1.0f, 0.0f),  // up
        50.0f                   // fov
    );
    
    // Film
    Film film(width, height);
    
    // Integrator
    IntegratorConfig intConfig;
    intConfig.maxDepth = 8;
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
    ImageExporter::writeRGB8("results/simple_scene.png", width, height, pixels);
    std::cout << "Saved to: results/simple_scene.png" << std::endl;
    
    return 0;
}

// Example: Volume rendering with participating media

#include "render/core/common.h"
#include "render/core/vector.h"
#include "render/camera/pinhole_camera.h"
#include "render/core/film.h"
#include "render/core/sampler.h"

#include "render/materials/material.h"
#include "render/materials/medium.h"
#include "render/geometry/plane.h"
#include "render/geometry/quad.h"
#include "render/geometry/sphere.h"
#include "render/lighting/area_light.h"
#include "render/lighting/directional_light.h"
#include "render/lighting/point_light.h"
#include "render/scene/scene.h"
#include "render/scene/scene_builder.h"
#include "render/io/image_exporter.h"

#include "render/renderers/volume_integrator.h"
#include "render/renderers/volumetric_path_integrator.h"


#include <iostream>
#include <chrono>

using namespace render;

int main() {
    // Configuration
    const int width = 800;
    const int height = 600;
    const int samples = 64;
    
    std::cout << "=== Volume Rendering Example ===" << std::endl;
    std::cout << "Resolution: " << width << "x" << height << std::endl;
    std::cout << "Samples: " << samples << std::endl;
    
    // Build scene with volumetric fog
    SceneBuilder builder;
    
    // Ground
    auto groundMat = builder.diffuse(Color(0.3f, 0.35f, 0.3f));
    builder.plane(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), groundMat);
    
    // Objects in the fog
    auto sphereMat1 = builder.diffuse(Color(0.9f, 0.5f, 0.2f));  // Orange
    auto sphereMat2 = builder.diffuse(Color(0.2f, 0.5f, 0.9f));  // Blue
    auto mirrorMat = builder.mirror(Color(0.85f));
    
    builder.sphere(Vec3(-1.5f, 0.8f, 0.0f), 0.8f, sphereMat1);
    builder.sphere(Vec3(1.5f, 0.8f, 0.0f), 0.8f, sphereMat2);
    builder.sphere(Vec3(0.0f, 1.2f, -2.0f), 1.2f, mirrorMat);
    
    // Strong point light (like a streetlamp in fog)
    builder.pointLight(Vec3(0.0f, 5.0f, 4.0f), Color(1.0f, 0.95f, 0.85f), 200.0f);
    builder.pointLight(Vec3(-4.0f, 3.0f, 2.0f), Color(0.4f, 0.5f, 0.8f), 50.0f);
    
    // Add participating medium (fog/mist) with forward scattering (g > 0) for god rays
    builder.medium(
        Color(0.01f, 0.01f, 0.015f),   // sigma_a: slight blue absorption
        Color(0.08f, 0.09f, 0.10f),    // sigma_s: scattering (creates fog)
        0.3f                           // g: forward scattering phase parameter
    );
    
    auto scene_ptr = builder.build();
    Scene scene = std::move(*scene_ptr);
    scene.ambientLight = Color(0.02f, 0.02f, 0.03f);
    scene.backgroundColor = Color(0.1f, 0.12f, 0.15f);
    
    // Camera
    PinholeCamera camera(
        width, height,
        Vec3(0.0f, 2.0f, 8.0f),   // position
        Vec3(0.0f, 0.8f, 0.0f),   // lookAt
        Vec3(0.0f, 1.0f, 0.0f),   // up
        50.0f                    // fov
    );
    
    // Film
    Film film(width, height);
    
    // Volume integrator
    IntegratorConfig intConfig;
    intConfig.maxDepth = 8;
    intConfig.samplesPerPixel = samples;
    intConfig.russianRoulette = true;
    intConfig.useNEE = true;
    
    VolumeConfig volConfig;
    volConfig.numSteps = 128;
    volConfig.enableScattering = true;
    volConfig.multipleScattering = false;
    volConfig.stepSizeMultiplier = 1.0f;
    
    VolumeIntegrator integrator(intConfig, volConfig);
    
    // Render
    std::cout << "\nRendering with volumetric scattering..." << std::endl;
    auto startTime = std::chrono::high_resolution_clock::now();
    
    integrator.render(scene, camera, film, [](float p) {
        int bar = static_cast<int>(p * 40);
        std::cout << "\r[";
        for (int i = 0; i < 40; ++i) std::cout << (i < bar ? '=' : ' ');
        std::cout << "] " << static_cast<int>(p * 100) << "%" << std::flush;
    });
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "\n\nRender time: " << duration.count() / 1000.0f << " seconds" << std::endl;
    
    // Save with exposure adjustment for fog
    auto pixels = film.toRGB8ToneMapped(2.0f, 2.2f);
    ImageExporter::writeRGB8("results/volume_scene.png", width, height, pixels);
    std::cout << "Saved to: results/volume_scene.png" << std::endl;
    
    // Also render with volumetric path tracing for comparison
    std::cout << "\n--- Rendering with volumetric path tracing ---" << std::endl;
    
    Film film2(width, height);
    VolumetricPathIntegrator integrator2(intConfig, volConfig);
    
    startTime = std::chrono::high_resolution_clock::now();
    
    integrator2.render(scene, camera, film2, [](float p) {
        int bar = static_cast<int>(p * 40);
        std::cout << "\r[";
        for (int i = 0; i < 40; ++i) std::cout << (i < bar ? '=' : ' ');
        std::cout << "] " << static_cast<int>(p * 100) << "%" << std::flush;
    });
    
    endTime = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    std::cout << "\n\nRender time: " << duration.count() / 1000.0f << " seconds" << std::endl;
    
    auto pixels2 = film2.toRGB8ToneMapped(2.0f, 2.2f);
    ImageExporter::writeRGB8("results/volume_pathtracing.png", width, height, pixels2);
    std::cout << "Saved to: results/volume_pathtracing.png" << std::endl;
    
    return 0;
}

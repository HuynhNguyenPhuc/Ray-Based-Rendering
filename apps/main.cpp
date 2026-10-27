// Unified renderer entry point

#include "render/config/config.h"
#include "render/config/render_factory.h"
#include "render/core/film.h"
#include "render/io/image_exporter.h"
#include "render/scene/scene.h"
#include "render/scene/scene_builder.h"
#include "cornell_box/cornell_box.h"

#include <chrono>
#include <functional>
#include <iostream>
#include <memory>

using namespace render;

Scene createVolumeScene() {
    SceneBuilder builder;
    
    // Ground
    auto groundMat = builder.diffuse(Color(0.3f, 0.3f, 0.3f));
    builder.plane(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), groundMat);
    
    // Sphere in fog
    auto sphereMat = builder.diffuse(Color(0.8f, 0.6f, 0.2f));
    builder.sphere(Vec3(0.0f, 1.0f, 0.0f), 1.0f, sphereMat);
    
    // Lights
    builder.pointLight(Vec3(0.0f, 5.0f, 5.0f), Color(1.0f, 0.9f, 0.8f), 100.0f);
    
    // Add homogeneous fog medium
    builder.medium(
        Color(0.02f, 0.02f, 0.02f),  // sigma_a (absorption)
        Color(0.1f, 0.1f, 0.12f),    // sigma_s (scattering)
        0.0f                           // g (anisotropy, isotropic)
    );
    
    auto scene_ptr = builder.build();
    Scene scene = std::move(*scene_ptr);
    scene.ambientLight = Color(0.1f);
    scene.backgroundColor = Color(0.2f, 0.25f, 0.3f);
    
    return scene;
}

int main(int argc, char** argv) {
    // Parse configuration
    RenderConfig config = Config::parse(argc, argv);

    // Print configuration if verbose
    if (config.verbose) {
        std::cout << "Configuration:\n" << Config::serialize(config) << std::endl;
    }

    std::cout << "=== Ray-Based Rendering Framework ===" << std::endl;
    std::cout << "Image: " << config.image.width << "x" << config.image.height << std::endl;
    std::cout << "Samples: " << config.samplesPerPixel << std::endl;

    // Create scene
    Scene scene;
    if (!config.scenePath.empty()) {
        // TODO: Load scene from file
        std::cout << "Scene loading not yet implemented, using default scene" << std::endl;
        scene = cornell_box::createScene();
    } else {
        // Use default scene based on integrator
        if (config.integratorType == IntegratorType::Volume ||
            config.integratorType == IntegratorType::VolumetricPath) {
            scene = createVolumeScene();
        } else {
            scene = cornell_box::createScene();
        }
    }

    // Rebuild with the requested acceleration structure (scenes default to BVH)
    if (config.accelerator != AcceleratorType::BVH) {
        scene.setAcceleratorType(config.accelerator);
        scene.buildAccelerator();
    }
    std::cout << "Accelerator: " << acceleratorName(config.accelerator) << std::endl;

    // Create integrator
    auto integrator = createIntegrator(config);
    std::cout << "Integrator: " << integrator->name() << std::endl;
    std::cout << "Max depth: " << config.maxDepth << std::endl;

    // Create camera
    auto camera = createCamera(config);

    // Create film
    Film film(config.image.width, config.image.height);

    // Render
    std::cout << "\nRendering..." << std::endl;
    auto startTime = std::chrono::high_resolution_clock::now();

    // Progress callback
    auto progressCallback = config.showProgress ?
        [](float progress) {
            int bar = static_cast<int>(progress * 50);
            std::cout << "\r[";
            for (int i = 0; i < 50; ++i) {
                std::cout << (i < bar ? '=' : ' ');
            }
            std::cout << "] " << static_cast<int>(progress * 100) << "%" << std::flush;
        } : std::function<void(float)>(nullptr);

    integrator->render(scene, *camera, film, progressCallback);

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    std::cout << "\n\nRendering complete!" << std::endl;
    std::cout << "Time: " << duration.count() / 1000.0f << " seconds" << std::endl;

    // Save image
    if (!ImageExporter::writeFilm(config.image.outputPath, film,
                                  config.image.useToneMapping,
                                  config.image.exposure, config.image.gamma)) {
        std::cerr << "Error: failed to write " << config.image.outputPath << std::endl;
        return 1;
    }
    std::cout << "Saved to: " << config.image.outputPath << std::endl;

    return 0;
}

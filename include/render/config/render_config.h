// Plain-data render configuration

#pragma once

#include "../acceleration/accelerator_factory.h"
#include "../core/common.h"
#include "../core/vector.h"
#include <string>

namespace render {

/**
 * @enum IntegratorType
 * @brief Enumeration of available integrators.
 */
enum class IntegratorType {
    Simple,
    Whitted,
    PathTracing,
    Volume,
    VolumetricPath,
    Normal,
    Depth,
    AmbientOcclusion
};

/**
 * @enum CameraType
 * @brief Enumeration of available camera types.
 */
enum class CameraType {
    Perspective,
    Orthographic,
    Environment
};

/**
 * @struct ImageConfig
 * @brief Image output configuration.
 */
struct ImageConfig {
    int width = 800;
    int height = 600;
    std::string outputPath = "output.png";
    bool useToneMapping = true;
    float gamma = 2.2f;
    float exposure = 1.0f;
};

/**
 * @struct CameraConfig
 * @brief Camera configuration.
 */
struct CameraConfig {
    CameraType type = CameraType::Perspective;
    Vec3 position = Vec3(0.0f, 0.0f, 5.0f);
    Vec3 lookAt = Vec3(0.0f, 0.0f, 0.0f);
    Vec3 up = Vec3(0.0f, 1.0f, 0.0f);
    float fov = 60.0f;               // For perspective
    float aperture = 0.0f;           // For DOF (0 = pinhole)
    float focusDistance = 1.0f;      // For DOF
    float orthoHeight = 2.0f;        // For orthographic
};

/**
 * @struct RenderConfig
 * @brief Complete render configuration.
 */
struct RenderConfig {
    // Integrator settings
    IntegratorType integratorType = IntegratorType::PathTracing;
    int maxDepth = 10;
    int samplesPerPixel = 64;
    bool useRussianRoulette = true;
    bool useNEE = true;
    bool useMIS = true;
    
    // Volume settings
    int volumeSteps = 64;
    bool volumeScattering = true;
    bool volumeMultipleScattering = false;
    
    // AO settings
    int aoSamples = 16;
    float aoMaxDistance = 2.0f;
    
    // Camera
    CameraConfig camera;
    
    // Image
    ImageConfig image;
    
    // Scene
    std::string scenePath;
    AcceleratorType accelerator = AcceleratorType::BVH;
    Color backgroundColor = Color(0.0f);
    Color ambientLight = Color(0.1f);
    
    // Debug
    bool verbose = false;
    bool showProgress = true;
};

} // namespace render

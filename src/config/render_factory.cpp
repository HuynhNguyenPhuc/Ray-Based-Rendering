#include "render/config/render_factory.h"

#include "render/camera/environment_camera.h"
#include "render/camera/orthographic_camera.h"
#include "render/camera/pinhole_camera.h"
#include "render/camera/thin_lens_camera.h"
#include "render/renderers/debug_integrators.h"
#include "render/renderers/path_tracing_integrator.h"
#include "render/renderers/simple_integrator.h"
#include "render/renderers/volume_integrator.h"
#include "render/renderers/volumetric_path_integrator.h"
#include "render/renderers/whitted_integrator.h"

namespace render {

std::unique_ptr<Camera> createCamera(const RenderConfig& config) {
    int width = static_cast<int>(config.image.width);
    int height = static_cast<int>(config.image.height);
    
    switch (config.camera.type) {
        case CameraType::Perspective:
            if (config.camera.aperture > 0.0f) {
                return std::make_unique<ThinLensCamera>(
                    width, height,
                    config.camera.position,
                    config.camera.lookAt,
                    config.camera.up,
                    config.camera.fov,
                    config.camera.aperture,
                    config.camera.focusDistance
                );
            }
            return std::make_unique<PinholeCamera>(
                width, height,
                config.camera.position,
                config.camera.lookAt,
                config.camera.up,
                config.camera.fov
            );

        case CameraType::Orthographic:
            return std::make_unique<OrthographicCamera>(
                width, height,
                config.camera.position,
                config.camera.lookAt,
                config.camera.up,
                config.camera.orthoHeight
            );
            
        case CameraType::Environment:
            return std::make_unique<EnvironmentCamera>(
                width, height,
                config.camera.position
            );
            
        default:
            return std::make_unique<PinholeCamera>(
                width, height,
                config.camera.position,
                config.camera.lookAt,
                config.camera.up,
                config.camera.fov
            );
    }
}

std::unique_ptr<Integrator> createIntegrator(const RenderConfig& config) {
    IntegratorConfig intConfig;
    intConfig.maxDepth = config.maxDepth;
    intConfig.samplesPerPixel = config.samplesPerPixel;
    intConfig.russianRoulette = config.useRussianRoulette;
    intConfig.useNEE = config.useNEE;
    intConfig.useMIS = config.useMIS;
    
    switch (config.integratorType) {
        case IntegratorType::Simple:
            return std::make_unique<SimpleIntegrator>(intConfig);
            
        case IntegratorType::Whitted:
            return std::make_unique<WhittedIntegrator>(intConfig);
            
        case IntegratorType::PathTracing:
            return std::make_unique<PathTracingIntegrator>(intConfig);
            
        case IntegratorType::Volume: {
            VolumeConfig volConfig;
            volConfig.numSteps = config.volumeSteps;
            volConfig.enableScattering = config.volumeScattering;
            volConfig.multipleScattering = config.volumeMultipleScattering;
            return std::make_unique<VolumeIntegrator>(intConfig, volConfig);
        }
            
        case IntegratorType::VolumetricPath: {
            VolumeConfig volConfig;
            volConfig.numSteps = config.volumeSteps;
            volConfig.enableScattering = config.volumeScattering;
            volConfig.multipleScattering = config.volumeMultipleScattering;
            return std::make_unique<VolumetricPathIntegrator>(intConfig, volConfig);
        }
            
        case IntegratorType::Normal:
            return std::make_unique<NormalIntegrator>();
            
        case IntegratorType::Depth:
            return std::make_unique<DepthIntegrator>(0.1f, 20.0f);
            
        case IntegratorType::AmbientOcclusion:
            return std::make_unique<AOIntegrator>(config.aoSamples, config.aoMaxDistance);
            
        default:
            return std::make_unique<PathTracingIntegrator>(intConfig);
    }
}

} // namespace render

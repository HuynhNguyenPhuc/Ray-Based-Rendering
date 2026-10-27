#include "render/renderers/integrator.h"
#include <atomic>
#include <memory>

namespace render {

void Integrator::render(const Scene& scene, const Camera& camera, Film& film,
        std::function<void(float)> progress) const {
    int totalPixels = film.width * film.height;
    std::atomic<int> pixelsDone{0};

    #pragma omp parallel
    {
        auto sampler = std::make_unique<UniformSampler>();
        
        #pragma omp for schedule(dynamic, 16)
        for (int y = 0; y < film.height; ++y) {
            sampler->setSeed(y * film.width);
            
            for (int x = 0; x < film.width; ++x) {
                Color pixelColor(0.0f);
                
                for (int s = 0; s < config.samplesPerPixel; ++s) {
                    // Jittered sampling within pixel
                    auto [jx, jy] = sampler->get2D();
                    float px = x + jx;
                    float py = y + jy;
                    
                    CameraSample cameraSample{px, py, 
                                              sampler->get1D(), sampler->get1D(), 0.0f};
                    Ray ray = camera.generateRay(cameraSample);
                    
                    pixelColor += Li(ray, scene, *sampler);
                }
                
                film.addSample(x, y, pixelColor / config.samplesPerPixel);
            }
            
            // Progress reporting
            if (progress) {
                int done = pixelsDone.fetch_add(film.width);
                if (done % (totalPixels / 100 + 1) == 0) {
                    progress(static_cast<float>(done) / totalPixels);
                }
            }
        }
    }
    
    if (progress) progress(1.0f);
}

Color Integrator::directLighting(const SurfaceInteraction& isect, const Scene& scene,
        Sampler& sampler) const {
    Color Ld(0.0f);

    // The BSDF and the shading frame only depend on the hit point, not on the light.
    auto bsdf = isect.material->createBSDF();
    const sampling::Frame frame(isect.outwardNormal());
    const Vec3 woLocal = frame.toLocal(isect.wo);
    
    for (const auto& light : scene.lights()) {
        auto lightSample = light->sample(isect, sampler.get1D(), sampler.get1D());
        
        if (lightSample.pdf <= 0.0f || lightSample.Li.isBlack()) continue;
        
        // Visibility test
        Ray shadowRay = isect.spawnRayTo(isect.point + lightSample.wi * lightSample.distance);
        if (scene.intersectP(shadowRay)) continue;
        
        // Evaluate BSDF
        Vec3 wiLocal = frame.toLocal(lightSample.wi);
        Color f = bsdf->evaluate(woLocal, wiLocal);
        
        float cosTheta = std::abs(dot(lightSample.wi, isect.normal));
        Ld += f * lightSample.Li * cosTheta / lightSample.pdf;
    }
    
    return Ld;
}

} // namespace render

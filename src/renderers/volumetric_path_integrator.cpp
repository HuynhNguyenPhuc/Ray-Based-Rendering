#include "render/renderers/volumetric_path_integrator.h"
#include "render/lighting/point_light.h"

namespace render {

Color VolumetricPathIntegrator::Li(const Ray& ray, const Scene& scene, Sampler& sampler) const {
    Color L(0.0f);
    Color throughput(1.0f);
    Ray currentRay = ray;
    const Medium* medium = scene.medium();
    bool inMedium = (medium != nullptr);

    for (int depth = 0; depth < config.maxDepth; ++depth) {
        // Find surface intersection
        SurfaceInteraction isect;
        bool hitSurface = scene.intersect(currentRay, isect);
        
        // Sample medium if present
        if (inMedium && medium) {
            float maxDist = hitSurface ? isect.t : INFINITY_F;
            float t;
            Color mediumWeight;
            
            bool scattered = medium->sampleDistance(currentRay, maxDist, sampler.get1D(),
                                                    t, mediumWeight);
            throughput *= mediumWeight;
            
            if (scattered) {
                // Scattering event in medium
                Vec3 scatterPoint = currentRay.at(t);
                
                // Direct lighting at scatter point
                if (config.useNEE) {
                    L += throughput * directLightingVolume(scatterPoint, currentRay.direction,
                                                           medium, scene, sampler);
                }
                
                // Sample phase function for next direction
                Vec3 wi = medium->getPhaseFunction()->sample(currentRay.direction,
                                                              sampler.get1D(), sampler.get1D());
                currentRay = Ray(scatterPoint, wi);
                continue;
            }
        }
        
        // Handle surface or background
        if (!hitSurface) {
            L += throughput * scene.backgroundColor;
            break;
        }
        
        const Material* mat = isect.material;
        if (!mat) break;
        
        if (mat->isEmissive()) {
            L += throughput * mat->emission;
        }
        
        // Surface scattering
        auto bsdf = mat->createBSDF();
        const sampling::Frame frame(isect.outwardNormal());
        Vec3 woLocal = frame.toLocal(isect.wo);
        
        // Direct lighting
        if (config.useNEE && !bsdf->isDelta()) {
            L += throughput * directLightingSurface(isect, bsdf.get(), scene, sampler);
        }
        
        // Sample BSDF
        auto bsdfSample = bsdf->sample(woLocal, sampler.get1D(), sampler.get1D());
        if (bsdfSample.pdf <= 0.0f || bsdfSample.f.isBlack()) break;
        
        Vec3 wiWorld = frame.toWorld(bsdfSample.wi);
        float cosTheta = std::abs(dot(wiWorld, isect.normal));
        
        throughput *= bsdfSample.f * (cosTheta / bsdfSample.pdf);
        
        // Russian Roulette
        if (depth > 3) {
            float q = std::max(0.05f, 1.0f - throughput.maxComponent());
            if (sampler.get1D() < q) break;
            throughput /= (1.0f - q);
        }
        
        currentRay = isect.spawnRay(wiWorld);
    }
    
    return L;
}

Color VolumetricPathIntegrator::directLightingVolume(const Vec3& point, const Vec3& wo,
        const Medium* medium, const Scene& scene,
        Sampler& sampler) const {
    Color Ld(0.0f);
    const PhaseFunction* phase = medium->getPhaseFunction();
    
    for (const auto& light : scene.lights()) {
        // Simple point light handling
        if (auto pointLight = dynamic_cast<const PointLight*>(light.get())) {
            Vec3 toLight = pointLight->position - point;
            float dist = toLight.length();
            Vec3 wi = toLight / dist;
            
            // Visibility
            Ray shadowRay(point, wi, dist);
            if (scene.intersectP(shadowRay)) continue;
            
            // Transmittance
            Color tr = medium->transmittance(shadowRay, dist);
            
            Color Li = pointLight->color * pointLight->intensity / (dist * dist);
            float phaseVal = phase->evaluate(wo, wi);
            
            Ld += Li * tr * phaseVal;
        }
    }
    
    return Ld;
}

Color VolumetricPathIntegrator::directLightingSurface(const SurfaceInteraction& isect, const BSDF* bsdf,
        const Scene& scene, Sampler& sampler) const {
    Color Ld(0.0f);
    
    for (const auto& light : scene.lights()) {
        auto lightSample = light->sample(isect, sampler.get1D(), sampler.get1D());
        if (lightSample.pdf <= 0.0f || lightSample.Li.isBlack()) continue;
        
        Ray shadowRay = isect.spawnRay(lightSample.wi);
        shadowRay.tMax = lightSample.distance - EPSILON;
        if (scene.intersectP(shadowRay)) continue;
        
        const sampling::Frame frame(isect.outwardNormal());
        Vec3 woLocal = frame.toLocal(isect.wo);
        Vec3 wiLocal = frame.toLocal(lightSample.wi);
        Color f = bsdf->evaluate(woLocal, wiLocal);
        
        float cosTheta = std::abs(dot(lightSample.wi, isect.normal));
        Ld += f * lightSample.Li * cosTheta / lightSample.pdf;
    }
    
    return Ld;
}

} // namespace render

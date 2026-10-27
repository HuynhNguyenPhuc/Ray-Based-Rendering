#include "render/renderers/path_tracing_integrator.h"

namespace render {

Color PathTracingIntegrator::Li(const Ray& ray, const Scene& scene, Sampler& sampler) const {
    Color L(0.0f);           // Accumulated radiance
    Color throughput(1.0f);   // Path throughput
    Ray currentRay = ray;
    bool specularBounce = true;

    for (int depth = 0; depth < config.maxDepth; ++depth) {
        SurfaceInteraction isect;
        
        if (!scene.intersect(currentRay, isect)) {
            L += throughput * scene.backgroundColor;
            break;
        }

        const Material* mat = isect.material;
        if (!mat) break;

        // Add emission (only for first bounce or after specular)
        if (mat->isEmissive() && (depth == 0 || specularBounce)) {
            L += throughput * mat->emission;
        }

        // Create BSDF
        auto bsdf = mat->createBSDF();
        
        // Local shading frame around the outward normal (wo.z < 0 means the ray is inside)
        const sampling::Frame frame(isect.outwardNormal());
        Vec3 woLocal = frame.toLocal(isect.wo);

        // Direct lighting (Next Event Estimation)
        if (config.useNEE && !bsdf->isDelta()) {
            L += throughput * estimateDirectLighting(isect, bsdf.get(), scene, sampler);
        }

        // Sample BSDF for next direction
        auto bsdfSample = bsdf->sample(woLocal, sampler.get1D(), sampler.get1D());
        
        if (bsdfSample.pdf <= 0.0f || bsdfSample.f.isBlack()) {
            break;
        }

        // Transform sampled direction to world space
        Vec3 wiWorld = frame.toWorld(bsdfSample.wi);
        
        // Update throughput (delta lobes follow the same formula, see BSDFSample)
        float cosTheta = std::abs(dot(wiWorld, isect.normal));
        throughput *= bsdfSample.f * (cosTheta / bsdfSample.pdf);

        // Russian Roulette
        if (depth > 3) {
            float q = std::max(0.05f, 1.0f - throughput.maxComponent());
            if (sampler.get1D() < q) {
                break;
            }
            throughput /= (1.0f - q);
        }

        // Spawn next ray
        currentRay = isect.spawnRay(wiWorld);
        specularBounce = bsdfSample.isDelta;
    }

    return L;
}

Color PathTracingIntegrator::estimateDirectLighting(const SurfaceInteraction& isect, const BSDF* bsdf,
        const Scene& scene, Sampler& sampler) const {
    Color Ld(0.0f);

    // Sample all lights
    for (const auto& light : scene.lights()) {
        Color LdLight = sampleLightDirect(isect, bsdf, light.get(), scene, sampler);
        Ld += LdLight;
    }

    return Ld;
}

Color PathTracingIntegrator::sampleLightDirect(const SurfaceInteraction& isect, const BSDF* bsdf,
        const Light* light, const Scene& scene, Sampler& sampler) const {
    Color Ld(0.0f);

    // Sample light source
    auto lightSample = light->sample(isect, sampler.get1D(), sampler.get1D());
    
    if (lightSample.pdf > 0.0f && !lightSample.Li.isBlack()) {
        // Evaluate BSDF
        const sampling::Frame frame(isect.outwardNormal());
        Vec3 woLocal = frame.toLocal(isect.wo);
        Vec3 wiLocal = frame.toLocal(lightSample.wi);
        Color f = bsdf->evaluate(woLocal, wiLocal);
        float bsdfPdf = bsdf->pdf(woLocal, wiLocal);
        
        if (!f.isBlack()) {
            // Visibility test
            Ray shadowRay = isect.spawnRay(lightSample.wi);
            shadowRay.tMax = lightSample.distance - EPSILON;
            
            if (!scene.intersectP(shadowRay)) {
                float cosTheta = std::abs(dot(lightSample.wi, isect.normal));
                
                if (config.useMIS && !light->isDelta()) {
                    // MIS weight
                    float weight = sampling::powerHeuristic(1, lightSample.pdf, 1, bsdfPdf);
                    Ld += f * lightSample.Li * cosTheta * weight / lightSample.pdf;
                } else {
                    Ld += f * lightSample.Li * cosTheta / lightSample.pdf;
                }
            }
        }
    }

    // Sample BSDF for MIS (only for non-delta lights)
    if (config.useMIS && !light->isDelta()) {
        const sampling::Frame frame(isect.outwardNormal());
        Vec3 woLocal = frame.toLocal(isect.wo);
        auto bsdfSample = bsdf->sample(woLocal, sampler.get1D(), sampler.get1D());
        
        if (!bsdfSample.isDelta && bsdfSample.pdf > 0.0f && !bsdfSample.f.isBlack()) {
            Vec3 wiWorld = frame.toWorld(bsdfSample.wi);
            
            // Trace ray to see if it hits a light
            Ray testRay = isect.spawnRay(wiWorld);
            SurfaceInteraction lightIsect;
            
            if (scene.intersect(testRay, lightIsect)) {
                // Check if we hit an emissive surface (would need area light tracking)
                // For now, skip BSDF sampling for direct lighting
            }
        }
    }

    return Ld;
}

} // namespace render

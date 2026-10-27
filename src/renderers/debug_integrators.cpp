#include "render/renderers/debug_integrators.h"

namespace render {

Color NormalIntegrator::Li(const Ray& ray, const Scene& scene, Sampler& sampler) const {
    SurfaceInteraction isect;
    
    if (!scene.intersect(ray, isect)) {
        return Color(0.0f);
    }

    // Map normal from [-1,1] to [0,1] for visualization
    return (isect.normal + Vec3(1.0f)) * 0.5f;
}

Color DepthIntegrator::Li(const Ray& ray, const Scene& scene, Sampler& sampler) const {
    SurfaceInteraction isect;
    
    if (!scene.intersect(ray, isect)) {
        return Color(1.0f);  // Far = white
    }

    // Linear depth mapping
    float depth = (isect.t - nearPlane) / (farPlane - nearPlane);
    depth = clamp(depth, 0.0f, 1.0f);
    
    // Invert so near = white, far = black
    float value = 1.0f - depth;
    return Color(value);
}

Color AOIntegrator::Li(const Ray& ray, const Scene& scene, Sampler& sampler) const {
    SurfaceInteraction isect;
    
    if (!scene.intersect(ray, isect)) {
        return Color(1.0f);
    }

    // Sample hemisphere and count occlusion
    float occlusion = 0.0f;
    
    for (int i = 0; i < numSamples; ++i) {
        // Cosine-weighted hemisphere sampling
        Vec3 localDir = sampling::cosineHemisphere(sampler.get1D(), sampler.get1D());
        Vec3 worldDir = sampling::localToWorld(localDir, isect.normal);
        
        Ray aoRay = isect.spawnRay(worldDir);
        aoRay.tMax = maxDistance;
        
        SurfaceInteraction aoIsect;
        if (scene.intersect(aoRay, aoIsect)) {
            // Weight by distance (closer = more occlusion)
            float weight = 1.0f - (aoIsect.t / maxDistance);
            occlusion += weight;
        }
    }

    float ao = 1.0f - (occlusion / numSamples);
    return Color(ao);
}

} // namespace render

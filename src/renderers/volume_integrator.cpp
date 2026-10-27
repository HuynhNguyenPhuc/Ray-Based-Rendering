#include "render/renderers/volume_integrator.h"
#include "render/lighting/point_light.h"
#include "render/lighting/directional_light.h"
#include "classic_shading.h"

namespace render {

Color VolumeIntegrator::Li(const Ray& ray, const Scene& scene, Sampler& sampler) const {
    // Check if scene has a medium
    if (!scene.hasMedium()) {
        return traceSurface(ray, scene, sampler, 0);
    }

    return traceVolume(ray, scene, sampler, 0);
}

Color VolumeIntegrator::traceVolume(const Ray& ray, const Scene& scene, Sampler& sampler, int depth) const {
    if (depth > config.maxDepth) {
        return scene.backgroundColor;
    }

    const Medium* medium = scene.medium();
    
    // Find volume bounds (using scene bounds or specific volume object)
    float tMin = 0.0f;
    float tMax = ray.tMax;
    
    // Check for surface intersection to limit ray marching
    SurfaceInteraction surfaceIsect;
    bool hitSurface = scene.intersect(ray, surfaceIsect);
    if (hitSurface) {
        tMax = surfaceIsect.t;
    }

    // Ray march through the volume
    Color L(0.0f);
    Color transmittance(1.0f);
    
    float stepSize = (tMax - tMin) / volumeConfig.numSteps * volumeConfig.stepSizeMultiplier;
    
    for (int i = 0; i < volumeConfig.numSteps; ++i) {
        float t = tMin + (i + sampler.get1D()) * stepSize;
        if (t >= tMax) break;
        
        Vec3 samplePoint = ray.at(t);
        
        // Get medium properties at this point
        Color sigma_a = medium->getSigmaA(samplePoint);
        Color sigma_s = medium->getSigmaS(samplePoint);
        Color sigma_t = sigma_a + sigma_s;
        
        // Compute transmittance for this step
        Color stepTr = Color(
            std::exp(-sigma_t.x * stepSize),
            std::exp(-sigma_t.y * stepSize),
            std::exp(-sigma_t.z * stepSize)
        );
        
        // In-scattering contribution
        if (volumeConfig.enableScattering && !sigma_s.isBlack()) {
            Color Li_scatter = computeInScattering(samplePoint, ray.direction,
                                                   medium, scene, sampler);
            L += transmittance * sigma_s * Li_scatter * stepSize;
        }
        
        // Update transmittance
        transmittance *= stepTr;
        
        // Early termination if transmittance is negligible
        if (transmittance.maxComponent() < 0.001f) break;
    }
    
    // Add contribution from surface or background
    Color surfaceContribution;
    if (hitSurface) {
        surfaceContribution = shadeSurface(surfaceIsect, scene, sampler, depth);
    } else {
        surfaceContribution = scene.backgroundColor;
    }
    
    L += transmittance * surfaceContribution;
    
    return L;
}

Color VolumeIntegrator::computeInScattering(const Vec3& point, const Vec3& wo,
        const Medium* medium, const Scene& scene,
        Sampler& sampler) const {
    Color Li(0.0f);
    const PhaseFunction* phase = medium->getPhaseFunction();
    
    // Sample lights for single scattering
    for (const auto& light : scene.lights()) {
        Vec3 lightDir;
        float lightDist;
        Color lightIntensity;
        
        // Get light contribution
        if (auto pointLight = dynamic_cast<const PointLight*>(light.get())) {
            Vec3 toLight = pointLight->position - point;
            lightDist = toLight.length();
            lightDir = toLight / lightDist;
            lightIntensity = pointLight->color * pointLight->intensity / (lightDist * lightDist);
        } else if (auto dirLight = dynamic_cast<const DirectionalLight*>(light.get())) {
            lightDir = -dirLight->direction;
            lightDist = INFINITY_F;
            lightIntensity = dirLight->color * dirLight->intensity;
        } else {
            continue;
        }
        
        // Shadow ray through medium
        Color shadowTr = computeTransmittance(point, lightDir, lightDist, medium);
        
        // Also check for surface occlusion
        Ray shadowRay(point, lightDir, lightDist);
        if (scene.intersectP(shadowRay)) {
            continue;
        }
        
        // Phase function
        float phaseVal = phase->evaluate(wo, lightDir);
        
        Li += lightIntensity * shadowTr * phaseVal;
    }
    
    return Li;
}

Color VolumeIntegrator::computeTransmittance(const Vec3& start, const Vec3& dir, float distance,
        const Medium* medium) const {
    if (distance == INFINITY_F) {
        // For directional lights, use a large but finite distance
        distance = 1000.0f;
    }
    
    // Simple numerical integration
    int steps = std::max(1, static_cast<int>(distance / 0.1f));
    float stepSize = distance / steps;
    
    Color opticalDepth(0.0f);
    for (int i = 0; i < steps; ++i) {
        Vec3 p = start + dir * ((i + 0.5f) * stepSize);
        opticalDepth += medium->getSigmaT(p) * stepSize;
    }
    
    return Color(
        std::exp(-opticalDepth.x),
        std::exp(-opticalDepth.y),
        std::exp(-opticalDepth.z)
    );
}

Color VolumeIntegrator::shadeSurface(const SurfaceInteraction& isect, const Scene& scene,
        Sampler& sampler, int depth) const {
    const Material* mat = isect.material;
    if (!mat) return scene.backgroundColor;
    
    if (mat->isEmissive()) {
        return mat->emission;
    }
    
    // Simple diffuse shading
    return classic::shade(*mat, isect, scene, sampler, 1.0f - clamp(mat->metallic, 0.0f, 1.0f),
                          0.0f, false);
}

Color VolumeIntegrator::traceSurface(const Ray& ray, const Scene& scene, Sampler& sampler, int depth) const {
    if (depth > config.maxDepth) {
        return scene.backgroundColor;
    }

    SurfaceInteraction isect;
    if (!scene.intersect(ray, isect)) {
        return scene.backgroundColor;
    }

    return shadeSurface(isect, scene, sampler, depth);
}

} // namespace render

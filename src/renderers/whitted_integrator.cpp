#include "render/renderers/whitted_integrator.h"
#include "render/materials/fresnel.h"
#include "classic_shading.h"

namespace render {

Color WhittedIntegrator::Li(const Ray& ray, const Scene& scene, Sampler& sampler) const {
    return trace(ray, scene, sampler, 0);
}

Color WhittedIntegrator::trace(const Ray& ray, const Scene& scene, Sampler& sampler, int depth) const {
    if (depth > config.maxDepth) {
        return scene.backgroundColor;
    }

    SurfaceInteraction isect;
    if (!scene.intersect(ray, isect)) {
        return scene.backgroundColor;
    }

    const Material* mat = isect.material;
    if (!mat) {
        return scene.backgroundColor;
    }

    // Handle emissive materials
    if (mat->isEmissive()) {
        return mat->emission;
    }

    // The material's parameters decide which parts apply; they add up like the lobes of its BSDF.
    const float m = clamp(mat->metallic, 0.0f, 1.0f);
    const float t = clamp(mat->transmission, 0.0f, 1.0f);
    const bool smooth = mat->roughness < Material::SMOOTH_ROUGHNESS;

    Color result(0.0f);

    // Opaque dielectric part, and rough conductor part: local shading.
    const float dielectricWeight = (1.0f - m) * (1.0f - t);
    const float conductorWeight = smooth ? 0.0f : m;
    if (dielectricWeight > 0.0f || conductorWeight > 0.0f) {
        result += classic::shade(*mat, isect, scene, sampler, dielectricWeight, conductorWeight);
    }

    // Smooth conductor: mirror reflection.
    if (smooth && m > 0.0f) {
        result += traceReflection(ray, isect, scene, sampler, depth) * m;
    }

    // Transmissive dielectric (rough glass is treated as smooth here): Fresnel reflection + refraction.
    if (t > 0.0f && m < 1.0f) {
        result += traceRefraction(ray, isect, scene, sampler, depth) * ((1.0f - m) * t);
    }

    return result;
}

Color WhittedIntegrator::traceReflection(const Ray& ray, const SurfaceInteraction& isect,
        const Scene& scene, Sampler& sampler, int depth) const {
    Vec3 reflectDir = reflect(ray.direction, isect.normal);
    Ray reflectRay = isect.spawnRay(reflectDir);
    return isect.material->baseColor * trace(reflectRay, scene, sampler, depth + 1);
}

Color WhittedIntegrator::traceRefraction(const Ray& ray, const SurfaceInteraction& isect,
        const Scene& scene, Sampler& sampler, int depth) const {
    const Material* mat = isect.material;
    
    // Compute Fresnel reflectance
    float cosI = std::abs(dot(ray.direction, isect.normal));
    float etaI = 1.0f, etaT = mat->ior;
    if (!isect.frontFace) std::swap(etaI, etaT);
    
    float kr = fresnelDielectric(cosI, etaI, etaT);
    
    Color result(0.0f);
    
    // Reflection component
    Vec3 reflectDir = reflect(ray.direction, isect.normal);
    Ray reflectRay = isect.spawnRay(reflectDir);
    Color reflected = trace(reflectRay, scene, sampler, depth + 1);
    result += reflected * kr;
    
    // Refraction component (if not total internal reflection)
    if (kr < 1.0f) {
        Vec3 refractDir;
        if (refract(ray.direction, isect.normal, etaI / etaT, refractDir)) {
            Ray refractRay(isect.point - isect.normal * EPSILON, refractDir);
            Color refracted = trace(refractRay, scene, sampler, depth + 1);
            result += refracted * (1.0f - kr) * mat->baseColor;
        }
    }
    
    return result;
}

bool WhittedIntegrator::refract(const Vec3& incident, const Vec3& normal, float eta, Vec3& refracted) {
    float cosI = -dot(incident, normal);
    float sin2T = eta * eta * (1.0f - cosI * cosI);
    
    if (sin2T > 1.0f) return false; // Total internal reflection
    
    float cosT = std::sqrt(1.0f - sin2T);
    refracted = (incident * eta + normal * (eta * cosI - cosT)).normalized();
    return true;
}

} // namespace render

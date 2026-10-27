// Local (Phong-style) shading shared by the non-physical integrators.
// Not part of the public headers: only the Simple, Whitted and Volume integrators use it.

#pragma once

#include "render/renderers/integrator.h"
#include "render/materials/fresnel.h"

namespace render {
namespace classic {

// Fixed lighting constants of the classic model (they are not material properties).
constexpr float AMBIENT = 0.1f;
constexpr float DIFFUSE = 0.7f;

/** Blinn-Phong exponent equivalent to the material's GGX roughness: n = 2 / alpha^2 - 2. */
inline float shininess(const Material& mat) {
    const float a = std::max(mat.alpha(), 1e-2f);
    return std::max(2.0f / (a * a) - 2.0f, 1.0f);
}

/**
 * Ambient + per-light diffuse and (optionally) specular shading.
 * @param dielectricWeight share of the surface that behaves as an opaque dielectric
 *        (Lambertian base + white highlight with the interface's F0)
 * @param conductorWeight  share that behaves as a rough conductor (highlight tinted by baseColor)
 */
inline Color shade(const Material& mat, const SurfaceInteraction& isect, const Scene& scene,
                   Sampler& sampler, float dielectricWeight, float conductorWeight,
                   bool withSpecular = true) {
    Color result = mat.baseColor * (AMBIENT * dielectricWeight) * scene.ambientLight;

    const float n = shininess(mat);
    const float f0 = std::abs(mat.ior - 1.0f) > 1e-3f ? dielectricF0(mat.ior) : 0.0f;
    const Color specColor = Color(f0 * dielectricWeight) + mat.baseColor * conductorWeight;
    const float specNorm = (n + 8.0f) / (8.0f * PI);

    for (const auto& light : scene.lights()) {
        auto lightSample = light->sample(isect, sampler.get1D(), sampler.get1D());
        if (lightSample.pdf <= 0.0f) continue;

        Ray shadowRay = isect.spawnRay(lightSample.wi);
        shadowRay.tMax = lightSample.distance - EPSILON;
        if (scene.intersectP(shadowRay)) continue;

        const float NdotL = std::max(0.0f, dot(isect.normal, lightSample.wi));
        if (NdotL <= 0.0f) continue;

        Color shaded = mat.baseColor * (DIFFUSE * dielectricWeight * NdotL);
        if (withSpecular) {
            const Vec3 halfVec = (lightSample.wi + isect.wo).normalized();
            const float NdotH = std::max(0.0f, dot(isect.normal, halfVec));
            shaded += specColor * (specNorm * std::pow(NdotH, n) * NdotL);
        }
        result += shaded * lightSample.Li;
    }
    return result;
}

} // namespace classic
} // namespace render

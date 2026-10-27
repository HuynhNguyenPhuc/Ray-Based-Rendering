#include "render/renderers/simple_integrator.h"
#include "classic_shading.h"

namespace render {

Color SimpleIntegrator::Li(const Ray& ray, const Scene& scene, Sampler& sampler) const {
    SurfaceInteraction isect;
    
    if (!scene.intersect(ray, isect)) {
        return scene.backgroundColor;
    }

    const Material* mat = isect.material;
    if (!mat) return scene.backgroundColor;

    // Emissive materials
    if (mat->isEmissive()) {
        return mat->emission;
    }

    return classic::shade(*mat, isect, scene, sampler, 1.0f - clamp(mat->metallic, 0.0f, 1.0f),
                          clamp(mat->metallic, 0.0f, 1.0f));
}

} // namespace render

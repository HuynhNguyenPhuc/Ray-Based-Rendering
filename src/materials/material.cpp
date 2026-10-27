#include "render/materials/material.h"
#include "render/materials/lambertian_bsdf.h"
#include "render/materials/microfacet_bsdf.h"
#include "render/materials/mixture_bsdf.h"
#include "render/materials/specular_bsdf.h"

namespace render {

Material Material::diffuse(const Color& albedo) {
    Material m;
    m.baseColor = albedo;
    m.roughness = 1.0f;
    m.ior = 1.0f;  // no dielectric interface: ideal Lambertian
    return m;
}

Material Material::mirror(const Color& tint) {
    Material m;
    m.baseColor = tint;
    m.metallic = 1.0f;
    m.roughness = 0.0f;
    return m;
}

Material Material::glass(float ior, const Color& tint) {
    Material m;
    m.baseColor = tint;
    m.roughness = 0.0f;
    m.ior = ior;
    m.transmission = 1.0f;
    return m;
}

Material Material::metal(const Color& color, float roughness) {
    Material m;
    m.baseColor = color;
    m.metallic = 1.0f;
    m.roughness = roughness;
    return m;
}

Material Material::plastic(const Color& color, float roughness) {
    Material m;
    m.baseColor = color;
    m.roughness = roughness;
    return m;
}

Material Material::emissive(const Color& emission, float intensity) {
    Material m;
    m.baseColor = Color(0.0f);
    m.ior = 1.0f;  // nothing to reflect with: a pure light source
    m.emission = emission * intensity;
    return m;
}

bool Material::isEmissive() const {
    return emission.x > 0.0f || emission.y > 0.0f || emission.z > 0.0f;
}

bool Material::isTransmissive() const {
    return transmission > 0.0f && metallic < 1.0f;
}

bool Material::isSpecular() const {
    return roughness < SMOOTH_ROUGHNESS && (metallic > 0.0f || isTransmissive());
}

std::unique_ptr<BSDF> Material::createBSDF() const {
    const float m = clamp(metallic, 0.0f, 1.0f);
    const float t = clamp(transmission, 0.0f, 1.0f);
    const float dielectric = 1.0f - m;
    const bool smooth = roughness < SMOOTH_ROUGHNESS;
    const bool hasInterface = std::abs(ior - 1.0f) > 1e-3f;
    const bool tinted = !baseColor.isBlack();

    auto mix = std::make_unique<MixtureBSDF>();

    // Conductor: reflects with a Fresnel term that goes from baseColor to white at grazing angles.
    if (m > 0.0f && tinted) {
        const Fresnel fresnel = Fresnel::schlick(baseColor);
        if (smooth) mix->add(std::make_unique<SpecularReflectionBSDF>(Color(1.0f), fresnel), m);
        else        mix->add(std::make_unique<MicrofacetReflectionBSDF>(Color(1.0f), fresnel, alpha()), m);
    }

    if (dielectric > 0.0f) {
        if (hasInterface) {
            const Fresnel fresnel = Fresnel::dielectric(1.0f, ior);
            if (smooth) mix->add(std::make_unique<SpecularReflectionBSDF>(Color(1.0f), fresnel), dielectric);
            else        mix->add(std::make_unique<MicrofacetReflectionBSDF>(Color(1.0f), fresnel, alpha()), dielectric);
            mix->setSubstrateFresnel(fresnel);

            if (t > 0.0f && tinted) {
                if (smooth) mix->add(std::make_unique<SpecularTransmissionBSDF>(baseColor, 1.0f, ior), dielectric * t);
                else        mix->add(std::make_unique<MicrofacetTransmissionBSDF>(baseColor, 1.0f, ior, alpha()), dielectric * t);
            }
            if (t < 1.0f && tinted) {
                mix->add(std::make_unique<LambertianBSDF>(baseColor), dielectric * (1.0f - t), true);
            }
        } else {
            // Index 1: no interface, so there is no reflection and transmitted light goes straight on.
            if (t > 0.0f && tinted) mix->add(std::make_unique<SpecularTransmissionBSDF>(baseColor, 1.0f, 1.0f), dielectric * t);
            if (t < 1.0f && tinted) mix->add(std::make_unique<LambertianBSDF>(baseColor), dielectric * (1.0f - t));
        }
    }

    return mix;
}

} // namespace render

// Material definition

#pragma once

#include "bsdf.h"
#include <memory>

namespace render {

/**
 * @class Material
 * @brief Surface appearance described by six independent parameters.
 *
 * No parameter can be derived from the others:
 *   - baseColor    diffuse albedo of a dielectric, or normal-incidence reflectance (F0) of a metal;
 *                  it also tints transmitted light
 *   - metallic     0 = dielectric, 1 = conductor (blends the two)
 *   - roughness    0 = perfectly smooth (delta lobes), 1 = very rough; GGX alpha = roughness^2
 *   - ior          index of refraction of the dielectric interface; 1 means no interface at all
 *                  (no specular reflection or bending), which gives a purely diffuse surface
 *   - transmission 0 = opaque dielectric, 1 = light passes through (glass)
 *   - emission     radiance leaving the surface (HDR RGB)
 *
 * Everything else follows from these: mirror, glass, plastic, metal and light sources are just
 * different values (see the factory functions). A material that needs extra parameters derives
 * from Material, adds them, and overrides createBSDF(); the integrators only use this interface.
 */
class Material {
public:
    Color baseColor{0.8f, 0.8f, 0.8f};
    float metallic = 0.0f;
    float roughness = 1.0f;
    float ior = 1.5f;
    float transmission = 0.0f;
    Color emission{0.0f};

    // Below this roughness a lobe is treated as a perfectly specular (delta) one.
    static constexpr float SMOOTH_ROUGHNESS = 0.03f;

    Material() = default;
    virtual ~Material() = default;

    // Presets
    static Material diffuse(const Color& albedo);
    static Material mirror(const Color& tint = Color(1.0f));
    static Material glass(float ior = 1.5f, const Color& tint = Color(1.0f));
    static Material metal(const Color& color, float roughness = 0.3f);
    static Material plastic(const Color& color, float roughness = 0.3f);
    static Material emissive(const Color& emission, float intensity = 1.0f);

    // Derived properties
    bool isEmissive() const;
    bool isTransmissive() const;   // some light passes through
    bool isSpecular() const;       // smooth mirror/glass-like: scatters only along delta directions
    float alpha() const { return roughness * roughness; }

    /**
     * Build the BSDF at a surface point (expressed in the local frame, normal = +z).
     * The base implementation covers the six parameters above.
     */
    virtual std::unique_ptr<BSDF> createBSDF() const;
};

} // namespace render

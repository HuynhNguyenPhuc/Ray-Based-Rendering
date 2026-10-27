// Fluent scene builder

#pragma once

#include "scene.h"
#include "../geometry/mesh.h"

namespace render {

/**
 * @class SceneBuilder
 * @brief Fluent interface for building scenes.
 */
class SceneBuilder {
public:
    SceneBuilder() : scene_(std::make_unique<Scene>()) {}

    // Add primitives
    SceneBuilder& sphere(const Vec3& center, float radius, Material* mat);

    SceneBuilder& plane(const Vec3& point, const Vec3& normal, Material* mat);

    SceneBuilder& triangle(const Vec3& v0, const Vec3& v1, const Vec3& v2, Material* mat);

    SceneBuilder& quad(const Vec3& origin, const Vec3& u, const Vec3& v, Material* mat);

    // Add every triangle of a mesh with a single material
    SceneBuilder& mesh(const TriangleMesh& mesh, Material* mat);

    // Add lights
    SceneBuilder& pointLight(const Vec3& pos, const Color& color, float intensity);

    SceneBuilder& directionalLight(const Vec3& dir, const Color& color, float intensity);

    SceneBuilder& areaLight(const Vec3& pos, const Vec3& u, const Vec3& v,
        const Color& color, float intensity);

    // Materials
    Material* diffuse(const Color& albedo);

    Material* mirror(const Color& tint = Color(1.0f));

    Material* glass(float ior = 1.5f, const Color& tint = Color(1.0f));

    Material* metal(const Color& color, float roughness = 0.3f);

    Material* plastic(const Color& color, float roughness = 0.3f);

    Material* emissive(const Color& emission, float intensity = 1.0f);

    // Any material, including ones derived from Material (the scene takes ownership)
    Material* material(std::unique_ptr<Material> material);

    // Scene settings
    SceneBuilder& accelerator(AcceleratorType type);

    SceneBuilder& background(const Color& color);

    SceneBuilder& ambient(const Color& color);

    SceneBuilder& medium(const Color& sigma_a, const Color& sigma_s, float g = 0.0f);

    // Build
    std::unique_ptr<Scene> build();

private:
    std::unique_ptr<Scene> scene_;
};

} // namespace render

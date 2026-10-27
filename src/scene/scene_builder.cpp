#include "render/scene/scene_builder.h"
#include "render/geometry/sphere.h"
#include "render/geometry/plane.h"
#include "render/geometry/triangle.h"
#include "render/geometry/quad.h"
#include "render/lighting/point_light.h"
#include "render/lighting/directional_light.h"
#include "render/lighting/area_light.h"

namespace render {

SceneBuilder& SceneBuilder::sphere(const Vec3& center, float radius, Material* mat) {
    auto s = std::make_unique<Sphere>(center, radius, mat);
    scene_->addPrimitive(std::move(s));
    return *this;
}

SceneBuilder& SceneBuilder::plane(const Vec3& point, const Vec3& normal, Material* mat) {
    auto p = std::make_unique<Plane>(point, normal, mat);
    scene_->addPrimitive(std::move(p));
    return *this;
}

SceneBuilder& SceneBuilder::triangle(const Vec3& v0, const Vec3& v1, const Vec3& v2, Material* mat) {
    auto t = std::make_unique<Triangle>(v0, v1, v2, mat);
    scene_->addPrimitive(std::move(t));
    return *this;
}

SceneBuilder& SceneBuilder::quad(const Vec3& origin, const Vec3& u, const Vec3& v, Material* mat) {
    auto q = std::make_unique<Quad>(origin, u, v, mat);
    scene_->addPrimitive(std::move(q));
    return *this;
}

SceneBuilder& SceneBuilder::mesh(const TriangleMesh& mesh, Material* mat) {
    for (auto& primitive : mesh.toPrimitives(mat)) {
        scene_->addPrimitive(std::move(primitive));
    }
    return *this;
}

SceneBuilder& SceneBuilder::pointLight(const Vec3& pos, const Color& color, float intensity) {
    scene_->addLight(std::make_unique<PointLight>(pos, color, intensity));
    return *this;
}

SceneBuilder& SceneBuilder::directionalLight(const Vec3& dir, const Color& color, float intensity) {
    scene_->addLight(std::make_unique<DirectionalLight>(dir, color, intensity));
    return *this;
}

SceneBuilder& SceneBuilder::areaLight(const Vec3& pos, const Vec3& u, const Vec3& v,
        const Color& color, float intensity) {
    scene_->addLight(std::make_unique<AreaLight>(pos, u, v, color, intensity));
    return *this;
}

Material* SceneBuilder::diffuse(const Color& albedo) {
    return scene_->addMaterial(std::make_unique<Material>(Material::diffuse(albedo)));
}

Material* SceneBuilder::mirror(const Color& tint) {
    return scene_->addMaterial(std::make_unique<Material>(Material::mirror(tint)));
}

Material* SceneBuilder::glass(float ior, const Color& tint) {
    return scene_->addMaterial(std::make_unique<Material>(Material::glass(ior, tint)));
}

Material* SceneBuilder::metal(const Color& color, float roughness) {
    return scene_->addMaterial(std::make_unique<Material>(Material::metal(color, roughness)));
}

Material* SceneBuilder::plastic(const Color& color, float roughness) {
    return scene_->addMaterial(std::make_unique<Material>(Material::plastic(color, roughness)));
}

Material* SceneBuilder::material(std::unique_ptr<Material> material) {
    return scene_->addMaterial(std::move(material));
}

Material* SceneBuilder::emissive(const Color& emission, float intensity) {
    return scene_->addMaterial(std::make_unique<Material>(Material::emissive(emission, intensity)));
}

SceneBuilder& SceneBuilder::accelerator(AcceleratorType type) {
    scene_->setAcceleratorType(type);
    return *this;
}

SceneBuilder& SceneBuilder::background(const Color& color) {
    scene_->backgroundColor = color;
    return *this;
}

SceneBuilder& SceneBuilder::ambient(const Color& color) {
    scene_->ambientLight = color;
    return *this;
}

SceneBuilder& SceneBuilder::medium(const Color& sigma_a, const Color& sigma_s, float g) {
    scene_->setMedium(std::make_unique<HomogeneousMedium>(sigma_a, sigma_s, g));
    return *this;
}

std::unique_ptr<Scene> SceneBuilder::build() {
    scene_->buildAccelerator();
    return std::move(scene_);
}

} // namespace render

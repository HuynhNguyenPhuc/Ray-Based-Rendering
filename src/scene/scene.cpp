#include "render/scene/scene.h"

namespace render {

void Scene::addPrimitive(std::unique_ptr<Primitive> primitive) {
    primitives_.push_back(std::move(primitive));
}

Material* Scene::addMaterial(std::unique_ptr<Material> material) {
    materials_.push_back(std::move(material));
    return materials_.back().get();
}

void Scene::addLight(std::unique_ptr<Light> light) {
    lights_.push_back(std::move(light));
}

void Scene::setMedium(std::unique_ptr<Medium> medium) {
    medium_ = std::move(medium);
}

void Scene::buildAccelerator() {
    std::vector<Primitive*> primPtrs;
    for (const auto& p : primitives_) {
        primPtrs.push_back(p.get());
    }
    accelerator_->build(std::move(primPtrs));
}

bool Scene::intersect(const Ray& ray, SurfaceInteraction& isect) const {
    return accelerator_->intersect(ray, isect);
}

bool Scene::intersectP(const Ray& ray) const {
    return accelerator_->intersectP(ray);
}

bool Scene::isVisible(const Vec3& p1, const Vec3& p2) const {
    Vec3 dir = p2 - p1;
    float dist = dir.length();
    Ray ray(p1, dir / dist, dist - 2.0f * EPSILON);
    return !intersectP(ray);
}

std::pair<const Light*, float> Scene::sampleLight(float u) const {
    if (lights_.empty()) return {nullptr, 0.0f};
    
    // Uniform light sampling (can be improved with power-based sampling)
    size_t index = std::min(
        static_cast<size_t>(u * lights_.size()),
        lights_.size() - 1
    );
    
    return {lights_[index].get(), 1.0f / lights_.size()};
}

float Scene::lightPdf(const Light* light) const {
    if (lights_.empty()) return 0.0f;
    return 1.0f / lights_.size();
}

} // namespace render

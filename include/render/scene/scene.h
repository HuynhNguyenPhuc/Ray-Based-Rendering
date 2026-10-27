// Scene container

#pragma once

#include "../core/vector.h"
#include "../core/ray.h"
#include "../core/interaction.h"
#include "../camera/camera.h"
#include "../geometry/primitive.h"
#include "../acceleration/accelerator.h"
#include "../acceleration/accelerator_factory.h"
#include "../materials/material.h"
#include "../materials/medium.h"
#include "../lighting/light.h"
#include <memory>
#include <vector>

namespace render {

/**
 * @class Scene
 * @brief Container for all scene objects, lights, and media.
 */
class Scene {
public:
    // Scene properties
    Color backgroundColor{0.0f};
    Color ambientLight{0.1f};

    Scene() = default;

    // =========================================================================
    // Scene Building
    // =========================================================================

    /**
     * Add a primitive to the scene.
     */
    void addPrimitive(std::unique_ptr<Primitive> primitive);

    /**
     * Add a material (scene takes ownership).
     */
    Material* addMaterial(std::unique_ptr<Material> material);

    /**
     * Add a light source.
     */
    void addLight(std::unique_ptr<Light> light);

    /**
     * Set the scene's participating medium.
     */
    void setMedium(std::unique_ptr<Medium> medium);

    /**
     * Build acceleration structure. Call after adding all primitives.
     */
    void buildAccelerator();

    /**
     * Choose the acceleration structure (takes effect on the next buildAccelerator()).
     */
    void setAcceleratorType(AcceleratorType type) { accelerator_ = createAccelerator(type); }

    // =========================================================================
    // Ray Intersection
    // =========================================================================

    /**
     * Intersect ray with scene.
     * @param ray The ray to test
     * @param[out] isect Surface interaction if hit
     * @return True if ray hits something
     */
    bool intersect(const Ray& ray, SurfaceInteraction& isect) const;

    /**
     * Test if ray is occluded (for shadow rays).
     */
    bool intersectP(const Ray& ray) const;

    /**
     * Test visibility between two points.
     */
    bool isVisible(const Vec3& p1, const Vec3& p2) const;

    // =========================================================================
    // Accessors
    // =========================================================================

    const std::vector<std::unique_ptr<Light>>& lights() const { return lights_; }
    const std::vector<std::unique_ptr<Primitive>>& primitives() const { return primitives_; }
    const std::vector<std::unique_ptr<Material>>& materials() const { return materials_; }
    const Medium* medium() const { return medium_.get(); }

    // Camera (optional): a scene is the combination of camera, geometry,
    // materials, lights and medium.
    void setCamera(std::unique_ptr<Camera> camera) { camera_ = std::move(camera); }
    const Camera* camera() const { return camera_.get(); }
    bool hasCamera() const { return camera_ != nullptr; }

    BoundingBox worldBounds() const { return accelerator_->worldBounds(); }

    bool hasLights() const { return !lights_.empty(); }
    bool hasMedium() const { return medium_ != nullptr; }

    // =========================================================================
    // Light Sampling
    // =========================================================================

    /**
     * Sample a light from the scene.
     * @param u Random value for selection
     * @return Pointer to sampled light and PDF
     */
    std::pair<const Light*, float> sampleLight(float u) const;

    /**
     * PDF for selecting a specific light.
     */
    float lightPdf(const Light* light) const;

private:
    std::vector<std::unique_ptr<Primitive>> primitives_;
    std::vector<std::unique_ptr<Material>> materials_;
    std::vector<std::unique_ptr<Light>> lights_;
    std::unique_ptr<Medium> medium_;
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<Accelerator> accelerator_ = createAccelerator(AcceleratorType::BVH);
};

} // namespace render

#include "cornell_box.h"

#include "render/camera/pinhole_camera.h"
#include "render/scene/scene_builder.h"

namespace render {
namespace cornell_box {

Scene createScene(bool withFog) {
    SceneBuilder builder;
    
    // Materials
    auto whiteDiffuse = builder.diffuse(Color(0.73f, 0.73f, 0.73f));
    auto redDiffuse = builder.diffuse(Color(0.65f, 0.05f, 0.05f));
    auto greenDiffuse = builder.diffuse(Color(0.12f, 0.45f, 0.15f));
    auto lightMaterial = builder.emissive(Color(15.0f, 15.0f, 15.0f));
    auto mirrorMaterial = builder.mirror(Color(0.9f, 0.9f, 0.9f));
    auto glassMaterial = builder.glass(1.5f, Color(1.0f, 1.0f, 1.0f));
    
    // Floor
    builder.quad(Vec3(-1.0f, 0.0f, -1.0f), Vec3(2.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 2.0f), whiteDiffuse);
    
    // Ceiling
    builder.quad(Vec3(-1.0f, 2.0f, -1.0f), Vec3(2.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 2.0f), whiteDiffuse);
    
    // Back wall
    builder.quad(Vec3(-1.0f, 0.0f, -1.0f), Vec3(2.0f, 0.0f, 0.0f), Vec3(0.0f, 2.0f, 0.0f), whiteDiffuse);
    
    // Left wall (red)
    builder.quad(Vec3(-1.0f, 0.0f, -1.0f), Vec3(0.0f, 0.0f, 2.0f), Vec3(0.0f, 2.0f, 0.0f), redDiffuse);
    
    // Right wall (green)
    builder.quad(Vec3(1.0f, 0.0f, -1.0f), Vec3(0.0f, 0.0f, 2.0f), Vec3(0.0f, 2.0f, 0.0f), greenDiffuse);
    
    // Light (area light on ceiling)
    builder.areaLight(Vec3(-0.25f, 1.99f, -0.25f), Vec3(0.5f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 0.5f), 
                      Color(1.0f, 1.0f, 1.0f), 15.0f);
    
    // Objects
    // Glass sphere
    builder.sphere(Vec3(-0.4f, 0.4f, 0.0f), 0.4f, glassMaterial);
    
    // Mirror sphere
    builder.sphere(Vec3(0.4f, 0.3f, 0.3f), 0.3f, mirrorMaterial);
    
    // Point light for additional illumination
    builder.pointLight(Vec3(0.0f, 1.8f, 0.5f), Color(1.0f, 1.0f, 1.0f), 2.0f);
    
    if (withFog) {
        // Light homogeneous fog so the volume techniques have a medium to march through
        builder.medium(Color(0.02f, 0.02f, 0.02f), Color(0.1f, 0.1f, 0.12f), 0.0f);
    }

    // Build scene
    auto scene_ptr = builder.build();
    Scene scene = std::move(*scene_ptr);
    scene.ambientLight = Color(0.02f, 0.02f, 0.02f);
    scene.backgroundColor = Color(0.0f);
    
    return scene;
}

std::unique_ptr<Camera> createCamera(int width, int height) {
    return std::make_unique<PinholeCamera>(
        width, height,
        Vec3(0.0f, 1.0f, 5.0f),     // position (in front of the open face)
        Vec3(0.0f, 1.0f, 0.0f),     // look at the centre of the box
        Vec3(0.0f, 1.0f, 0.0f),
        30.0f);
}

} // namespace cornell_box
} // namespace render

# Ray-Based Rendering Framework

A modular, extensible C++17 framework (static library + applications) for ray-based rendering: ray casting, Whitted ray tracing, path tracing and volume rendering behind one camera / scene / integrator interface.

## Features

### Rendering Techniques
- **Simple** - Direct lighting with Blinn-Phong shading
- **Whitted** - Recursive ray tracing with reflection/refraction
- **Path Tracing** - Monte Carlo global illumination with NEE and MIS
- **Volume** - Ray marching through participating media
- **Volumetric Path** - Path tracing with volumetric scattering
- **Debug** - Normal, depth, and ambient occlusion visualization

### Materials
One `Material` type described by six independent parameters; mirror, glass, plastic, metal and
light sources are different parameter values:

| Parameter | Meaning |
|---|---|
| `baseColor` | Diffuse albedo of a dielectric, or normal-incidence reflectance (F0) of a metal; also tints transmitted light |
| `metallic` | 0 = dielectric, 1 = conductor |
| `roughness` | 0 = perfectly smooth (delta lobes), 1 = very rough (GGX, alpha = roughness²) |
| `ior` | Index of refraction of the dielectric interface (1 = no interface: pure diffuse or light source) |
| `transmission` | 0 = opaque, 1 = light passes through (glass) |
| `emission` | Emitted radiance (HDR RGB) |

Presets: `Material::diffuse / mirror / glass / metal / plastic / emissive`. The BSDF is assembled from
Lambertian, specular and GGX microfacet (reflection and transmission) lobes with Fresnel weighting.
A material that needs more parameters derives from `Material` and overrides `createBSDF()`.  See [docs/MATERIAL.md](docs/MATERIAL.md) for the theory and figures.

### Participating Media
- Homogeneous medium (uniform fog)
- Exponential medium (height fog)
- Phase functions: Isotropic, Henyey-Greenstein

### Other Features
- Multiple camera types (pinhole, thin-lens depth of field, orthographic, environment) with explicit extrinsics/intrinsics
- Interchangeable acceleration structures: BVH (SAH), uniform grid, kd-tree (SAH), octree
- OpenMP parallelization
- JSON configuration with CLI
- Tone mapping and gamma correction

## Architecture

```
Ray-Based-Rendering/
├── include/render/          # Public headers (one folder per module)
│   ├── core/                # vector, ray, bbox, sampler, interaction, film, common
│   ├── camera/              # camera.h (base), extrinsics + per-model intrinsics, pinhole / thin-lens / orthographic / environment cameras
│   ├── geometry/            # primitive.h, sphere/plane/triangle/quad, mesh.h
│   ├── acceleration/        # accelerator.h (interface), bvh.h, uniform_grid.h, kd_tree.h, octree.h, accelerator_factory.h
│   ├── materials/           # material.h, BSDFs (bsdf, lambertian, specular, microfacet, mixture), fresnel.h, media, phase functions
│   ├── lighting/            # light.h (base), point / directional / area / spot lights
│   ├── scene/               # scene.h (camera + geometry + materials + lights + medium), scene_builder.h
│   ├── renderers/           # integrator.h (base), simple, whitted, path tracing, volume, volumetric path, debug
│   ├── io/                  # image_exporter.h (PNG/JPG/BMP), obj_importer.h, json_parser.h
│   └── config/              # render_config.h, config.h (JSON + CLI), render_factory.h
├── src/                     # Implementation files, mirroring include/render/<module>
├── cornell_box/             # Cornell box scene + comparison of all rendering techniques
├── apps/main.cpp            # Command-line renderer
├── examples/                # Small example programs
├── tests/                   # Unit tests (test_*.cpp, built with -DBUILD_TESTS=ON)
├── configs/                 # Sample JSON render configurations
├── assets/                  # OBJ models and textures
├── results/                 # Rendered output images
├── docs/                    # Extra documentation (CAMERA.md, MATERIAL.md)
├── third_party/             # stb_image / stb_image_write
└── CMakeLists.txt
```

## Cornell box comparison

`cornell_box/` holds the Cornell box scene and `render_cornell_compare`, which renders it with every
technique (direct lighting, Whitted, path tracing with/without NEE, ambient occlusion, normals, depth,
and - in a foggy variant - volume ray marching and volumetric path tracing), prints a timing table and
writes one PNG per technique plus a `comparison.png` contact sheet:

```bash
./render_cornell_compare -w 256 -h 256 -s 16 -o results/cornell_box
```

## Acceleration structures

Every structure implements the same `Accelerator` interface and returns identical hits (checked by
`tests/test_accelerators.cpp` against a brute-force loop). Select one with `-a bvh|grid|kdtree|octree`
(or `"accelerator"` in a JSON config). `render_accel_compare` builds all four on an OBJ mesh, times
construction and queries, and verifies that the answers agree with the BVH:

```bash
./render -i whitted -a kdtree
./render_accel_compare assets/motocompo.obj -r 200000 --plane
```

## Importing meshes

```cpp
TriangleMesh mesh;
if (ObjImporter::load("assets/barrel.obj", mesh)) {
    SceneBuilder builder;
    builder.mesh(mesh, builder.diffuse(Color(0.7f, 0.5f, 0.3f)));
}
```

## Building

### Prerequisites
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.16+
- OpenMP (optional, for parallelization)

### Build Commands

```bash
# Create build directory
mkdir build && cd build

# Configure (Release build)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . -j8
```

### Build Options

```bash
cmake .. -DBUILD_EXAMPLES=ON   # Enable example programs
cmake .. -DBUILD_TESTS=ON      # Enable unit tests
```

## Usage

### Command Line

```bash
# Basic usage
./render

# Specify configuration
./render -c configs/cornell.json

# Override settings
./render -w 1920 -h 1080 -s 256 -o result.png

# Choose integrator
./render -i pathtracing

# Help
./render --help
```

### Configuration Files

Example JSON configuration:

```json
{
    "integrator": {
        "type": "pathtracing",
        "maxDepth": 10,
        "samplesPerPixel": 128
    },
    "camera": {
        "type": "perspective",
        "position": [0, 2, 8],
        "lookAt": [0, 1, 0],
        "fov": 60
    },
    "image": {
        "width": 800,
        "height": 600,
        "output": "output.png"
    }
}
```

### Programmatic Usage

```cpp
#include "render/core/vector.h"
#include "render/camera/pinhole_camera.h"
#include "render/core/film.h"
#include "render/materials/material.h"
#include "render/geometry/sphere.h"
#include "render/geometry/plane.h"
#include "render/scene/scene_builder.h"
#include "render/renderers/path_tracing_integrator.h"
#include "render/io/image_exporter.h"

using namespace render;

int main() {
    // Build scene using the fluent builder
    SceneBuilder builder;
    
    builder.addPrimitive<Sphere>(Vec3(0, 0.5f, 0), 0.5f)
           .withMaterial(Material::diffuse(Color(0.8f, 0.2f, 0.2f)));
    
    builder.addPrimitive<Plane>(Vec3(0, 0, 0), Vec3(0, 1, 0))
           .withMaterial(Material::diffuse(Color(0.5f)));
    
    builder.addLight<PointLight>(Vec3(0, 5, 5), Color(1.0f), 100.0f);
    
    auto scene = builder.build();
    
    // Create camera
    PinholeCamera camera(
        800, 600,
        Vec3(0, 2, 5), Vec3(0, 0.5f, 0), Vec3(0, 1, 0),
        60.0f
    );
    
    // Create film and integrator
    Film film(800, 600);
    IntegratorConfig config;
    config.samplesPerPixel = 64;
    
    PathTracingIntegrator integrator(config);
    integrator.render(scene, camera, film, nullptr);
    
    // Save result
    auto pixels = film.toRGB8ToneMapped(1.0f, 2.2f);
    // ... write to file
}
```

## Extending the Framework

### Adding a New Integrator

```cpp
#include "render/renderers/integrator.h"

class MyIntegrator : public Integrator {
public:
    std::string name() const override { return "My Integrator"; }
    
    Color Li(const Ray& ray, const Scene& scene, Sampler& sampler) const override {
        // Implement your rendering algorithm here
    }
};
```

### Adding a New Primitive

```cpp
class MyPrimitive : public Primitive {
public:
    bool intersect(const Ray& ray, SurfaceInteraction& isect) const override { ... }
    BBox getBounds() const override { ... }
};
```

### Adding a New BSDF

`evaluate` and `sample` return the plain BSDF value, without the cosine term; a path is weighted by
`f * |cos| / pdf`. A delta lobe returns `R / |cos|` with a pdf equal to the probability of choosing it.

```cpp
class MyBSDF : public BSDF {
public:
    Color evaluate(const Vec3& wo, const Vec3& wi) const override { ... }
    BSDFSample sample(const Vec3& wo, float u1, float u2) const override { ... }
    float pdf(const Vec3& wo, const Vec3& wi) const override { ... }
    BSDFType type() const override { ... }
};
```

### Adding a New Material

Derive from `Material` for extra parameters and build the BSDF yourself (see `tests/test_materials.cpp`):

```cpp
class MyMaterial : public Material {
public:
    float myParameter = 0.5f;
    std::unique_ptr<BSDF> createBSDF() const override { ... }
};
```

## Results

The framework successfully replaces the original implementations:

### Ray Tracing with Blinn-Phong Shading
<p align="center">
  <img src="results/sphere.png" width="75%" alt="Sphere"/>
</p>
<p align="center"><i>Sphere on the ground</i></p>
<p align="center">
  <img src="results/barrel.png" width="45%" alt="Barrel"/>
  <img src="results/motocompo.png" width="45%" alt="Motocompo"/>
</p>
<p align="center"><i>Left: Barrel | Right: Motocompo</i></p>

### Whitted Ray Tracing 
![Whitted Ray Tracing with Reflection and Refraction](results/whitted_ray_tracing.png)

### Path Tracing
![Path Tracing with BRDF Sampling + Next Event Estimation](results/path_tracing.png)

### Volume Rendering  
<p align="center">
  <img src="results/forward_ray_marching.png" width="45%" alt="Volume Rendering"/>
  <img src="results/backward_ray_marching.png" width="45%" alt="Volume Rendering"/>
</p>
<p align="center"><i>Unified volume rendering replaces separate forward/backward implementations</i></p>

## License

MIT License
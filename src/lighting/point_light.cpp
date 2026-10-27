#include "render/lighting/point_light.h"

namespace render {

LightSample PointLight::sample(const SurfaceInteraction& isect, float u1, float u2) const {
    LightSample sample;
    
    Vec3 toLight = position - isect.point;
    float dist2 = dot(toLight, toLight);
    sample.distance = std::sqrt(dist2);
    sample.wi = toLight / sample.distance;
    
    // Radiance falls off with inverse square law
    sample.Li = color * intensity / dist2;
    sample.pdf = 1.0f;  // Delta distribution
    sample.isDelta = true;
    
    return sample;
}

Color PointLight::power() const {
    return color * intensity * 4.0f * PI;
}

} // namespace render

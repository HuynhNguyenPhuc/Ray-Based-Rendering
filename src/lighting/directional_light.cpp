#include "render/lighting/directional_light.h"

namespace render {

LightSample DirectionalLight::sample(const SurfaceInteraction& isect, float u1, float u2) const {
    LightSample sample;
    
    sample.wi = -direction;  // Direction TO light
    sample.distance = INFINITY_F;
    sample.Li = color * intensity;
    sample.pdf = 1.0f;  // Delta distribution
    sample.isDelta = true;
    
    return sample;
}

Color DirectionalLight::power() const {
    // Infinite power conceptually, return a representative value
    return color * intensity;
}

} // namespace render

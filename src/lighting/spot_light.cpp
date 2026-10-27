#include "render/lighting/spot_light.h"

namespace render {

LightSample SpotLight::sample(const SurfaceInteraction& isect, float u1, float u2) const {
    LightSample sample;
    
    Vec3 toLight = position - isect.point;
    float dist2 = dot(toLight, toLight);
    sample.distance = std::sqrt(dist2);
    sample.wi = toLight / sample.distance;
    
    // Compute falloff
    float cosAngle = dot(-sample.wi, direction);
    if (cosAngle < cosTotalWidth) {
        sample.Li = Color(0.0f);
        sample.pdf = 0.0f;
        return sample;
    }
    
    float falloff = 1.0f;
    if (cosAngle < cosFalloffStart) {
        float delta = (cosAngle - cosTotalWidth) / (cosFalloffStart - cosTotalWidth);
        falloff = delta * delta * delta * delta; // Smooth falloff
    }
    
    sample.Li = color * intensity * falloff / dist2;
    sample.pdf = 1.0f;
    sample.isDelta = true;
    
    return sample;
}

Color SpotLight::power() const {
    return color * intensity * TWO_PI * 
           (1.0f - 0.5f * (cosFalloffStart + cosTotalWidth));
}

} // namespace render

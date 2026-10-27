#include "render/lighting/area_light.h"

namespace render {

AreaLight::AreaLight(const Vec3& position, const Vec3& u, const Vec3& v,
        const Color& color, float intensity)
    : Light(LightType::Area, color, intensity)
    , position(position), u(u), v(v)
{
    normal = cross(u, v).normalized();
}

LightSample AreaLight::sample(const SurfaceInteraction& isect, float u1, float u2) const {
    LightSample sample;
    
    // Sample point on light surface
    Vec3 lightPoint = position + u * (u1 - 0.5f) + v * (u2 - 0.5f);
    
    Vec3 toLight = lightPoint - isect.point;
    float dist2 = dot(toLight, toLight);
    sample.distance = std::sqrt(dist2);
    sample.wi = toLight / sample.distance;
    
    // Check if light faces the surface point
    float cosTheta = -dot(normal, sample.wi);
    if (cosTheta <= 0.0f) {
        sample.Li = Color(0.0f);
        sample.pdf = 0.0f;
        return sample;
    }
    
    // Convert area PDF to solid angle PDF
    float area = cross(u, v).length();
    sample.pdf = dist2 / (cosTheta * area);
    sample.Li = color * intensity;
    sample.isDelta = false;
    
    return sample;
}

Color AreaLight::Le(const Ray& ray) const {
    // Only emit from front face
    if (dot(normal, -ray.direction) > 0.0f) {
        return color * intensity;
    }
    return Color(0.0f);
}

Color AreaLight::power() const {
    float area = cross(u, v).length();
    return color * intensity * PI * area;
}

float AreaLight::area() const {
    return cross(u, v).length();
}

} // namespace render

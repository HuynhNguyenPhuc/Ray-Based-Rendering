#include "render/materials/medium.h"

namespace render {

Color Medium::getSigmaT(const Vec3& p) const {
    return getSigmaA(p) + getSigmaS(p);
}

bool HomogeneousMedium::sampleDistance(const Ray& ray, float maxDist, float u,
        float& t, Color& weight) const {
    // Use average extinction for sampling
    Color sigma_t = sigma_a + sigma_s;
    float avgSigmaT = (sigma_t.x + sigma_t.y + sigma_t.z) / 3.0f;
    
    if (avgSigmaT < 1e-6f) {
        // Transparent medium
        weight = Color(1.0f);
        return false;
    }

    // Sample exponential distribution
    t = -std::log(1.0f - u) / avgSigmaT;
    
    if (t < maxDist) {
        // Scattering event
        Color tr = transmittance(ray, t);
        weight = sigma_s * tr / (avgSigmaT * avgSigmaT);
        return true;
    } else {
        // No scattering, ray exits medium
        weight = transmittance(ray, maxDist);
        return false;
    }
}

Color HomogeneousMedium::transmittance(const Ray& ray, float distance) const {
    Color sigma_t = sigma_a + sigma_s;
    return Color(
        std::exp(-sigma_t.x * distance),
        std::exp(-sigma_t.y * distance),
        std::exp(-sigma_t.z * distance)
    );
}

const PhaseFunction* HomogeneousMedium::getPhaseFunction() const {
    return phase.get();
}

float ExponentialMedium::densityScale(const Vec3& p) const {
    return std::exp(-falloff * std::max(0.0f, p.y - baseHeight));
}

Color ExponentialMedium::getSigmaA(const Vec3& p) const {
    return sigma_a * densityScale(p);
}

Color ExponentialMedium::getSigmaS(const Vec3& p) const {
    return sigma_s * densityScale(p);
}

bool ExponentialMedium::sampleDistance(const Ray& ray, float maxDist, float u,
        float& t, Color& weight) const {
    // Simplified: use delta tracking with uniform majorant
    Color sigma_t_max = sigma_a + sigma_s;
    float avgSigmaT = (sigma_t_max.x + sigma_t_max.y + sigma_t_max.z) / 3.0f;
    
    t = 0.0f;
    while (true) {
        t -= std::log(1.0f - u) / avgSigmaT;
        
        if (t >= maxDist) {
            weight = transmittance(ray, maxDist);
            return false;
        }
        
        Vec3 p = ray.at(t);
        Color sigma_t = getSigmaT(p);
        float density = (sigma_t.x + sigma_t.y + sigma_t.z) / 3.0f;
        
        if (u < density / avgSigmaT) {
            weight = getSigmaS(p) / sigma_t;
            return true;
        }
    }
}

Color ExponentialMedium::transmittance(const Ray& ray, float distance) const {
    // Numerical integration for varying density
    const int steps = 64;
    float stepSize = distance / steps;
    Color optical_depth(0.0f);
    
    for (int i = 0; i < steps; ++i) {
        Vec3 p = ray.at((i + 0.5f) * stepSize);
        optical_depth += getSigmaT(p) * stepSize;
    }
    
    return Color(
        std::exp(-optical_depth.x),
        std::exp(-optical_depth.y),
        std::exp(-optical_depth.z)
    );
}

const PhaseFunction* ExponentialMedium::getPhaseFunction() const {
    return phase.get();
}

} // namespace render

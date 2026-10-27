// Checks the BSDF/Material model: sampling, evaluation and pdf must agree, energy must be
// conserved, and the delta and non-delta lobes must follow the same weighting convention.

#include "render/materials/lambertian_bsdf.h"
#include "render/materials/material.h"
#include "render/materials/microfacet_bsdf.h"
#include "render/materials/mixture_bsdf.h"
#include "render/materials/specular_bsdf.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <random>
#include <string>

using namespace render;

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        std::cerr << "FAIL: " << what << "\n";
        ++failures;
    }
}

void checkNear(double value, double expected, double tolerance, const std::string& what) {
    if (!(std::abs(value - expected) <= tolerance)) {
        std::cerr << "FAIL: " << what << ": got " << value << ", expected " << expected
                  << " +- " << tolerance << "\n";
        ++failures;
    }
}

Vec3 directionFromCos(float cosTheta, float phi = 0.3f) {
    const float s = std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));
    return Vec3(s * std::cos(phi), s * std::sin(phi), cosTheta);
}

struct Rng {
    std::mt19937 gen{12345};
    std::uniform_real_distribution<float> dist{0.0f, 1.0f};
    float next() { return std::min(dist(gen), 0.99999994f); }
};

// Mean of f * |cos| / pdf over BSDF::sample (the quantity a path tracer accumulates).
double sampledAlbedo(const BSDF& bsdf, const Vec3& wo, int n, Rng& rng, double* red = nullptr) {
    double sum = 0.0;
    for (int i = 0; i < n; ++i) {
        BSDFSample s = bsdf.sample(wo, rng.next(), rng.next());
        if (s.pdf <= 0.0f) continue;
        sum += s.f.x * std::abs(s.wi.z) / s.pdf;
    }
    if (red) *red = sum / n;
    return sum / n;
}

// The same integral estimated with uniformly distributed directions and BSDF::evaluate.
double evaluatedAlbedo(const BSDF& bsdf, const Vec3& wo, int n, Rng& rng) {
    double sum = 0.0;
    for (int i = 0; i < n; ++i) {
        const float z = 1.0f - 2.0f * rng.next();
        const float phi = TWO_PI * rng.next();
        const float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
        const Vec3 wi(r * std::cos(phi), r * std::sin(phi), z);
        sum += bsdf.evaluate(wo, wi).x * std::abs(wi.z);
    }
    return sum / n * 4.0 * PI;
}

// Integral of the pdf over the sphere.
double pdfIntegral(const BSDF& bsdf, const Vec3& wo, int n, Rng& rng) {
    double sum = 0.0;
    for (int i = 0; i < n; ++i) {
        const float z = 1.0f - 2.0f * rng.next();
        const float phi = TWO_PI * rng.next();
        const float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
        sum += bsdf.pdf(wo, Vec3(r * std::cos(phi), r * std::sin(phi), z));
    }
    return sum / n * 4.0 * PI;
}

// sample() must report the same f and pdf as evaluate() and pdf() for the direction it returns.
void checkSampleConsistency(const BSDF& bsdf, const Vec3& wo, const std::string& name) {
    Rng rng;
    int checked = 0, bad = 0;
    for (int i = 0; i < 20000; ++i) {
        BSDFSample s = bsdf.sample(wo, rng.next(), rng.next());
        if (s.pdf <= 0.0f || s.isDelta) continue;
        ++checked;
        const float pdf = bsdf.pdf(wo, s.wi);
        const Color f = bsdf.evaluate(wo, s.wi);
        if (std::abs(pdf - s.pdf) > 1e-3f * std::max(1.0f, s.pdf) ||
            std::abs(f.x - s.f.x) > 1e-3f * std::max(1.0f, s.f.x)) {
            ++bad;
        }
    }
    check(checked > 0, name + ": produced samples");
    check(bad == 0, name + ": sample() matches evaluate()/pdf() (" + std::to_string(bad) + " mismatches)");
}

// Sampling and evaluation must estimate the same albedo.
void checkAlbedoAgreement(const BSDF& bsdf, const Vec3& wo, const std::string& name, double tol = 0.03) {
    Rng a, b;
    b.gen.seed(777);
    const double viaSampling = sampledAlbedo(bsdf, wo, 400000, a);
    const double viaEvaluate = evaluatedAlbedo(bsdf, wo, 2000000, b);
    checkNear(viaSampling, viaEvaluate, tol * std::max(0.1, viaEvaluate), name + ": sampled vs evaluated albedo");
}

void testLambertian() {
    Rng rng;
    LambertianBSDF white(Color(1.0f));
    for (float c : {1.0f, 0.6f, 0.15f}) {
        const Vec3 wo = directionFromCos(c);
        checkNear(sampledAlbedo(white, wo, 200000, rng), 1.0, 0.01, "Lambertian furnace");
        checkNear(pdfIntegral(white, wo, 400000, rng), 1.0, 0.02, "Lambertian pdf integral");
    }
    checkNear(sampledAlbedo(white, directionFromCos(-0.5f), 100000, rng), 1.0, 0.01, "Lambertian from below");
    checkSampleConsistency(white, directionFromCos(0.5f), "Lambertian");
}

void testSpecular() {
    // Delta lobes: f * |cos| / pdf must equal the reflectance times the Fresnel term.
    const Vec3 wo = directionFromCos(0.7f);
    SpecularReflectionBSDF mirror(Color(0.8f));
    BSDFSample s = mirror.sample(wo, 0.5f, 0.5f);
    checkNear(s.f.x * std::abs(s.wi.z) / s.pdf, 0.8, 1e-5, "mirror weight");
    check(s.isDelta, "mirror sample is delta");
    checkNear(s.wi.z, wo.z, 1e-6, "mirror keeps z");
    checkNear(s.wi.x, -wo.x, 1e-6, "mirror flips x");

    const float F = fresnelDielectric(wo.z, 1.0f, 1.5f);
    SpecularReflectionBSDF dielectric(Color(1.0f), Fresnel::dielectric(1.0f, 1.5f));
    s = dielectric.sample(wo, 0.5f, 0.5f);
    checkNear(s.f.x * std::abs(s.wi.z) / s.pdf, F, 1e-5, "dielectric reflection weight");

    // Refraction: Snell's law and the (1-F) * eta^2 radiance weight.
    SpecularTransmissionBSDF glass(Color(1.0f), 1.0f, 1.5f);
    s = glass.sample(wo, 0.5f, 0.5f);
    const float sinI = std::sqrt(1.0f - wo.z * wo.z);
    const float sinT = std::sqrt(1.0f - s.wi.z * s.wi.z);
    checkNear(sinI, 1.5 * sinT, 1e-5, "Snell's law");
    check(s.wi.z < 0.0f, "refracted ray crosses the surface");
    const double eta = 1.0 / 1.5;
    checkNear(s.f.x * std::abs(s.wi.z) / s.pdf, (1.0 - F) * eta * eta, 1e-5, "transmission weight");

    // Leaving the glass beyond the critical angle: no transmission.
    check(glass.sample(directionFromCos(-0.5f), 0.5f, 0.5f).pdf == 0.0f, "total internal reflection");
}

void testMicrofacet() {
    for (float alpha : {0.15f, 0.5f, 1.0f}) {
        for (float c : {0.95f, 0.5f, 0.2f}) {
            const Vec3 wo = directionFromCos(c);
            const std::string tag = " (alpha " + std::to_string(alpha) + ", cos " + std::to_string(c) + ")";

            MicrofacetReflectionBSDF metal(Color(1.0f), Fresnel::schlick(Color(0.9f)), alpha);
            checkSampleConsistency(metal, wo, "GGX reflection" + tag);
            checkAlbedoAgreement(metal, wo, "GGX reflection" + tag);
            Rng rngA, rngB;
            const double albedo = sampledAlbedo(metal, wo, 200000, rngA);
            check(albedo <= 1.0 + 1e-2, "GGX reflection conserves energy" + tag);
            const double p = pdfIntegral(metal, wo, 1000000, rngB);
            check(p <= 1.03 && p > 0.5, "GGX reflection pdf integral" + tag);

            MicrofacetTransmissionBSDF glass(Color(1.0f), 1.0f, 1.5f, alpha);
            checkSampleConsistency(glass, wo, "GGX transmission" + tag);
            checkAlbedoAgreement(glass, wo, "GGX transmission" + tag, 0.05);
            // Leaving the medium.
            const Vec3 woInside = directionFromCos(-c);
            checkSampleConsistency(glass, woInside, "GGX transmission, inside" + tag);
            checkAlbedoAgreement(glass, woInside, "GGX transmission, inside" + tag, 0.05);
        }
    }

    // Reciprocity of the reflection lobe.
    MicrofacetReflectionBSDF m(Color(1.0f), Fresnel::schlick(Color(0.7f)), 0.4f);
    const Vec3 a = directionFromCos(0.8f, 0.2f), b = directionFromCos(0.3f, 2.0f);
    checkNear(m.evaluate(a, b).x, m.evaluate(b, a).x, 1e-4, "GGX reciprocity");

    // A nearly smooth microfacet lobe approaches the delta lobe.
    const Vec3 wo = directionFromCos(0.7f);
    Rng rng;
    MicrofacetReflectionBSDF sharp(Color(1.0f), Fresnel::dielectric(1.0f, 1.5f), 0.02f);
    SpecularReflectionBSDF delta(Color(1.0f), Fresnel::dielectric(1.0f, 1.5f));
    checkNear(sampledAlbedo(sharp, wo, 200000, rng), sampledAlbedo(delta, wo, 10, rng), 0.01,
              "sharp GGX reflection ~ mirror");
    MicrofacetTransmissionBSDF sharpGlass(Color(1.0f), 1.0f, 1.5f, 0.02f);
    SpecularTransmissionBSDF smoothGlass(Color(1.0f), 1.0f, 1.5f);
    checkNear(sampledAlbedo(sharpGlass, wo, 200000, rng), sampledAlbedo(smoothGlass, wo, 10, rng), 0.02,
              "sharp GGX transmission ~ refraction");
}

void testMaterials() {
    Rng rng;
    const Vec3 wo = directionFromCos(0.6f);

    struct Case { std::string name; Material material; };
    const Case cases[] = {
        {"diffuse", Material::diffuse(Color(0.7f))},
        {"plastic", Material::plastic(Color(0.7f), 0.3f)},
        {"rough plastic", Material::plastic(Color(0.7f), 0.8f)},
        {"smooth plastic", Material::plastic(Color(0.7f), 0.0f)},
        {"rough metal", Material::metal(Color(0.9f, 0.7f, 0.4f), 0.4f)},
        {"mirror", Material::mirror(Color(0.9f))},
        {"half metal", [] { Material m = Material::plastic(Color(0.8f), 0.3f); m.metallic = 0.5f; return m; }()},
    };
    for (const Case& c : cases) {
        auto bsdf = c.material.createBSDF();
        const double albedo = sampledAlbedo(*bsdf, wo, 300000, rng);
        check(albedo <= 1.0 + 1e-2, c.name + ": energy conserving (albedo " + std::to_string(albedo) + ")");
        check(albedo > 0.1, c.name + ": reflects something");
        // Evaluation never sees delta lobes, so the albedo comparison needs a non-delta BSDF.
        if (!bsdf->isDelta() && !hasType(bsdf->type(), BSDFType::Specular)) {
            checkSampleConsistency(*bsdf, wo, c.name);
            checkAlbedoAgreement(*bsdf, wo, c.name);
        }
    }

    // A white diffuse surface returns exactly the light it receives.
    {
        auto bsdf = Material::diffuse(Color(1.0f)).createBSDF();
        checkNear(sampledAlbedo(*bsdf, wo, 300000, rng), 1.0, 0.01, "white diffuse furnace");
    }

    // Glass: the mixed reflection/refraction weights add up to F + (1 - F) * eta^2.
    {
        auto bsdf = Material::glass(1.5f).createBSDF();
        const double F = fresnelDielectric(wo.z, 1.0f, 1.5f);
        checkNear(sampledAlbedo(*bsdf, wo, 400000, rng), F + (1.0 - F) / 2.25, 0.01, "glass weights");
        check(bsdf->isDelta(), "glass is delta");

        // Rough glass keeps about the same total.
        Material rough = Material::glass(1.5f);
        rough.roughness = 0.3f;
        auto roughBsdf = rough.createBSDF();
        check(!roughBsdf->isDelta(), "rough glass is not delta");
        const double r = sampledAlbedo(*roughBsdf, wo, 400000, rng);
        check(std::abs(r - (F + (1.0 - F) / 2.25)) < 0.08, "rough glass total " + std::to_string(r));
        checkSampleConsistency(*roughBsdf, wo, "rough glass");
    }

    // A light source emits but does not scatter.
    {
        Material light = Material::emissive(Color(5.0f));
        check(light.isEmissive(), "emissive material emits");
        auto bsdf = light.createBSDF();
        check(bsdf->sample(wo, 0.3f, 0.6f).pdf == 0.0f, "emissive material does not reflect");
        check(bsdf->evaluate(wo, directionFromCos(0.5f)).isBlack(), "emissive material evaluates to black");
    }

    // Derived properties.
    check(Material::mirror().isSpecular() && !Material::mirror().isTransmissive(), "mirror flags");
    check(Material::glass().isSpecular() && Material::glass().isTransmissive(), "glass flags");
    check(!Material::diffuse(Color(0.5f)).isSpecular(), "diffuse is not specular");
    check(!Material::metal(Color(1.0f), 0.5f).isSpecular(), "rough metal is not specular");
}

// A material with an extra parameter derives from Material and overrides createBSDF().
class ScaledDiffuseMaterial : public Material {
public:
    float scale = 0.5f;

    std::unique_ptr<BSDF> createBSDF() const override {
        return std::make_unique<LambertianBSDF>(baseColor * scale);
    }
};

void testDerivedMaterial() {
    ScaledDiffuseMaterial derived;
    derived.baseColor = Color(1.0f);
    derived.scale = 0.25f;
    const Material* base = &derived;
    auto bsdf = base->createBSDF();
    Rng rng;
    checkNear(sampledAlbedo(*bsdf, directionFromCos(0.5f), 100000, rng), 0.25, 0.01,
              "derived material goes through the shared interface");
}

} // namespace

int main() {
    testLambertian();
    testSpecular();
    testMicrofacet();
    testMaterials();
    testDerivedMaterial();

    if (failures == 0) std::cout << "All material tests passed\n";
    else std::cerr << failures << " material check(s) failed\n";
    return failures == 0 ? 0 : 1;
}

#include "render/materials/mixture_bsdf.h"

namespace render {

void MixtureBSDF::add(std::unique_ptr<BSDF> lobe, float weight, bool substrate) {
    if (!lobe || weight <= 0.0f || lobes_.size() >= MAX_LOBES) return;
    lobes_.push_back({std::move(lobe), weight, substrate});
}

float MixtureBSDF::effectiveWeight(const Lobe& lobe, const Vec3& wo) const {
    if (!lobe.substrate) return lobe.weight;
    return lobe.weight * std::max(0.0f, 1.0f - substrateFresnel_(cosTheta(wo)).x);
}

float MixtureBSDF::selection(const Vec3& wo, float weights[MAX_LOBES], float scores[MAX_LOBES]) const {
    float total = 0.0f;
    for (size_t i = 0; i < lobes_.size(); ++i) {
        weights[i] = effectiveWeight(lobes_[i], wo);
        scores[i] = weights[i] * lobes_[i].bsdf->rho(wo).maxComponent();
        total += scores[i];
    }
    return total;
}

Color MixtureBSDF::evaluate(const Vec3& wo, const Vec3& wi) const {
    Color f(0.0f);
    for (const Lobe& lobe : lobes_) {
        if (lobe.bsdf->isDelta()) continue;
        f += lobe.bsdf->evaluate(wo, wi) * effectiveWeight(lobe, wo);
    }
    return f;
}

float MixtureBSDF::pdf(const Vec3& wo, const Vec3& wi) const {
    float weights[MAX_LOBES], scores[MAX_LOBES];
    const float total = selection(wo, weights, scores);
    if (total <= 0.0f) return 0.0f;

    float p = 0.0f;
    for (size_t i = 0; i < lobes_.size(); ++i) {
        if (lobes_[i].bsdf->isDelta()) continue;
        p += scores[i] * lobes_[i].bsdf->pdf(wo, wi);
    }
    return p / total;
}

BSDFSample MixtureBSDF::sample(const Vec3& wo, float u1, float u2) const {
    BSDFSample none;
    float weights[MAX_LOBES], scores[MAX_LOBES];
    const float total = selection(wo, weights, scores);
    if (total <= 0.0f) return none;

    // Pick a lobe and reuse the leftover of u1 as that lobe's first random number.
    float u = u1 * total;
    size_t chosen = lobes_.size() - 1;
    for (size_t i = 0; i < lobes_.size(); ++i) {
        if (scores[i] <= 0.0f) continue;
        if (u < scores[i] || i + 1 == lobes_.size()) {
            chosen = i;
            break;
        }
        u -= scores[i];
    }
    if (scores[chosen] <= 0.0f) return none;
    const float v1 = std::min(u / scores[chosen], 0.99999994f);

    const Lobe& lobe = lobes_[chosen];
    BSDFSample s = lobe.bsdf->sample(wo, v1, u2);
    if (s.pdf <= 0.0f || s.f.isBlack()) return none;

    const float pChosen = scores[chosen] / total;
    if (lobe.bsdf->isDelta()) {
        s.f = s.f * weights[chosen];
        s.pdf *= pChosen;
        return s;
    }

    s.f = evaluate(wo, s.wi);
    s.pdf = pdf(wo, s.wi);
    if (s.pdf <= 0.0f) return none;
    return s;
}

BSDFType MixtureBSDF::type() const {
    BSDFType t = BSDFType::None;
    for (const Lobe& lobe : lobes_) t = t | lobe.bsdf->type();
    return t;
}

bool MixtureBSDF::isDelta() const {
    if (lobes_.empty()) return false;
    for (const Lobe& lobe : lobes_) {
        if (!lobe.bsdf->isDelta()) return false;
    }
    return true;
}

Color MixtureBSDF::rho(const Vec3& wo) const {
    Color r(0.0f);
    for (const Lobe& lobe : lobes_) r += lobe.bsdf->rho(wo) * effectiveWeight(lobe, wo);
    return r;
}

} // namespace render

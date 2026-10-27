// Weighted combination of BSDF lobes

#pragma once

#include "bsdf.h"
#include "fresnel.h"
#include <memory>
#include <vector>

namespace render {

/**
 * @class MixtureBSDF
 * @brief Sum of weighted lobes, sampled by picking one lobe at a time.
 *
 * A lobe is chosen with probability proportional to weight * rho(wo). Delta lobes return
 * their own sample (pdf = selection probability); for a non-delta sample the value and pdf
 * are the sums over all non-delta lobes, so the estimator stays unbiased whichever lobe
 * produced the direction.
 *
 * Lobes added with `substrate = true` sit under a dielectric interface: the light they
 * receive is scaled by (1 - Fresnel(wo)), which keeps the whole BSDF energy conserving.
 */
class MixtureBSDF : public BSDF {
public:
    void add(std::unique_ptr<BSDF> lobe, float weight, bool substrate = false);

    /** Interface that the substrate lobes sit below. */
    void setSubstrateFresnel(const Fresnel& fresnel) { substrateFresnel_ = fresnel; }

    bool empty() const { return lobes_.empty(); }
    size_t lobeCount() const { return lobes_.size(); }

    Color evaluate(const Vec3& wo, const Vec3& wi) const override;
    BSDFSample sample(const Vec3& wo, float u1, float u2) const override;
    float pdf(const Vec3& wo, const Vec3& wi) const override;

    BSDFType type() const override;
    bool isDelta() const override;
    Color rho(const Vec3& wo) const override;

private:
    struct Lobe {
        std::unique_ptr<BSDF> bsdf;
        float weight;
        bool substrate;
    };

    static constexpr size_t MAX_LOBES = 6;

    std::vector<Lobe> lobes_;
    Fresnel substrateFresnel_;

    float effectiveWeight(const Lobe& lobe, const Vec3& wo) const;

    // Fills the lobe weights for `wo` and the selection scores; returns their sum.
    float selection(const Vec3& wo, float weights[MAX_LOBES], float scores[MAX_LOBES]) const;
};

} // namespace render

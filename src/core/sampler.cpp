#include "render/core/sampler.h"

namespace render {

UniformSampler::UniformSampler(uint64_t seed)
    : rng_(seed), dist_(0.0f, 1.0f) {}

float UniformSampler::get1D() {
    return dist_(rng_);
}

std::pair<float, float> UniformSampler::get2D() {
    return {dist_(rng_), dist_(rng_)};
}

void UniformSampler::setSeed(uint64_t seed) {
    rng_.seed(seed);
}

} // namespace render

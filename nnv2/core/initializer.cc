#include <chrono>
#include <random>

#include "initializer.h"

namespace nnv2 {

void LecunNormal::initialize(std::vector<float> &data) {
    float s = std::sqrt(1.f / fan_in);
    normal_init(data, s);
}

void XavierNormal::initialize(std::vector<float> &data) {
    float s = std::sqrt(2.f / (fan_in + fan_out));
    normal_init(data, s);
}

void KaimingNormal::initialize(std::vector<float> &data) {
    float s = std::sqrt(2.f / fan_in);
    normal_init(data, s);
}

void SimplyNormal::initialize(std::vector<float> &data) {
    normal_init(data, 0.1f);
}

void LecunUniform::initialize(std::vector<float> &data) {
    float r = std::sqrt(1.f / fan_in);
    uniform_init(data, r);
}

void XavierUniform::initialize(std::vector<float> &data) {
    float r = std::sqrt(6.f / (fan_in + fan_out));
    uniform_init(data, r);
}

void KaomingUniform::initialize(std::vector<float> &data) {
    float r = std::sqrt(6.f / fan_in);
    uniform_init(data, r);
}

void SimplyUniform::initialize(std::vector<float> &data) {
    uniform_init(data, 0.01f);
}

static void normal_init(std::vector<float> &data, float s) {
    unsigned seed = (unsigned)std::chrono::steady_clock::now().time_since_epoch().count();
    std::default_random_engine rng(seed);

    std::normal_distribution<float> dist(0.f, s);

    std::for_each(data.begin(), data.end(), [&](float &e) { e = dist(rng); });
}

static void uniform_init(std::vector<float> &data, float r) {
    unsigned seed = (unsigned)std::chrono::steady_clock::now().time_since_epoch().count();
    std::default_random_engine rng(seed);

    std::uniform_real_distribution<float> dist(-r, r);

    std::for_each(data.begin(), data.end(), [&](float &e) { e = dist(rng); });
}

} // namespace nnv2
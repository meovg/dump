#pragma once

#include <Eigen/Core>
#include <algorithm>
#include <assert.h>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

using namespace Eigen;

namespace ann {

typedef Matrix<float, Dynamic, Dynamic, RowMajor> MatXf;
typedef Matrix<float, Dynamic, 1> VecXf;
typedef Matrix<float, 1, Dynamic> RowVecXf;
typedef Matrix<int, Dynamic, Dynamic, RowMajor> MatXi;
typedef Matrix<int, Dynamic, 1> VecXi;

void init_weight(MatXf &W, int fan_in, int fan_out, std::string option) {
    unsigned seed = (unsigned)std::chrono::steady_clock::now().time_since_epoch().count();
    std::default_random_engine e(seed);

    if (option == "lecun_normal") {
        float s = std::sqrt(1.f / fan_in);
        std::normal_distribution<float> dist(0, s);
        std::for_each(W.data(), W.data() + W.size(), [&](float &elem) { elem = dist(e); });
    } else if (option == "lecun_uniform") {
        float r = std::sqrt(1.f / fan_in);
        std::uniform_real_distribution<float> dist(-r, r);
        std::for_each(W.data(), W.data() + W.size(), [&](float &elem) { elem = dist(e); });
    } else if (option == "xavier_normal") {
        float s = std::sqrt(2.f / (fan_in + fan_out));
        std::normal_distribution<float> dist(0, s);
        std::for_each(W.data(), W.data() + W.size(), [&](float &elem) { elem = dist(e); });
    } else if (option == "xavier_uniform") {
        float r = std::sqrt(6.f / (fan_in + fan_out));
        std::uniform_real_distribution<float> dist(-r, r);
        std::for_each(W.data(), W.data() + W.size(), [&](float &elem) { elem = dist(e); });
    } else if (option == "kaiming_normal") {
        float s = std::sqrt(2.f / fan_in);
        std::normal_distribution<float> dist(0, s);
        std::for_each(W.data(), W.data() + W.size(), [&](float &elem) { elem = dist(e); });
    } else if (option == "kaiming_uniform") {
        float r = std::sqrt(6.f / fan_in);
        std::uniform_real_distribution<float> dist(-r, r);
        std::for_each(W.data(), W.data() + W.size(), [&](float &elem) { elem = dist(e); });
    } else if (option == "normal") {
        std::normal_distribution<float> dist(0.f, 0.1f);
        std::for_each(W.data(), W.data() + W.size(), [&](float &elem) { elem = dist(e); });
    } else if (option == "uniform") {
        std::uniform_real_distribution<float> dist(-0.01f, 0.01f);
        std::for_each(W.data(), W.data() + W.size(), [&](float &elem) { elem = dist(e); });
    } else {
        std::cout << "Invalid initialization." << std::endl;
        exit(1);
    }
}

int calc_outsize(int in_size, int kernel_size, int stride, int pad) {
    return (int)std::floor((in_size + 2 * pad - kernel_size) / stride) + 1;
}

} // namespace ann
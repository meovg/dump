#pragma once

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace nnv2 {

// Check macros
#define CHECK_EQ(val1, val2, message)                                          \
    do {                                                                       \
        if ((val1) != (val2)) {                                                \
            std::cerr << __FILE__ << "(" << __LINE__ << "): " << (message)     \
                      << std::endl;                                            \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

#define CHECK_COND(cond, message)                                              \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::cerr << __FILE__ << "(" << __LINE__ << "): " << (message)     \
                      << std::endl;                                            \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

//
// array.cc
// Array object similar to numpy array
class Array {
public:
    explicit Array(const std::vector<int> &_shape);
    explicit Array(const std::vector<int> &_shape, float value);
    explicit Array(const std::vector<int> &_shape,
                   const std::vector<float> &_vec);

    Array(const Array &other);
    Array(Array &&other);
    Array &operator=(const Array &other);
    Array &operator=(Array &&other);

    void reshape(const std::vector<int> &_shape);
    void resize(const std::vector<int> &_shape);

    void zero();

    const std::vector<int> &get_shape() const { return shape; }

    std::vector<float> &get_vec() { return vec; }
    const std::vector<float> &get_vec() const { return vec; }

private:
    void check_shape();

    std::vector<float> vec;
    std::vector<int> shape;
};

// Initialzing Array from smart pointer
void init_array(std::unique_ptr<Array> &ptr, const std::vector<int> &shape);

// As several functions in the training process require temporary Array objects,
// and they can be called multiple times if the size of train data is large,
// it's better to reuse these temporary Array objects instead of creating new
// ones on every call
using ArrayMap = std::unordered_map<std::string, std::unique_ptr<Array>>;

void init_cache(ArrayMap &map, std::string key, const std::vector<int> &shape);

//
// Data type representing a pair of parameter and gradient Array
// Used by neural network layers to pass their parameters and gradients to an
// Optimizer, where parameters are updated after each training step
using Param = std::pair<Array *, Array *>;

} // namespace nnv2
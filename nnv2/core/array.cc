#include <algorithm>
#include <cassert>
#include <cfloat>
#include <numeric>
#include <utility>

#include "array.h"
#include "utils.h"

namespace nnv2 {

Array::Array(const std::vector<int> &_shape) : shape(_shape) {
    int size =
        std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<int>());
    vec.resize(size, FLT_MIN);
}

Array::Array(const std::vector<int> &_shape, float _value) : shape(_shape) {
    int size =
        std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<int>());
    vec.resize(size, _value);
}

Array::Array(const std::vector<int> &_shape, const std::vector<float> &_vec)
    : shape(_shape), vec(_vec.begin(), _vec.end()) {
    check_shape();
}

Array::Array(const Array &other) {
    *this = other;
}

Array::Array(Array &&other) {
    *this = std::move(other);
}

Array &Array::operator=(const Array &other) {
    if (this != &other) {
        shape = other.shape;
        vec = other.vec;
    }
    return *this;
}

Array &Array::operator=(Array &&other) {
    if (this != &other) {
        shape = std::move(other.shape);
        vec = std::move(other.vec);
    }
    return *this;
}

void Array::zero() {
    // Fill the array with the smallest positive float value instead of 0 to
    // avoid division by 0
    std::fill(vec.begin(), vec.end(), FLT_MIN);
}

void Array::reshape(const std::vector<int> &_shape) {
    shape = _shape;
    check_shape();
}

void Array::resize(const std::vector<int> &_shape) {
    shape = _shape;
    int size =
        std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<int>());
    if (size != vec.size()) {
        vec.resize(size);
    }
}

void Array::check_shape() {
    int size =
        std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<int>());
    CHECK_EQ(size, vec.size(), "Array: shape mismatched with size");
}

} // namespace nnv2
#include <algorithm>
#include <cassert>
#include <cfloat>
#include <utility>

#include "array.h"
#include "utils.h"

namespace nnv2 {

Array::Array(const std::vector<int> &_shape) : shape(_shape) {
    int size = 1;
    for (int i = 0; i < shape.size(); i++) {
        size *= shape[i];
    }
    vec.resize(size, FLT_MIN);
}

Array::Array(const std::vector<int> &_shape, float _value) : shape(_shape) {
    int size = 1;
    for (int i = 0; i < shape.size(); i++) {
        size *= shape[i];
    }
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
    std::fill(vec.begin(), vec.end(), FLT_MIN);
}

void Array::reshape(const std::vector<int> &_shape) {
    shape = _shape;
    check_shape();
}

void Array::resize(const std::vector<int> &_shape) {
    shape = _shape;

    int size = 1;
    for (int i = 0; i < _shape.size(); i++) {
        size *= _shape[i];
    }

    if (size != vec.size()) {
        vec.resize(size);
    }
}

void Array::check_shape() {
    int size = 1;
    for (int i = 0; i < shape.size(); i++) {
        size *= shape[i];
    }

    CHECK_EQ(size, vec.size(),
             "Array::check_shape: shape mismatched with number of elements");
}

} // namespace nnv2
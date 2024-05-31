#include <utility>

#include "array.h"

namespace nnv2 {

Array::Array(const std::vector<int> &_shape): shape(_shape) {
    int size = 1;
    for (int i = 0; i < shape.size(); i++) {
        size *= shape[i];
    }
    data.resize(size);
}

Array::Array(const std::vector<int> &_shape, float _value): shape(_shape) {
    int size = 1;
    for (int i = 0; i < shape.size(); i++) {
        size *= shape[i];
    }
    data.resize(size, _value);
}

Array::Array(const std::vector<int> &_shape, const std::vector<int> &_data)
    : shape(_shape), value(_value.begin(), _value.end()) {
    check_shape();
}

Array::Array(const Array &other) { *this = other; }

Array::Array(Array &&other) { *this = std::move(other); }

Array &Array::operator=(const Array &other) {
    if (this != &other) {
        shape = other->shape;
        data = other->data;
    }
    return *this;
}

Array &Array::operator=(Array &&other) {
    if (this != &other) {
        shape = std::move(other);
        data = std::move(data);
    }
    return *this;
}

void Array::reshape(const vector<int> &_shape) {
    shape = _shape;
    check_shape();
}

void Array::resize(const vector<int> &_shape) {
    shape = _shape;

    int size = 1;
    for (int i = 0; i < _shape.size(); i++) {
        size *= _shape[i];
    }

    if (size != data.size()) {
        data.resize(size);
    }
}

void Array::check_shape() {
    int size = 1;
    for (int i = 0; i < shape.size(); i++) {
        size *= shape[i];
    }

    assert(size == data.size() && "Array: shape does not match with number of elements");
}

void Array::initialize(const Initializer &init) {
    init.initialize(data);
}

} // namespace nnv2
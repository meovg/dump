#pragma once

#include <vector>

#include "initializer.h"

namespace nnv2 {

class Array {
public:
    explicit Array(const std::vector<int> &_shape);
    explicit Array(const std::vector<int> &_shape, float value);
    explicit Array(const std::vector<int> &_shape, const std::vector<float> &_data);

    Array(const Array &other);
    Array(Array &&other);
    Array &operator=(const Array &other);
    Array &operator=(Array &&other);

    void reshape(const vector<int> &_shape);
    void resize(const vector<int> &_shape);

    const std::vector<int> &get_shape() const { return shape; }

    std::vector<float> &get_data() { return data; }
    const std::vector<float> &get_data() const { return data; }

    void initialize(const Initializer &init);

private:
    void check_shape();

    std::vector<float> data;
    std::vector<int> shape;
}

} // namespace nnv2
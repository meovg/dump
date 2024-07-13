#pragma once

#include <vector>

namespace nnv2 {

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

} // namespace nnv2
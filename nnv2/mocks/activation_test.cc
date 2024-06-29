#include <cassert>
#include <iomanip>
#include <iostream>
#include <vector>

#include "activation.h"

using namespace nnv2;

void test_relu_forward() {
    Array input({1, 1, 4, 3}, {-1, 2, -3, 4, -5, 6, -7, 8, -9, 10, -11, 12});
    Array output({1, 1, 4, 3});

    relu_forward(&output, &input);
    assert(output.get_vec() ==
           std::vector<float>({0, 2, 0, 4, 0, 6, 0, 8, 0, 10, 0, 12}));

    std::cout << "test_relu_forward: Passed" << std::endl;
}

void test_relu_backward() {
    Array input({1, 1, 4, 3}, {-1, 2, -3, 4, -5, 6, -7, 8, -9, 10, -11, 12});
    Array output_grad({1, 1, 4, 3}, {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
    Array input_grad({1, 1, 4, 3});

    relu_backward(&input_grad, &output_grad, &input);
    assert(input_grad.get_vec() ==
           std::vector<float>({0, 2, 0, 4, 0, 6, 0, 8, 0, 10, 0, 12}));

    std::cout << "test_relu_backward: Passed" << std::endl;
}

void test_softmax_forward() {
    Array input({1, 1, 4, 3}, {1, 2, 3, 1, 2, -1, 0, 0, 0, -1, 2, 0});
    Array output({1, 1, 4, 3});

    softmax_forward(&output, &input);

    const std::vector<float> &output_ref = output.get_vec();
    for (float x : output_ref) {
        std::cout << x << " ";
    }
    std::cout << std::endl;

    std::cout << "test_softmax_forward: DIY!" << std::endl;
}

int main() {
    test_relu_forward();
    test_relu_backward();
    test_softmax_forward();
}
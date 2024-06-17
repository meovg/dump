#include <cassert>
#include <iostream>
#include <vector>

#include "linear.h"

using namespace nnv2;

void test_linear_transform(void) {
    Array input({ 3, 4 }, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 });
    Array weights({ 4, 2 }, { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8 });
    Array output({ 3, 2 });

    linear_transform(&output, &input, &weights);
    assert(output.get_vec() == std::vector<float>({ 5, 6, 11.4, 14, 17.8, 22 }));

    std::cout << "test_linear_transform: Passed" << std::endl;
}

void test_add_bias_to_output(void) {
    Array bias({ 1, 2 }, { 1, 2 });
    Array output({ 3, 2 }, { 5, 6, 11, 14, 17, 22 });

    linear_add_bias_to_output(&output, &bias);
    assert(output.get_vec() == std::vector<float>({ 6, 8, 12, 16, 18, 24 }));

    std::cout << "test_add_bias_to_output: Passed" << std::endl;
}

void test_propagate_linear_gradient(void) {
    Array output_grad({ 2, 3 }, { 0, 1, 2, 3, 4, 5 });
    Array input({ 2, 3 }, { 0, 1, 2, 3, 4, 5 });
    Array weights({ 3, 3 }, { 0, 1, 2, 3, 4, 5, 6, 7, 8 });
    Array input_grad({ 2, 3 });
    Array weights_grad({ 3, 3 });

    linear_propagate_gradient(&input_grad, &weights_grad, &input, &weights, &output_grad);

    assert(input_grad.get_vec() == std::vector<float>({ 5, 14, 23, 14, 50, 86 }));
    assert(weights_grad.get_vec() == std::vector<float>({ 9, 12, 15, 12, 17, 22, 15, 22, 29 }));

    std::cout << "test_propagate_linear_gradient: Passed" << std::endl;
}

void test_propagate_bias_gradient(void) {
    Array output_grad({ 2, 3 }, { 0, 1, 2, 3, 4, 5 });
    Array bias_grad({ 1, 3 });

    linear_propagate_bias_gradient(&bias_grad, &output_grad);
    assert(bias_grad.get_vec() == std::vector<float>({ 3, 5, 7 }));

    std::cout << "test_propagate_bias_gradient: Passed" << std::endl;
}

int main(void) {
    test_linear_transform();
    test_add_bias_to_output();
    test_propagate_linear_gradient();
    test_propagate_bias_gradient();
}
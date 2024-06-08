#include <vector>

#include "linear.h"
#include "mathfunc.h"
#include "utils.h"

namespace nnv2 {

Linear::Linear(int in_size, int out_size, const Initializer *init)
        : in_size(in_size), out_size(out_size) {
    weights.reset(new Array({ in_size, out_size }));
    weights->initialize(init);
    weights_grad.reset(new Array({ in_size, out_size }));

    bias.reset(new Array({ 1, out_size }, 0.f));
    bias_grad.reset(new Array({ 1, out_size }));
}

void linear_transform(Array *output, const Array *input, const Array *weights) {
    func_matmul(output, input, weights);
}

void add_bias_to_output(Array *output, const Array *bias) {
    int batch_size = output->get_shape()[0];
    int out_size = output->get_shape()[1];

    for (int i = 0; i < batch_size; i++) {
        for (int j = 0; j < out_size; j++) {
            output->get_vec()[i * out_size + j] += bias->get_vec()[j];
        }
    }
}

void Linear::forward() {
    const Array *input = prev->get_output();
    int batch_size = input->get_shape()[0];

    // initialize storage for output
    std::vector<int> output_shape = { batch_size, out_size };
    INIT_ARRAY(output, output_shape);

    // calculate output neurons from input neurons and weights (bias is not added yet)
    linear_transform(output.get(), input, weights.get());

    // adding bias vector to output neuron
    add_bias_to_output(output.get(), bias.get());
}

void propagate_linear_gradient(Array *input_grad, Array *weights_grad, const Array *input,
                               const Array *weights, const Array *output_grad) {
    // calculate transpose of input: X^T
    std::vector<int> input_t_shape = { input->get_shape()[1], input->get_shape()[0] };
    Array input_t(input_t_shape);
    func_transpose(&input_t, input);

    // calculate transpose of output: W^T
    std::vector<int> weights_t_shape = { weights->get_shape()[1], weights->get_shape()[0] };
    Array weights_t(weights_t_shape);
    func_transpose(&weights_t, weights);

    // calculate gradient with respect to weight: dW = X^T * dA
    func_matmul(weights_grad, &input_t, output_grad);

    // calculate gradient with respect to input: dX = dA * W^T
    func_matmul(input_grad, output_grad, &weights_t);
}

void propagate_bias_gradient(Array *bias_grad, const Array *output_grad) {
    // calculate gradient with respect to bias: db = sum(dA, axis=0)
    func_sum(bias_grad, output_grad, 0, false);
}

void Linear::backward() {
    const Array *input = prev->get_output();
    const Array *output_grad = next->get_grad();

    INIT_ARRAY(grad, input->get_shape());

    propagate_bias_gradient(bias_grad.get(), output_grad);
    propagate_linear_gradient(grad.get(), weights_grad.get(), input, weights.get(), output_grad);
}

} // namespace nnv2
#include <vector>

#include "linear.h"
#include "mathfunc.h"
#include "utils.h"

namespace nnv2 {

Linear::Linear(int in_feats, int out_feats, const Initializer *init)
        : in_feats(in_feats), out_feats(out_feats) {
    weight.reset(new Array({ in_feats, out_feats }));
    weight->initialize(init);
    weight_grad.reset(new Array({ in_feats, out_feats }));

    bias.reset(new Array({ 1, out_feats }, 0.f));
    bias_grad.reset(new Array({ 1, out_feats }));
}

void linear_transform(Array *output, const Array *input, const Array *weight) {
    func_matmul(output, input, weight);
}

void linear_add_bias_to_output(Array *output, const Array *bias) {
    int batch_size = output->get_shape()[0];
    int out_feats = output->get_shape()[1];

    for (int i = 0; i < batch_size; i++) {
        for (int j = 0; j < out_feats; j++) {
            output->get_vec()[i * out_feats + j] += bias->get_vec()[j];
        }
    }
}

void Linear::forward() {
    const Array *input = prev->get_output();
    int batch_size = input->get_shape()[0];

    // initialize storage for output
    std::vector<int> output_shape = { batch_size, out_feats };
    INIT_ARRAY(output, output_shape);

    // calculate output neurons from input neurons and weight (bias is not added yet)
    linear_transform(output.get(), input, weight.get());

    // adding bias vector to output neuron
    linear_add_bias_to_output(output.get(), bias.get());
}

void linear_propagate_gradient(Array *input_grad, Array *weight_grad, const Array *input,
                               const Array *weight, const Array *output_grad) {
    // calculate transpose of input: X^T
    std::vector<int> input_t_shape = { input->get_shape()[1], input->get_shape()[0] };
    Array input_t(input_t_shape);
    func_transpose(&input_t, input);

    // calculate transpose of output: W^T
    std::vector<int> weight_t_shape = { weight->get_shape()[1], weight->get_shape()[0] };
    Array weight_t(weight_t_shape);
    func_transpose(&weight_t, weight);

    // calculate gradient with respect to weight: dW = X^T * dA
    func_matmul(weight_grad, &input_t, output_grad);

    // calculate gradient with respect to input: dX = dA * W^T
    func_matmul(input_grad, output_grad, &weight_t);
}

void linear_propagate_bias_gradient(Array *bias_grad, const Array *output_grad) {
    // calculate gradient with respect to bias: db = sum(dA, axis=0)
    func_sum(bias_grad, output_grad, 0, false);
}

void Linear::backward() {
    const Array *input = prev->get_output();
    const Array *output_grad = next->get_grad();

    INIT_ARRAY(grad, input->get_shape());

    linear_propagate_bias_gradient(bias_grad.get(), output_grad);
    linear_propagate_gradient(grad.get(), weight_grad.get(), input, weight.get(), output_grad);
}

} // namespace nnv2
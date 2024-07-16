#include <vector>

#include "common.h"
#include "linear.h"
#include "mathfunc.h"

namespace nnv2 {

void linear_forward(Array *output, const Array *input, const Array *weight) {
    func_matmul(output, input, weight);
}

void linear_forward_bias(Array *output, const Array *bias) {
    int batch_size = output->get_shape()[0];
    int out_feats = output->get_shape()[1];

    for (int i = 0; i < batch_size; i++) {
        for (int j = 0; j < out_feats; j++) {
            output->get_vec()[i * out_feats + j] += bias->get_vec()[j];
        }
    }
}

void linear_backward(Array *input_grad, Array *weight_grad, const Array *input,
                     const Array *weight, const Array *output_grad,
                     ArrayMap &cache) {
    // calculate transpose of input: X^T
    init_cache(cache, "input_t",
               {input->get_shape()[1], input->get_shape()[0]});
    func_transpose(cache["input_t"].get(), input);

    // calculate transpose of weight: W^T
    init_cache(cache, "weight_t",
               {weight->get_shape()[1], weight->get_shape()[0]});
    func_transpose(cache["weight_t"].get(), weight);

    // calculate gradient with respect to weight: dW = X^T * dA
    func_matmul(weight_grad, cache["input_t"].get(), output_grad);
    // calculate gradient with respect to input: dX = dA * W^T
    func_matmul(input_grad, output_grad, cache["weight_t"].get());
}

void linear_backward_bias(Array *bias_grad, const Array *output_grad) {
    // calculate gradient with respect to bias: db = sum(dA, axis=0)
    func_sum(bias_grad, output_grad, 0, false);
}

Linear::Linear(int in_feats, int out_feats, const Initializer *init)
    : in_feats(in_feats), out_feats(out_feats) {
    weight.reset(new Array({in_feats, out_feats}));
    init->initialize(weight.get(), in_feats, out_feats);

    weight_grad.reset(new Array({in_feats, out_feats}));

    bias.reset(new Array({1, out_feats}, 0.f));
    bias_grad.reset(new Array({1, out_feats}));
}

std::vector<Param> Linear::get_parameters() {
    return {std::make_pair(weight.get(), weight_grad.get()),
            std::make_pair(bias.get(), bias_grad.get())};
}

void Linear::forward() {
    const Array *input = prev->get_output();
    int batch_size = input->get_shape()[0];

    // initialize storage for output
    init_array(output, {batch_size, out_feats});

    linear_forward(output.get(), input, weight.get());
    linear_forward_bias(output.get(), bias.get());
}

void Linear::backward() {
    const Array *input = prev->get_output();
    const Array *output_grad = next->get_grad();

    init_array(grad, input->get_shape());

    linear_backward_bias(bias_grad.get(), output_grad);
    linear_backward(grad.get(), weight_grad.get(), input, weight.get(),
                    output_grad, cache);
}

} // namespace nnv2
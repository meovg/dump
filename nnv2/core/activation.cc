#include <algorithm>
#include <cmath>
#include <numeric>

#include "activation.h"

namespace nnv2 {

void relu_forward(Array *output, const Array *input) {
    CHECK_EQ(output->get_vec().size(), input->get_vec().size(),
             "relu_forward: output size isn't equal to input size");

    std::transform(input->get_vec().begin(), input->get_vec().end(),
                   output->get_vec().begin(),
                   [](float x) { return std::max(0.f, x); });
}

void relu_backward(Array *input_grad, const Array *output_grad,
                   const Array *input) {
    CHECK_EQ(input_grad->get_vec().size(), output_grad->get_vec().size(),
             "relu_backward: input grad size isn't equal to output grad size");
    CHECK_EQ(input_grad->get_vec().size(), input->get_vec().size(),
             "relu_backward: input grad size isn't equal to input size");

    std::transform(input->get_vec().begin(), input->get_vec().end(),
                   output_grad->get_vec().begin(),
                   input_grad->get_vec().begin(),
                   [](float i, float od) { return i <= 0 ? 0 : od; });
}

void ReLU::forward() {
    Array *input = prev->get_output();
    relu_forward(input, input);
}

void ReLU::backward() {
    const Array *input = prev->get_output();
    Array *output_grad = next->get_grad();
    relu_backward(output_grad, output_grad, input);
}

void softmax_forward(Array *output, const Array *input) {
    CHECK_EQ(output->get_vec().size(), input->get_vec().size(),
             "softmax_forward: output size not equal to input size");

    int batch_size = input->get_shape()[0];
    int batch_stride =
        std::accumulate(input->get_shape().begin() + 1,
                        input->get_shape().end(), 1, std::multiplies<int>());

    for (int i = 0; i < batch_size; i++) {
        auto in_begin = input->get_vec().begin() + i * batch_stride;
        auto out_begin = output->get_vec().begin() + i * batch_stride;

        float max_val = *std::max_element(in_begin, in_begin + batch_stride);
        std::transform(in_begin, in_begin + batch_stride, out_begin,
                       [&](float x) { return expf(x - max_val); });

        float exp_sum =
            std::accumulate(out_begin, out_begin + batch_stride, 0.f);
        for (int k = 0; k < batch_stride; k++) {
            out_begin[k] /= exp_sum;
        }
    }
}

void Softmax::forward() {
    const Array *input = prev->get_output();
    init_array(output, input->get_shape());
    softmax_forward(output.get(), input);

    // for calculating gradient in Loss::calcualte_loss()
    init_array(grad, input->get_shape());
}

void Softmax::backward() {
    // Note: Softmax is often the last activation layer in the neural network
    // and that the gradient is computed in Loss::calculate_loss()
}

} // namespace nnv2
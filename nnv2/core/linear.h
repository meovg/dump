#pragma once

#include "layer.h"
#include "initializer.h"

namespace nnv2 {

class Linear: public Layer {
public:
    Linear(int in_size, int out_size, const Initializer *initializer);

    void forward();
    void backward();

private:
    int in_size;
    int out_size;

    std::unique_ptr<Array> weights;
    std::unique_ptr<Array> weights_grad;
    std::unique_ptr<Array> bias;
    std::unique_ptr<Array> bias_grad;
};

// for testing purpose, local functions are defined here
void linear_transform(Array *output, const Array *input, const Array *weights);
void add_bias_to_output(Array *output, const Array *bias);

void propagate_linear_gradient(Array *input_grad, Array *weights_grad, const Array *input,
                               const Array *weights, const Array *output_grad);
void propagate_bias_gradient(Array *bias_grad, const Array *output_grad);

} // namespace nnv2
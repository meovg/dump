#pragma once

#include "layer.h"
#include "initializer.h"

namespace nnv2 {

class Linear: public Layer {
public:
    Linear(int in_feats, int out_feats, const Initializer *init);

    void forward() override;
    void backward() override;

private:
    int in_feats;
    int out_feats;

    std::unique_ptr<Array> weight;
    std::unique_ptr<Array> weight_grad;
    std::unique_ptr<Array> bias;
    std::unique_ptr<Array> bias_grad;
};

// for testing purpose, local functions are defined here
void linear_forward(Array *output, const Array *input, const Array *weight);
void linear_forward_bias(Array *output, const Array *bias);

void linear_backward(Array *input_grad, Array *weight_grad, const Array *input,
                     const Array *weight, const Array *output_grad);
void linear_backward_bias(Array *bias_grad, const Array *output_grad);

} // namespace nnv2
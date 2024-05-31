#pragma once

#include "array.h"

namespace nnv2 {

// implementation idea:
// a Layer represents a layer in a neural network, which consists of input and output neurons on
// both sides, weights and a temporary gradient values used to re-evaluate the weights
// This Layer should be implemented as abstract class which is later extended into multiple
// types of Layer such as: convolution, linear, activation (softmax, maxpool, avgpool)

class Layer {
public:


private:

}

} // namespace nnv2
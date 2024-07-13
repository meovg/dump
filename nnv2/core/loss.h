#pragma once

#include "layer.h"

namespace nnv2 {

class Loss : public Layer {
public:
    Loss() : Layer() {}
    virtual float calculate_loss(const Array *results) = 0;
};

class CrossEntropyLoss : public Loss {
public:
    CrossEntropyLoss() : Loss() {}
    float calculate_loss(const Array *results) override;
};

} // namespace nnv2
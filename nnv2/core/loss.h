#pragma once

#include "layer.h"

namespace nnv2 {

class Loss {
public:
    Loss() {}

    void set_target(Layer *layer) { target = layer; }
    virtual float calculate_loss(const Array *results) = 0;

protected:
    Layer *target;
};

class CrossEntropyLoss : public Loss {
public:
    CrossEntropyLoss() : Loss() {}
    float calculate_loss(const Array *results) override;
};

} // namespace nnv2
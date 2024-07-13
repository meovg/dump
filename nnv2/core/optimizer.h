#pragma once

#include <vector>

#include "array.h"
#include "layer.h"

namespace nnv2 {

class SGD {
public:
    SGD(float lr, float decay) : learning_rate(lr), decay(decay) {}

    void add_parameters(std::vector<Param> params);
    void update_parameters(int batch_size);

private:
    float learning_rate;
    float decay;

    std::vector<Array *> parameter_list;
    std::vector<Array *> grad_list;
};

} // namespace nnv2
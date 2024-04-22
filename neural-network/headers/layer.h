#pragma once

#include "common.h"

namespace ann {

class Layer {
public:
    bool is_first;
    bool is_last;
    MatXf output;
    MatXf delta;

public:
    Layer(): is_first(false), is_last(false) {}
    virtual void set_layer(const std::vector<int> &input_shape) = 0;
    virtual void forward(const MatXf &prev_out, bool is_training = true) = 0;
    virtual void backward(const MatXf &prev_out, MatXf &prev_delta) = 0;
    virtual void update_weight(float lr, float decay) { return; }
    virtual void zero_grad() { return; }
    virtual std::vector<int> output_shape() const = 0;
    virtual int count_params() const { return 0; }
    virtual void write_params(std::fstream &fs) const { return; }
    virtual void read_params(std::fstream &fs) const { return; }
};

} // namespace ann
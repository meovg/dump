#pragma once

#include <memory>
#include <stdexcept>

#include "array.h"

namespace nnv2 {

class Layer {
public:
    Layer(){};

    Layer(const Layer &other) = delete;
    Layer(Layer &&other) = delete;
    Layer &operator=(const Layer &other) = delete;
    Layer &operator=(Layer &&other) = delete;

    Layer &connect_layer(Layer &next_layer) {
        this->next = &next_layer;
        next_layer.prev = this;
        return next_layer;
    }

    virtual Array *get_output() { return output.get(); }
    virtual const Array *get_output() const { return output.get(); }

    virtual Array *get_grad() { return grad.get(); }
    virtual const Array *get_grad() const { return grad.get(); }

    virtual void forward() {
        throw std::runtime_error("Layer::forward: not implemented");
    }
    virtual void backward() {
        throw std::runtime_error("Layer::forward: not implemented");
    }

protected:
    Layer *prev;
    Layer *next;

    std::unique_ptr<Array> output;
    std::unique_ptr<Array> grad;
};

} // namespace nnv2
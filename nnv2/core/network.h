#pragma once

#include <memory>
#include <vector>

#include "common.h"
#include "dataloader.h"
#include "layer.h"
#include "loss.h"
#include "optimizer.h"

namespace nnv2 {

class Network {
public:
    void add(Layer *layer);

    void init(DataLoader *loader, Loss *loss, SGD *optimizer);

    void train(int epochs);
    void test();

private:
    void train_epoch();
    float top1_accuracy(const Array *preds, const Array *results);

    std::vector<std::unique_ptr<Layer>> layers;

    DataLoader *loader;
    Loss *loss;
    SGD *optimizer;
};

} // namespace nnv2
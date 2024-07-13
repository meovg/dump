#include <algorithm>
#include <iostream>

#include "network.h"
#include "utils.h"

namespace nnv2 {

void Network::add(Layer *layer) {
    std::unique_ptr<Layer> next(layer);
    layers.push_back(std::move(next));
}

void Network::init(DataLoader *loader_, Loss *loss_, SGD *optimizer_) {
    loader.reset(loader_);
    loss.reset(loss_);
    optimizer.reset(optimizer_);

    CHECK_COND(layers.size() > 0, "No layers found in the network");

    // connect loader to the first layer
    std::shared_ptr<Layer> loader_base = std::make_shared<Layer>();
    loader_base = std::static_pointer_cast<Layer>(loader);
    loader_base->connect(layers.front().get());

    // connect each layer to the subsequent one
    for (int i = 1; i < layers.size(); i++) {
        layers[i - 1]->connect(layers[i].get());
    }

    // connect the last layer to the loss layer
    layers.back()->connect(loss.get());

    // register parameters to the optimizer
    for (int i = 0; i < layers.size(); i++) {
        optimizer->add_parameters(layers[i]->get_parameters());
    }
}

void Network::train(int epochs) {
    for (int e = 0; e < epochs; e++) {
        float loss_sum = 0.0;
        float accuracy = 0.0;
        int batch_count = 0;

        loader->reset();

        while (loader->has_next_train_batch()) {
            batch_count++;
            int batch_size = loader->load_train_batch();

            // perform forward propagation to calculate prediction
            for (int i = 0; i < layers.size(); i++) {
                layers[i]->forward();
            }
            // calculate loss & accuracy of prediction compared to actual result
            loss_sum += loss->calculate_loss(loader->get_labels());
            accuracy += top1_accuracy(layers.back()->get_output(),
                                      loader->get_labels());

            // backpropagate the loss gradient to the layers
            for (int i = layers.size() - 1; i >= 0; i--) {
                layers[i]->backward();
            }
            // update the parameters with regard to the gradient
            optimizer->update_parameters(batch_size);
        }

        std::cout << "[Epoch: " << e + 1 << "/" << epochs << "] ";
        std::cout << "Avg. loss: " << loss_sum / batch_count << ", ";
        std::cout << "Avg. accuracy: " << accuracy / batch_count << std::endl;

        test();
    }
}

void Network::test() {
    float loss_sum = 0.0;
    float accuracy = 0.0;
    int batch_count = 0;

    while (loader->has_next_test_batch()) {
        batch_count++;
        loader->load_test_batch();

        // perform forward propagation to calculate prediction
        for (int i = 0; i < layers.size(); i++) {
            layers[i]->forward();
        }
        // calculate loss & accuracy of prediction compared to actual result
        loss_sum += loss->calculate_loss(loader->get_labels());
        accuracy +=
            top1_accuracy(layers.back()->get_output(), loader->get_labels());
    }

    // print some stats here
    std::cout << "[Test] ";
    std::cout << "Avg. loss (test): " << loss_sum / batch_count << ", ";
    std::cout << "Avg. accuracy (test): " << accuracy / batch_count
              << std::endl;
}

float Network::top1_accuracy(const Array *preds, const Array *results) {
    int batch_size = preds->get_shape()[0];
    int labels = preds->get_shape()[1];
    int count = 0;

    for (int k = 0; k < batch_size; k++) {
        // find label with highest distribution in prediction batch
        auto pred_start = preds->get_vec().begin() + k * labels;
        auto pred_top1 = std::max_element(pred_start, pred_start + labels);
        int pred_top1_idx = (int)(pred_top1 - pred_start);

        // do the same with actual result batch
        auto result_start = results->get_vec().begin() + k * labels;
        auto result_top1 =
            std::max_element(result_start, result_start + labels);
        int result_top1_idx = (int)(result_top1 - result_start);

        if (pred_top1_idx == result_top1_idx) {
            count++;
        }
    }

    return (float)count / (float)batch_size;
}

} // namespace nnv2
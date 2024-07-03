#include <cassert>
#include <iostream>
#include <memory>
#include <string>

#include "dataloader.h"
#include "mnist.h"

using namespace nnv2;

void test_mnist_dataloader(std::string data_path) {
    auto dataset = std::make_unique<Mnist>(data_path);
    DataLoader loader(std::move(dataset), 32);

    int epoch = 30;
    for (int e = 0; e < epoch; e++) {
        std::cout << "Epoch " << e + 1 << std::endl;

        // preparation
        loader.reset();

        // load all train batches
        while (loader.has_next_train_batch()) {
            loader.load_train_batch();
        }
        // debug
        loader.health_check();
        const std::vector<int> &batch_shape = loader.get_output()->get_shape();
        std::cout << "Final train image batch shape:" << std::endl;
        for (auto x : batch_shape) {
            std::cout << x << " ";
        }
        std::cout << std::endl;

        // load all test batches
        while (loader.has_next_test_batch()) {
            loader.load_test_batch();
        }
        // debug
        loader.health_check();
        const std::vector<int> &test_batch_shape =
            loader.get_output()->get_shape();
        std::cout << "Final test image batch shape:" << std::endl;
        for (auto x : test_batch_shape) {
            std::cout << x << " ";
        }
        std::cout << std::endl;
    }
}

int main() {
    std::string data_path = "../../neural-network/dataset";
    test_mnist_dataloader(data_path);
}
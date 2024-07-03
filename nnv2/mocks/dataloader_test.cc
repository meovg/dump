#include <cassert>
#include <iostream>
#include <memory>
#include <string>

#include "dataloader.h"
#include "mnist.h"

using namespace nnv2;

const std::string data_path = "../../neural-network/dataset";

void test_mnist_dataloader() {
    auto dataset = std::make_unique<Mnist>(data_path);
    DataLoader loader(std::move(dataset), 32);

    while (loader.has_next_train_batch()) {
        loader.load_train_batch();
    }

    // Problem: somehow performing batch loading of test data is much slower
    // after batch loading of train data, while it's not that slow loading
    // both data separately and in reversed order

    const std::vector<int> &batch_shape = loader.get_output()->get_shape();
    std::cout << "Final train image batch shape:" << std::endl;
    for (auto x : batch_shape) {
        std::cout << x << " ";
    }
    std::cout << std::endl;
    loader.health_check();

    while (loader.has_next_test_batch()) {
        loader.load_test_batch();
    }

    const std::vector<int> &test_batch_shape = loader.get_output()->get_shape();
    std::cout << "Final test image batch shape:" << std::endl;
    for (auto x : test_batch_shape) {
        std::cout << x << " ";
    }
    std::cout << std::endl;
    loader.health_check();
}

int main() { test_mnist_dataloader(); }
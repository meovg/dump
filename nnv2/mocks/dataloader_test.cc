#include <cassert>
#include <iostream>
#include <memory>
#include <string>

#include "mnist.h"
#include "dataloader.h"

using namespace nnv2;

const std::string data_path = "../../neural-network/dataset";

void test_mnist_dataloader() {
    auto dataset = std::make_unique<Mnist>(data_path);
    DataLoader loader(std::move(dataset), 32);

    int batch_count = 0;
    while (loader.has_next(true)) {
        loader.forward(true);
        batch_count++;
        // {
        //     loader.health_check();
        //     std::cout << std::endl;
        // }
    }

    std::cout << "Train batch count" << std::endl;
    const std::vector<int> &batch_shape = loader.get_output()->get_shape();
    std::cout << "Final train image batch shape:" << std::endl;
    for (auto x: batch_shape) {
        std::cout << x << " ";
    }
    std::cout << std::endl;
    loader.health_check();

    batch_count = 0;
    while (loader.has_next(false)) {
        loader.forward(false);
        batch_count++;
        // {
        //     loader.health_check();
        //     std::cout << std::endl;
        // }
    }

    std::cout << "Test batch count" << std::endl;
    const std::vector<int> &test_batch_shape = loader.get_output()->get_shape();
    std::cout << "Final test image batch shape:" << std::endl;
    for (auto x: test_batch_shape) {
        std::cout << x << " ";
    }
    std::cout << std::endl;
}

int main() {
    test_mnist_dataloader();
}
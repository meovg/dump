// LeNet-5 architecture example

#include <iostream>

#include "activation.h"
#include "conv.h"
#include "dataloader.h"
#include "flatten.h"
#include "initializer.h"
#include "linear.h"
#include "loss.h"
#include "maxpool.h"
#include "mnist.h"
#include "network.h"
#include "optimizer.h"

using namespace nnv2;

int main() {
    std::string data_path = "../../neural-network/dataset";

    Network net;
    auto initz = std::make_unique<LecunUniform>();

    net.add(new Conv2D(1, 6, 28, 28, 2, 2, 5, 5, 1, 1, initz.get()));
    net.add(new ReLU);
    net.add(new MaxPool2D(0, 0, 2, 2, 2, 2));
    net.add(new Conv2D(6, 16, 14, 14, 0, 0, 5, 5, 1, 1, initz.get()));
    net.add(new ReLU);
    net.add(new MaxPool2D(0, 0, 2, 2, 2, 2));
    net.add(new Flatten);
    net.add(new Linear(400, 120, initz.get()));
    net.add(new ReLU);
    net.add(new Linear(120, 84, initz.get()));
    net.add(new ReLU);
    net.add(new Linear(84, 10, initz.get()));
    net.add(new Softmax);

    auto dataset = std::make_unique<Mnist>(data_path);
    DataLoader loader(std::move(dataset), 64);
    auto loss = std::make_unique<CrossEntropyLoss>();
    auto optimizer = std::make_unique<SGD>(0.01, 0);

    std::cout << "network setup complete" << std::endl;

    net.init(&loader, loss.get(), optimizer.get());
    net.train(30);

    return 0;
}
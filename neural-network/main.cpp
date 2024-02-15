#include "headers/network.h"

using namespace ann;
using namespace Eigen;

int main(void) {
    int n_train = 60000, n_test = 10000, ch = 1, h = 28, w = 28;
    DataLoader train_loader, test_loader;
    std::string data_dir = "dataset";

    MatXf train_X = read_mnist(data_dir, "train-images-idx3-ubyte", n_train);
    VecXi train_Y =
        read_mnist_label(data_dir, "train-labels-idx1-ubyte", n_train);
    train_loader.load(train_X, train_Y, 32, ch, h, w, true);

    MatXf test_X = read_mnist(data_dir, "t10k-images-idx3-ubyte", n_test);
    VecXi test_Y = read_mnist_label(data_dir, "t10k-labels-idx1-ubyte", n_test);
    test_loader.load(test_X, test_Y, 32, ch, h, w, false);

    std::cout << "Dataset loaded" << std::endl;

    // LeNet-5 architecture example
    Network model;
    model.add(new Conv2d(1, 6, 5, 2, "lecun_uniform"));
    model.add(new ReLU);
    model.add(new MaxPool2d(2, 2));
    model.add(new Conv2d(6, 16, 5, 0, "lecun_uniform"));
    model.add(new ReLU);
    model.add(new MaxPool2d(2, 2));
    model.add(new Flatten);
    model.add(new Linear(400, 120, "lecun_uniform"));
    model.add(new ReLU);
    model.add(new Linear(120, 84, "lecun_uniform"));
    model.add(new ReLU);
    model.add(new Linear(84, 10, "lecun_uniform"));
    model.add(new Softmax);

    std::cout << "Model construction completed." << std::endl;

    float lr = 0.01, decay = 0;
    model.compile({32, ch, h, w}, new SGD(lr, decay), new CrossEntropyLoss);
    model.fit(train_loader, 30, test_loader);
    model.save("./models", "lenet5.pth");

    return 0;
}
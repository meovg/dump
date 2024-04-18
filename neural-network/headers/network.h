#pragma once

#include "common.h"
#include "data_loader.h"
#include "layer.h"
#include "loss.h"
#include "optimizers.h"

namespace ann {

class Network {
private:
    std::vector<Layer *> layers;
    Optimizer *optim;
    Loss *loss;

public:
    void add(Layer *layer);
    void compile(std::vector<int> input_shape, Optimizer *optim = nullptr, Loss *loss = nullptr);
    void fit(const DataLoader &train_loader, int epochs, const DataLoader &valid_loader);
    void save(std::string save_dir, std::string fname);
    void load(std::string save_dir, std::string fname);
    void evaluate(const DataLoader &data_loader);

private:
    void forward(const MatXf &X, bool is_training);
    void classify(const MatXf &output, VecXi &classified);
    void error_criterion(const VecXi &classified, const VecXi &labels, float &error_acc);
    void loss_criterion(const MatXf &output, const VecXi &labels, float &loss_acc);
    void zero_grad();
    void backward(const MatXf &X);
    void update_weight();
    int count_params();
    void write_params(std::fstream &fs);
    void read_params(std::fstream &fs);
};

void Network::add(Layer *layer) { layers.push_back(layer); }

void Network::compile(std::vector<int> input_shape, Optimizer *optim, Loss *loss) {
    // set optimizer & loss
    this->optim = optim;
    this->loss = loss;

    // set first & last layer
    layers.front()->is_first = true;
    layers.back()->is_last = true;

    // set layerswork
    for (int i = 0; i < layers.size(); i++) {
        if (i == 0)
            layers[i]->set_layer(input_shape);
        else
            layers[i]->set_layer(layers[i - 1]->output_shape());
    }

    // set Loss layer
    if (loss != nullptr) {
        loss->set_layer(layers.back()->output_shape());
    }
}

void Network::fit(const DataLoader &train_loader, int epochs, const DataLoader &valid_loader) {
    if (optim == nullptr || loss == nullptr) {
        std::cout << "The model must be compiled before fitting the data." << std::endl;
        exit(1);
    }

    int batch = train_loader.input_shape()[0];
    int n_batch = train_loader.size();

    MatXf X;
    VecXi Y;
    VecXi classified(batch);

    for (int e = 0; e < epochs; e++) {
        float loss = 0.f;
        float error = 0.f;

        std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
        for (int n = 0; n < n_batch; n++) {
            X = train_loader.get_x(n);
            Y = train_loader.get_y(n);

            forward(X, true);
            classify(layers.back()->output, classified);
            error_criterion(classified, Y, error);

            zero_grad();
            loss_criterion(layers.back()->output, Y, loss);
            backward(X);
            update_weight();

            std::cout << "[Epoch:" << std::setw(3) << e + 1 << "/" << epochs << ", ";
            std::cout << "Batch: " << std::setw(4) << n + 1 << "/" << n_batch << "]";

            if (n + 1 < n_batch)
                std::cout << "\r";
        }

        std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
        std::chrono::duration<float> sec = end - start;

        float loss_valid = 0.f;
        float error_valid = 0.f;

        int n_batch_valid = valid_loader.size();
        if (n_batch_valid != 0) {
            for (int n = 0; n < n_batch_valid; n++) {
                X = valid_loader.get_x(n);
                Y = valid_loader.get_y(n);

                forward(X, false);
                classify(layers.back()->output, classified);
                error_criterion(classified, Y, error_valid);
                loss_criterion(layers.back()->output, Y, loss_valid);
            }
        }

        std::cout << std::fixed << std::setprecision(2);
        std::cout << " - t: " << sec.count() << 's';
        std::cout << " - loss: " << loss / n_batch;
        std::cout << " - error: " << error / n_batch * 100 << "%";
        if (n_batch_valid != 0) {
            std::cout << " - loss(valid): " << loss_valid / n_batch_valid;
            std::cout << " - error(valid): " << error_valid / n_batch_valid * 100 << "%";
        }
        std::cout << std::endl;
    }
}

void Network::forward(const MatXf &X, bool is_training) {
    for (int i = 0; i < layers.size(); i++) {
        if (i == 0)
            layers[i]->forward(X, is_training);
        else
            layers[i]->forward(layers[i - 1]->output, is_training);
    }
}

void Network::classify(const MatXf &output, VecXi &classified) {
    // assume that the last layer is linear, not 2d.
    assert(output.rows() == classified.size());

    for (int i = 0; i < classified.size(); i++)
        output.row(i).maxCoeff(&classified[i]);
}

void Network::error_criterion(const VecXi &classified, const VecXi &labels, float &error_acc) {
    int batch = (int)classified.size();

    float error = 0.f;
    for (int i = 0; i < batch; i++) {
        if (classified[i] != labels[i])
            error++;
    }
    error_acc += error / batch;
}

void Network::loss_criterion(const MatXf &output, const VecXi &labels, float &loss_acc) {
    loss_acc += loss->calc_loss(output, labels, layers.back()->delta);
}

void Network::zero_grad() {
    for (const auto &l : layers)
        l->zero_grad();
}

void Network::backward(const MatXf &X) {
    for (int i = (int)layers.size() - 1; i >= 0; i--) {
        if (i == 0) {
            MatXf empty;
            layers[i]->backward(X, empty);
        } else {
            layers[i]->backward(layers[i - 1]->output, layers[i - 1]->delta);
        }
    }
}

void Network::update_weight() {
    float lr = optim->lr();
    float decay = optim->decay();
    for (const auto &l : layers) {
        l->update_weight(lr, decay);
    }
}

void Network::save(std::string save_dir, std::string fname) {
    std::string path = save_dir + "/" + fname;
    std::fstream fout(path, std::ios::out | std::ios::binary);

    int total_params = count_params();
    fout.write((char *)&total_params, sizeof(int));

    write_params(fout);
    std::cout << "Model parameters are saved in " << path << std::endl;

    fout.close();
}

void Network::load(std::string save_dir, std::string fname) {
    std::string path = save_dir + "/" + fname;
    std::fstream fin(path, std::ios::in | std::ios::binary);

    if (!fin) {
        std::cout << path << " does not exist." << std::endl;
        exit(1);
    }

    int total_params;
    fin.read((char *)&total_params, sizeof(int));

    if (total_params != count_params()) {
        std::cout << "The number of parameters does not match." << std::endl;
        fin.close();
        exit(1);
    }

    read_params(fin);
    fin.close();

    std::cout << "Pretrained weights are loaded." << std::endl;
}

int Network::count_params() {
    int total_params = 0;
    for (const auto &l : layers)
        total_params += l->count_params();
    return total_params;
}

void Network::write_params(std::fstream &fs) {
    for (const auto &l : layers)
        l->write_params(fs);
}

void Network::read_params(std::fstream &fs) {
    for (const auto &l : layers)
        l->read_params(fs);
}

void Network::evaluate(const DataLoader &data_loader) {
    int batch = data_loader.input_shape()[0];
    int n_batch = data_loader.size();
    float error_acc = 0.f;

    MatXf X;
    VecXi Y;
    VecXi classified(batch);

    std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
    for (int n = 0; n < n_batch; n++) {
        MatXf X = data_loader.get_x(n);
        VecXi Y = data_loader.get_y(n);

        forward(X, false);
        classify(layers.back()->output, classified);
        error_criterion(classified, Y, error_acc);

        std::cout << "[Batch: " << std::setw(3) << n + 1 << "/" << n_batch << "]";
        if (n + 1 < n_batch)
            std::cout << "\r";
    }

    std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
    std::chrono::duration<float> sec = end - start;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << " - t: " << sec.count() << "s";
    std::cout << " - error(" << batch * n_batch << " images): ";
    std::cout << error_acc / n_batch * 100 << "%" << std::endl;
}

} // namespace ann
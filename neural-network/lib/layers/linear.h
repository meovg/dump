#pragma once

#include "../layer.h"

namespace ann {

class Linear : public Layer {
private:
    int batch;
    int in_feat;
    int out_feat;
    std::string option;
    MatXf dW;
    RowVecXf db;

public:
    MatXf W;
    RowVecXf b;
    Linear(int in_features, int out_features, std::string option);
    void set_layer(const std::vector<int> &input_shape) override;
    void forward(const MatXf &prev_out, bool is_training) override;
    void backward(const MatXf &prev_out, MatXf &prev_delta) override;
    void update_weight(float lr, float decay) override;
    void zero_grad() override;
    std::vector<int> output_shape() const override;
    int count_params() const override;
    void write_params(std::fstream &fs) const override;
    void read_params(std::fstream &fs) const override;
};

Linear::Linear(int in_features, int out_features, std::string option)
    : Layer(), batch(0), in_feat(in_features), out_feat(out_features), option(option) {}

void Linear::set_layer(const std::vector<int> &input_shape) {
    batch = input_shape[0];

    output.resize(batch, out_feat);
    delta.resize(batch, out_feat);
    W.resize(out_feat, in_feat);
    dW.resize(out_feat, in_feat);
    b.resize(out_feat);
    db.resize(out_feat);

    init_weight(W, in_feat, out_feat, option);
    b.setZero();
}

void Linear::forward(const MatXf &prev_out, bool is_training) {
    for (int n = 0; n < batch; n++) {
        output.row(n).noalias() = W * prev_out.row(n).transpose();
        output.row(n).noalias() += b;
    }
}

void Linear::backward(const MatXf &prev_out, MatXf &prev_delta) {
    // dW = delta(Vector) * prev_out(RowVector)
    // db = delta
    for (int n = 0; n < batch; n++) {
        dW.noalias() += delta.row(n).transpose() * prev_out.row(n);
        db.noalias() += delta.row(n);
    }

    // prev_delta = W.T * delta(Vector)
    if (!is_first) {
        for (int n = 0; n < batch; n++) {
            prev_delta.row(n).noalias() =
                W.transpose() * delta.row(n).transpose();
        }
    }
}

void Linear::update_weight(float lr, float decay) {
    float t1 = (1 - (2 * lr * decay) / batch);
    float t2 = lr / batch;

    if (t1 != 1) {
        W *= t1;
        b *= t1;
    }

    W -= t2 * dW;
    b -= t2 * db;
}

void Linear::zero_grad() {
    delta.setZero();
    dW.setZero();
    db.setZero();
}

std::vector<int> Linear::output_shape() const {
    return {batch, out_feat};
}

int Linear::count_params() const {
    return (int)W.size() + (int)b.size();
}

void Linear::write_params(std::fstream &fs) const {
    fs.write((char *)W.data(), sizeof(float) * W.size());
    fs.write((char *)b.data(), sizeof(float) * b.size());
}

void Linear::read_params(std::fstream &fs) const {
    fs.read((char *)W.data(), sizeof(float) * W.size());
    fs.read((char *)b.data(), sizeof(float) * b.size());
}

} // namespace ann
#pragma once

#include "../layer.h"

static void col2im_add_pixel(float *im, int height, int width, int channels,
                             int row, int col, int channel, int pad, float val) {
    row -= pad;
    col -= pad;

    if (row < 0 || col < 0 || row >= height || col >= width)
        return;
    im[col + width * (row + height * channel)] += val;
}

static void col2im(const float *data_col, int channels, int height, int width,
                   int ksize, int stride, int pad, float *data_im) {
    int c, h, w;
    int height_col = (height + 2 * pad - ksize) / stride + 1;
    int width_col = (width + 2 * pad - ksize) / stride + 1;

    int channels_col = channels * ksize * ksize;
    for (c = 0; c < channels_col; ++c) {
        int w_offset = c % ksize;
        int h_offset = (c / ksize) % ksize;
        int c_im = c / ksize / ksize;
        for (h = 0; h < height_col; ++h) {
            for (w = 0; w < width_col; ++w) {
                int im_row = h_offset + h * stride;
                int im_col = w_offset + w * stride;
                int col_index = (c * height_col + h) * width_col + w;
                float val = data_col[col_index];
                col2im_add_pixel(data_im, height, width, channels, im_row,
                                 im_col, c_im, pad, val);
            }
        }
    }
}

static float im2col_get_pixel(const float *im, int height, int width, int channels,
                              int row, int col, int channel, int pad) {
    row -= pad;
    col -= pad;

    if (row < 0 || col < 0 || row >= height || col >= width)
        return 0;
    return im[col + width * (row + height * channel)];
}

static void im2col(const float *data_im, int channels, int height, int width,
                   int ksize, int stride, int pad, float *data_col) {
    int c, h, w;
    int height_col = (height + 2 * pad - ksize) / stride + 1;
    int width_col = (width + 2 * pad - ksize) / stride + 1;

    int channels_col = channels * ksize * ksize;
    for (c = 0; c < channels_col; ++c) {
        int w_offset = c % ksize;
        int h_offset = (c / ksize) % ksize;
        int c_im = c / ksize / ksize;
        for (h = 0; h < height_col; ++h) {
            for (w = 0; w < width_col; ++w) {
                int im_row = h_offset + h * stride;
                int im_col = w_offset + w * stride;
                int col_index = (c * height_col + h) * width_col + w;
                data_col[col_index] =
                    im2col_get_pixel(data_im, height, width, channels, im_row,
                                     im_col, c_im, pad);
            }
        }
    }
}

namespace ann {

class Conv2d : public Layer {
private:
    int batch;
    int ic;
    int oc;
    int ih;
    int iw;
    int ihw;
    int oh;
    int ow;
    int ohw;
    int kh;
    int kw;
    int pad;
    std::string option;
    MatXf dkernel;
    VecXf dbias;
    MatXf im_col;

public:
    MatXf kernel;
    VecXf bias;
    Conv2d(int in_channels, int out_channels, int kernel_size, int padding,
           std::string option);
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

Conv2d::Conv2d(int in_channels, int out_channels, int kernel_size, int padding,
               std::string option)
    : Layer(LayerType::CONV2D), batch(0), ic(in_channels), oc(out_channels),
      ih(0), iw(0), ihw(0), oh(0), ow(0), ohw(0), kh(kernel_size),
      kw(kernel_size), pad(padding), option(option) {}

void Conv2d::set_layer(const std::vector<int> &input_shape) {
    batch = input_shape[0];
    ic = input_shape[1];
    ih = input_shape[2];
    iw = input_shape[3];
    ihw = ih * iw;
    oh = calc_outsize(ih, kh, 1, pad);
    ow = calc_outsize(iw, kw, 1, pad);
    ohw = oh * ow;

    output.resize(batch * oc, ohw);
    delta.resize(batch * oc, ohw);
    kernel.resize(oc, ic * kh * kw);
    dkernel.resize(oc, ic * kh * kw);
    bias.resize(oc);
    dbias.resize(oc);
    im_col.resize(ic * kh * kw, ohw);

    int fan_in = kh * kw * ic;
    int fan_out = kh * kw * oc;
    init_weight(kernel, fan_in, fan_out, option);
    bias.setZero();
}

void Conv2d::forward(const MatXf &prev_out, bool is_training) {
    for (int n = 0; n < batch; n++) {
        const float *im = prev_out.data() + (ic * ihw) * n;
        im2col(im, ic, ih, iw, kh, 1, pad, im_col.data());
        output.block(oc * n, 0, oc, ohw).noalias() = kernel * im_col;
        output.block(oc * n, 0, oc, ohw).colwise() += bias;
    }
}

void Conv2d::backward(const MatXf &prev_out, MatXf &prev_delta) {
    for (int n = 0; n < batch; n++) {
        const float *im = prev_out.data() + (ic * ihw) * n;
        im2col(im, ic, ih, iw, kh, 1, pad, im_col.data());
        dkernel += delta.block(oc * n, 0, oc, ohw) * im_col.transpose();
        dbias += delta.block(oc * n, 0, oc, ohw).rowwise().sum();
    }

    if (!is_first) {
        for (int n = 0; n < batch; n++) {
            float *begin = prev_delta.data() + ic * ihw * n;
            im_col = kernel.transpose() * delta.block(oc * n, 0, oc, ohw);
            col2im(im_col.data(), ic, ih, iw, kh, 1, pad, begin);
        }
    }
}

void Conv2d::update_weight(float lr, float decay) {
    float t1 = (1 - (2 * lr * decay) / batch);
    float t2 = lr / batch;

    if (t1 != 1) {
        kernel *= t1;
        bias *= t1;
    }

    kernel -= t2 * dkernel;
    bias -= t2 * dbias;
}

void Conv2d::zero_grad() {
    delta.setZero();
    dkernel.setZero();
    dbias.setZero();
}

std::vector<int> Conv2d::output_shape() const {
    return {batch, oc, oh, ow};
}

int Conv2d::count_params() const {
    return (int)kernel.size() + (int)bias.size();
}

void Conv2d::write_params(std::fstream &fs) const {
    fs.write((char *)kernel.data(), sizeof(float) * kernel.size());
    fs.write((char *)bias.data(), sizeof(float) * bias.size());
}

void Conv2d::read_params(std::fstream &fs) const {
    fs.read((char *)kernel.data(), sizeof(float) * kernel.size());
    fs.read((char *)bias.data(), sizeof(float) * bias.size());
}

} // namespace ann
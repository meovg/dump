#include <cassert>
#include <iostream>
#include <iomanip>
#include <vector>

#include "conv.h"
#include "mathfunc.h"

using namespace nnv2;

void test_im2col() {
    int batch_size = 1;
    int in_feats = 2;
    int width = 5;
    int height = 5;

    int kernel_w = 3;
    int kernel_h = 3;

    int pad_h = 1;
    int pad_w = 1;

    int stride_h = 1;
    int stride_w = 1;

    int out_h = (height + 2 * pad_h - kernel_h) / stride_h + 1;
    int out_w = (width + 2 * pad_w - kernel_w) / stride_w + 1;

    Array im(
        { batch_size, in_feats, height, width },
        { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
          17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33,
          34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49 });
    Array col({ batch_size, in_feats * kernel_h * kernel_w, out_h * out_w });

    im2col(&im, &col, pad_h, pad_w, kernel_h, kernel_w, stride_h, stride_w);

    // use your eyes
    const std::vector<float> &col_vec = col.get_vec();
    for (int i = 0; i < col_vec.size(); i++) {
        std::cout << std::setw(3) << col_vec[i] << " ";
        if ((i + 1) % (out_h * out_w) == 0) std::cout << std::endl;
    }

    std::cout << "test_im2col: DIY!" << std::endl;
}

void test_conv_transform() {
    int batch_size = 1;
    int in_feats = 2;
    int out_feats = 3;

    int height = 5;
    int width = 5;
    int kernel_w = 3;
    int kernel_h = 3;
    int pad_h = 1;
    int pad_w = 1;
    int stride_h = 1;
    int stride_w = 1;

    int out_h = (height + 2 * pad_h - kernel_h) / stride_h + 1;
    int out_w = (width + 2 * pad_w - kernel_w) / stride_w + 1;

    Array input(
        { batch_size, in_feats, height, width },
        { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
          17, 18, 19, 20, 21, 22, 23, 24, 0,  1,  2,  3,  4,  5,  6,  7,  8,
          9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 });

    Array kernel(
        { out_feats, in_feats, kernel_h, kernel_w },
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 1, 2, 3, 4, 5, 6, 7, 8,
          0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 1, 2, 3, 4, 5, 6, 7, 8,
          0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 1, 2, 3, 4, 5, 6, 7, 8 });

    Array cols({ batch_size, in_feats * kernel_h * kernel_w, out_h * out_w });
    Array output({ batch_size, out_feats, out_h, out_w });

    conv_transform(&output, &input, &cols, &kernel, pad_h, pad_w, stride_h, stride_w);

    assert(output.get_vec() == std::vector<float>(
        { 176,  284,  350,  416,  272,  420,  624, 696, 768, 480, 690,  984,  1056,
          1128, 690,  960,  1344, 1416, 1488, 900, 464, 608, 638, 668,  368,  176,
          284,  350,  416,  272,  420,  624,  696, 768, 480, 690, 984,  1056, 1128,
          690,  960,  1344, 1416, 1488, 900,  464, 608, 638, 668, 368,  176,  284,
          350,  416,  272,  420,  624,  696,  768, 480, 690, 984, 1056, 1128, 690,
          960,  1344, 1416, 1488, 900,  464,  608, 638, 668, 368 }));

    std::cout << "test_conv_transform: Passed" << std::endl;
}

int main() {
    test_im2col();
    test_conv_transform();
}
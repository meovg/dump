#include <vector>

#include "conv.h"
#include "mathfunc.h"
#include "utils.h"

namespace nnv2 {

static void col2im_add_pixel(Array *im, int batch_idx, int feat_idx, int r, int c,
                             int pad_h, int pad_w, float value) {
    int feats = im->get_shape()[1];
    int h = im->get_shape()[2];
    int w = im->get_shape()[3];

    r -= pad_h;
    c -= pad_w;

    if (r < 0 || r >= h || c < 0 || c >= w) {
        return;
    }

    int im_idx = ((batch_idx * feats + feat_idx) * h + r) * w + c;
    im->get_vec()[im_idx] += value;
}

void col2im(const Array *imcols, Array *im, int pad_h, int pad_w, int kernel_h, int kernel_w,
            int stride_h, int stride_w) {
    int batch_size = im->get_shape()[0];
    int im_feats = im->get_shape()[1];
    int im_h = im->get_shape()[2];
    int im_w = im->get_shape()[3];

    int out_h = (im_h + 2 * pad_h - kernel_h) / stride_h + 1;
    int out_w = (im_w + 2 * pad_w - kernel_w) / stride_w + 1;

    int col_size = batch_size * im_feats * kernel_h * kernel_w;
    for (int i = 0; i < col_size; i++) {
        int kernel_offset_r = (i / kernel_w) % kernel_h;
        int kernel_offset_c = i % kernel_w;

        int feat_idx = (i / kernel_w / kernel_h) % im_feats;
        int batch_idx = (i / kernel_w / kernel_h) / im_feats;

        for (int r = 0; r < out_h; r++) {
            for (int c = 0; c < out_w; c++) {
                int im_r = kernel_offset_r + stride_h * r;
                int im_c = kernel_offset_c + stride_w * c;

                int col_idx = (i * out_h + r) * out_w + c;
                float value = imcols->get_vec()[col_idx];
                col2im_add_pixel(im, batch_idx, feat_idx, im_r, im_c, pad_h, pad_w, value);
            }
        }
    }
}

static float im2col_get_pixel(const Array *im, int batch_idx, int feat_idx, int r, int c,
                              int pad_h, int pad_w) {
    int feats = im->get_shape()[1];
    int h = im->get_shape()[2];
    int w = im->get_shape()[3];

    r -= pad_h;
    c -= pad_w;

    if (r < 0 || r >= h || c < 0 || c >= w) {
        return 0;
    }

    int im_idx = ((batch_idx * feats + feat_idx) * h + r) * w + c;
    return im->get_vec()[im_idx];
}

void im2col(const Array *im, Array *imcols, int pad_h, int pad_w, int kernel_h, int kernel_w,
            int stride_h, int stride_w) {
    int batch_size = im->get_shape()[0];
    int im_feats = im->get_shape()[1];
    int im_h = im->get_shape()[2];
    int im_w = im->get_shape()[3];

    int out_h = (im_h + 2 * pad_h - kernel_h) / stride_h + 1;
    int out_w = (im_w + 2 * pad_w - kernel_w) / stride_w + 1;

    int col_size = batch_size * im_feats * kernel_h * kernel_w;
    for (int i = 0; i < col_size; i++) {
        int kernel_offset_r = (i / kernel_w) % kernel_h;
        int kernel_offset_c = i % kernel_w;

        int feat_idx = (i / kernel_w / kernel_h) % im_feats;
        int batch_idx = (i / kernel_w / kernel_h) / im_feats;

        for (int r = 0; r < out_h; r++) {
            for (int c = 0; c < out_w; c++) {
                int im_r = kernel_offset_r + stride_h * r;
                int im_c = kernel_offset_c + stride_w * c;

                int col_idx = (i * out_h + r) * out_w + c;
                imcols->get_vec()[col_idx] =
                        im2col_get_pixel(im, batch_idx, feat_idx, im_r, im_c, pad_h, pad_w);
            }
        }
    }
}

void conv_transform(Array *output, const Array *input, Array *cols, Array *kernel,
                    int pad_h, int pad_w, int stride_h, int stride_w) {
    // output: shape (n, o_f, o_h, o_w)
    // input: shape (n, i_f, i_h, i_w)
    // cols: shape (n, i_f * k_h * k_w, o_h * o_w)
    // kernel: shape (o_f, i_f, k_h, k_w)

    CHECK_EQ(output->get_shape().size(), 4, "conv_transform: output shape error");
    CHECK_EQ(input->get_shape().size(), 4, "conv_transform: input shape error");
    CHECK_EQ(cols->get_shape().size(), 3, "conv_transform: cols shape error");
    CHECK_EQ(kernel->get_shape().size(), 4, "conv_transform: kernel shape error");

    int batch_size = input->get_shape()[0];
    int in_feats = input->get_shape()[1];
    int in_h = input->get_shape()[2];
    int in_w = input->get_shape()[3];

    CHECK_EQ(output->get_shape()[0], batch_size, "conv_transform: batch size error");

    int out_feats = output->get_shape()[1];
    int out_h = output->get_shape()[2];
    int out_w = output->get_shape()[3];

    CHECK_EQ(kernel->get_shape()[0], out_feats, "conv_transform: feature size error");
    CHECK_EQ(kernel->get_shape()[1], in_feats, "conv_transform: feature size error");

    int kernel_h = kernel->get_shape()[2];
    int kernel_w = kernel->get_shape()[3];

    // perform im2col to flatten images to columns of pixels that corresponds to their kernel
    im2col(input, cols, pad_h, pad_w, kernel_h, kernel_w, stride_w, stride_h);

    // perform matrix multiplication between kernel and image columns
    // since the kernel is of shape (o_f, i_f, k_h, k_w) while the image columns matrix is
    // of shape (n, i_f * k_h * k_w, o_h * o_w), the kernel is temporarily reshaped to
    // (o_f, i_f * k_h * k_w);
    kernel->reshape({ out_feats, in_feats * kernel_h * kernel_w });

    // after multiplication, the resulting matrix is of shape (n, o_f, o_h * o_w) and since
    // the shape of output is (n, o_f, o_h, o_w), it is temporarily reshaped.
    output->reshape({ batch_size, out_feats, out_h * out_w });

    // output = kernel * input imcols
    func_matmul(output, kernel, cols, 1);

    // recover shape
    kernel->reshape({ out_feats, in_feats, kernel_h, kernel_w });
    output->reshape({ batch_size, out_feats, out_h, out_w });
}

void conv_add_bias_to_output(Array *output, const Array *bias) {
    int out_feats = output->get_shape()[1];
    int out_h = output->get_shape()[2];
    int out_w = output->get_shape()[3];

    std::vector<float> &output_ref = output->get_vec();
    for (int i = 0; i < output_ref.size(); i++) {
        int feat_idx = (i / (out_h * out_w)) % out_feats;
        output_ref[i] += bias->get_vec()[feat_idx];
    }
}

Conv2D::Conv2D(int in_feats, int out_feats, int in_h, int in_w, int pad_h, int pad_w,
               int kernel_h, int kernel_w, int stride_h, int stride_w, const Initializer *init)
        : in_feats(in_feats), out_feats(out_feats), in_h(in_h), in_w(in_w),
          pad_h(pad_h), pad_w(pad_w),
          kernel_h(kernel_h), kernel_w(kernel_w),
          stride_h(pad_h), stride_w(pad_w),
          out_h(in_h + 2 * pad_h - kernel_h + 1),
          out_w(in_w + 2 * pad_w - kernel_w + 1) {
    kernel.reset(new Array({ out_feats, in_feats, kernel_h, kernel_w }));
    kernel->initialize(init);

    kernel_grad.reset(new Array({ out_feats, in_feats, kernel_h, kernel_w }));

    bias.reset(new Array({ 1, out_feats }));
    bias->initialize(init);

    bias_grad.reset(new Array({ 1, out_feats }));
}

void Conv2D::forward() {
    const Array *input = prev->get_output();

    int batch = input->get_shape()[0];
    std::vector<int> output_shape = { batch, out_feats, out_h, out_w };
    std::vector<int> imcols_shape = { batch, in_feats * kernel_h * kernel_w, out_h * out_w };

    INIT_ARRAY(output, output_shape);
    INIT_ARRAY(imcols, imcols_shape);

    conv_transform(output.get(), input, imcols.get(), kernel.get(), pad_h, pad_w,
                   stride_w, stride_h);

    conv_add_bias_to_output(output.get(), bias.get());
}

void Conv2D::backward() {
    // TODO
}


} // namespace nnv2
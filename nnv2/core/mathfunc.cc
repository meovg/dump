#include <algorithm>
#include <numeric>
#include <vector>

#include <cblas.h>

#include "mathfunc.h"
#include "utils.h"

namespace nnv2 {

void func_add(Array *output, const Array *input1, const Array *input2) {
    int input1_size = input1->get_vec().size();
    int input2_size = input2->get_vec().size();
    int output_size = output->get_vec().size();

    CHECK_EQ(input1_size, input2_size, "func_add: size of inputs mismatched");
    CHECK_EQ(output_size, input1_size,
             "func_add: size of output mismatched with size of inputs");

    std::transform(input1->get_vec().begin(), input1->get_vec().end(),
                   input2->get_vec().begin(), output->get_vec().begin(),
                   std::plus<float>());
}

void func_add(Array *output, const Array *input, float value) {
    CHECK_EQ(output->get_vec().size(), input->get_vec().size(),
             "func_add: size of output mismatched with size of input");

    std::transform(input->get_vec().begin(), input->get_vec().end(),
                   output->get_vec().begin(),
                   [value](const float &e) { return e + value; });
}

void func_sub(Array *output, const Array *input1, const Array *input2) {
    int input1_size = input1->get_vec().size();
    int input2_size = input2->get_vec().size();
    int output_size = output->get_vec().size();

    CHECK_EQ(input1_size, input2_size, "func_sub: size of inputs mismatched");
    CHECK_EQ(output_size, input1_size,
             "func_sub: size of output mismatched with size of inputs");

    std::transform(input1->get_vec().begin(), input1->get_vec().end(),
                   input2->get_vec().begin(), output->get_vec().begin(),
                   std::minus<float>());
}

void func_sub(Array *output, const Array *input, float value) {
    CHECK_EQ(output->get_vec().size(), input->get_vec().size(),
             "func_sub: size of output mismatched with size of input");

    std::transform(input->get_vec().begin(), input->get_vec().end(),
                   output->get_vec().begin(),
                   [value](const float &e) { return e - value; });
}

void func_mul(Array *output, const Array *input1, const Array *input2) {
    int input1_size = input1->get_vec().size();
    int input2_size = input2->get_vec().size();
    int output_size = output->get_vec().size();

    CHECK_EQ(input1_size, input2_size, "func_mul: size of inputs mismatched");
    CHECK_EQ(output_size, input1_size,
             "func_mul: size of output mismatched with size of inputs");

    std::transform(input1->get_vec().begin(), input1->get_vec().end(),
                   input2->get_vec().begin(), output->get_vec().begin(),
                   std::multiplies<float>());
}

void func_mul(Array *output, const Array *input, float value) {
    CHECK_EQ(output->get_vec().size(), input->get_vec().size(),
             "func_mul: size of output mismatched with size of input");

    std::transform(input->get_vec().begin(), input->get_vec().end(),
                   output->get_vec().begin(),
                   [value](const float &e) { return e * value; });
}

void func_div(Array *output, const Array *input1, const Array *input2) {
    int input1_size = input1->get_vec().size();
    int input2_size = input2->get_vec().size();
    int output_size = output->get_vec().size();

    CHECK_EQ(input1_size, input2_size, "func_div: size of inputs mismatched");
    CHECK_EQ(output_size, input1_size,
             "func_div: size of output mismatched with size of inputs");

    std::transform(input1->get_vec().begin(), input1->get_vec().end(),
                   input2->get_vec().begin(), output->get_vec().begin(),
                   std::divides<float>());
}

void func_matmul(Array *output, const Array *input1, const Array *input2,
                 int broadcast) {
    // Peforms matrix multiplication in two main cases based on the value of
    // `broadcast`
    //   broadcast is 0 (default)
    //     If both inputs are 2-dimensional it performs single matrix
    //     multiplication. If both inputs have more than 2 dimensions, it
    //     performs matrix multiplication in batch. In this case, input and
    //     output arrays are seen as collections of matrices. The i-th matrix in
    //     the output batch is the result of matrix multiplication between the
    //     i-th matrices in the first and the second input batch.
    //   broadcast is 1 or 2
    //     Performs batch matrix multiplication where the input consists of a
    //     matrix batch A and a single matrix b. In this case, the function
    //     performs matrix multiplication between each matrix in batch A and the
    //     matrix b. `broadcast` indicates whether b is the first or second
    //     input

    // check if the dimensions are at least 2
    CHECK_COND(input1->get_shape().size() > 1,
               "func_matmul: array dimension must be at least 2");
    CHECK_COND(input2->get_shape().size() > 1,
               "func_matmul: array dimension must be at least 2");
    CHECK_COND(output->get_shape().size() > 1,
               "func_matmul: array dimension must be at least 2");

    // additional dimension check for broadcast case
    if (broadcast == 1) {
        CHECK_COND(input1->get_shape().size() == 2,
                   "func_matmul: input1 must be 2-dimensional");
    } else if (broadcast == 2) {
        CHECK_COND(input2->get_shape().size() == 2,
                   "func_matmul: input2 must be 2-dimensional");
    }

    // batch size check
    int batch_size = std::accumulate(output->get_shape().begin(),
                                     output->get_shape().end() - 2, 1,
                                     std::multiplies<int>());
    int batch_size_input1 = std::accumulate(input1->get_shape().begin(),
                                            input1->get_shape().end() - 2, 1,
                                            std::multiplies<int>());
    int batch_size_input2 = std::accumulate(input2->get_shape().begin(),
                                            input2->get_shape().end() - 2, 1,
                                            std::multiplies<int>());

    if (broadcast != 1) {
        CHECK_EQ(batch_size, batch_size_input1,
                 "func_matmul: batch size mismatched");
    }
    if (broadcast != 2) {
        CHECK_EQ(batch_size, batch_size_input2,
                 "func_matmul: batch size mismatched");
    }

    // matrix dimension check
    int m = *(input1->get_shape().rbegin() + 1);
    int k = *(input1->get_shape().rbegin());
    int n = *(input2->get_shape().rbegin());

    int input2_h = *(input2->get_shape().rbegin() + 1);
    int output_h = *(output->get_shape().rbegin() + 1);
    int output_w = *(output->get_shape().rbegin());

    CHECK_EQ(k, input2_h,
             "func_matmul: number of cols in input1 must be equal to number of "
             "rows in input2");
    CHECK_EQ(m, output_h,
             "func_matmul: number of rows in output must be equal to number of "
             "rows in input1");
    CHECK_EQ(n, output_w,
             "func_matmul: number of cols in output must be equal to number of "
             "cols in input2");

    // matrix multiplication process
    for (int i = 0; i < batch_size; i++) {
        const float *input1_mat = input1->get_vec().data();
        if (broadcast != 1) {
            input1_mat += i * m * k;
        }

        const float *input2_mat = input2->get_vec().data();
        if (broadcast != 2) {
            input2_mat += i * k * n;
        }

        float *output_mat = output->get_vec().data() + i * output_h * output_w;

        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, m, n, k, 1.0f,
                    input1_mat, k, input2_mat, n, 0.0f, output_mat, n);
    }
}

void func_transpose(Array *output, const Array *input) {
    // check if the dimensions are at least 2
    CHECK_COND(input->get_shape().size() > 1,
               "func_transpose: array dimension must be at least 2");
    CHECK_COND(output->get_shape().size() > 1,
               "func_transpose: array dimension must be at least 2");

    int batch_size = std::accumulate(output->get_shape().begin(),
                                     output->get_shape().end() - 2, 1,
                                     std::multiplies<int>());
    int batch_size_input = std::accumulate(input->get_shape().begin(),
                                           input->get_shape().end() - 2, 1,
                                           std::multiplies<int>());

    CHECK_EQ(batch_size, batch_size_input,
             "func_transpose: batch size mismatched");

    int input_h = *(input->get_shape().rbegin() + 1);
    int input_w = *(input->get_shape().rbegin());
    int output_h = *(output->get_shape().rbegin() + 1);
    int output_w = *(output->get_shape().rbegin());

    CHECK_EQ(input_h, output_w, "func_transpose: shapes of inputs mismatched");
    CHECK_EQ(input_w, output_h, "func_transpose: shapes of inputs mismatched");

    for (int i = 0; i < batch_size; i++) {
        const float *input_mat =
            input->get_vec().data() + i * input_h * input_w;
        float *output_mat = output->get_vec().data() + i * output_h * output_w;

        cblas_somatcopy(CblasRowMajor, CblasTrans, input_h, input_w, 1.0f,
                        input_mat, input_w, output_mat, output_w);
    }
}

void func_sum(Array *output, const Array *input, int axis, bool reduce) {
    CHECK_COND(axis >= 0,
               "func_sum: support for negative axis not implemented");
    CHECK_COND(axis < input->get_shape().size(),
               "func_sum: axis is out of bound");

    // validate output shape
    std::vector<int> output_shape = input->get_shape();

    if (reduce) {
        output_shape.erase(output_shape.begin() + axis);
    } else {
        output_shape[axis] = 1;
    }

    CHECK_EQ(output->get_shape(), output_shape,
             "func_sum: output shape does not equal to the shape of reduced "
             "form of input");

    // calculate stride
    int stride = 1;
    for (int i = axis + 1; i < (int)input->get_shape().size(); i++) {
        stride *= input->get_shape()[i];
    }

    // calculate the sum over the axis
    int output_size = output->get_vec().size();
    int axis_size = input->get_shape()[axis];

    for (int i = 0; i < output_size; i++) {
        int base_index = (i / stride) * stride * axis_size + (i % stride);
        float total = 0.f;

        for (int j = 0; j < axis_size; j++) {
            total += input->get_vec()[base_index + j * stride];
        }

        output->get_vec()[i] = total;
    }
}

void func_mean(Array *output, const Array *input, int axis, bool reduce) {
    CHECK_COND(axis >= 0,
               "func_mean: support for negative axis not implemented");
    CHECK_COND(axis < input->get_shape().size(),
               "func_mean: axis is out of bound");

    // validate output shape
    std::vector<int> output_shape = input->get_shape();

    if (reduce) {
        output_shape.erase(output_shape.begin() + axis);
    } else {
        output_shape[axis] = 1;
    }

    CHECK_EQ(output->get_shape(), output_shape,
             "func_mean: output shape does not equal to the shape of reduced "
             "form of input");

    // calculate stride
    int stride = 1;
    for (int i = axis + 1; i < (int)input->get_shape().size(); i++) {
        stride *= input->get_shape()[i];
    }

    // calculate the mean over the axis
    int output_size = output->get_vec().size();
    int axis_size = input->get_shape()[axis];

    for (int i = 0; i < output_size; i++) {
        int base_index = (i / stride) * stride * axis_size + (i % stride);
        float total = 0.f;

        for (int j = 0; j < axis_size; j++) {
            total += input->get_vec()[base_index + j * stride];
        }

        output->get_vec()[i] = total / axis_size;
    }
}

} // namespace nnv2
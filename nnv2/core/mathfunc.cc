#include <algorithm>
#include <vector>

#include <cblas.h>

#include "mathfunc.h"
#include "utils.h"

namespace nnv2 {

void func_add(Array &output, const Array &input1, const Array &input2) {
    int input1_size = input1.get_data().size();
    int input2_size = input2.get_data().size();
    int output_size = output.get_data().size();

    CHECK_EQ(input1_size, input2_size, "func_add: size of inputs mismatched");
    CHECK_EQ(output_size, input1_size, "func_add: size of output mismatched with size of inputs");

    std::transform(input1.get_data().begin(), input1.get_data().end(),
                   input2.get_data().begin(), output.get_data().begin(),
                   std::plus<float>());
}

void func_add(Array &output, const Array &input, float value) {
    CHECK_EQ(output.get_data().size(), input.get_data().size(),
             "func_add: size of output mismatched with size of input");

    std::transform(input.get_data().begin(), input.get_data().end(), output.get_data().begin(),
                   [](const float &e) { return e + value; });
}

void func_sub(Array &output, const Array &input1, const Array &input2) {
    int input1_size = input1.get_data().size();
    int input2_size = input2.get_data().size();
    int output_size = output.get_data().size();

    CHECK_EQ(input1_size, input2_size, "func_sub: size of inputs mismatched");
    CHECK_EQ(output_size, input1_size, "func_sub: size of output mismatched with size of inputs");

    std::transform(input1.get_data().begin(), input1.get_data().end(),
                   input2.get_data().begin(), output.get_data().begin(),
                   std::minus<float>());
}

void func_sub(Array &output, const Array &input, float value) {
    CHECK_EQ(output.get_data().size(), input.get_data().size(),
             "func_add: size of output mismatched with size of input");

    std::transform(input.get_data().begin(), input.get_data().end(), output.get_data().begin(),
                   [](const float &e) { return e - value; });
}

void func_mul(Array &output, const Array &input1, const Array &input2) {
    int input1_size = input1.get_data().size();
    int input2_size = input2.get_data().size();
    int output_size = output.get_data().size();

    CHECK_EQ(input1_size, input2_size, "func_mul: size of inputs mismatched");
    CHECK_EQ(output_size, input1_size, "func_mul: size of output mismatched with size of inputs");

    std::transform(input1.get_data().begin(), input1.get_data().end(),
                   input2.get_data().begin(), output.get_data().begin(),
                   std::multiplies<float>());
}

void func_mul(Array &output, const Array &input, float value) {
    CHECK_EQ(output.get_data().size(), input.get_data().size(),
             "func_add: size of output mismatched with size of input");

    std::transform(input.get_data().begin(), input.get_data().end(), output.get_data().begin(),
                   [](const float &e) { return e * value; });
}

void func_div(Array &output, const Array &input1, const Array &input2) {
    int input1_size = input1.get_data().size();
    int input2_size = input2.get_data().size();
    int output_size = output.get_data().size();

    CHECK_EQ(input1_size, input2_size, "func_mul: size of inputs mismatched");
    CHECK_EQ(output_size, input1_size, "func_mul: size of output mismatched with size of inputs");

    std::transform(input1.get_data().begin(), input1.get_data().end(),
                   input2.get_data().begin(), output.get_data().begin(),
                   std::divides<float>());
}

void func_matmul(Array &output, const Array &input1, const Array &input2) {
    // Idea: Multidimensional arrays are treated as batches of matrices. The size of matrix batch
    // is the product of all but the last two elements of array's shape, which will then be the
    // dimensions of the member matrices

    // check if the dimensions are at least 2
    CHECK_COND(input1.get_shape().size() > 1, "func_matmul: array dimension should be at least 2");
    CHECK_COND(input2.get_shape().size() > 1, "func_matmul: array dimension should be at least 2");
    CHECK_COND(output.get_shape().size() > 1, "func_matmul: array dimension should be at least 2");

    int batch_size =
        std::accumulate(output.get_shape().begin(), output.get_shape().end() - 2, 1,
                        std::multiplies<int>());
    int batch_size_input1 =
        std::accumulate(input1.get_shape().begin(), input1.get_shape().end() - 2, 1,
                        std::multiplies<int>());
    int batch_size_input2 =
        std::accumulate(input2.get_shape().begin(), input2.get_shape().end() - 2, 1,
                        std::multiplies<int>());

    CHECK_EQ(batch_size, batch_size_input1, "func_matmul: batch size mismatched");
    CHECK_EQ(batch_size, batch_size_input2, "func_matmul: batch size mismatched");

    int input1_row = *(input1.get_shape().rbegin() + 1);
    int input1_col = *(input1.get_shape().rbegin());
    int input2_row = *(input2.get_shape().rbegin() + 1);
    int input2_col = *(input2.get_shape().rbegin());
    int output_row = *(output.get_shape().rbegin() + 1);
    int output_col = *(output.get_shape().rbegin());

    CHECK_EQ(input1_col, input2_row, "func_matmul: shapes of inputs mismatched");
    CHECK_EQ(output_row, input1_row,
             "func_matmul: row size of output mismatched with row size of input1");
    CHECK_EQ(output_col, input2_col,
             "func_matmul: column size of output mismatched with column size of input2");

    for (int i = 0; i < batch_size; i++) {
        float *input1_mat = input1.get_data().data() + i * input1_row * input1_col;
        float *input2_mat = input2.get_data().data() + i * input2_row * input2_col;
        float *output_mat = output.get_data().data() + i * output_row * output_col;

        cblas_sgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
                    output_row, output_col, input1_col,
                    1.0f,
                    input1_mat, input1_col,
                    input2_mat, input2_col,
                    0.0f,
                    output_mat, output_col);
    }
}

void func_transpose(Array &output, const Array &input) {
    int batch_size =
        std::accumulate(output.get_shape().begin(), output.get_shape().end() - 2, 1,
                        std::multiplies<int>());
    int batch_size_input =
        std::accumulate(input.get_shape().begin(), input.get_shape().end() - 2, 1,
                        std::multiplies<int>());

    CHECK_EQ(batch_size, batch_size_input, "func_transpose: batch size mismatched");

    int input_row = *(input.get_shape().rbegin() + 1);
    int input_col = *(input.get_shape().rbegin());
    int output_row = *(output.get_shape().rbegin() + 1);
    int output_col = *(output.get_shape().rbegin());

    CHECK_EQ(input_row, output_col, "func_transpose: shapes of inputs mismatched");
    CHECK_EQ(input_col, output_row, "func_transpose: shapes of inputs mismatched");

    for (int i = 0; i < batch_size; i++) {
        float *input_mat = input.get_data().data() + i * input_row * input_col;
        float *output_mat = output.get_data().data() + i * output_row * output_col;

        // divide the matrix into 32x32 grids and apply strip mining technique
        // https://wgropp.cs.illinois.edu/courses/cs598-s15/lectures/lecture07.pdf
        int stride = 32;

        for (int j = 0; j < output_col; j += stride) {
            for (int i = 0; i < output_row; i += stride) {
                for (int n = j; n < j + stride && n < output_col; n++) {
                    for (int m = i; m < i + stride && m < output_row; m++) {
                        output_mat[m * output_col + n] * input_mat[n * input_col + m];
                    }
                }
            }
        }
    }
}

} // namespace nnv2
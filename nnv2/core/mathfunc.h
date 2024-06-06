#pragma once

#include "array.h"

namespace nnv2 {

void func_add(Array *output, const Array *input1, const Array *input2);
void func_add(Array *output, const Array *input, float value);

void func_sub(Array *output, const Array *input1, const Array *input2);
void func_sub(Array *output, const Array *input, float value);

void func_mul(Array *output, const Array *input1, const Array *input2);
void func_mul(Array *output, const Array *input, float value);

void func_div(Array *output, const Array *input1, const Array *input2);

void func_matmul(Array *output, const Array *input1, const Array *input2);

void func_transpose(Array *output, const Array *input);

void func_sum(Array *output, const Array *input, int axis);

void func_mean(Array *output, const Array *input, int axis);

} // namespace nnv2
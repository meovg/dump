#include "optimizer.h"
#include "mathfunc.h"

namespace nnv2 {

void SGD::add_parameters(std::vector<Param> params) {
    for (const auto &[param, grad] : params) {
        parameter_list.push_back(param);
        grad_list.push_back(grad);
    }
}

void SGD::update_parameters(int batch_size) {
    float t1 = 1 - (2 * learning_rate * decay) / batch_size;
    float t2 = learning_rate / batch_size;

    int param_size = parameter_list.size();
    for (int i = 0; i < param_size; i++) {
        Array *param = parameter_list[i];
        Array *grad = grad_list[i];

        // W := W * t1 - dW * t2
        func_mul(param, param, t1);
        func_mul(grad, grad, t2);
        func_add(param, param, grad);

        grad->zero();
    }
}

} // namespace nnv2
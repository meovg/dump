#include "loss.h"
#include "common.h"
#include "mathfunc.h"

namespace nnv2 {

float CrossEntropyLoss::calculate_loss(const Array *results) {
    const Array *preds = target->get_output();

    CHECK_EQ(preds->get_shape(), results->get_shape(),
             "calculate_loss: shape of predictions mismatched with results");

    // get distribution that matches the label in the result
    init_cache(cache, "top1_dist", preds->get_shape());
    func_mul(cache["top1_dist"].get(), preds, results);

    // reduce the distribution matrix to one-dimensional array
    init_cache(cache, "top1_dist_reduced", {preds->get_shape()[0]});
    func_sum(cache["top1_dist_reduced"].get(), cache["top1_dist"].get(), 1);

    // calculate average log loss value of a batch
    init_cache(cache, "loss", {preds->get_shape()[0]});
    func_log(cache["loss"].get(), cache["top1_dist_reduced"].get());
    init_cache(cache, "loss_mean", {1});
    func_mean(cache["loss_mean"].get(), cache["loss"].get(), 0, false);

    // calculate loss gradient and propagate it back to the target layer
    Array *grad = target->get_grad();
    func_sub(grad, preds, results);

    return -1.0 * cache["loss_mean"]->get_vec()[0];
}

} // namespace nnv2
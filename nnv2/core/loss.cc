#include "loss.h"
#include "mathfunc.h"
#include "utils.h"

namespace nnv2 {

float CrossEntropyLoss::calculate_loss(const Array *results) {
    const Array *preds = target->get_output();

    CHECK_EQ(preds->get_shape(), results->get_shape(),
             "calculate_loss: shape of predictions mismatched with results");

    // get distribution that matches the label in the result
    INIT_CACHE(cache, "top1_dist", preds->get_shape());
    func_mul(cache["top1_dist"].get(), preds, results);

    // reduce the distribution matrix to one-dimensional array
    std::vector<int> preds_reduced_shape = {preds->get_shape()[0]};
    INIT_CACHE(cache, "top1_dist_reduced", preds_reduced_shape);
    func_sum(cache["top1_dist_reduced"].get(), cache["top1_dist"].get(), 1);

    // calculate average log loss value of a batch
    INIT_CACHE(cache, "loss", preds_reduced_shape);
    func_log(cache["loss"].get(), cache["top1_dist_reduced"].get());

    std::vector<int> loss_mean_shape = {1};
    INIT_CACHE(cache, "loss_mean", loss_mean_shape);
    func_mean(cache["loss_mean"].get(), cache["loss"].get(), 0, false);

    // calculate loss gradient and propagate it back to the target layer
    Array *grad = target->get_grad();
    func_sub(grad, preds, results);

    return -1.0 * cache["loss_mean"]->get_vec()[0];
}

} // namespace nnv2
#include "loss.h"
#include "mathfunc.h"
#include "utils.h"

namespace nnv2 {

float CrossEntropyLoss::calculate_loss(const Array *results) {
    const Array *preds = target->get_output();

    CHECK_EQ(preds->get_shape(), results->get_shape(),
             "calculate_loss: shape of predictions mismatched with results");

    // get distribution that matches the label in the result
    Array top1_dist(preds->get_shape());
    func_mul(&top1_dist, preds, results);

    // reduce the distribution matrix to one-dimensional array
    Array top1_dist_reduced({preds->get_shape()[0]});
    func_sum(&top1_dist_reduced, &top1_dist, 1);

    // calculate average log loss value of a batch
    Array loss({preds->get_shape()[0]});
    func_log(&loss, &top1_dist_reduced);

    Array loss_mean({1});
    func_mean(&loss_mean, &loss, 0, false);

    // calculate loss gradient and propagate it back to the target layer
    Array *grad = target->get_grad();
    func_sub(grad, preds, results);

    return -1.0 * loss_mean.get_vec()[0];
}

} // namespace nnv2
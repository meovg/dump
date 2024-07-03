#pragma once

#include "dataset.h"

namespace nnv2 {

class Mnist: public Dataset {
public:
    Mnist(std::string data_path);

private:
    void read_images(std::vector<std::vector<float>> &output,
                     std::string filename, bool is_train) override;
    void read_labels(std::vector<unsigned char> &output,
                     std::string filename) override;

    // i'm just lazy
    const float train_mean = 0.1306604762738431;
    const float train_stddev = 0.3081078038564622;
    const float test_mean = 0.13251460584233699;
    const float test_stddev = 0.3104802479305348;
};

} // namespace nnv2
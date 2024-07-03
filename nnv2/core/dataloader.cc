#include <algorithm>
#include <iostream>
#include <vector>

#include "dataloader.h"
#include "utils.h"

namespace nnv2 {

void DataLoader::load_train_batch() {
    // update offset
    int start = train_data_offset;
    int end =
        std::min(start + batch_size, (int)dataset->get_train_images().size());
    train_data_offset = end;
    int size = end - start;

    // initialize batch memory
    int h = dataset->get_image_height();
    int w = dataset->get_image_width();
    std::vector<int> output_shape = {size, 1, h, w};
    INIT_ARRAY(output, output_shape);

    int n_labels = dataset->get_label_count();
    std::vector<int> output_labels_shape = {size, n_labels};
    INIT_ARRAY(output_labels, output_labels_shape);
    std::fill(output_labels->get_vec().begin(), output_labels->get_vec().end(),
              0);

    // extract a batch of train data
    for (int i = start; i < end; i++) {
        // train images
        output->get_vec().insert(output->get_vec().begin() + i * h * w,
                                 dataset->get_train_images()[i].begin(),
                                 dataset->get_train_images()[i].end());
        // train labels, with one-hot encoding
        int one_hot_index =
            (i - start) * n_labels + (int)(dataset->get_train_labels()[i]);
        output_labels->get_vec()[one_hot_index] = 1;
    }
}

void DataLoader::load_test_batch() {
    // update offset
    int start = test_data_offset;
    int end =
        std::min(start + batch_size, (int)dataset->get_test_images().size());
    test_data_offset = end;
    int size = end - start;

    // initialize batch memory
    int h = dataset->get_image_height();
    int w = dataset->get_image_width();
    std::vector<int> output_shape = {size, 1, h, w};
    INIT_ARRAY(output, output_shape);

    int n_labels = dataset->get_label_count();
    std::vector<int> output_labels_shape = {size, n_labels};
    INIT_ARRAY(output_labels, output_labels_shape);
    std::fill(output_labels->get_vec().begin(), output_labels->get_vec().end(),
              0);

    // extract a batch of test data
    for (int i = start; i < end; i++) {
        // test images
        output->get_vec().insert(output->get_vec().begin() + i * h * w,
                                 dataset->get_test_images()[i].begin(),
                                 dataset->get_test_images()[i].end());
        // test labels, with one-hot encoding
        int one_hot_index =
            (i - start) * n_labels + (int)(dataset->get_test_labels()[i]);
        output_labels->get_vec()[one_hot_index] = 1;
    }
}

bool DataLoader::has_next_train_batch() {
    return train_data_offset < dataset->get_train_images().size();
}

bool DataLoader::has_next_test_batch() {
    return test_data_offset < dataset->get_test_images().size();
}

void DataLoader::health_check() const {
    std::cout << "Batch size  : " << batch_size << std::endl;
    std::cout << "Train offset: " << train_data_offset << std::endl;
    std::cout << "Train size  : " << dataset->get_train_images().size()
              << std::endl;
    std::cout << "Test offset : " << test_data_offset << std::endl;
    std::cout << "Test size   : " << dataset->get_test_images().size()
              << std::endl;
}

void DataLoader::reset() {
    train_data_offset = 0;
    test_data_offset = 0;

    dataset->shuffle_train_data();
}

} // namespace nnv2
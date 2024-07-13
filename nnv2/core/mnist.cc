#include <cstdlib>
#include <fstream>

#include "mnist.h"
#include "utils.h"

namespace nnv2 {

Mnist::Mnist(std::string data_path) : Dataset(data_path, 28, 28, 10) {
    // read train data
    read_images(train_images, data_path + "/train-images-idx3-ubyte", true);
    read_labels(train_labels, data_path + "/train-labels-idx1-ubyte");

    // read test data
    read_images(test_images, data_path + "/t10k-images-idx3-ubyte", false);
    read_labels(test_labels, data_path + "/t10k-labels-idx1-ubyte");
};

static unsigned reverse_int(unsigned i) {
    unsigned char ch1, ch2, ch3, ch4;
    ch1 = i & 255;
    ch2 = (i >> 8) & 255;
    ch3 = (i >> 16) & 255;
    ch4 = (i >> 24) & 255;
    return ((unsigned)ch1 << 24) + ((unsigned)ch2 << 16) +
           ((unsigned)ch3 << 8) + (unsigned)ch4;
}

void Mnist::read_images(std::vector<std::vector<float>> &output,
                        std::string filename, bool is_train) {
    std::ifstream file(filename, std::ios::binary);
    if (file.is_open()) {
        unsigned magic_number = 0;
        unsigned n_images = 0;
        unsigned n_rows = 0;
        unsigned n_cols = 0;

        // read metadata
        file.read((char *)&magic_number, sizeof(magic_number));
        file.read((char *)&n_images, sizeof(n_images));
        file.read((char *)&n_rows, sizeof(n_rows));
        file.read((char *)&n_cols, sizeof(n_cols));
        magic_number = reverse_int(magic_number);
        n_images = reverse_int(n_images);
        n_rows = reverse_int(n_rows);
        n_cols = reverse_int(n_cols);

        std::cout << filename << std::endl;
        std::cout << "magic number = " << magic_number << std::endl;
        std::cout << "number of images = " << n_images << std::endl;
        std::cout << "rows = " << n_rows << std::endl;
        std::cout << "cols = " << n_cols << std::endl;

        CHECK_EQ(h, n_rows, "Mnist::read_images: row length must be 28");
        CHECK_EQ(w, n_cols, "Mnist::read_images: column length must be 28");

        // read and normalize images
        float mean = is_train ? train_mean : test_mean;
        float stddev = is_train ? train_stddev : test_stddev;

        std::vector<unsigned char> image(h * w);
        std::vector<float> normalized_image(h * w);
        for (int i = 0; i < n_images; i++) {
            file.read((char *)image.data(), sizeof(unsigned char) * h * w);

            // normalize image
            for (int k = 0; k < h * w; k++)
                normalized_image[k] = ((float)image[k] / 255 - mean) / stddev;

            output.push_back(normalized_image);
        }
    } else {
        std::cerr << filename << " not found" << std::endl;
        exit(1);
    }
}

void Mnist::read_labels(std::vector<unsigned char> &output,
                        std::string filename) {
    std::ifstream file(filename, std::ios::binary);
    if (file.is_open()) {
        unsigned magic_number = 0;
        unsigned n_images = 0;

        file.read((char *)&magic_number, sizeof(magic_number));
        file.read((char *)&n_images, sizeof(n_images));
        magic_number = reverse_int(magic_number);
        n_images = reverse_int(n_images);

        std::cout << filename << std::endl;
        std::cout << "magic number = " << magic_number << std::endl;
        std::cout << "number of images = " << n_images << std::endl;

        for (int i = 0; i < n_images; i++) {
            unsigned char label = 0;
            file.read((char *)&label, sizeof(label));
            output.push_back(label);
        }
    } else {
        std::cerr << filename << " not found" << std::endl;
        exit(1);
    }
}

} // namespace nnv2
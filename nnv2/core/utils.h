#pragma once

#include <iostream>
#include <cstdlib>

#include "array.h"

namespace nnv2 {

#define CHECK_EQ(val1, val2, message)                           \
    do {                                                        \
        if ((val1) != (val2)) {                                 \
            std::cerr << __FILE__ << "(" << __LINE__ << "): "   \
                      << (message) << std::endl;                \
            exit(1);                                            \
        }                                                       \
    } while (0)

#define CHECK_COND(cond, message)                               \
    do {                                                        \
        if (!(cond)) {                                            \
            std::cerr << __FILE__ << "(" << __LINE__ << "): "   \
                      << (message) << std::endl;                \
            exit(1);                                            \
        }                                                       \
    } while (0)

#define INIT_ARRAY(arr_ptr, shape)                  \
    do {                                            \
        if ((arr_ptr).get() == nullptr) {           \
            (arr_ptr).reset(new Array((shape)));    \
        } else if (arr_ptr->get_shape() != shape) { \
            (arr_ptr)->resize((shape));             \
        }                                           \
    } while (0)

} // namespace nnv2
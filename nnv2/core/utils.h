#pragma once

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
        if (cond) {                                             \
            std::cerr << __FILE__ << "(" << __LINE__ << "): "   \
                      << (message) << std::endl;                \
            exit(1);                                            \
        }                                                       \
    } while (0)

} // namespace nnv2
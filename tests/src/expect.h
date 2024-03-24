#include <core/logger.h>
#include <math/kmath.h>

// @brief expects expected to be equal to actual -- checks if 2 values are equal, if not, tells you the results and the file and line that they are on
#define expect_should_be(expected, actual)                                                              \
    if (actual != expected) {                                                                           \
        KERROR("--> Expected %lld, but got: %lld. File: %s:%d.", expected, actual, __FILE__, __LINE__); \
        return false;                                                                                   \
    }

// @brief expects expected to not be equal to actual -- checks if 2 values are equal, if they are equal, tells you the results and the file and line that they are on
#define expect_should_not_be(expected, actual)                                                                         \
    if (actual == expected) {                                                                                          \
        KERROR("--> Expected %lld !=  %lld., but they are equal. File: %s:%d.", expected, actual, __FILE__, __LINE__); \
        return false;                                                                                                  \
    }

// @brief expects expected to be actual given a tolerance of K_FLOAT_EPSILON - check if the absolute value of (the difference btween expected and actual) is less than 0.001
#define expect_float_to_be(expected, actual)                                                        \
    if (kabs(expected - actual) > 0.001f) {                                                         \
        KERROR("--> expected %f, but got: %f. File: %s:%d.", expected, actual, __FILE__, __LINE__); \
        return false;                                                                               \
    }

// @brief expects actual to be true - check a boolean and see if it is true
#define expect_to_be_true(actual)                                                      \
    if (actual != true) {                                                              \
        KERROR("--> expected true, but got: false. File: %s:%d.", __FILE__, __LINE__); \
        return false;                                                                  \
    }

// @brief expects actual to be false - check a boolean and see if it is false
#define expect_to_be_false(actual)                                                     \
    if (actual != false) {                                                             \
        KERROR("--> expected false, but got: true. File: %s:%d.", __FILE__, __LINE__); \
        return false;                                                                  \
    }
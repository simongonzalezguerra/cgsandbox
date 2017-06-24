#ifndef TEST_UTILS_HPP
#define TEST_UTILS_HPP

#include "gtest/gtest.h"

#define ASSERT_SEQUENCE_NEAR(a_value, b_value, len, threshold) \
  do { \
    auto _a = a_value; \
    auto _b = b_value; \
    for (std::size_t i = 0; i < len; i++) { \
      ASSERT_NEAR(_a[i], _b[i], threshold); \
    } \
  } while (0)

#define ASSERT_MATRIX_NEAR(a_value, b_value, threshold) \
  do { \
    auto _a = a_value; \
    auto _b = b_value; \
    for (std::size_t i = 0; i < 4; ++i) { \
      for (std::size_t j = 0; j < 4; ++j) { \
        ASSERT_NEAR(_a[i][j], _b[i][j], threshold); \
      } \
    } \
  } while (0)

#endif


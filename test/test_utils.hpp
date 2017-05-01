#ifndef TEST_UTILS_HPP
#define TEST_UTILS_HPP

#include "gtest/gtest.h"

#define ASSERT_SEQUENCE_NEAR(a, b, len, threshold) \
  do { \
    for (std::size_t i = 0; i < len; i++) { \
      ASSERT_NEAR(a[i], b[i], threshold); \
    } \
  } while (0)

#endif


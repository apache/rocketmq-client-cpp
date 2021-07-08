#include "gtest/gtest.h"
#include <iostream>

TEST(AsanTest, testMemoryLeakage) {
  auto p = new int(1);
  std::cout << *p << std::endl;
}
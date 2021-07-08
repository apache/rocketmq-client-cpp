#include "absl/strings/str_cat.h"
#include "gtest/gtest.h"
#include <string>

TEST(HexTest, testHex) {
  const char* data = "hello";
  const char dict[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
  std::string s;
  for (std::size_t i = 0; i < strlen(data); i++) {
    char c = *(data + i);
    s.append(&dict[(0xF0 & c) >> 4], 1);
    s.append(&dict[(0x0F & c)], 1);
  }

  std::string expect{"68656C6C6F"};
  EXPECT_EQ(expect, s);
}
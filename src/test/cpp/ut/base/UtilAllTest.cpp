#include "UtilAll.h"
#include "MixAll.h"
#include "absl/strings/str_split.h"
#include "asio.hpp"
#include "rocketmq/RocketMQ.h"
#include "spdlog/spdlog.h"
#include "gtest/gtest.h"
#include <chrono>
#include <cstdint>

ROCKETMQ_NAMESPACE_BEGIN

class UtilAllTest : public testing::Test {
public:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(UtilAllTest, testCompress) {
  std::string src("How are you doing?");
  std::string dst;
  bool success = UtilAll::compress(src, dst);
  EXPECT_TRUE(success);
  EXPECT_FALSE(dst.empty());
}

TEST_F(UtilAllTest, testUncompress) {
  std::string raw("What is your favorite color?");
  std::string compressed;
  EXPECT_TRUE(UtilAll::compress(raw, compressed));
  std::string uncompressed;
  EXPECT_TRUE(UtilAll::uncompress(compressed, uncompressed));
  EXPECT_EQ(raw.length(), uncompressed.length());
  EXPECT_EQ(raw, uncompressed);
}

TEST_F(UtilAllTest, benchmarkTest) {
  std::string raw;
  uint32_t len = 1024 * 1024;
  raw.reserve(len);
  for (uint32_t i = 0; i < len; i++) {
    raw.push_back(i % 128);
  }

  std::string compressed;
  auto now = std::chrono::steady_clock::now();
  UtilAll::compress(raw, compressed);
  auto elapsed = std::chrono::steady_clock::now() - now;
  EXPECT_TRUE(elapsed < std::chrono::milliseconds(100));
  EXPECT_TRUE(len / compressed.length() >= 5);
}

TEST_F(UtilAllTest, split) {
  std::string ip("8.8.8.8");
  std::vector<std::string> segments = absl::StrSplit(ip, '.');
  std::vector<std::string> expected = {"8", "8", "8", "8"};
  EXPECT_EQ(expected, segments);
}

TEST_F(UtilAllTest, macAddrss) {
  std::vector<unsigned char> mac;
  bool success = UtilAll::macAddress(mac);
  ASSERT_TRUE(success);
  std::cout << MixAll::hex(mac.data(), mac.size()) << std::endl;
}

TEST_F(UtilAllTest, testAsioGetHostName) {
  auto&& host_name = asio::ip::host_name();
  std::cout << host_name << std::endl;
}

ROCKETMQ_NAMESPACE_END
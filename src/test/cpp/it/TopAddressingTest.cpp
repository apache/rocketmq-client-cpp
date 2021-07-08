#include "TopAddressing.h"
#include "gtest/gtest.h"
#include <cstdlib>
#include <thread>

ROCKETMQ_NAMESPACE_BEGIN

class TopAddressingTest : public testing::Test {
public:
  void SetUp() override {}

  void TearDown() override {}

protected:
  TopAddressing top_addressing;
};

TEST_F(TopAddressingTest, testFetchNameServerAddresses) {
  std::vector<std::string> list;
  EXPECT_TRUE(top_addressing.fetchNameServerAddresses(list));
  EXPECT_FALSE(list.empty());
  for (const auto& item : list) {
    std::cout << item << std::endl;
  }
}

TEST_F(TopAddressingTest, testFetchNameServerAddresses_env) {
  int override = 1;
  setenv(HostInfo::ENV_LABEL_UNIT, "CENTER_UNIT.center", override);
  setenv(HostInfo::ENV_LABEL_STAGE, "DAILY", override);
  std::vector<std::string> list;
  EXPECT_TRUE(top_addressing.fetchNameServerAddresses(list));
  EXPECT_FALSE(list.empty());
  for (const auto& item : list) {
    std::cout << item << std::endl;
  }
}

TEST_F(TopAddressingTest, testFetchNameServerAddresses_benchmark) {
  int override = 1;
  setenv(HostInfo::ENV_LABEL_UNIT, "CENTER_UNIT.center", override);
  setenv(HostInfo::ENV_LABEL_STAGE, "DAILY", override);
  auto benchmark = [&]() {
    for (int i = 0; i < 256; ++i) {
      std::vector<std::string> list;
      EXPECT_TRUE(top_addressing.fetchNameServerAddresses(list));
      EXPECT_FALSE(list.empty());
    }
  };

  std::vector<std::thread> threads;
  threads.reserve(std::thread::hardware_concurrency() + 1);
  for (int i = 0; i < std::thread::hardware_concurrency() + 1; i++) {
    threads.emplace_back(benchmark);
  }

  for (auto& thread : threads) {
    thread.join();
  }
}

ROCKETMQ_NAMESPACE_END
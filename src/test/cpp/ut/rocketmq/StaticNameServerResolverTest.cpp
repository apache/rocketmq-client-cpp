#include "StaticNameServerResolver.h"

#include "absl/strings/str_split.h"

#include "gtest/gtest.h"
#include <vector>

ROCKETMQ_NAMESPACE_BEGIN

class StaticNameServerResolverTest : public testing::Test {
public:
  StaticNameServerResolverTest() : resolver_(name_server_list_) {}

  void SetUp() override { resolver_.start(); }

  void TearDown() override { resolver_.shutdown(); }

protected:
  std::string name_server_list_{"10.0.0.1:9876;10.0.0.2:9876"};
  StaticNameServerResolver resolver_;
};

TEST_F(StaticNameServerResolverTest, testResolve) {
  std::vector<std::string> segments = absl::StrSplit(name_server_list_, ';');
  ASSERT_EQ(segments, resolver_.resolve());
}

TEST_F(StaticNameServerResolverTest, testCurrentNext) {
  std::string&& name_server_1 = resolver_.current();
  std::string expected = "10.0.0.1:9876";
  EXPECT_EQ(expected, name_server_1);

  expected = "10.0.0.2:9876";
  std::string&& name_server_2 = resolver_.next();
  EXPECT_EQ(expected, name_server_2);
}

ROCKETMQ_NAMESPACE_END
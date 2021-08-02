#include "gtest/gtest.h"
#include "HostInfo.h"
#include "rocketmq/RocketMQ.h"
#include <cstdlib>
#include "fmt/format.h"

ROCKETMQ_NAMESPACE_BEGIN

class HostInfoTest : public testing::Test {
public:
  void SetUp() override {}

  void TearDown() override {}

protected:
  std::string site_{"site"};
  std::string unit_{"unit"};
  std::string app_{"app"};
  std::string stage_{"stage"};
};

TEST_F(HostInfoTest, testQueryString) {
  int overwrite = 1;
  setenv(HostInfo::ENV_LABEL_SITE, site_.c_str(), overwrite);
  setenv(HostInfo::ENV_LABEL_UNIT, unit_.c_str(), overwrite);
  setenv(HostInfo::ENV_LABEL_APP, app_.c_str(), overwrite);
  setenv(HostInfo::ENV_LABEL_STAGE, stage_.c_str(), overwrite);

  HostInfo host_info;
  std::string query_string = host_info.queryString();
  std::string query_string_template("labels=site:{},unit:{},app:{},stage:{}");
  EXPECT_EQ(query_string, fmt::format(query_string_template, site_, unit_, app_, stage_));
}

ROCKETMQ_NAMESPACE_END
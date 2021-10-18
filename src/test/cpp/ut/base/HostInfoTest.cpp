/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "HostInfo.h"
#include "fmt/format.h"
#include "rocketmq/RocketMQ.h"
#include "gtest/gtest.h"
#include <cstdlib>

ROCKETMQ_NAMESPACE_BEGIN

class HostInfoTest : public testing::Test {
public:
  void SetUp() override {
  }

  void TearDown() override {
  }

protected:
  std::string site_{"site"};
  std::string unit_{"unit"};
  std::string app_{"app"};
  std::string stage_{"stage"};

  void SetEnv(const char* key, const char* value) {
    int overwrite = 1;
#ifdef _WIN32
    std::string env;
    env.append(key);
    env.push_back('=');
    env.append(value);
    _putenv(env.c_str());
#else
    setenv(key, value, overwrite);
#endif
  }
};

TEST_F(HostInfoTest, testQueryString) {
  SetEnv(HostInfo::ENV_LABEL_SITE, site_.c_str());
  SetEnv(HostInfo::ENV_LABEL_UNIT, unit_.c_str());
  SetEnv(HostInfo::ENV_LABEL_APP, app_.c_str());
  SetEnv(HostInfo::ENV_LABEL_STAGE, stage_.c_str());

  HostInfo host_info;
  std::string query_string = host_info.queryString();
  std::string query_string_template("labels=site:{},unit:{},app:{},stage:{}");
  EXPECT_EQ(query_string, fmt::format(query_string_template, site_, unit_, app_, stage_));
}

ROCKETMQ_NAMESPACE_END
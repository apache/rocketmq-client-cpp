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
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "gtest/gtest.h"

#include "HttpClientImpl.h"
#include "LoggerImpl.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class HttpClientTest : public testing::Test {
public:
  void SetUp() override {
    SPDLOG_DEBUG("GHttpClient::SetUp() starts");
    http_client.start();
    SPDLOG_DEBUG("GHttpClient::SetUp() completed");
  }

  void TearDown() override {
    SPDLOG_DEBUG("GHttpClientTest::TearDown() starts");
    http_client.shutdown();
    SPDLOG_DEBUG("GHttpClientTest::TearDown() completed");
  }

protected:
  HttpClientImpl http_client;
};

TEST_F(HttpClientTest, testBasics) {
}

TEST_F(HttpClientTest, testGet) {
  auto cb = [](int code, const std::multimap<std::string, std::string>& headers, const std::string& body) {
    SPDLOG_INFO("Response received. Status-code: {}, Body: {}", code, body);
  };

  http_client.get(HttpProtocol::HTTP, "www.baidu.com", 80, "/", cb);
}

TEST_F(HttpClientTest, DISABLED_testJMEnv) {
  auto cb = [](int code, const std::multimap<std::string, std::string>& headers, const std::string& body) {
    SPDLOG_INFO("Response received. Status-code: {}, Body: {}", code, body);
  };

  http_client.get(HttpProtocol::HTTP, "jmenv.tbsite.net", 8080, "/rocketmq/nsaddr", cb);
}

ROCKETMQ_NAMESPACE_END
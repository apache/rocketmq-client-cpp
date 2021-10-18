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
#include "rocketmq/Logger.h"
#include "LoggerImpl.h"
#include "rocketmq/RocketMQ.h"
#include "spdlog/spdlog.h"
#include "gtest/gtest.h"
ROCKETMQ_NAMESPACE_BEGIN

class LoggerTest : public testing::Test {
protected:
  std::size_t log_file_size_{1024 * 4};
};

TEST_F(LoggerTest, testLogger_Trace) {
  Logger& logger = getLogger();
  logger.setFileSize(log_file_size_);
  logger.setLevel(Level::Trace);
  logger.setConsoleLevel(Level::Trace);
  logger.init();
  SPDLOG_ERROR("=========Trace===========");
  SPDLOG_TRACE("Should show up!");
  SPDLOG_DEBUG("Should show up!");
  SPDLOG_INFO("Should show up!");
  SPDLOG_WARN("Should show up!");
  SPDLOG_ERROR("Should show up!");
}

TEST_F(LoggerTest, testLogger_Debug) {
  Logger& logger = getLogger();
  logger.setFileSize(log_file_size_);
  logger.setLevel(Level::Debug);
  logger.init();
  SPDLOG_ERROR("=========Debug===========");
  SPDLOG_TRACE("Should NOT get logged");
  SPDLOG_DEBUG("Should show up");
  SPDLOG_INFO("Should show up!!");
  SPDLOG_WARN("Should show up!");
  SPDLOG_ERROR("Should show up!");
}

TEST_F(LoggerTest, testLogger_Info) {
  Logger& logger = getLogger();
  logger.setFileSize(log_file_size_);
  logger.setLevel(Level::Info);
  logger.init();
  SPDLOG_ERROR("=========Info===========");
  SPDLOG_TRACE("Should NOT get logged");
  SPDLOG_DEBUG("Should NOT get logged");
  SPDLOG_INFO("Should show up!");
  SPDLOG_WARN("Should show up!");
  SPDLOG_ERROR("Should show up!");
}

TEST_F(LoggerTest, testLogger_Warn) {
  Logger& logger = getLogger();
  logger.setFileSize(log_file_size_);
  logger.setLevel(Level::Warn);
  logger.init();
  SPDLOG_ERROR("=========Warn===========");
  SPDLOG_TRACE("Should NOT get logged");
  SPDLOG_DEBUG("Should NOT get logged");
  SPDLOG_INFO("Should NOT get logged");
  SPDLOG_WARN("Should show up!");
  SPDLOG_ERROR("Should show up!");
}

TEST_F(LoggerTest, testLogger_Error) {
  Logger& logger = getLogger();
  logger.setFileSize(log_file_size_);
  logger.setLevel(Level::Error);
  logger.init();
  SPDLOG_ERROR("=========Error===========");
  SPDLOG_TRACE("Should NOT get logged");
  SPDLOG_DEBUG("Should NOT get logged");
  SPDLOG_INFO("Should NOT get logged");
  SPDLOG_WARN("Should NOT get logged");
  SPDLOG_ERROR("Should show up!");
}

TEST_F(LoggerTest, testFileRolling) {
  Logger& logger = getLogger();
  logger.setFileSize(log_file_size_);
  logger.setLevel(Level::Trace);
  logger.init();
  for (size_t i = 0; i < 32; i++) {
    SPDLOG_TRACE("Should show up!");
    SPDLOG_DEBUG("Should show up!");
    SPDLOG_INFO("Should show up!");
    SPDLOG_WARN("Should show up!");
    SPDLOG_ERROR("Should show up!");
  }
}

ROCKETMQ_NAMESPACE_END
#include "rocketmq/Logger.h"
#include "spdlog/spdlog.h"
#include "gtest/gtest.h"

using namespace rocketmq;

class LoggerTest : public testing::Test {};

TEST_F(LoggerTest, testDefaultLogger) {
  Logger& logger = getLogger();
  logger.setFileSize(1024);
  logger.setLevel(Level::Info);

// TODO: Figure out constraints of file system when building with bazel
#ifndef BAZEL
  logger.init();
#endif

  for (int i = 0; i < 32; ++i) {
    SPDLOG_DEBUG("Should not appear by default");
    SPDLOG_INFO("Should show up!!");
    SPDLOG_WARN("Warn message is here");
    SPDLOG_ERROR("This is an error message");
  }
}
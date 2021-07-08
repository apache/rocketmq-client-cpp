#pragma once

#include <thread>
#include <mutex>
#include <string>

#include "RocketMQ.h"

#ifndef SPDLOG_ACTIVE_LEVEL
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#endif

ROCKETMQ_NAMESPACE_BEGIN

enum Level { Trace = 0, Debug = 1, Info = 2, Warn = 3, Error = 4 };

class Logger {
public:
  Logger();

  ~Logger();

  void setLevel(Level level);

  void setFileSize(int32_t file_size) {
    file_size_ = file_size;
  }

  void setFileCount(int32_t file_count) {
    file_count_ = file_count;
  }

  void setPattern(std::string pattern) {
    pattern_ = std::move(pattern);
  }

  void init();

private:
  Level level_;
  std::string log_home_;
  int32_t file_size_;
  int32_t file_count_;
  std::string pattern_;

  std::once_flag init_once_;

  void init0();

  static const char* DEFAULT_PATTERN;
  static const char* USER_HOME_ENV;
};

Logger& getLogger();

ROCKETMQ_NAMESPACE_END
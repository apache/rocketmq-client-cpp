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
#ifndef ROCKETMQ_LOG_LOGGING_H_
#define ROCKETMQ_LOG_LOGGING_H_

#include <memory>  // std::shared_ptr
#include <string>  // std::string
#include <vector>  // std::vector

// clang-format off
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bundled/printf.h>
// clang-format on

namespace rocketmq {

enum LogLevel {
  LOG_LEVEL_FATAL = 1,
  LOG_LEVEL_ERROR = 2,
  LOG_LEVEL_WARN = 3,
  LOG_LEVEL_INFO = 4,
  LOG_LEVEL_DEBUG = 5,
  LOG_LEVEL_TRACE = 6,
  LOG_LEVEL_LEVEL_NUM = 7
};

class LogAdapter {
 public:
  virtual ~LogAdapter();

  static LogAdapter* getLogInstance();

  inline spdlog::logger* getSeverityLogger() { return logger_.get(); }

  void setLogFileNumAndSize(int logNum, int sizeOfPerFile);

  inline LogLevel log_level() const { return log_level_; }
  inline void set_log_level(LogLevel logLevel) {
    log_level_ = logLevel;
    setLogLevelInner(logLevel);
  }

 private:
  LogAdapter();
  void setLogLevelInner(LogLevel logLevel);

 private:
  LogLevel log_level_;
  std::string log_file_;

  std::shared_ptr<spdlog::logger> logger_;
#if SPDLOG_VER_MAJOR >= 1
  std::vector<spdlog::sink_ptr> log_sinks_;
#endif
};

#define DEFAULT_LOG_ADAPTER LogAdapter::getLogInstance()
#define DEFAULT_LOGGER DEFAULT_LOG_ADAPTER->getSeverityLogger()

#define SPDLOG_PRINTF(logger, level, format, ...)                        \
  do {                                                                   \
    if (logger->should_log(level)) {                                     \
      std::string message = fmt::sprintf(format, ##__VA_ARGS__);         \
      logger->log(level, "{} [{}:{}]", message, __FUNCTION__, __LINE__); \
    }                                                                    \
  } while (0)

#define LOG_FATAL(...) SPDLOG_PRINTF(DEFAULT_LOGGER, spdlog::level::critical, __VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_PRINTF(DEFAULT_LOGGER, spdlog::level::err, __VA_ARGS__)
#define LOG_WARN(...) SPDLOG_PRINTF(DEFAULT_LOGGER, spdlog::level::warn, __VA_ARGS__)
#define LOG_INFO(...) SPDLOG_PRINTF(DEFAULT_LOGGER, spdlog::level::info, __VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_PRINTF(DEFAULT_LOGGER, spdlog::level::debug, __VA_ARGS__)

#define SPDLOG_EXT(logger, level, format, ...)                                    \
  do {                                                                            \
    logger->log(level, format " [{}:{}]", ##__VA_ARGS__, __FUNCTION__, __LINE__); \
  } while (0)

#define LOG_FATAL_NEW(...) SPDLOG_EXT(DEFAULT_LOGGER, spdlog::level::critical, __VA_ARGS__)
#define LOG_ERROR_NEW(...) SPDLOG_EXT(DEFAULT_LOGGER, spdlog::level::err, __VA_ARGS__)
#define LOG_WARN_NEW(...) SPDLOG_EXT(DEFAULT_LOGGER, spdlog::level::warn, __VA_ARGS__)
#define LOG_INFO_NEW(...) SPDLOG_EXT(DEFAULT_LOGGER, spdlog::level::info, __VA_ARGS__)
#define LOG_DEBUG_NEW(...) SPDLOG_EXT(DEFAULT_LOGGER, spdlog::level::debug, __VA_ARGS__)

}  // namespace rocketmq

#endif  // ROCKETMQ_LOG_LOGGING_H_

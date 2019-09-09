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
#ifndef __ROCKETMQ_LOGGING_H__
#define __ROCKETMQ_LOGGING_H__

#include <memory>
#include <string>

// clang-format off
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bundled/printf.h>
// clang-format on

#include "MQClient.h"

namespace rocketmq {

class logAdapter {
 public:
  ~logAdapter();

  static logAdapter* getLogInstance();

  void setLogLevel(elogLevel logLevel);
  elogLevel getLogLevel();

  void setLogFileNumAndSize(int logNum, int sizeOfPerFile);

  spdlog::logger* getSeverityLogger() { return m_logger.get(); }

 private:
  logAdapter();
  void setLogLevelInner(elogLevel logLevel);

  elogLevel m_logLevel;
  std::string m_logFile;

  std::shared_ptr<spdlog::logger> m_logger;
  std::vector<spdlog::sink_ptr> m_logSinks;
};

#define ALOG_ADAPTER logAdapter::getLogInstance()
#define AGENT_LOGGER ALOG_ADAPTER->getSeverityLogger()

#define SPDLOG_PRINTF(logger, level, format, ...)                        \
  do {                                                                   \
    if (logger->should_log(level)) {                                     \
      std::string message = fmt::sprintf(format, ##__VA_ARGS__);         \
      logger->log(level, "{} [{}:{}]", message, __FUNCTION__, __LINE__); \
    }                                                                    \
  } while (0)

#define LOG_FATAL(...) SPDLOG_PRINTF(AGENT_LOGGER, spdlog::level::critical, __VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_PRINTF(AGENT_LOGGER, spdlog::level::err, __VA_ARGS__)
#define LOG_WARN(...) SPDLOG_PRINTF(AGENT_LOGGER, spdlog::level::warn, __VA_ARGS__)
#define LOG_INFO(...) SPDLOG_PRINTF(AGENT_LOGGER, spdlog::level::info, __VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_PRINTF(AGENT_LOGGER, spdlog::level::debug, __VA_ARGS__)

#define SPDLOG_EXT(logger, level, format, ...)                                    \
  do {                                                                            \
    logger->log(level, format " [{}:{}]", ##__VA_ARGS__, __FUNCTION__, __LINE__); \
  } while (0)

#define LOG_FATAL_NEW(...) SPDLOG_EXT(AGENT_LOGGER, spdlog::level::critical, __VA_ARGS__)
#define LOG_ERROR_NEW(...) SPDLOG_EXT(AGENT_LOGGER, spdlog::level::err, __VA_ARGS__)
#define LOG_WARN_NEW(...) SPDLOG_EXT(AGENT_LOGGER, spdlog::level::warn, __VA_ARGS__)
#define LOG_INFO_NEW(...) SPDLOG_EXT(AGENT_LOGGER, spdlog::level::info, __VA_ARGS__)
#define LOG_DEBUG_NEW(...) SPDLOG_EXT(AGENT_LOGGER, spdlog::level::debug, __VA_ARGS__)

}  // namespace rocketmq

#endif  // __ROCKETMQ_LOGGING_H__

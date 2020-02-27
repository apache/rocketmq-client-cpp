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
#include "Logging.h"

#include <iostream>

#if SPDLOG_VER_MAJOR >= 1
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#else
#include <spdlog/async_logger.h>
#endif

#include "UtilAll.h"

namespace rocketmq {

logAdapter::~logAdapter() {
  spdlog::drop("default");
}

logAdapter* logAdapter::getLogInstance() {
  static logAdapter singleton_;
  return &singleton_;
}

logAdapter::logAdapter() : m_logLevel(eLOG_LEVEL_INFO) {
  std::string logDir;
  const char* dir = std::getenv(ROCKETMQ_CPP_LOG_DIR_ENV.c_str());
  if (dir != nullptr && dir[0] != '\0') {
    // FIXME: replace '~' by home directory.
    logDir = dir;
  } else {
    logDir = UtilAll::getHomeDirectory();
    logDir.append("/logs/rocketmq-cpp/");
  }
  if (logDir[logDir.size() - 1] != '/') {
    logDir.append("/");
  }

  if (!UtilAll::existDirectory(logDir)) {
    UtilAll::createDirectory(logDir);
  }
  if (!UtilAll::existDirectory(logDir)) {
    std::cerr << "create log dir error, will exit" << std::endl;
    exit(1);
  }

  std::string fileName = UtilAll::to_string(UtilAll::getProcessId()) + "_" + "rocketmq-cpp.log";
  m_logFile = logDir + fileName;

#if SPDLOG_VER_MAJOR >= 1
  spdlog::init_thread_pool(8192, 1);

  auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(m_logFile, 1024 * 1024 * 100, 3);
  rotating_sink->set_level(spdlog::level::debug);
  rotating_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread@%t] - %v");
  m_logSinks.push_back(rotating_sink);

  m_logger = std::make_shared<spdlog::async_logger>("default", m_logSinks.begin(), m_logSinks.end(),
                                                    spdlog::thread_pool(), spdlog::async_overflow_policy::block);

  // register it if you need to access it globally
  spdlog::register_logger(m_logger);

  // when an error occurred, flush disk immediately
  m_logger->flush_on(spdlog::level::err);

  spdlog::flush_every(std::chrono::seconds(3));
#else
  size_t q_size = 4096;
  spdlog::set_async_mode(q_size);
  m_logger = spdlog::rotating_logger_mt("default", m_logFile, 1024 * 1024 * 100, 3);
  m_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread@%t] - %v");
#endif

  setLogLevelInner(m_logLevel);
}

void logAdapter::setLogLevelInner(elogLevel logLevel) {
  switch (logLevel) {
    case eLOG_LEVEL_FATAL:
      m_logger->set_level(spdlog::level::critical);
      break;
    case eLOG_LEVEL_ERROR:
      m_logger->set_level(spdlog::level::err);
      break;
    case eLOG_LEVEL_WARN:
      m_logger->set_level(spdlog::level::warn);
      break;
    case eLOG_LEVEL_INFO:
      m_logger->set_level(spdlog::level::info);
      break;
    case eLOG_LEVEL_DEBUG:
      m_logger->set_level(spdlog::level::debug);
      break;
    case eLOG_LEVEL_TRACE:
      m_logger->set_level(spdlog::level::trace);
      break;
    default:
      m_logger->set_level(spdlog::level::info);
      break;
  }
}

void logAdapter::setLogLevel(elogLevel logLevel) {
  m_logLevel = logLevel;
  setLogLevelInner(logLevel);
}

elogLevel logAdapter::getLogLevel() {
  return m_logLevel;
}

void logAdapter::setLogFileNumAndSize(int logNum, int sizeOfPerFile) {}

}  // namespace rocketmq

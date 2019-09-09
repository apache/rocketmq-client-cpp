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

#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>

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
  string homeDir(UtilAll::getHomeDirectory());
  homeDir.append("/logs/rocketmq-cpp/");
  if (!UtilAll::existDirectory(homeDir)) {
    UtilAll::createDirectory(homeDir);
  }
  if (!UtilAll::existDirectory(homeDir)) {
    std::cerr << "create log dir error, will exit" << std::endl;
    exit(1);
  }

  m_logFile += homeDir;
  std::string fileName = UtilAll::to_string(getpid()) + "_" + "rocketmq-cpp.log";
  m_logFile += fileName;

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

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

#include <iostream>  // std::cerr, std::endl

#if SPDLOG_VER_MAJOR >= 1
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#else
#include <spdlog/async_logger.h>
#endif

#include "UtilAll.h"

namespace rocketmq {

LogAdapter* LogAdapter::getLogInstance() {
  static LogAdapter singleton_;
  return &singleton_;
}

LogAdapter::LogAdapter() : log_level_(LOG_LEVEL_INFO) {
  std::string log_dir;
  const char* dir = std::getenv(ROCKETMQ_CPP_LOG_DIR_ENV.c_str());
  if (dir != nullptr && dir[0] != '\0') {
    // FIXME: replace '~' by home directory.
    log_dir = dir;
  } else {
    log_dir = UtilAll::getHomeDirectory();
    log_dir.append("/logs/rocketmq-cpp/");
  }
  if (log_dir[log_dir.size() - 1] != '/') {
    log_dir.append("/");
  }

  if (!UtilAll::existDirectory(log_dir)) {
    UtilAll::createDirectory(log_dir);
  }
  if (!UtilAll::existDirectory(log_dir)) {
    std::cerr << "create log dir error, will exit" << std::endl;
    exit(1);
  }

  std::string fileName = UtilAll::to_string(UtilAll::getProcessId()) + "_" + "rocketmq-cpp.log";
  log_file_ = log_dir + fileName;

#if SPDLOG_VER_MAJOR >= 1
  spdlog::init_thread_pool(8192, 1);

  auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_file_, 1024 * 1024 * 100, 3);
  rotating_sink->set_level(spdlog::level::debug);
  rotating_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread@%t] - %v");
  log_sinks_.push_back(rotating_sink);

  logger_ = std::make_shared<spdlog::async_logger>("default", log_sinks_.begin(), log_sinks_.end(),
                                                   spdlog::thread_pool(), spdlog::async_overflow_policy::block);

  // register it if you need to access it globally
  spdlog::register_logger(logger_);

  // when an error occurred, flush disk immediately
  logger_->flush_on(spdlog::level::err);

  spdlog::flush_every(std::chrono::seconds(3));
#else
  size_t q_size = 4096;
  spdlog::set_async_mode(q_size);
  logger_ = spdlog::rotating_logger_mt("default", log_file_, 1024 * 1024 * 100, 3);
  logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread@%t] - %v");
#endif

  setLogLevelInner(log_level_);
}

LogAdapter::~LogAdapter() {
  spdlog::drop("default");
}

void LogAdapter::setLogLevelInner(LogLevel log_level) {
  switch (log_level) {
    case LOG_LEVEL_FATAL:
      logger_->set_level(spdlog::level::critical);
      break;
    case LOG_LEVEL_ERROR:
      logger_->set_level(spdlog::level::err);
      break;
    case LOG_LEVEL_WARN:
      logger_->set_level(spdlog::level::warn);
      break;
    case LOG_LEVEL_INFO:
      logger_->set_level(spdlog::level::info);
      break;
    case LOG_LEVEL_DEBUG:
      logger_->set_level(spdlog::level::debug);
      break;
    case LOG_LEVEL_TRACE:
      logger_->set_level(spdlog::level::trace);
      break;
    default:
      logger_->set_level(spdlog::level::info);
      break;
  }
}

void LogAdapter::setLogFileNumAndSize(int logNum, int sizeOfPerFile) {}

}  // namespace rocketmq

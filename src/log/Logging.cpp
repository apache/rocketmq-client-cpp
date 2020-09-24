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

Logger* Logger::getLoggerInstance() {
  static Logger singleton_("default");
  return &singleton_;
}

Logger::Logger(const std::string& name) : log_level_(LOG_LEVEL_INFO) {
  try {
    init_log_dir_();
    init_spdlog_env_();
    logger_ = create_rotating_logger_(name, log_file_, 1024 * 1024 * 100, 3);
    set_log_level_(log_level_);
  } catch (std::exception& e) {
    std::cerr << "initialite logger failed" << std::endl;
    exit(-1);
  }
}

Logger::~Logger() {
  if (logger_ != nullptr) {
    spdlog::drop(logger_->name());
  }
}

void Logger::init_log_dir_() {
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
  std::string log_file_name = UtilAll::to_string(UtilAll::getProcessId()) + "_" + "rocketmq-cpp.log";
  log_file_ = log_dir + log_file_name;
}

void Logger::init_spdlog_env_() {
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread@%t] - %v");
  // when an error occurred, flush disk immediately
  spdlog::flush_on(spdlog::level::err);
#if SPDLOG_VER_MAJOR >= 1
  spdlog::init_thread_pool(spdlog::details::default_async_q_size, 1);
  spdlog::flush_every(std::chrono::seconds(3));
#else
  spdlog::set_async_mode(8192, async_overflow_policy::block_retry, nullptr, std::chrono::milliseconds(3000), nullptr);
#endif
}

std::shared_ptr<spdlog::logger> Logger::create_rotating_logger_(const std::string& name,
                                                                const std::string& filepath,
                                                                std::size_t max_size,
                                                                std::size_t max_files) {
#if SPDLOG_VER_MAJOR >= 1
  return spdlog::create_async<spdlog::sinks::rotating_file_sink_mt>(name, filepath, max_size, max_files);
#else
  return spdlog::rotating_logger_mt(name, filepath, max_size, max_files);
#endif
}

void Logger::set_log_level_(LogLevel log_level) {
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

void Logger::setLogFileNumAndSize(int logNum, int sizeOfPerFile) {
  // FIXME: set after clients started
  auto name = logger_->name();
  spdlog::drop(name);
  logger_ = create_rotating_logger_(name, log_file_, sizeOfPerFile, logNum);
  set_log_level_(log_level_);
}

}  // namespace rocketmq

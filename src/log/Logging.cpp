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
#include <memory>
#include <mutex>

#if SPDLOG_VER_MAJOR >= 1
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#else
#include <spdlog/async_logger.h>
#endif

#include "UtilAll.h"

namespace rocketmq {

static void ConfigSpdlog() {
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread@%t] - %v [%@ (%!)]");
  // when an error occurred, flush disk immediately
  spdlog::flush_on(spdlog::level::err);
#if SPDLOG_VER_MAJOR >= 1
  spdlog::init_thread_pool(spdlog::details::default_async_q_size, 1);
  spdlog::flush_every(std::chrono::seconds(3));
#else
  spdlog::set_async_mode(8192, async_overflow_policy::block_retry, nullptr, std::chrono::milliseconds(3000), nullptr);
#endif
}

static std::shared_ptr<spdlog::logger> CreateRotatingLogger(const std::string& name,
                                                            const std::string& filepath,
                                                            std::size_t max_size,
                                                            std::size_t max_files) {
#if SPDLOG_VER_MAJOR >= 1
  return spdlog::create_async<spdlog::sinks::rotating_file_sink_mt>(name, filepath, max_size, max_files);
#else
  return spdlog::rotating_logger_mt(name, filepath, max_size, max_files);
#endif
}

static spdlog::level::level_enum ConvertLogLevel(LogLevel log_level) {
  int level = static_cast<int>(log_level);
  return static_cast<spdlog::level::level_enum>(6 - level);
}

static std::string GetDefaultLogDir() {
  std::string log_dir;
  const char* dir = std::getenv(ROCKETMQ_CPP_LOG_DIR_ENV.c_str());
  if (dir != nullptr && dir[0] != '\0') {
    // FIXME: replace '~' by home directory.
    log_dir = dir;
  } else {
    log_dir = UtilAll::getHomeDirectory();
    log_dir.append("/logs/rocketmq-cpp/");
  }
  if (log_dir[log_dir.size() - 1] != FILE_SEPARATOR) {
    log_dir += FILE_SEPARATOR;
  }
  std::string log_file_name = UtilAll::to_string(UtilAll::getProcessId()) + "_" + "rocketmq-cpp.log";
  return log_dir + log_file_name;
}

LoggerConfig& GetDefaultLoggerConfig() {
  static LoggerConfig default_logger_config("default", GetDefaultLogDir());
  return default_logger_config;
}

Logger& GetDefaultLogger() {
  static Logger default_logger = []() {
    auto& default_logger_config = GetDefaultLoggerConfig();
    if (default_logger_config.config_spdlog()) {
      ConfigSpdlog();
    }
    return Logger(default_logger_config);
  }();
  return default_logger;
}

Logger::Logger(const LoggerConfig& config) {
  try {
    logger_ = CreateRotatingLogger(config.name(), config.path(), config.file_size(), config.file_count());
    logger_->set_level(ConvertLogLevel(config.level()));
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

}  // namespace rocketmq

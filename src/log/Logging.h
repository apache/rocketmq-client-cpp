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
#include <utility>
#include <vector>  // std::vector

// clang-format off
#include <spdlog/spdlog.h>
#if !defined(SPDLOG_FMT_EXTERNAL)
#include <spdlog/fmt/bundled/printf.h>
#else
#include <fmt/printf.h>
#endif
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

class LoggerConfig {
 public:
  LoggerConfig(const std::string& name, const std::string& path)
      : LoggerConfig(name, LOG_LEVEL_INFO, path, 1024 * 1024 * 100, 3) {}
  LoggerConfig(const std::string& name, LogLevel level, const std::string& path, int file_size, int file_count)
      : name_(name), level_(level), path_(path), file_size_(file_size), file_count_(file_count) {}

 public:
  inline const std::string& name() const { return name_; }
  inline void set_name(const std::string& name) { name_ = name; }

  inline LogLevel level() const { return level_; }
  inline void set_level(LogLevel level) { level_ = level; }

  inline const std::string& path() const { return path_; }
  inline void set_path(const std::string& path) { path_ = path; }

  inline int file_size() const { return file_size_; }
  inline void set_file_size(int file_size) { file_size_ = file_size; }

  inline int file_count() const { return file_count_; }
  inline void set_file_count(int file_count) { file_count_ = file_count; }

  inline bool config_spdlog() const { return config_spdlog_; }
  inline void set_config_spdlog(bool config_spdlog) { config_spdlog_ = config_spdlog; }

 private:
  std::string name_;
  LogLevel level_;
  std::string path_;
  int file_size_;
  int file_count_;
  bool config_spdlog_{true};
};

class Logger {
 public:
  Logger(const LoggerConfig& config);

  virtual ~Logger();

  template <typename FormatString, typename... Args>
  inline void Log(spdlog::source_loc&& location,
                  spdlog::level::level_enum level,
                  FormatString&& format,
                  Args&&... args) {
    logger_->log(std::forward<spdlog::source_loc>(location), level, format, std::forward<Args>(args)...);
  }

  template <typename FormatString, typename... Args>
  inline void Trace(spdlog::source_loc&& location, FormatString&& format, Args&&... args) {
    Log(std::forward<spdlog::source_loc>(location), spdlog::level::trace, std::forward<FormatString>(format),
        std::forward<Args>(args)...);
  }

  template <typename FormatString, typename... Args>
  inline void Debug(spdlog::source_loc&& location, FormatString&& format, Args&&... args) {
    Log(std::forward<spdlog::source_loc>(location), spdlog::level::debug, std::forward<FormatString>(format),
        std::forward<Args>(args)...);
  }

  template <typename FormatString, typename... Args>
  inline void Info(spdlog::source_loc&& location, FormatString&& format, Args&&... args) {
    Log(std::forward<spdlog::source_loc>(location), spdlog::level::info, std::forward<FormatString>(format),
        std::forward<Args>(args)...);
  }

  template <typename FormatString, typename... Args>
  inline void Warn(spdlog::source_loc&& location, FormatString&& format, Args&&... args) {
    Log(std::forward<spdlog::source_loc>(location), spdlog::level::warn, std::forward<FormatString>(format),
        std::forward<Args>(args)...);
  }

  template <typename FormatString, typename... Args>
  inline void Error(spdlog::source_loc&& location, FormatString&& format, Args&&... args) {
    Log(std::forward<spdlog::source_loc>(location), spdlog::level::err, std::forward<FormatString>(format),
        std::forward<Args>(args)...);
  }

  template <typename FormatString, typename... Args>
  inline void Fatal(spdlog::source_loc&& location, FormatString&& format, Args&&... args) {
    Log(std::forward<spdlog::source_loc>(location), spdlog::level::critical, std::forward<FormatString>(format),
        std::forward<Args>(args)...);
  }

  template <typename FormatString, typename... Args>
  inline void Printf(spdlog::source_loc&& location,
                     spdlog::level::level_enum level,
                     FormatString&& format,
                     Args&&... args) {
    if (logger_->should_log(level)) {
      std::string message = fmt::sprintf(format, std::forward<Args>(args)...);
      logger_->log(std::forward<spdlog::source_loc>(location), level, message);
    }
  }

  template <typename FormatString, typename... Args>
  inline void TracePrintf(spdlog::source_loc&& location, FormatString&& format, Args&&... args) {
    Printf(std::forward<spdlog::source_loc>(location), spdlog::level::trace, std::forward<FormatString>(format),
           std::forward<Args>(args)...);
  }

  template <typename FormatString, typename... Args>
  inline void DebugPrintf(spdlog::source_loc&& location, FormatString&& format, Args&&... args) {
    Printf(std::forward<spdlog::source_loc>(location), spdlog::level::debug, std::forward<FormatString>(format),
           std::forward<Args>(args)...);
  }

  template <typename FormatString, typename... Args>
  inline void InfoPrintf(spdlog::source_loc&& location, FormatString&& format, Args&&... args) {
    Printf(std::forward<spdlog::source_loc>(location), spdlog::level::info, std::forward<FormatString>(format),
           std::forward<Args>(args)...);
  }

  template <typename FormatString, typename... Args>
  inline void WarnPrintf(spdlog::source_loc&& location, FormatString&& format, Args&&... args) {
    Printf(std::forward<spdlog::source_loc>(location), spdlog::level::warn, std::forward<FormatString>(format),
           std::forward<Args>(args)...);
  }

  template <typename FormatString, typename... Args>
  inline void ErrorPrintf(spdlog::source_loc&& location, FormatString&& format, Args&&... args) {
    Printf(std::forward<spdlog::source_loc>(location), spdlog::level::err, std::forward<FormatString>(format),
           std::forward<Args>(args)...);
  }

  template <typename FormatString, typename... Args>
  inline void FatalPrintf(spdlog::source_loc&& location, FormatString&& format, Args&&... args) {
    Printf(std::forward<spdlog::source_loc>(location), spdlog::level::critical, std::forward<FormatString>(format),
           std::forward<Args>(args)...);
  }

 private:
  std::shared_ptr<spdlog::logger> logger_;
};

LoggerConfig& GetDefaultLoggerConfig();
Logger& GetDefaultLogger();

#define LOG_SOURCE_LOCATION \
  spdlog::source_loc { __FILE__, __LINE__, SPDLOG_FUNCTION }

#define LOG_FATAL(...) GetDefaultLogger().FatalPrintf(LOG_SOURCE_LOCATION, __VA_ARGS__)
#define LOG_ERROR(...) GetDefaultLogger().ErrorPrintf(LOG_SOURCE_LOCATION, __VA_ARGS__)
#define LOG_WARN(...) GetDefaultLogger().WarnPrintf(LOG_SOURCE_LOCATION, __VA_ARGS__)
#define LOG_INFO(...) GetDefaultLogger().InfoPrintf(LOG_SOURCE_LOCATION, __VA_ARGS__)
#define LOG_DEBUG(...) GetDefaultLogger().DebugPrintf(LOG_SOURCE_LOCATION, __VA_ARGS__)

#define LOG_FATAL_NEW(...) GetDefaultLogger().Fatal(LOG_SOURCE_LOCATION, __VA_ARGS__)
#define LOG_ERROR_NEW(...) GetDefaultLogger().Error(LOG_SOURCE_LOCATION, __VA_ARGS__)
#define LOG_WARN_NEW(...) GetDefaultLogger().Warn(LOG_SOURCE_LOCATION, __VA_ARGS__)
#define LOG_INFO_NEW(...) GetDefaultLogger().Info(LOG_SOURCE_LOCATION, __VA_ARGS__)
#define LOG_DEBUG_NEW(...) GetDefaultLogger().Debug(LOG_SOURCE_LOCATION, __VA_ARGS__)

}  // namespace rocketmq

#endif  // ROCKETMQ_LOG_LOGGING_H_

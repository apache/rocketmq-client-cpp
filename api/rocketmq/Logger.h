#pragma once

#include <cstddef>
#include <memory>
#include <thread>
#include <mutex>
#include <string>
#include <cstdint>

#include "RocketMQ.h"

#ifndef SPDLOG_ACTIVE_LEVEL
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#endif

ROCKETMQ_NAMESPACE_BEGIN

enum class Level : uint8_t { Trace = 0, Debug = 1, Info = 2, Warn = 3, Error = 4 };

class Logger {
public:
  virtual ~Logger() = default;

  /**
   * @brief Set log level for file sink. Its default log level is: Info.
   * 
   * @param level 
   */
  virtual void setLevel(Level level) = 0;

  /**
   * @brief Set log level for stdout, aka, console. Its default log level is: Warn.
   * 
   * @param level 
   */
  virtual void setConsoleLevel(Level level) = 0;

  virtual void setFileSize(std::size_t file_size) = 0;

  virtual void setFileCount(std::size_t file_count) = 0;

  virtual void setPattern(std::string pattern) = 0;

  virtual void init() = 0;
};

Logger& getLogger();

ROCKETMQ_NAMESPACE_END
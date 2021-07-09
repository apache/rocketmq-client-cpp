#include "rocketmq/Logger.h"
#include <cstdlib>
#include <iostream>

#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

// Check C++17 is used
#if defined(__cplusplus) && __cplusplus >= 201703L && defined(__has_include)
#if __has_include(<filesystem>)
#define GHC_USE_STD_FS
#include <filesystem>
namespace fs = std::filesystem;
#endif
#endif

#ifndef GHC_USE_STD_FS
#include "ghc/filesystem.hpp"
namespace fs = ghc::filesystem;
#endif

ROCKETMQ_NAMESPACE_BEGIN

Logger::Logger() : level_(Level::Info), file_size_(1048576 * 256), file_count_(16) {}

Logger::~Logger() {}

void Logger::setLevel(Level level) { level_ = level; }

void Logger::init() {
  std::call_once(init_once_, [this] { init0(); });
}

void Logger::init0() {
  if (log_home_.empty()) {
    char* home = getenv(USER_HOME_ENV);
    log_home_.append(home).append("/logs/rocketmq/client.log");
  }

  // Create directories if necessary
  fs::path log_file(log_home_);
  fs::path log_dir = log_file.parent_path();
  if (!fs::exists(log_dir)) {
    if (!fs::create_directories(log_dir)) {
      std::cerr << "Failed to mkdir " << log_dir << std::endl;
      abort();
    }
  }

  if (pattern_.empty()) {
    pattern_ = DEFAULT_PATTERN;
  }

  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::info);
  console_sink->set_pattern(pattern_);
  auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_home_, file_size_, file_count_);
  file_sink->set_pattern(pattern_);

  switch (level_) {
  case Trace:
    file_sink->set_level(spdlog::level::trace);
    break;
  case Debug:
    file_sink->set_level(spdlog::level::debug);
    break;
  case Info:
    file_sink->set_level(spdlog::level::info);
    break;
  case Warn:
    file_sink->set_level(spdlog::level::warn);
    break;
  case Error:
    file_sink->set_level(spdlog::level::err);
    break;
  default:
    file_sink->set_level(spdlog::level::critical);
    break;
  }

  // std::make_shared does not with initializer_list.
  auto default_logger =
      std::make_shared<spdlog::logger>("rocketmq_logger", spdlog::sinks_init_list{console_sink, file_sink});

  default_logger->set_level(spdlog::level::debug);
  spdlog::flush_on(spdlog::level::info);
  spdlog::flush_every(std::chrono::seconds(1));
  spdlog::set_default_logger(default_logger);
}

Logger& getLogger() {
  static Logger logger;
  return logger;
}

const char* Logger::USER_HOME_ENV = "HOME";
const char* Logger::DEFAULT_PATTERN = "[%Y/%m/%d-%H:%M:%S.%e %z] [%n] [%^---%L---%$] [thread %t] %v %@";

ROCKETMQ_NAMESPACE_END
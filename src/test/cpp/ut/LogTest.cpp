#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include "gtest/gtest.h"
#include <iostream>

TEST(SpdlogTest, testLog) {
  std::cout << "It works" << std::endl;
  std::cout << "It works" << std::endl;
  spdlog::set_pattern("[%H:%M:%S %z] [%^%L%$] [thread %t] %v");
  spdlog::set_level(spdlog::level::trace);
  spdlog::info("Welcome to the world of spdlog");
  spdlog::info("Code assistant is working");
  spdlog::info("It works in Visual Studio Code");

  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(spdlog::level::trace);
  console_sink->set_pattern("[%H:%M:%S %z] [%^%L%$] [thread %t] %v");

  auto rotating_file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/log.txt", 65536, 3, true);

  spdlog::logger logger("multi-sink-logger", {console_sink, rotating_file_sink});
  logger.set_level(spdlog::level::debug);
  logger.info("This log is expected present in both console and file");

  spdlog::log(spdlog::source_loc(__FILE__, __LINE__, "testLog"), spdlog::level::debug, "Hello {}", "world");

  SPDLOG_INFO("Hello, macro {}, {}:{}", "world", __FILE__, __LINE__);
}

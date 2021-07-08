#include <chrono>
#include <iostream>
#include <vector>

#include "ThreadPool.h"
#include "spdlog/spdlog.h"
#include "gtest/gtest.h"

TEST(ThreadPoolTest, testLambda) {
  ThreadPool pool(4);
  std::vector<std::future<int>> results;
  auto task = [](int i) {
    { spdlog::info("hello {}", i); }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    { spdlog::info("world {}", i); }
    return i * i;
  };

  for (int i = 0; i < 8; ++i) {
    results.emplace_back(pool.enqueue(task, i));
  }

  for (auto&& result : results) {
    { spdlog::info("result {}", result.get()); }
  }
}

int task(int input) {
  { spdlog::info("hello {}", input); }
  std::this_thread::sleep_for(std::chrono::seconds(1));
  { spdlog::info("world {}", input); }
  return input * input;
}

TEST(ThreadPoolTest, testFuntion) {

  ThreadPool pool(4);
  std::vector<std::future<int>> results;

  for (int i = 0; i < 8; ++i) {
    results.emplace_back(pool.enqueue(&task, i));
  }

  for (auto&& result : results) {
    { spdlog::info("result {}", result.get()); }
  }
}

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
#include "rocketmq/RocketMQ.h"
#include "rocketmq/State.h"
#include "gtest/gtest.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

ROCKETMQ_NAMESPACE_BEGIN
class ExecutorImpl {
public:
  explicit ExecutorImpl(unsigned int core) : state_(State::CREATED), core_(core) {
    if (core_ <= 0) {
      core_ = std::thread::hardware_concurrency();
    }
  }

  virtual ~ExecutorImpl() {
    switch (state_.load(std::memory_order_relaxed)) {
      case CREATED:
      case STOPPING:
      case STOPPED:
        break;

      case STARTING:
      case STARTED:
        state_.store(State::STOPPED);
        for (auto& worker : workers_) {
          if (worker.joinable()) {
            worker.join();
          }
        }
        break;
    }
  }

  void submit(const std::function<void(void)>& task) {
    if (State::STOPPED == state_.load(std::memory_order_relaxed)) {
      return;
    }

    {
      std::unique_lock<std::mutex> lock(task_mtx_);
      tasks_.push_back(task);
    }
    cv_.notify_one();
  }

  void start() {
    State expected = State::CREATED;
    if (state_.compare_exchange_strong(expected, State::STARTING)) {
      for (unsigned int i = 0; i < core_; i++) {
        workers_.emplace_back(std::bind(&ExecutorImpl::loop, this));
      }
      state_.store(State::STARTED);
    }
  }

  void stop() {
    state_.store(State::STOPPED);
    for (auto& worker : workers_) {
      if (worker.joinable()) {
        worker.join();
      }
    }
  }

private:
  void loop() {
    while (state_.load(std::memory_order_relaxed) != State::STOPPED) {
      std::function<void(void)> func;
      {
        std::unique_lock<std::mutex> lk(task_mtx_);
        if (!tasks_.empty()) {
          func = tasks_.back();
        }
      }

      if (func) {
        func();
      } else {
        std::unique_lock<std::mutex> lk(task_mtx_);
        cv_.wait_for(lk, std::chrono::seconds(3),
                     [&]() { return state_.load(std::memory_order_relaxed) == State::STOPPED || !tasks_.empty(); });
      }
    }

    std::stringstream ss;
    ss << "ThreadId=" << std::this_thread::get_id() << " quit.";
    printf("%s\n", ss.str().c_str());
  }

  std::atomic<State> state_;
  std::vector<std::function<void(void)>> tasks_;
  std::mutex task_mtx_;
  std::condition_variable cv_;
  unsigned int core_;
  std::vector<std::thread> workers_;
};

class ExecutorTest : public ::testing::Test {
public:
  void SetUp() override {
    executor_.start();
  }

  void TearDown() override {
    executor_.stop();
  }

protected:
  ExecutorImpl executor_{0};
};

TEST_F(ExecutorTest, testBasic) {
}

std::mutex submit_mtx_;
std::condition_variable submit_cv_;
std::atomic_bool submit_status(false);

void f(int n) {
  std::this_thread::sleep_for(std::chrono::milliseconds(n));
  submit_status.store(true, std::memory_order_relaxed);
  submit_cv_.notify_all();
}

TEST_F(ExecutorTest, testSubmit) {
  auto task = std::bind(f, 100);
  executor_.submit(task);
  {
    std::unique_lock<std::mutex> lock(submit_mtx_);
    submit_cv_.wait(lock, [] { return submit_status.load(std::memory_order_relaxed); });
  }
  EXPECT_EQ(true, submit_status.load());
}

ROCKETMQ_NAMESPACE_END
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
#ifndef ROCKETMQ_CONCURRENT_THREADGROUP_HPP_
#define ROCKETMQ_CONCURRENT_THREADGROUP_HPP_

#include "thread.hpp"

namespace rocketmq {

class thread_group {
 public:
  // Constructor initialises an empty thread group.
  thread_group() : first_(nullptr) {}
  thread_group(const std::string& name) : name_(name), first_(nullptr) {}

  template <typename Function>
  thread_group(const std::string& name, Function f, std::size_t thread_nums) : name_(name), first_(nullptr) {
    create_threads(f, thread_nums);
  }

  // Destructor joins any remaining threads in the group.
  ~thread_group() { join(); }

  // Create a new thread in the group.
  template <typename Function>
  void create_thread(Function f) {
    first_ = new item(name_, f, first_);
  }

  // Create new threads in the group.
  template <typename Function>
  void create_threads(Function f, std::size_t thread_nums) {
    for (std::size_t i = 0; i < thread_nums; ++i) {
      create_thread(f);
    }
  }

  void start() {
    auto* it = first_;
    while (it != nullptr) {
      it->start();
      it = it->next_;
    }
  }

  // Wait for all threads in the group to exit.
  void join() {
    while (first_) {
      first_->thread_.join();
      auto* tmp = first_;
      first_ = first_->next_;
      delete tmp;
    }
  }

 private:
  // Structure used to track a single thread in the group.
  struct item {
    template <typename Function>
    explicit item(const std::string& name, Function f, item* next) : thread_(name), next_(next) {
      thread_.set_target(f);
    }

    void start() { thread_.start(); }

    thread thread_;
    item* next_;
  };

 private:
  std::string name_;

  // The first thread in the group.
  item* first_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_CONCURRENT_THREADGROUP_HPP_

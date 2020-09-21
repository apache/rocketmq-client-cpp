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
#ifndef ROCKETMQ_CONCURRENT_CONCURRENTQUEUE_HPP_
#define ROCKETMQ_CONCURRENT_CONCURRENTQUEUE_HPP_

#include <atomic>
#include <memory>
#include <thread>
#include <type_traits>
#include <utility>

namespace rocketmq {

template <typename T>
class concurrent_queue;

template <typename T>
class concurrent_queue_node {
 public:
  // types:
  typedef T value_type;
  typedef concurrent_queue_node<value_type> type;

 private:
  template <typename E,
            typename std::enable_if<std::is_same<typename std::decay<E>::type, value_type>::value, int>::type = 0>
  explicit concurrent_queue_node(E&& v) : value_(new value_type(std::forward<E>(v))) {}

  // for pointer
  template <class E, typename std::enable_if<std::is_convertible<E, value_type*>::value, int>::type = 0>
  explicit concurrent_queue_node(E v) : value_(v) {}

  value_type* value_;
  std::atomic<type*> next_;

  friend concurrent_queue<value_type>;
};

template <typename T>
class concurrent_queue {
 public:
  // types:
  typedef T value_type;
  typedef concurrent_queue_node<value_type> node_type;

  virtual ~concurrent_queue() {
    // clear this queue
    while (_clear_when_destruct) {
      if (nullptr == pop_front()) {
        break;
      }
    }

    delete[](char*) sentinel;
  }

  concurrent_queue(bool clear_when_destruct = true)
      : sentinel((node_type*)new char[sizeof(node_type)]), size_(0), _clear_when_destruct(clear_when_destruct) {
    sentinel->next_.store(sentinel);
    head_ = tail_ = sentinel;
  }

  bool empty() { return sentinel == tail_.load(); }

  size_t size() { return size_.load(); }

  template <class E>
  void push_back(E&& v) {
    auto* node = new node_type(std::forward<E>(v));
    push_back_impl(node);
    size_.fetch_add(1);
  }

  std::unique_ptr<value_type> pop_front() {
    node_type* node = pop_front_impl();
    if (node == sentinel) {
      return std::unique_ptr<value_type>();
    } else {
      size_.fetch_sub(1);
      auto val = node->value_;
      delete node;
      return std::unique_ptr<value_type>(val);
    }
  }

 private:
  void push_back_impl(node_type* node) noexcept {
    node->next_.store(sentinel);
    auto tail = tail_.exchange(node);
    if (tail == sentinel) {
      head_.store(node);
    } else {
      // guarantee: tail is not released
      tail->next_.store(node);
    }
  }

  node_type* pop_front_impl() noexcept {
    auto head = head_.load();
    for (size_t i = 1;; i++) {
      if (head == sentinel) {
        // no task, or it is/are not ready
        return sentinel;
      }
      if (head != nullptr) {
        if (head_.compare_exchange_weak(head, nullptr)) {
          auto next = head->next_.load();
          if (next == sentinel) {
            auto t = head;
            // only one element
            if (tail_.compare_exchange_strong(t, sentinel)) {
              t = nullptr;
              head_.compare_exchange_strong(t, sentinel);
              return head;
            }
            size_t j = 0;
            do {
              // push-pop conflict, spin
              if (0 == ++j % 10) {
                std::this_thread::yield();
              }
              next = head->next_.load();
            } while (next == sentinel);
          }
          head_.store(next);
          return head;
        }
      } else {
        head = head_.load();
      }
      if (0 == i % 15 && head != sentinel) {
        std::this_thread::yield();
        head = head_.load();
      }
    }
  }

  std::atomic<node_type *> head_, tail_;
  node_type* const sentinel;
  std::atomic<size_t> size_;
  bool _clear_when_destruct;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_CONCURRENT_CONCURRENTQUEUE_HPP_

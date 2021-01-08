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
#ifndef ROCKETMQ_ARRAY_H_
#define ROCKETMQ_ARRAY_H_

#include <cstdlib>  // std::calloc, std::free
#include <cstring>  // std::memcpy

#include <new>          // std::bad_alloc
#include <type_traits>  // std::enable_if, std::is_arithmetic, std::is_pointer, std::is_class

#include "RocketMQClient.h"

namespace rocketmq {

template <typename T, typename Enable = void>
struct array_traits {};

template <typename T>
struct array_traits<T, typename std::enable_if<std::is_arithmetic<T>::value || std::is_pointer<T>::value>::type> {
  typedef T element_type;
};  // specialization for arithmetic and pointer types

template <typename T>
struct array_traits<T, typename std::enable_if<std::is_class<T>::value>::type> {
  typedef T* element_type;
};  // specialization for class types

template <typename T>
using array_element_t = typename array_traits<T>::element_type;

template <typename T>
class ROCKETMQCLIENT_API Array {
 public:
  using element_type = array_element_t<T>;

 public:
  Array(element_type* array, size_t size, bool auto_clean = false)
      : array_(array), size_(size), auto_clean_(auto_clean) {}
  Array(size_t size) : Array((element_type*)std::calloc(size, sizeof(element_type)), size, true) {
    if (nullptr == array_) {
      throw std::bad_alloc();
    }
  }

  Array(const Array& other) : Array(other.size_) { std::memcpy(array_, other.array_, size_); }
  Array(Array&& other) : Array(other.array_, other.size_, other.auto_clean_) { other.auto_clean_ = false; }

  virtual ~Array() {
    if (auto_clean_) {
      std::free(array_);
    }
  }

  element_type& operator[](size_t index) { return array_[index]; }
  const element_type& operator[](size_t index) const { return array_[index]; }

 public:
  inline T* array() { return array_; }
  inline const T* array() const { return array_; }
  inline size_t size() const { return size_; }

 protected:
  element_type* array_;
  size_t size_;
  bool auto_clean_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_ARRAY_H_

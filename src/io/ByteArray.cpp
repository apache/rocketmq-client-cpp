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
#include "ByteArray.h"

#include <cstring>  // std::memcpy

#include <algorithm>  // std::move
#include <stdexcept>  //
#include <string>     // std::string

namespace rocketmq {

class ByteArrayView : public ByteArray {
 public:
  ByteArrayView(ByteArrayRef ba, size_t offset, size_t size) : ByteArray(ba->array() + offset, size), origin_(ba) {}
  ByteArrayView(ByteArrayRef ba, size_t offset) : ByteArrayView(ba, offset, ba->size() - offset) {}

  ByteArrayRef origin() { return origin_; }

 private:
  ByteArrayRef origin_;
};

ByteArrayRef slice(ByteArrayRef ba, size_t offset) {
  return slice(ba, offset, ba->size() - offset);
}

ByteArrayRef slice(ByteArrayRef ba, size_t offset, size_t size) {
  if (offset < 0 || size < 0 || ba->size() < offset + size) {
    std::invalid_argument("");
  }
  return std::make_shared<ByteArrayView>(ba, offset, size);
}

class StringBaseByteArray : public ByteArray {
 public:
  StringBaseByteArray(const std::string& str) : ByteArray(nullptr, str.size()), string_(str) {
    // Since C++11, The returned array is null-terminated, that is, data() and c_str() perform the same function.
    // If empty() returns true, the pointer points to a single null character.
    array_ = (char*)string_.data();
  }
  StringBaseByteArray(std::string&& str) : ByteArray(nullptr, str.size()), string_(std::move(str)) {
    array_ = (char*)string_.data();
  }

  std::string sting() { return string_; }

 private:
  std::string string_;
};

ByteArrayRef stoba(const std::string& str) {
  return std::make_shared<StringBaseByteArray>(str);
}

ByteArrayRef stoba(std::string&& str) {
  return std::make_shared<StringBaseByteArray>(std::move(str));
}

ByteArrayRef catoba(const char* str, size_t len) {
  return stoba(std::string(str, len));
}

std::string batos(ByteArrayRef ba) {
  return std::string(ba->array(), ba->size());
}

}  // namespace rocketmq

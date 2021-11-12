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
#pragma once

#include <cstdint>
#include <type_traits>

#include "absl/strings/numbers.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/struct.pb.h"
#include "google/protobuf/util/json_util.h"

#include "LoggerImpl.h"

ROCKETMQ_NAMESPACE_BEGIN

class CommandCustomHeader {
public:
  virtual ~CommandCustomHeader() = default;

  virtual void encode(google::protobuf::Value& root) const = 0;
};

template <typename T>
typename std::enable_if<std::is_integral<T>::value, bool>::type
assign(const google::protobuf::Map<std::string, google::protobuf::Value>& fields, const char* label, T* ptr) {
  if (fields.contains(label)) {
    auto& item = fields.at(label);
    if (item.has_number_value()) {
      *ptr = item.number_value();
      return true;
    } else if (item.has_string_value()) {
      if (!absl::SimpleAtoi(item.string_value(), ptr)) {
        SPDLOG_WARN("Failed to acquire integral value for {}", label);
        return false;
      }
      return true;
    }
  }
  return false;
}

bool assign(const google::protobuf::Map<std::string, google::protobuf::Value>& fields, const char* label,
            std::string* ptr);

template <typename T>
typename std::enable_if<std::is_integral<T>::value>::type
addEntry(google::protobuf::Map<std::string, google::protobuf::Value>* fields, absl::string_view key, T value) {
  google::protobuf::Value item;
  item.set_number_value(value);
  fields->insert({std::string(key.data(), key.size()), item});
}

void addEntry(google::protobuf::Map<std::string, google::protobuf::Value>* fields, absl::string_view key,
              std::string value);

ROCKETMQ_NAMESPACE_END
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
#include "StaticNameServerResolver.h"

#include "absl/strings/str_split.h"
#include <atomic>
#include <cstdint>

ROCKETMQ_NAMESPACE_BEGIN

StaticNameServerResolver::StaticNameServerResolver(absl::string_view name_server_list)
    : name_server_list_(absl::StrSplit(name_server_list, ';')) {
}

std::string StaticNameServerResolver::current() {
  std::uint32_t index = index_.load(std::memory_order_relaxed) % name_server_list_.size();
  return name_server_list_[index];
}

std::string StaticNameServerResolver::next() {
  index_.fetch_add(1, std::memory_order_relaxed);
  return current();
}

std::vector<std::string> StaticNameServerResolver::resolve() {
  return name_server_list_;
}

ROCKETMQ_NAMESPACE_END
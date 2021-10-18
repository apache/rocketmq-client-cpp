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

#include <atomic>
#include <cstdint>
#include <vector>

#include "absl/strings/string_view.h"

#include "NameServerResolver.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class StaticNameServerResolver : public NameServerResolver {
public:
  explicit StaticNameServerResolver(absl::string_view name_server_list);

  void start() override {
  }

  void shutdown() override {
  }

  std::string current() override;

  std::string next() override;

  std::vector<std::string> resolve() override;

private:
  std::vector<std::string> name_server_list_;
  std::atomic<std::uint32_t> index_{0};
};

ROCKETMQ_NAMESPACE_END
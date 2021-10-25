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

#include <chrono>
#include <string>
#include <vector>

#include "absl/time/clock.h"
#include "absl/time/time.h"

#include "DigestType.h"
#include "Encoding.h"
#include "rocketmq/MessageType.h"

ROCKETMQ_NAMESPACE_BEGIN

class Protocol {
public:
  static const char* PROTOCOL_VERSION;
};

struct Digest {
  DigestType digest_type{DigestType::MD5};
  std::string checksum;
  Digest() = default;
};

struct Resource {
  /**
   * Abstract resource namespace
   */
  std::string resource_namespace;

  /**
   * Resource name, which remains unique within given abstract resource namespace.
   */
  std::string name;
};

struct SystemAttribute {
  std::string tag;
  std::vector<std::string> keys;
  std::string message_id;
  Digest digest;
  Encoding body_encoding;
  MessageType message_type;
  absl::Time born_timestamp{absl::Now()};
  std::string born_host;
  absl::Time store_timestamp{absl::UnixEpoch()};
  std::string store_host;
  absl::Time delivery_timestamp{absl::UnixEpoch()};
  absl::Time decode_timestamp{absl::Now()};
  int32_t delay_level{0};
  std::string receipt_handle;
  int32_t partition_id{0};
  int64_t partition_offset{0};
  absl::Duration invisible_period;
  int32_t attempt_times{0};
  Resource publisher_group;
  std::string trace_context;
  std::string target_endpoint;
  std::string message_group;
};

ROCKETMQ_NAMESPACE_END

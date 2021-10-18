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
#include <cstdlib>
#include <functional>

#include "MQMessageQueue.h"

ROCKETMQ_NAMESPACE_BEGIN

enum ConsumeType
{
  CONSUME_ACTIVELY,
  CONSUME_PASSIVELY,
};

enum ConsumeFromWhere
{
  CONSUME_FROM_LAST_OFFSET,
  CONSUME_FROM_LAST_OFFSET_AND_FROM_MIN_WHEN_BOOT_FIRST,
  CONSUME_FROM_MIN_OFFSET,
  CONSUME_FROM_MAX_OFFSET,
  CONSUME_FROM_FIRST_OFFSET,
  CONSUME_FROM_TIMESTAMP,
};

enum ConsumeInitialMode
{
  MIN,
  MAX,
};

enum QueryOffsetPolicy : uint8_t
{
  BEGINNING = 0,
  END = 1,
  TIME_POINT = 2,
};

struct OffsetQuery {
  MQMessageQueue message_queue;
  QueryOffsetPolicy policy;
  std::chrono::system_clock::time_point time_point;
};

struct PullMessageQuery {
  MQMessageQueue message_queue;
  int64_t offset;
  int32_t batch_size;
  std::chrono::system_clock::duration await_time;
  std::chrono::system_clock::duration timeout;
};

ROCKETMQ_NAMESPACE_END
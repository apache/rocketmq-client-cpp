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

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

enum class ResponseCode : std::uint16_t {
  Success = 0,
  InternalSystemError = 1,
  TooManyRequests = 2,
  NotSupported = 3,
  FlushDiskTimeout = 10,
  SlaveNotAvailable = 11,
  FlushSlaveTimeout = 12,
  MessageIllegal = 13,
  ServiceNotAvailable = 14,
  VersionNotSupported = 15,
  NoPermission = 16,
  TopicNotFound = 17,
  PullNotFound = 19,
  PullRetryImmediately = 20,
  PullOffsetMoved = 21,
  SubscriptionFormatError = 23,
  SubscriptionAbsent = 24,
  SubscriptionNotLatest = 25,
  TooManyReceiveRequests = 209,
  ReceiveMessageTimeout = 210,
};

ROCKETMQ_NAMESPACE_END
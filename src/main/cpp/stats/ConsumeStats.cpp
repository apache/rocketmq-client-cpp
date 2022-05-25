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

#include "ConsumeStats.h"

ROCKETMQ_NAMESPACE_BEGIN

ConsumeStats::ConsumeStats()
    : process_success_(
          opencensus::stats::MeasureInt64::Register("process_success", "Number of messages processed", "1")),
      process_failure_(opencensus::stats::MeasureInt64::Register(
          "process_failure", "Number of failures when processing messages", "1")),
      ack_success_(opencensus::stats::MeasureInt64::Register("ack_success", "Number of messages acknowledged", "1")),
      ack_failure_(opencensus::stats::MeasureInt64::Register(
          "ack_failure", "Number of failures when acknowledging messages", "1")),
      change_invisible_time_success_(opencensus::stats::MeasureInt64::Register(
          "change_invisible_time_success", "Number of change-invisible-time performed", "1")),
      change_invisible_time_failure_(opencensus::stats::MeasureInt64::Register(
          "change_invisible_time_failure", "Number of failures when changing message invisible time", "1")),
      cached_message_quantity_(opencensus::stats::MeasureInt64::Register(
          "cached_message_quantity", "Number of locally cached messages", "1")),
      cached_message_bytes_(opencensus::stats::MeasureInt64::Register(
          "cached_message_bytes", "Number of locally cached messages in bytes", "1")),
      delivery_latency_(opencensus::stats::MeasureInt64::Register(
          "delivery_latency", "Time spent delivering messages from servers to clients", "1")),
      await_time_(opencensus::stats::MeasureInt64::Register(
          "await_time", "Client side queuing time of messages before getting processed", "1")),
      process_time_(opencensus::stats::MeasureInt64::Register("process_time", "Process message time", "1")) {
}

ROCKETMQ_NAMESPACE_END
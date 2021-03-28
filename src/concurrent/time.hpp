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
#ifndef ROCKETMQ_CONCURRENT_TIME_HPP_
#define ROCKETMQ_CONCURRENT_TIME_HPP_

#include <cassert>

#include <chrono>

namespace rocketmq {

enum time_unit { nanoseconds, microseconds, milliseconds, seconds, minutes, hours };

inline std::chrono::steady_clock::time_point until_time_point(long delay, time_unit unit) {
  auto now = std::chrono::steady_clock::now();
  switch (unit) {
    case nanoseconds:
      return now + std::chrono::nanoseconds(delay);
    case microseconds:
      return now + std::chrono::microseconds(delay);
    case milliseconds:
      return now + std::chrono::milliseconds(delay);
    case seconds:
      return now + std::chrono::seconds(delay);
    case minutes:
      return now + std::chrono::minutes(delay);
    case hours:
      return now + std::chrono::hours(delay);
    default:
      break;
  }
  assert(false);
  return now;
}

}  // namespace rocketmq

#endif  // ROCKETMQ_CONCURRENT_TIME_HPP_

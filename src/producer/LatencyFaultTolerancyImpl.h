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
#ifndef ROCKETMQ_PRODUCER_LATENCYFAULTTOLERANCYIMPL_H_
#define ROCKETMQ_PRODUCER_LATENCYFAULTTOLERANCYIMPL_H_

#include <atomic>  // std::atomic
#include <map>     // std::map
#include <mutex>   // std::mutex
#include <string>  // std::string

namespace rocketmq {

class LatencyFaultTolerancyImpl {
 public:
  void updateFaultItem(const std::string& name, const long currentLatency, const long notAvailableDuration);

  bool isAvailable(const std::string& name);

  void remove(const std::string& name);

  std::string pickOneAtLeast();

 private:
  class FaultItem {
   public:
    FaultItem(const std::string& name);

    bool isAvailable() const;

    std::string toString() const;

   public:
    std::string name_;
    volatile long current_latency_;
    volatile int64_t start_timestamp_;
  };

  class ComparableFaultItem {
   public:
    ComparableFaultItem(const FaultItem& item)
        : name_(item.name_),
          is_available_(item.isAvailable()),
          current_latency_(item.current_latency_),
          start_timestamp_(item.start_timestamp_) {}

    bool operator<(const ComparableFaultItem& other) const;

   public:
    std::string name_;
    bool is_available_;
    long current_latency_;
    int64_t start_timestamp_;
  };

 private:
  // brokerName -> FaultItem
  std::map<std::string, FaultItem> fault_item_table_;
  std::mutex fault_item_table_mutex_;

  std::atomic<int> which_item_worst_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_PRODUCER_LATENCYFAULTTOLERANCYIMPL_H_

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
#ifndef __LATENCY_FAULT_TOLERANCY_IMPL__
#define __LATENCY_FAULT_TOLERANCY_IMPL__

#include <algorithm>
#include <atomic>
#include <map>
#include <mutex>
#include <sstream>
#include <string>

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

    std::string toString() const {
      std::stringstream ss;
      ss << "FaultItem{"
         << "name='" << m_name << "'"
         << ", currentLatency=" << m_currentLatency << ", startTimestamp=" << m_startTimestamp << "}";
      return ss.str();
    }

   public:
    std::string m_name;
    volatile long m_currentLatency;
    volatile int64_t m_startTimestamp;
  };

  class ComparableFaultItem {
   public:
    ComparableFaultItem(const FaultItem& item)
        : m_name(item.m_name),
          m_isAvailable(item.isAvailable()),
          m_currentLatency(item.m_currentLatency),
          m_startTimestamp(item.m_startTimestamp) {}

    bool operator<(const ComparableFaultItem& other) const;

   public:
    std::string m_name;
    bool m_isAvailable;
    long m_currentLatency;
    int64_t m_startTimestamp;
  };

 private:
  // brokerName -> FaultItem
  std::map<std::string, FaultItem> m_faultItemTable;
  std::mutex m_faultItemTableMutex;

  std::atomic<int> m_whichItemWorst;
};

}  // namespace rocketmq

#endif  // __LATENCY_FAULT_TOLERANCY_IMPL__

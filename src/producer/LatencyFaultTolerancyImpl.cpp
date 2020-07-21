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
#include "LatencyFaultTolerancyImpl.h"

#include <algorithm>  // std::sort
#include <sstream>    // std::stringstream
#include <vector>     // std::vector

#include "UtilAll.h"

namespace rocketmq {

void LatencyFaultTolerancyImpl::updateFaultItem(const std::string& name,
                                                const long currentLatency,
                                                const long notAvailableDuration) {
  std::lock_guard<std::mutex> lock(fault_item_table_mutex_);
  auto it = fault_item_table_.find(name);
  if (it == fault_item_table_.end()) {
    auto pair = fault_item_table_.emplace(name, name);
    it = pair.first;
  }
  auto& faultItem = it->second;
  faultItem.current_latency_ = currentLatency;
  faultItem.start_timestamp_ = UtilAll::currentTimeMillis() + notAvailableDuration;
}

bool LatencyFaultTolerancyImpl::isAvailable(const std::string& name) {
  std::lock_guard<std::mutex> lock(fault_item_table_mutex_);
  const auto& it = fault_item_table_.find(name);
  if (it != fault_item_table_.end()) {
    return it->second.isAvailable();
  }
  return true;
}

void LatencyFaultTolerancyImpl::remove(const std::string& name) {
  std::lock_guard<std::mutex> lock(fault_item_table_mutex_);
  fault_item_table_.erase(name);
}

std::string LatencyFaultTolerancyImpl::pickOneAtLeast() {
  std::lock_guard<std::mutex> lock(fault_item_table_mutex_);
  if (fault_item_table_.empty()) {
    return null;
  }

  if (fault_item_table_.size() == 1) {
    return fault_item_table_.begin()->second.name_;
  }

  std::vector<ComparableFaultItem> tmpList;
  tmpList.reserve(fault_item_table_.size());
  for (const auto& it : fault_item_table_) {
    tmpList.push_back(ComparableFaultItem(it.second));
  }

  std::sort(tmpList.begin(), tmpList.end());

  auto half = tmpList.size() / 2;
  auto i = which_item_worst_.fetch_add(1) % half;
  return tmpList[i].name_;
}

LatencyFaultTolerancyImpl::FaultItem::FaultItem(const std::string& name) : name_(name) {}

bool LatencyFaultTolerancyImpl::FaultItem::isAvailable() const {
  return UtilAll::currentTimeMillis() - start_timestamp_ >= 0;
}

std::string LatencyFaultTolerancyImpl::FaultItem::toString() const {
  std::stringstream ss;
  ss << "FaultItem{"
     << "name='" << name_ << "'"
     << ", currentLatency=" << current_latency_ << ", startTimestamp=" << start_timestamp_ << "}";
  return ss.str();
}

bool LatencyFaultTolerancyImpl::ComparableFaultItem::operator<(const ComparableFaultItem& other) const {
  if (is_available_ != other.is_available_) {
    return is_available_;
  }

  if (current_latency_ != other.current_latency_) {
    return current_latency_ < other.current_latency_;
  }

  return start_timestamp_ < other.start_timestamp_;
}

}  // namespace rocketmq

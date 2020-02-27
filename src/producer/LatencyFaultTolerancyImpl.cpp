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

#include <vector>

#include "UtilAll.h"

namespace rocketmq {

void LatencyFaultTolerancyImpl::updateFaultItem(const std::string& name,
                                                const long currentLatency,
                                                const long notAvailableDuration) {
  std::lock_guard<std::mutex> lock(m_faultItemTableMutex);
  auto it = m_faultItemTable.find(name);
  if (it == m_faultItemTable.end()) {
    auto pair = m_faultItemTable.emplace(name, name);
    it = pair.first;
  }
  auto& faultItem = it->second;
  faultItem.m_currentLatency = currentLatency;
  faultItem.m_startTimestamp = UtilAll::currentTimeMillis() + notAvailableDuration;
}

bool LatencyFaultTolerancyImpl::isAvailable(const std::string& name) {
  std::lock_guard<std::mutex> lock(m_faultItemTableMutex);
  auto it = m_faultItemTable.find(name);
  if (it != m_faultItemTable.end()) {
    return it->second.isAvailable();
  }
  return true;
}

void LatencyFaultTolerancyImpl::remove(const std::string& name) {
  std::lock_guard<std::mutex> lock(m_faultItemTableMutex);
  m_faultItemTable.erase(name);
}

std::string LatencyFaultTolerancyImpl::pickOneAtLeast() {
  std::lock_guard<std::mutex> lock(m_faultItemTableMutex);
  if (m_faultItemTable.empty()) {
    return null;
  }

  if (m_faultItemTable.size() == 1) {
    return m_faultItemTable.begin()->second.m_name;
  }

  std::vector<ComparableFaultItem> tmpList;
  tmpList.reserve(m_faultItemTable.size());
  for (const auto& it : m_faultItemTable) {
    tmpList.push_back(ComparableFaultItem(it.second));
  }

  std::sort(tmpList.begin(), tmpList.end());

  auto half = tmpList.size() / 2;
  auto i = m_whichItemWorst.fetch_add(1) % half;
  return tmpList[i].m_name;
}

LatencyFaultTolerancyImpl::FaultItem::FaultItem(const std::string& name) : m_name(name) {}

bool LatencyFaultTolerancyImpl::FaultItem::isAvailable() const {
  return UtilAll::currentTimeMillis() - m_startTimestamp >= 0;
}

bool LatencyFaultTolerancyImpl::ComparableFaultItem::operator<(const ComparableFaultItem& other) const {
  if (m_isAvailable != other.m_isAvailable) {
    return m_isAvailable;
  }

  if (m_currentLatency != other.m_currentLatency) {
    return m_currentLatency < other.m_currentLatency;
  }

  return m_startTimestamp < other.m_startTimestamp;
}

}  // namespace rocketmq

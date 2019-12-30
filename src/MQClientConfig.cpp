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
#include "MQClientConfig.h"

#include <algorithm>
#include <thread>

#include "NameSpaceUtil.h"
#include "UtilAll.h"

namespace rocketmq {

static const std::string DEFAULT_INSTANCE_NAME = "DEFAULT";

MQClientConfig::MQClientConfig()
    : m_instanceName(DEFAULT_INSTANCE_NAME),
      m_tcpWorkerThreadNum(std::min(4, (int)std::thread::hardware_concurrency())),
      m_tcpConnectTimeout(3000),
      m_tcpTransportTryLockTimeout(3) {
  const char* addr = std::getenv(ROCKETMQ_NAMESRV_ADDR_ENV.c_str());
  if (addr != nullptr) {
    m_namesrvAddr = addr;
  }
}

std::string MQClientConfig::buildMQClientId() const {
  std::string clientId;
  clientId.append(UtilAll::getLocalAddress());  // clientIP
  clientId.append("@");
  clientId.append(m_instanceName);  // processId
  if (!m_unitName.empty()) {
    clientId.append("@");
    clientId.append(m_unitName);  // unitName
  }
  return clientId;
}

const std::string& MQClientConfig::getGroupName() const {
  return m_groupName;
}

void MQClientConfig::setGroupName(const std::string& groupname) {
  m_groupName = groupname;
}

const std::string& MQClientConfig::getNamesrvAddr() const {
  return m_namesrvAddr;
}

void MQClientConfig::setNamesrvAddr(const std::string& namesrvAddr) {
  m_namesrvAddr = NameSpaceUtil::formatNameServerURL(namesrvAddr);
}

const std::string& MQClientConfig::getInstanceName() const {
  return m_instanceName;
}

void MQClientConfig::setInstanceName(const std::string& instanceName) {
  m_instanceName = instanceName;
}

void MQClientConfig::changeInstanceNameToPID() {
  if (m_instanceName == DEFAULT_INSTANCE_NAME) {
    m_instanceName = UtilAll::to_string(UtilAll::getProcessId());
  }
}

int MQClientConfig::getTcpTransportWorkerThreadNum() const {
  return m_tcpWorkerThreadNum;
}

void MQClientConfig::setTcpTransportWorkerThreadNum(int num) {
  if (num > m_tcpWorkerThreadNum) {
    m_tcpWorkerThreadNum = num;
  }
}

uint64_t MQClientConfig::getTcpTransportConnectTimeout() const {
  return m_tcpConnectTimeout;
}

void MQClientConfig::setTcpTransportConnectTimeout(uint64_t timeout) {
  m_tcpConnectTimeout = timeout;
}

uint64_t MQClientConfig::getTcpTransportTryLockTimeout() const {
  return m_tcpTransportTryLockTimeout;
}

void MQClientConfig::setTcpTransportTryLockTimeout(uint64_t timeout) {
  m_tcpTransportTryLockTimeout = std::max<uint64_t>(1000, timeout) / 1000;
}

const std::string& MQClientConfig::getUnitName() const {
  return m_unitName;
}

void MQClientConfig::setUnitName(std::string unitName) {
  m_unitName = unitName;
}

}  // namespace rocketmq

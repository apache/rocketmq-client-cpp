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
#ifndef __MQ_CLIENT_CONFIG_PROXY_H__
#define __MQ_CLIENT_CONFIG_PROXY_H__

#include "MQClientConfig.h"

namespace rocketmq {

class ROCKETMQCLIENT_API MQClientConfigProxy : virtual public MQClientConfig {
 public:
  MQClientConfigProxy(MQClientConfigPtr clientConfig) : m_clientConfig(clientConfig) {}
  virtual ~MQClientConfigProxy() = default;

  MQClientConfigPtr getRealConfig() const { return m_clientConfig; }

  std::string buildMQClientId() const override { return m_clientConfig->buildMQClientId(); }

  const std::string& getGroupName() const override { return m_clientConfig->getGroupName(); }

  void setGroupName(const std::string& groupname) override { m_clientConfig->setGroupName(groupname); }

  const std::string& getNamesrvAddr() const override { return m_clientConfig->getNamesrvAddr(); }

  void setNamesrvAddr(const std::string& namesrvAddr) override { m_clientConfig->setNamesrvAddr(namesrvAddr); }

  const std::string& getInstanceName() const override { return m_clientConfig->getInstanceName(); }

  void setInstanceName(const std::string& instanceName) override { m_clientConfig->setInstanceName(instanceName); }

  void changeInstanceNameToPID() override { m_clientConfig->changeInstanceNameToPID(); }

  const std::string& getUnitName() const override { return m_clientConfig->getUnitName(); }

  void setUnitName(std::string unitName) override { m_clientConfig->setUnitName(unitName); }

  int getTcpTransportWorkerThreadNum() const override { return m_clientConfig->getTcpTransportWorkerThreadNum(); }

  void setTcpTransportWorkerThreadNum(int num) override { m_clientConfig->setTcpTransportWorkerThreadNum(num); }

  uint64_t getTcpTransportConnectTimeout() const override { return m_clientConfig->getTcpTransportConnectTimeout(); }

  void setTcpTransportConnectTimeout(uint64_t timeout) override {
    m_clientConfig->setTcpTransportConnectTimeout(timeout);
  }

  uint64_t getTcpTransportTryLockTimeout() const override { return m_clientConfig->getTcpTransportTryLockTimeout(); }

  void setTcpTransportTryLockTimeout(uint64_t timeout) override {
    m_clientConfig->setTcpTransportTryLockTimeout(timeout);
  }

 private:
  MQClientConfigPtr m_clientConfig;
};

}  // namespace rocketmq

#endif  // __MQ_CLIENT_CONFIG_PROXY_H__

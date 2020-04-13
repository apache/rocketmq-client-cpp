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
#ifndef __MQ_CLIENT_CONFIG_IMPL_H__
#define __MQ_CLIENT_CONFIG_IMPL_H__

#include "MQClientConfig.h"

namespace rocketmq {

/**
 * MQ Client Config
 */
class MQClientConfigImpl : virtual public MQClientConfig {
 public:
  MQClientConfigImpl();
  virtual ~MQClientConfigImpl() = default;

  std::string buildMQClientId() const override;

  const std::string& getGroupName() const override;
  void setGroupName(const std::string& groupname) override;

  const std::string& getNamesrvAddr() const override;
  void setNamesrvAddr(const std::string& namesrvAddr) override;

  const std::string& getInstanceName() const override;
  void setInstanceName(const std::string& instanceName) override;

  void changeInstanceNameToPID() override;

  const std::string& getUnitName() const override;
  void setUnitName(std::string unitName) override;

  int getTcpTransportWorkerThreadNum() const override;
  void setTcpTransportWorkerThreadNum(int num) override;

  uint64_t getTcpTransportConnectTimeout() const override;
  void setTcpTransportConnectTimeout(uint64_t timeout) override;  // ms

  uint64_t getTcpTransportTryLockTimeout() const override;
  void setTcpTransportTryLockTimeout(uint64_t timeout) override;  // ms

 protected:
  std::string m_namesrvAddr;
  std::string m_instanceName;
  std::string m_groupName;
  std::string m_unitName;

  int m_tcpWorkerThreadNum;
  uint64_t m_tcpConnectTimeout;           // ms
  uint64_t m_tcpTransportTryLockTimeout;  // s
};

}  // namespace rocketmq

#endif  // __MQ_CLIENT_CONFIG_H__

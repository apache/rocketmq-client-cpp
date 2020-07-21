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
#ifndef ROCKETMQ_MQCLIENTCONFIGPROXY_H_
#define ROCKETMQ_MQCLIENTCONFIGPROXY_H_

#include "MQClientConfig.h"

namespace rocketmq {

/**
 * MQClientConfigProxy - proxy for MQClientConfig
 */
class ROCKETMQCLIENT_API MQClientConfigProxy : virtual public MQClientConfig  // interface
{
 public:
  MQClientConfigProxy(MQClientConfigPtr clientConfig) : client_config_(clientConfig) {}
  virtual ~MQClientConfigProxy() = default;

  inline std::string buildMQClientId() const override { return client_config_->buildMQClientId(); }

  inline const std::string& getGroupName() const override { return client_config_->getGroupName(); }

  inline void setGroupName(const std::string& groupname) override { client_config_->setGroupName(groupname); }

  inline const std::string& getNamesrvAddr() const override { return client_config_->getNamesrvAddr(); }

  inline void setNamesrvAddr(const std::string& namesrvAddr) override { client_config_->setNamesrvAddr(namesrvAddr); }

  inline const std::string& getInstanceName() const override { return client_config_->getInstanceName(); }

  inline void setInstanceName(const std::string& instanceName) override {
    client_config_->setInstanceName(instanceName);
  }

  inline void changeInstanceNameToPID() override { client_config_->changeInstanceNameToPID(); }

  inline const std::string& getUnitName() const override { return client_config_->getUnitName(); }

  inline void setUnitName(std::string unitName) override { client_config_->setUnitName(unitName); }

  inline int getTcpTransportWorkerThreadNum() const override {
    return client_config_->getTcpTransportWorkerThreadNum();
  }

  inline void setTcpTransportWorkerThreadNum(int num) override { client_config_->setTcpTransportWorkerThreadNum(num); }

  inline uint64_t getTcpTransportConnectTimeout() const override {
    return client_config_->getTcpTransportConnectTimeout();
  }

  inline void setTcpTransportConnectTimeout(uint64_t timeout) override {
    client_config_->setTcpTransportConnectTimeout(timeout);
  }

  inline uint64_t getTcpTransportTryLockTimeout() const override {
    return client_config_->getTcpTransportTryLockTimeout();
  }

  inline void setTcpTransportTryLockTimeout(uint64_t timeout) override {
    client_config_->setTcpTransportTryLockTimeout(timeout);
  }

  inline MQClientConfigPtr real_config() const { return client_config_; }

 protected:
  MQClientConfigPtr client_config_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_MQCLIENTCONFIGPROXY_H_

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

  std::string buildMQClientId() const override { return client_config_->buildMQClientId(); }
  void changeInstanceNameToPID() override { client_config_->changeInstanceNameToPID(); }

 public:
  const std::string& group_name() const override { return client_config_->group_name(); }
  void set_group_name(const std::string& groupname) override { client_config_->set_group_name(groupname); }

  const std::string& namesrv_addr() const override { return client_config_->namesrv_addr(); }
  void set_namesrv_addr(const std::string& namesrvAddr) override { client_config_->set_namesrv_addr(namesrvAddr); }

  const std::string& instance_name() const override { return client_config_->instance_name(); }
  void set_instance_name(const std::string& instanceName) override { client_config_->set_instance_name(instanceName); }

  const std::string& unit_name() const override { return client_config_->unit_name(); }
  void set_unit_name(const std::string& unitName) override { client_config_->set_unit_name(unitName); }

  const std::string& name_space() const override { return client_config_->name_space(); }
  void set_name_space(const std::string& name_space) override { client_config_->set_name_space(name_space); }

  int tcp_transport_worker_thread_nums() const override { return client_config_->tcp_transport_worker_thread_nums(); }
  void set_tcp_transport_worker_thread_nums(int num) override {
    client_config_->set_tcp_transport_worker_thread_nums(num);
  }

  uint64_t tcp_transport_connect_timeout() const override { return client_config_->tcp_transport_connect_timeout(); }
  void set_tcp_transport_connect_timeout(uint64_t timeout) override {
    client_config_->set_tcp_transport_connect_timeout(timeout);
  }

  uint64_t tcp_transport_try_lock_timeout() const override { return client_config_->tcp_transport_try_lock_timeout(); }
  void set_tcp_transport_try_lock_timeout(uint64_t timeout) override {
    client_config_->set_tcp_transport_try_lock_timeout(timeout);
  }

  inline MQClientConfigPtr real_config() const { return client_config_; }

 protected:
  MQClientConfigPtr client_config_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_MQCLIENTCONFIGPROXY_H_

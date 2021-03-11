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
#ifndef ROCKETMQ_MQCLIENTCONFIGIMPL_HPP_
#define ROCKETMQ_MQCLIENTCONFIGIMPL_HPP_

#include <algorithm>  // std::min, std::max
#include <thread>     // std::thread::hardware_concurrency

#include "MQClientConfig.h"
#include "NamespaceUtil.h"
#include "SocketUtil.h"
#include "UtilAll.h"

namespace rocketmq {

/**
 * MQ Client Config
 */
class MQClientConfigImpl : virtual public MQClientConfig {
 public:
  MQClientConfigImpl()
      : instance_name_("DEFAULT"),
        tcp_worker_thread_nums_(std::min(4, (int)std::thread::hardware_concurrency())),
        tcp_connect_timeout(3000),
        tcp_transport_try_lock_timeout_(3) {
    const char* addr = std::getenv(ROCKETMQ_NAMESRV_ADDR_ENV.c_str());
    if (addr != nullptr) {
      namesrv_addr_ = addr;
    }
  }
  virtual ~MQClientConfigImpl() = default;

  std::string buildMQClientId() const override {
    std::string clientId;
    clientId.append(GetLocalAddress());  // clientIP
    clientId.append("@");
    clientId.append(instance_name_);  // instanceName
    if (!unit_name_.empty()) {
      clientId.append("@");
      clientId.append(unit_name_);  // unitName
    }
    return clientId;
  }

  void changeInstanceNameToPID() override {
    if (instance_name_ == "DEFAULT") {
      instance_name_ = UtilAll::to_string(UtilAll::getProcessId());
    }
  }

 public:
  const std::string& group_name() const override { return group_name_; }
  void set_group_name(const std::string& groupname) override { group_name_ = groupname; }

  const std::string& namesrv_addr() const override { return namesrv_addr_; }
  void set_namesrv_addr(const std::string& namesrvAddr) override {
    namesrv_addr_ = NamespaceUtil::formatNameServerURL(namesrvAddr);
  }

  const std::string& instance_name() const override { return instance_name_; }
  void set_instance_name(const std::string& instanceName) override { instance_name_ = instanceName; }

  const std::string& unit_name() const override { return unit_name_; }
  void set_unit_name(const std::string& unitName) override { unit_name_ = unitName; }

  const std::string& name_space() const override { return name_space_; }
  void set_name_space(const std::string& name_space) override { name_space_ = name_space; }

  int tcp_transport_worker_thread_nums() const override { return tcp_worker_thread_nums_; }
  void set_tcp_transport_worker_thread_nums(int num) override {
    if (num > tcp_worker_thread_nums_) {
      tcp_worker_thread_nums_ = num;
    }
  }

  uint64_t tcp_transport_connect_timeout() const override { return tcp_connect_timeout; }
  void set_tcp_transport_connect_timeout(uint64_t millisec) override { tcp_connect_timeout = millisec; }

  uint64_t tcp_transport_try_lock_timeout() const override { return tcp_transport_try_lock_timeout_; }
  void set_tcp_transport_try_lock_timeout(uint64_t millisec) override {
    tcp_transport_try_lock_timeout_ = std::max<uint64_t>(1000, millisec) / 1000;
  }

 protected:
  std::string namesrv_addr_;
  std::string instance_name_;
  std::string group_name_;
  std::string unit_name_;
  std::string name_space_;

  int tcp_worker_thread_nums_;
  uint64_t tcp_connect_timeout;              // ms
  uint64_t tcp_transport_try_lock_timeout_;  // s
};

}  // namespace rocketmq

#endif  // ROCKETMQ_MQCLIENTCONFIGIMPL_HPP_

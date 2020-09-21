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
#ifndef ROCKETMQ_MQCLIENTCONFIG_H_
#define ROCKETMQ_MQCLIENTCONFIG_H_

#include <memory>  // std::shared_ptr
#include <string>  // std::string

#include "RocketMQClient.h"

namespace rocketmq {

class MQClientConfig;
typedef std::shared_ptr<MQClientConfig> MQClientConfigPtr;

/**
 * MQClientConfig - config interface for MQClient
 */
class ROCKETMQCLIENT_API MQClientConfig {
 public:
  virtual ~MQClientConfig() = default;

  // clientId = clientIP @ processId [ @ unitName ]
  virtual std::string buildMQClientId() const = 0;

  virtual void changeInstanceNameToPID() = 0;

 public:
  virtual const std::string& group_name() const = 0;
  virtual void set_group_name(const std::string& groupname) = 0;

  virtual const std::string& namesrv_addr() const = 0;
  virtual void set_namesrv_addr(const std::string& namesrvAddr) = 0;

  virtual const std::string& instance_name() const = 0;
  virtual void set_instance_name(const std::string& instanceName) = 0;

  virtual const std::string& unit_name() const = 0;
  virtual void set_unit_name(const std::string& unitName) = 0;

  virtual const std::string& name_space() const = 0;
  virtual void set_name_space(const std::string& name_space) = 0;

  /**
   * the num of threads to distribute network data
   **/
  virtual int tcp_transport_worker_thread_nums() const = 0;
  virtual void set_tcp_transport_worker_thread_nums(int num) = 0;

  /**
   * timeout of tcp connect
   **/
  virtual uint64_t tcp_transport_connect_timeout() const = 0;
  virtual void set_tcp_transport_connect_timeout(uint64_t timeout) = 0;  // ms

  /**
   * timeout of tryLock tcpTransport, the minimun value is 1000ms
   **/
  virtual uint64_t tcp_transport_try_lock_timeout() const = 0;
  virtual void set_tcp_transport_try_lock_timeout(uint64_t timeout) = 0;  // ms
};

}  // namespace rocketmq

#endif  // ROCKETMQ_MQCLIENTCONFIG_H_

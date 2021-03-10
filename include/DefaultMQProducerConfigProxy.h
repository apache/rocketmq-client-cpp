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
#ifndef ROCKETMQ_DEFAULTMQPRODUCERCONFIPROXY_H_
#define ROCKETMQ_DEFAULTMQPRODUCERCONFIPROXY_H_

#include "DefaultMQProducerConfig.h"
#include "MQClientConfigProxy.h"

namespace rocketmq {

/**
 * DefaultMQProducerConfigProxy - proxy for DefaultMQProducerConfig
 */
class ROCKETMQCLIENT_API DefaultMQProducerConfigProxy : public MQClientConfigProxy,             // base
                                                        virtual public DefaultMQProducerConfig  // interface
{
 public:
  DefaultMQProducerConfigProxy(DefaultMQProducerConfigPtr producerConfig) : MQClientConfigProxy(producerConfig) {}
  virtual ~DefaultMQProducerConfigProxy() = default;

  int async_send_thread_nums() const override {
    return dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->async_send_thread_nums();
  }

  void set_async_send_thread_nums(int async_send_thread_nums) override {
    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->set_async_send_thread_nums(async_send_thread_nums);
  }

  int max_message_size() const override {
    return dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->max_message_size();
  }

  void set_max_message_size(int max_message_size) override {
    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->set_max_message_size(max_message_size);
  }

  int compress_msg_body_over_howmuch() const override {
    return dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->compress_msg_body_over_howmuch();
  }

  void set_compress_msg_body_over_howmuch(int compress_msg_body_over_howmuch) override {
    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())
        ->set_compress_msg_body_over_howmuch(compress_msg_body_over_howmuch);
  }

  int compress_level() const override {
    return dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->compress_level();
  }

  void set_compress_level(int compress_level) override {
    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->set_compress_level(compress_level);
  }

  int send_msg_timeout() const override {
    return dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->send_msg_timeout();
  }

  void set_send_msg_timeout(int send_msg_timeout) override {
    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->set_send_msg_timeout(send_msg_timeout);
  }

  int retry_times() const override {
    return dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->retry_times();
  }

  void set_retry_times(int retry_times) override {
    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->set_retry_times(retry_times);
  }

  int retry_times_for_async() const override {
    return dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->retry_times_for_async();
  }

  void set_retry_times_for_async(int retry_times) override {
    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->set_retry_times_for_async(retry_times);
  }

  bool retry_another_broker_when_not_store_ok() const override {
    return dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->retry_another_broker_when_not_store_ok();
  }

  void set_retry_another_broker_when_not_store_ok(bool retry_another_broker_when_not_store_ok) override {
    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())
        ->set_retry_another_broker_when_not_store_ok(retry_another_broker_when_not_store_ok);
  }

  bool send_latency_fault_enable() const override {
    return dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->send_latency_fault_enable();
  }

  void set_send_latency_fault_enable(bool send_latency_fault_enable) override {
    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())
        ->set_send_latency_fault_enable(send_latency_fault_enable);
  }

  inline DefaultMQProducerConfigPtr real_config() const {
    return std::dynamic_pointer_cast<DefaultMQProducerConfig>(client_config_);
  }
};

}  // namespace rocketmq

#endif  // ROCKETMQ_DEFAULTMQPRODUCERCONFIPROXY_H_

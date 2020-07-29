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

  int max_message_size() const override {
    return dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->max_message_size();
  }

  void set_max_message_size(int maxMessageSize) override {
    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->set_max_message_size(maxMessageSize);
  }

  int compress_msg_body_over_howmuch() const override {
    return dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->compress_msg_body_over_howmuch();
  }

  void set_compress_msg_body_over_howmuch(int compressMsgBodyOverHowmuch) override {
    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())
        ->set_compress_msg_body_over_howmuch(compressMsgBodyOverHowmuch);
  }

  int compress_level() const override {
    return dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->compress_level();
  }

  void set_compress_level(int compressLevel) override {
    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->set_compress_level(compressLevel);
  }

  int send_msg_timeout() const override {
    return dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->send_msg_timeout();
  }

  void set_send_msg_timeout(int sendMsgTimeout) override {
    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->set_send_msg_timeout(sendMsgTimeout);
  }

  int retry_times() const override {
    return dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->retry_times();
  }

  void set_retry_times(int times) override {
    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->set_retry_times(times);
  }

  int retry_times_for_async() const override {
    return dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->retry_times_for_async();
  }

  void set_retry_times_for_async(int times) override {
    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->set_retry_times_for_async(times);
  }

  bool retry_another_broker_when_not_store_ok() const override {
    return dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->retry_another_broker_when_not_store_ok();
  }

  void set_retry_another_broker_when_not_store_ok(bool retryAnotherBrokerWhenNotStoreOK) override {
    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())
        ->set_retry_another_broker_when_not_store_ok(retryAnotherBrokerWhenNotStoreOK);
  }

  bool send_latency_fault_enable() const override {
    return dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->send_latency_fault_enable();
  }

  void set_send_latency_fault_enable(bool sendLatencyFaultEnable) override {
    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->set_send_latency_fault_enable(sendLatencyFaultEnable);
  }

  inline DefaultMQProducerConfigPtr real_config() const {
    return std::dynamic_pointer_cast<DefaultMQProducerConfig>(client_config_);
  }
};

}  // namespace rocketmq

#endif  // ROCKETMQ_DEFAULTMQPRODUCERCONFIPROXY_H_

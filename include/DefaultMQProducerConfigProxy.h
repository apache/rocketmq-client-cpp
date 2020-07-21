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

  inline int getMaxMessageSize() const override {
    return dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->getMaxMessageSize();
  }

  inline void setMaxMessageSize(int maxMessageSize) override {
    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->setMaxMessageSize(maxMessageSize);
  }

  inline int getCompressMsgBodyOverHowmuch() const override {
    return dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->getCompressMsgBodyOverHowmuch();
  }

  inline void setCompressMsgBodyOverHowmuch(int compressMsgBodyOverHowmuch) override {
    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())
        ->setCompressMsgBodyOverHowmuch(compressMsgBodyOverHowmuch);
  }

  inline int getCompressLevel() const override {
    return dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->getCompressLevel();
  }

  inline void setCompressLevel(int compressLevel) override {
    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->setCompressLevel(compressLevel);
  }

  inline int getSendMsgTimeout() const override {
    return dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->getSendMsgTimeout();
  }

  inline void setSendMsgTimeout(int sendMsgTimeout) override {
    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->setSendMsgTimeout(sendMsgTimeout);
  }

  inline int getRetryTimes() const override {
    return dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->getRetryTimes();
  }

  inline void setRetryTimes(int times) override {
    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->setRetryTimes(times);
  }

  inline int getRetryTimesForAsync() const override {
    return dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->getRetryTimesForAsync();
  }

  inline void setRetryTimesForAsync(int times) override {
    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->setRetryTimesForAsync(times);
  }

  inline bool isRetryAnotherBrokerWhenNotStoreOK() const override {
    return dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->isRetryAnotherBrokerWhenNotStoreOK();
  }

  inline void setRetryAnotherBrokerWhenNotStoreOK(bool retryAnotherBrokerWhenNotStoreOK) override {
    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())
        ->setRetryAnotherBrokerWhenNotStoreOK(retryAnotherBrokerWhenNotStoreOK);
  }

  inline bool isSendLatencyFaultEnable() const override {
    return dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->isSendLatencyFaultEnable();
  }

  inline void setSendLatencyFaultEnable(bool sendLatencyFaultEnable) override {
    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->setSendLatencyFaultEnable(sendLatencyFaultEnable);
  }

  inline DefaultMQProducerConfigPtr real_config() const {
    return std::dynamic_pointer_cast<DefaultMQProducerConfig>(client_config_);
  }
};

}  // namespace rocketmq

#endif  // ROCKETMQ_DEFAULTMQPRODUCERCONFIPROXY_H_

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
#ifndef __DEFAULT_MQ_PRODUCER_CONFI_PROXY_H__
#define __DEFAULT_MQ_PRODUCER_CONFI_PROXY_H__

#include "DefaultMQProducerConfig.h"
#include "MQClientConfigProxy.h"

namespace rocketmq {

class ROCKETMQCLIENT_API DefaultMQProducerConfigProxy : virtual public DefaultMQProducerConfig,
                                                        public MQClientConfigProxy {
 public:
  DefaultMQProducerConfigProxy(DefaultMQProducerConfigPtr producerConfig)
      : MQClientConfigProxy(producerConfig), m_producerConfig(producerConfig) {}
  virtual ~DefaultMQProducerConfigProxy() = default;

  DefaultMQProducerConfigPtr getRealConfig() const { return m_producerConfig; }

  int getMaxMessageSize() const override { return m_producerConfig->getMaxMessageSize(); }

  void setMaxMessageSize(int maxMessageSize) override { m_producerConfig->setMaxMessageSize(maxMessageSize); }

  int getCompressMsgBodyOverHowmuch() const override { return m_producerConfig->getCompressMsgBodyOverHowmuch(); }

  void setCompressMsgBodyOverHowmuch(int compressMsgBodyOverHowmuch) override {
    m_producerConfig->setCompressMsgBodyOverHowmuch(compressMsgBodyOverHowmuch);
  }

  int getCompressLevel() const override { return m_producerConfig->getCompressLevel(); }

  void setCompressLevel(int compressLevel) override { m_producerConfig->setCompressLevel(compressLevel); }

  int getSendMsgTimeout() const override { return m_producerConfig->getSendMsgTimeout(); }

  void setSendMsgTimeout(int sendMsgTimeout) override { m_producerConfig->setSendMsgTimeout(sendMsgTimeout); }

  int getRetryTimes() const override { return m_producerConfig->getRetryTimes(); }

  void setRetryTimes(int times) override { m_producerConfig->setRetryTimes(times); }

  int getRetryTimes4Async() const override { return m_producerConfig->getRetryTimes4Async(); }

  void setRetryTimes4Async(int times) override { m_producerConfig->setRetryTimes4Async(times); }

  bool isRetryAnotherBrokerWhenNotStoreOK() const override {
    return m_producerConfig->isRetryAnotherBrokerWhenNotStoreOK();
  }

  void setRetryAnotherBrokerWhenNotStoreOK(bool retryAnotherBrokerWhenNotStoreOK) override {
    m_producerConfig->setRetryAnotherBrokerWhenNotStoreOK(retryAnotherBrokerWhenNotStoreOK);
  }

  bool isSendLatencyFaultEnable() const override { return m_producerConfig->isSendLatencyFaultEnable(); }

  void setSendLatencyFaultEnable(bool sendLatencyFaultEnable) override {
    m_producerConfig->setSendLatencyFaultEnable(sendLatencyFaultEnable);
  }

 private:
  DefaultMQProducerConfigPtr m_producerConfig;
};

}  // namespace rocketmq

#endif  // __DEFAULT_MQ_PRODUCER_CONFI_PROXY_H__

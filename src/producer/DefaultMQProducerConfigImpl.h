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
#ifndef __DEFAULT_MQ_PRODUCER_CONFIG_IMPL_H__
#define __DEFAULT_MQ_PRODUCER_CONFIG_IMPL_H__

#include "DefaultMQProducerConfig.h"
#include "MQClientConfigImpl.h"

namespace rocketmq {

class DefaultMQProducerConfigImpl : virtual public DefaultMQProducerConfig, public MQClientConfigImpl {
 public:
  DefaultMQProducerConfigImpl();
  virtual ~DefaultMQProducerConfigImpl() = default;

  int getMaxMessageSize() const override;
  void setMaxMessageSize(int maxMessageSize) override;

  int getCompressMsgBodyOverHowmuch() const override;
  void setCompressMsgBodyOverHowmuch(int compressMsgBodyOverHowmuch) override;

  int getCompressLevel() const override;
  void setCompressLevel(int compressLevel) override;

  int getSendMsgTimeout() const override;
  void setSendMsgTimeout(int sendMsgTimeout) override;

  int getRetryTimes() const override;
  void setRetryTimes(int times) override;

  int getRetryTimes4Async() const override;
  void setRetryTimes4Async(int times) override;

  bool isRetryAnotherBrokerWhenNotStoreOK() const override;
  void setRetryAnotherBrokerWhenNotStoreOK(bool retryAnotherBrokerWhenNotStoreOK) override;

 protected:
  int m_maxMessageSize;              // default: 4 MB
  int m_compressMsgBodyOverHowmuch;  // default: 4 KB
  int m_compressLevel;
  int m_sendMsgTimeout;
  int m_retryTimes;
  int m_retryTimes4Async;
  bool m_retryAnotherBrokerWhenNotStoreOK;
};

}  // namespace rocketmq

#endif  // __DEFAULT_MQ_PRODUCER_CONFIG_IMPL_H__

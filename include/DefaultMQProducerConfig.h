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
#ifndef ROCKETMQ_DEFAULTMQPRODUCERCONFIG_H_
#define ROCKETMQ_DEFAULTMQPRODUCERCONFIG_H_

#include "MQClientConfig.h"

namespace rocketmq {

class DefaultMQProducerConfig;
typedef std::shared_ptr<DefaultMQProducerConfig> DefaultMQProducerConfigPtr;

/**
 * DefaultMQProducerConfig - config interface for DefaultMQProducer
 */
class ROCKETMQCLIENT_API DefaultMQProducerConfig : virtual public MQClientConfig  // base interface
{
 public:
  virtual ~DefaultMQProducerConfig() = default;

  // if msgbody size larger than maxMsgBodySize, exception will be throwed
  virtual int getMaxMessageSize() const = 0;
  virtual void setMaxMessageSize(int maxMessageSize) = 0;

  /*
   * if msgBody size is large than m_compressMsgBodyOverHowmuch
   *  rocketmq cpp will compress msgBody according to compressLevel
   */
  virtual int getCompressMsgBodyOverHowmuch() const = 0;
  virtual void setCompressMsgBodyOverHowmuch(int compressMsgBodyOverHowmuch) = 0;

  virtual int getCompressLevel() const = 0;
  virtual void setCompressLevel(int compressLevel) = 0;

  // set and get timeout of per msg
  virtual int getSendMsgTimeout() const = 0;
  virtual void setSendMsgTimeout(int sendMsgTimeout) = 0;

  // set msg max retry times, default retry times is 5
  virtual int getRetryTimes() const = 0;
  virtual void setRetryTimes(int times) = 0;

  virtual int getRetryTimesForAsync() const = 0;
  virtual void setRetryTimesForAsync(int times) = 0;

  virtual bool isRetryAnotherBrokerWhenNotStoreOK() const = 0;
  virtual void setRetryAnotherBrokerWhenNotStoreOK(bool retryAnotherBrokerWhenNotStoreOK) = 0;

  virtual bool isSendLatencyFaultEnable() const { return false; };
  virtual void setSendLatencyFaultEnable(bool sendLatencyFaultEnable){};
};

}  // namespace rocketmq

#endif  // ROCKETMQ_DEFAULTMQPRODUCERCONFIG_H_

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
  virtual int max_message_size() const = 0;
  virtual void set_max_message_size(int maxMessageSize) = 0;

  /*
   * if msgBody size is large than m_compressMsgBodyOverHowmuch
   *  rocketmq cpp will compress msgBody according to compressLevel
   */
  virtual int compress_msg_body_over_howmuch() const = 0;
  virtual void set_compress_msg_body_over_howmuch(int compressMsgBodyOverHowmuch) = 0;

  virtual int compress_level() const = 0;
  virtual void set_compress_level(int compressLevel) = 0;

  // set and get timeout of per msg
  virtual int send_msg_timeout() const = 0;
  virtual void set_send_msg_timeout(int sendMsgTimeout) = 0;

  // set msg max retry times, default retry times is 5
  virtual int retry_times() const = 0;
  virtual void set_retry_times(int times) = 0;

  virtual int retry_times_for_async() const = 0;
  virtual void set_retry_times_for_async(int times) = 0;

  virtual bool retry_another_broker_when_not_store_ok() const = 0;
  virtual void set_retry_another_broker_when_not_store_ok(bool retryAnotherBrokerWhenNotStoreOK) = 0;

  virtual bool send_latency_fault_enable() const { return false; };
  virtual void set_send_latency_fault_enable(bool sendLatencyFaultEnable){};
};

}  // namespace rocketmq

#endif  // ROCKETMQ_DEFAULTMQPRODUCERCONFIG_H_

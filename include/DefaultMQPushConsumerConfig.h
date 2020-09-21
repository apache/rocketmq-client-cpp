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
#ifndef ROCKETMQ_DEFAULTMQPUSHCONSUMERCONFIG_H_
#define ROCKETMQ_DEFAULTMQPUSHCONSUMERCONFIG_H_

#include "AllocateMQStrategy.h"
#include "ConsumeType.h"
#include "MQClientConfig.h"

namespace rocketmq {

class DefaultMQPushConsumerConfig;
typedef std::shared_ptr<DefaultMQPushConsumerConfig> DefaultMQPushConsumerConfigPtr;

/**
 * DefaultMQPushConsumerConfig - config for DefaultMQPushConsumer
 */
class ROCKETMQCLIENT_API DefaultMQPushConsumerConfig : virtual public MQClientConfig  // base interface
{
 public:
  virtual ~DefaultMQPushConsumerConfig() = default;

  virtual MessageModel message_model() const = 0;
  virtual void set_message_model(MessageModel messageModel) = 0;

  virtual ConsumeFromWhere consume_from_where() const = 0;
  virtual void set_consume_from_where(ConsumeFromWhere consumeFromWhere) = 0;

  virtual const std::string& consume_timestamp() const = 0;
  virtual void set_consume_timestamp(const std::string& consumeTimestamp) = 0;

  /**
   * consuming thread count, default value is cpu cores
   */
  virtual int consume_thread_nums() const = 0;
  virtual void set_consume_thread_nums(int threadNum) = 0;

  /**
   * max cache msg size per Queue in memory if consumer could not consume msgs immediately,
   * default maxCacheMsgSize per Queue is 1000, set range is:1~65535
   */
  virtual int pull_threshold_for_queue() const = 0;
  virtual void set_pull_threshold_for_queue(int maxCacheSize) = 0;

  /**
   * the pull number of message size by each pullMsg for orderly consume, default value is 1
   */
  virtual int consume_message_batch_max_size() const = 0;
  virtual void set_consume_message_batch_max_size(int consumeMessageBatchMaxSize) = 0;

  virtual int pull_batch_size() const = 0;
  virtual void set_pull_batch_size(int pull_batch_size) = 0;

  virtual int max_reconsume_times() const = 0;
  virtual void set_max_reconsume_times(int maxReconsumeTimes) = 0;

  virtual long pull_time_delay_millis_when_exception() const = 0;
  virtual void set_pull_time_delay_millis_when_exception(long pull_time_delay_millis_when_exception) = 0;

  virtual AllocateMQStrategy* allocate_mq_strategy() const = 0;
  virtual void set_allocate_mq_strategy(AllocateMQStrategy* strategy) = 0;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_DEFAULTMQPUSHCONSUMERCONFIG_H_

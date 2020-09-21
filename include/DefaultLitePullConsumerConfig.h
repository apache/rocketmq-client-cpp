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
#ifndef ROCKETMQ_DEFAULTLITEPULLCONSUMERCONFIG_H_
#define ROCKETMQ_DEFAULTLITEPULLCONSUMERCONFIG_H_

#include "AllocateMQStrategy.h"
#include "ConsumeType.h"
#include "MQClientConfig.h"

namespace rocketmq {

class DefaultLitePullConsumerConfig;
typedef std::shared_ptr<DefaultLitePullConsumerConfig> DefaultLitePullConsumerConfigPtr;

class ROCKETMQCLIENT_API DefaultLitePullConsumerConfig : virtual public MQClientConfig  // base interface
{
 public:
  virtual ~DefaultLitePullConsumerConfig() = default;

  virtual MessageModel message_model() const = 0;
  virtual void set_message_model(MessageModel message_model) = 0;

  virtual ConsumeFromWhere consume_from_where() const = 0;
  virtual void set_consume_from_where(ConsumeFromWhere consume_from_where) = 0;

  virtual const std::string& consume_timestamp() const = 0;
  virtual void set_consume_timestamp(const std::string& consume_timestamp) = 0;

  virtual long auto_commit_interval_millis() const = 0;
  virtual void set_auto_commit_interval_millis(long auto_commit_interval_millis) = 0;

  virtual int pull_batch_size() const = 0;
  virtual void set_pull_batch_size(int pull_batch_size) = 0;

  virtual int pull_thread_nums() const = 0;
  virtual void set_pull_thread_nums(int pull_thread_nums) = 0;

  virtual bool long_polling_enable() const = 0;
  virtual void set_long_polling_enable(bool long_polling_enable) = 0;

  virtual long consumer_pull_timeout_millis() const = 0;
  virtual void set_consumer_pull_timeout_millis(long consumer_pull_timeout_millis) = 0;

  virtual long consumer_timeout_millis_when_suspend() const = 0;
  virtual void set_consumer_timeout_millis_when_suspend(long consumer_timeout_millis_when_suspend) = 0;

  virtual long broker_suspend_max_time_millis() const = 0;
  virtual void set_broker_suspend_max_time_millis(long broker_suspend_max_time_millis) = 0;

  virtual long pull_threshold_for_all() const = 0;
  virtual void set_pull_threshold_for_all(long pull_threshold_for_all) = 0;

  virtual int pull_threshold_for_queue() const = 0;
  virtual void set_pull_threshold_for_queue(int pull_threshold_for_queue) = 0;

  virtual long pull_time_delay_millis_when_exception() const = 0;
  virtual void set_pull_time_delay_millis_when_exception(long pull_time_delay_millis_when_exception) = 0;

  virtual long poll_timeout_millis() const = 0;
  virtual void set_poll_timeout_millis(long poll_timeout_millis) = 0;

  virtual long topic_metadata_check_interval_millis() const = 0;
  virtual void set_topic_metadata_check_interval_millis(long topic_metadata_check_interval_millis) = 0;

  virtual AllocateMQStrategy* allocate_mq_strategy() const = 0;
  virtual void set_allocate_mq_strategy(AllocateMQStrategy* strategy) = 0;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_DEFAULTLITEPULLCONSUMERCONFIG_H_

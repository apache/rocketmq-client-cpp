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
#ifndef ROCKETMQ_DEFAULTMQPUSHCONSUMERCONFIGPROXY_H_
#define ROCKETMQ_DEFAULTMQPUSHCONSUMERCONFIGPROXY_H_

#include "DefaultMQPushConsumerConfig.h"
#include "MQClientConfigProxy.h"

namespace rocketmq {

class ROCKETMQCLIENT_API DefaultMQPushConsumerConfigProxy : public MQClientConfigProxy,                 // base
                                                            virtual public DefaultMQPushConsumerConfig  // interface
{
 public:
  DefaultMQPushConsumerConfigProxy(DefaultMQPushConsumerConfigPtr consumerConfig)
      : MQClientConfigProxy(consumerConfig) {}
  virtual ~DefaultMQPushConsumerConfigProxy() = default;

  MessageModel message_model() const override {
    return dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->message_model();
  }

  void set_message_model(MessageModel messageModel) override {
    dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->set_message_model(messageModel);
  }

  ConsumeFromWhere consume_from_where() const override {
    return dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->consume_from_where();
  }

  void set_consume_from_where(ConsumeFromWhere consumeFromWhere) override {
    dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->set_consume_from_where(consumeFromWhere);
  }

  const std::string& consume_timestamp() const override {
    return dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->consume_timestamp();
  }

  void set_consume_timestamp(const std::string& consumeTimestamp) override {
    dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->set_consume_timestamp(consumeTimestamp);
  }

  int consume_thread_nums() const override {
    return dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->consume_thread_nums();
  }

  void set_consume_thread_nums(int threadNum) override {
    dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->set_consume_thread_nums(threadNum);
  }

  int pull_threshold_for_queue() const override {
    return dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->pull_threshold_for_queue();
  }

  void set_pull_threshold_for_queue(int maxCacheSize) override {
    dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->set_pull_threshold_for_queue(maxCacheSize);
  }

  int consume_message_batch_max_size() const override {
    return dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->consume_message_batch_max_size();
  }

  void set_consume_message_batch_max_size(int consumeMessageBatchMaxSize) override {
    dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())
        ->set_consume_message_batch_max_size(consumeMessageBatchMaxSize);
  }

  int pull_batch_size() const override {
    return dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->pull_batch_size();
  }

  void set_pull_batch_size(int pull_batch_size) override {
    dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->set_pull_batch_size(pull_batch_size);
  }

  int max_reconsume_times() const override {
    return dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->max_reconsume_times();
  }

  void set_max_reconsume_times(int maxReconsumeTimes) override {
    dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->set_max_reconsume_times(maxReconsumeTimes);
  }

  long pull_time_delay_millis_when_exception() const override {
    return dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->pull_time_delay_millis_when_exception();
  }

  void set_pull_time_delay_millis_when_exception(long pull_time_delay_millis_when_exception) override {
    dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())
        ->set_pull_time_delay_millis_when_exception(pull_time_delay_millis_when_exception);
  }

  AllocateMQStrategy* allocate_mq_strategy() const override {
    return dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->allocate_mq_strategy();
  }

  void set_allocate_mq_strategy(AllocateMQStrategy* strategy) override {
    dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->set_allocate_mq_strategy(strategy);
  }

  inline DefaultMQPushConsumerConfigPtr real_config() const {
    return std::dynamic_pointer_cast<DefaultMQPushConsumerConfig>(client_config_);
  }
};

}  // namespace rocketmq

#endif  // ROCKETMQ_DEFAULTMQPUSHCONSUMERCONFIGPROXY_H_

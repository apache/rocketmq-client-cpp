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
#ifndef ROCKETMQ_DEFAULTLITEPULLCONSUMERCONFIGPROXY_H__
#define ROCKETMQ_DEFAULTLITEPULLCONSUMERCONFIGPROXY_H__

#include "DefaultLitePullConsumerConfig.h"
#include "MQClientConfigProxy.h"

namespace rocketmq {

class ROCKETMQCLIENT_API DefaultLitePullConsumerConfigProxy : public MQClientConfigProxy,                   // base
                                                              virtual public DefaultLitePullConsumerConfig  // interface
{
 public:
  DefaultLitePullConsumerConfigProxy(DefaultLitePullConsumerConfigPtr consumerConfig)
      : MQClientConfigProxy(consumerConfig) {}
  virtual ~DefaultLitePullConsumerConfigProxy() = default;

  MessageModel message_model() const override {
    return dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())->message_model();
  }

  void set_message_model(MessageModel messageModel) override {
    dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())->set_message_model(messageModel);
  }

  ConsumeFromWhere consume_from_where() const override {
    return dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())->consume_from_where();
  }

  void set_consume_from_where(ConsumeFromWhere consumeFromWhere) override {
    dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())->set_consume_from_where(consumeFromWhere);
  }

  const std::string& consume_timestamp() const override {
    return dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())->consume_timestamp();
  }
  void set_consume_timestamp(const std::string& consumeTimestamp) override {
    dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())->set_consume_timestamp(consumeTimestamp);
  }

  long auto_commit_interval_millis() const override {
    return dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())->auto_commit_interval_millis();
  }

  void set_auto_commit_interval_millis(long auto_commit_interval_millis) override {
    dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())
        ->set_auto_commit_interval_millis(auto_commit_interval_millis);
  }

  int pull_batch_size() const override {
    return dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())->pull_batch_size();
  }

  void set_pull_batch_size(int pull_batch_size) override {
    dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())->set_pull_batch_size(pull_batch_size);
  }

  int pull_thread_nums() const override {
    return dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())->pull_thread_nums();
  }

  void set_pull_thread_nums(int pullThreadNums) override {
    dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())->set_pull_thread_nums(pullThreadNums);
  }

  bool long_polling_enable() const override {
    return dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())->long_polling_enable();
  }

  void set_long_polling_enable(bool long_polling_enable) override {
    dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())->set_long_polling_enable(long_polling_enable);
  }

  long consumer_pull_timeout_millis() const override {
    return dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())->consumer_pull_timeout_millis();
  }

  void set_consumer_pull_timeout_millis(long consumer_pull_timeout_millis) override {
    dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())
        ->set_consumer_pull_timeout_millis(consumer_pull_timeout_millis);
  }

  long consumer_timeout_millis_when_suspend() const override {
    return dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())->consumer_timeout_millis_when_suspend();
  }

  void set_consumer_timeout_millis_when_suspend(long consumer_timeout_millis_when_suspend) override {
    dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())
        ->set_consumer_timeout_millis_when_suspend(consumer_timeout_millis_when_suspend);
  }

  long broker_suspend_max_time_millis() const override {
    return dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())->broker_suspend_max_time_millis();
  }

  void set_broker_suspend_max_time_millis(long broker_suspend_max_time_millis) override {
    dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())
        ->set_broker_suspend_max_time_millis(broker_suspend_max_time_millis);
  }

  long pull_threshold_for_all() const override {
    return dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())->pull_threshold_for_all();
  }

  void set_pull_threshold_for_all(long pull_threshold_for_all) override {
    dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())
        ->set_pull_threshold_for_all(pull_threshold_for_all);
  }

  int pull_threshold_for_queue() const override {
    return dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())->pull_threshold_for_queue();
  }

  void set_pull_threshold_for_queue(int pull_threshold_for_queue) override {
    dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())
        ->set_pull_threshold_for_queue(pull_threshold_for_queue);
  }

  long pull_time_delay_millis_when_exception() const override {
    return dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())->pull_time_delay_millis_when_exception();
  }

  void set_pull_time_delay_millis_when_exception(long pull_time_delay_millis_when_exception) override {
    dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())
        ->set_pull_time_delay_millis_when_exception(pull_time_delay_millis_when_exception);
  }

  long poll_timeout_millis() const override {
    return dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())->poll_timeout_millis();
  }

  void set_poll_timeout_millis(long poll_timeout_millis) override {
    dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())->set_poll_timeout_millis(poll_timeout_millis);
  }

  long topic_metadata_check_interval_millis() const override {
    return dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())->topic_metadata_check_interval_millis();
  }

  void set_topic_metadata_check_interval_millis(long topicMetadataCheckIntervalMillis) override {
    dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())
        ->set_topic_metadata_check_interval_millis(topicMetadataCheckIntervalMillis);
  }

  AllocateMQStrategy* allocate_mq_strategy() const override {
    return dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())->allocate_mq_strategy();
  }

  void set_allocate_mq_strategy(AllocateMQStrategy* strategy) override {
    dynamic_cast<DefaultLitePullConsumerConfig*>(client_config_.get())->set_allocate_mq_strategy(strategy);
  }

  inline DefaultLitePullConsumerConfigPtr real_config() const {
    return std::dynamic_pointer_cast<DefaultLitePullConsumerConfig>(client_config_);
  }
};

}  // namespace rocketmq

#endif  // ROCKETMQ_DEFAULTLITEPULLCONSUMERCONFIGPROXY_H__

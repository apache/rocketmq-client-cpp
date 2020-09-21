
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
#ifndef ROCKETMQ_CONSUMER_DEFAULTMQPUSHCONSUMERCONFIGIMPL_H_
#define ROCKETMQ_CONSUMER_DEFAULTMQPUSHCONSUMERCONFIGIMPL_H_

#include <algorithm>  // std::min
#include <thread>     // std::thread::hardware_concurrency

#include "AllocateMQAveragely.hpp"
#include "DefaultLitePullConsumerConfig.h"
#include "MQClientConfigImpl.hpp"

namespace rocketmq {

/**
 * DefaultLitePullConsumerConfigImpl - implement for DefaultLitePullConsumerConfig
 */
class DefaultLitePullConsumerConfigImpl : virtual public DefaultLitePullConsumerConfig, public MQClientConfigImpl {
 public:
  DefaultLitePullConsumerConfigImpl()
      : message_model_(MessageModel::CLUSTERING),
        consume_from_where_(ConsumeFromWhere::CONSUME_FROM_LAST_OFFSET),
        consume_timestamp_(UtilAll::to_string(UtilAll::currentTimeMillis() - (1000 * 60 * 30))),
        auto_commit_interval_millis_(5 * 1000),
        pull_batch_size_(10),
        pull_thread_nums_(20),
        long_polling_enable_(true),
        consumer_pull_timeout_millis_(1000 * 10),
        consumer_timeout_millis_when_suspend_(1000 * 30),
        broker_suspend_max_time_millis_(1000 * 20),
        pull_threshold_for_all_(10000),
        pull_threshold_for_queue_(1000),
        pull_time_delay_millis_when_exception_(1000),
        poll_timeout_millis_(1000 * 5),
        topic_metadata_check_interval_millis_(30 * 1000),
        allocate_mq_strategy_(new AllocateMQAveragely()) {}
  virtual ~DefaultLitePullConsumerConfigImpl() = default;

  MessageModel message_model() const override { return message_model_; }
  void set_message_model(MessageModel messageModel) override { message_model_ = messageModel; }

  ConsumeFromWhere consume_from_where() const override { return consume_from_where_; }
  void set_consume_from_where(ConsumeFromWhere consumeFromWhere) override { consume_from_where_ = consumeFromWhere; }

  const std::string& consume_timestamp() const override { return consume_timestamp_; }
  void set_consume_timestamp(const std::string& consumeTimestamp) override { consume_timestamp_ = consumeTimestamp; }

  long auto_commit_interval_millis() const override { return auto_commit_interval_millis_; }
  void set_auto_commit_interval_millis(long auto_commit_interval_millis) override {
    auto_commit_interval_millis_ = auto_commit_interval_millis;
  }

  int pull_batch_size() const override { return pull_batch_size_; }
  void set_pull_batch_size(int pull_batch_size) override { pull_batch_size_ = pull_batch_size; }

  int pull_thread_nums() const override { return pull_thread_nums_; }
  void set_pull_thread_nums(int pullThreadNums) override { pull_thread_nums_ = pullThreadNums; }

  bool long_polling_enable() const override { return long_polling_enable_; }
  void set_long_polling_enable(bool long_polling_enable) override { long_polling_enable_ = long_polling_enable; }

  long consumer_pull_timeout_millis() const override { return consumer_pull_timeout_millis_; }
  void set_consumer_pull_timeout_millis(long consumer_pull_timeout_millis) override {
    consumer_pull_timeout_millis_ = consumer_pull_timeout_millis;
  }

  long consumer_timeout_millis_when_suspend() const override { return consumer_timeout_millis_when_suspend_; }
  void set_consumer_timeout_millis_when_suspend(long consumer_timeout_millis_when_suspend) override {
    consumer_timeout_millis_when_suspend_ = consumer_timeout_millis_when_suspend;
  }

  long broker_suspend_max_time_millis() const override { return broker_suspend_max_time_millis_; }
  void set_broker_suspend_max_time_millis(long broker_suspend_max_time_millis) override {
    broker_suspend_max_time_millis_ = broker_suspend_max_time_millis;
  }

  long pull_threshold_for_all() const override { return pull_threshold_for_all_; }
  void set_pull_threshold_for_all(long pull_threshold_for_all) override {
    pull_threshold_for_all_ = pull_threshold_for_all;
  }

  int pull_threshold_for_queue() const override { return pull_threshold_for_queue_; }
  void set_pull_threshold_for_queue(int pull_threshold_for_queue) override {
    pull_threshold_for_queue_ = pull_threshold_for_queue;
  }

  long pull_time_delay_millis_when_exception() const override { return pull_time_delay_millis_when_exception_; }
  void set_pull_time_delay_millis_when_exception(long pull_time_delay_millis_when_exception) override {
    pull_time_delay_millis_when_exception_ = pull_time_delay_millis_when_exception;
  }

  long poll_timeout_millis() const override { return poll_timeout_millis_; }
  void set_poll_timeout_millis(long poll_timeout_millis) override { poll_timeout_millis_ = poll_timeout_millis; }

  long topic_metadata_check_interval_millis() const override { return topic_metadata_check_interval_millis_; }
  void set_topic_metadata_check_interval_millis(long topicMetadataCheckIntervalMillis) override {
    topic_metadata_check_interval_millis_ = topicMetadataCheckIntervalMillis;
  }

  AllocateMQStrategy* allocate_mq_strategy() const override { return allocate_mq_strategy_.get(); }
  void set_allocate_mq_strategy(AllocateMQStrategy* strategy) override { allocate_mq_strategy_.reset(strategy); }

 private:
  MessageModel message_model_;

  ConsumeFromWhere consume_from_where_;
  std::string consume_timestamp_;

  long auto_commit_interval_millis_;

  int pull_batch_size_;

  int pull_thread_nums_;

  bool long_polling_enable_;

  long consumer_pull_timeout_millis_;
  long consumer_timeout_millis_when_suspend_;
  long broker_suspend_max_time_millis_;

  long pull_threshold_for_all_;
  int pull_threshold_for_queue_;

  long pull_time_delay_millis_when_exception_;  // 1000

  long poll_timeout_millis_;

  long topic_metadata_check_interval_millis_;

  std::unique_ptr<AllocateMQStrategy> allocate_mq_strategy_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_CONSUMER_DEFAULTMQPUSHCONSUMERCONFIGIMPL_H_

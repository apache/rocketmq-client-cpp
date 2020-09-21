
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
#ifndef ROCKETMQ_CONSUMER_DEFAULTMQPUSHCONSUMERCONFIGIMPL_HPP_
#define ROCKETMQ_CONSUMER_DEFAULTMQPUSHCONSUMERCONFIGIMPL_HPP_

#include <algorithm>  // std::min
#include <thread>     // std::thread::hardware_concurrency

#include "AllocateMQAveragely.hpp"
#include "DefaultMQPushConsumerConfig.h"
#include "MQClientConfigImpl.hpp"

namespace rocketmq {

/**
 * DefaultMQPushConsumerConfigImpl - implement for DefaultMQPushConsumerConfig
 */
class DefaultMQPushConsumerConfigImpl : virtual public DefaultMQPushConsumerConfig, public MQClientConfigImpl {
 public:
  DefaultMQPushConsumerConfigImpl()
      : message_model_(MessageModel::CLUSTERING),
        consume_from_where_(ConsumeFromWhere::CONSUME_FROM_LAST_OFFSET),
        consume_timestamp_("0"),
        consume_thread_nums_(std::min(8, (int)std::thread::hardware_concurrency())),
        pull_threshold_for_queue_(1000),
        consume_message_batch_max_size_(1),
        pull_batch_size_(32),
        max_reconsume_times_(16),
        pull_time_delay_millis_when_exception_(3000),
        allocate_mq_strategy_(new AllocateMQAveragely()) {}
  virtual ~DefaultMQPushConsumerConfigImpl() = default;

  MessageModel message_model() const override { return message_model_; }
  void set_message_model(MessageModel messageModel) override { message_model_ = messageModel; }

  ConsumeFromWhere consume_from_where() const override { return consume_from_where_; }
  void set_consume_from_where(ConsumeFromWhere consumeFromWhere) override { consume_from_where_ = consumeFromWhere; }

  const std::string& consume_timestamp() const override { return consume_timestamp_; }
  void set_consume_timestamp(const std::string& consumeTimestamp) override { consume_timestamp_ = consumeTimestamp; }

  int consume_thread_nums() const override { return consume_thread_nums_; }
  void set_consume_thread_nums(int threadNum) override {
    if (threadNum > 0) {
      consume_thread_nums_ = threadNum;
    }
  }

  int pull_threshold_for_queue() const override { return pull_threshold_for_queue_; }
  void set_pull_threshold_for_queue(int maxCacheSize) override { pull_threshold_for_queue_ = maxCacheSize; }

  int consume_message_batch_max_size() const override { return consume_message_batch_max_size_; }
  void set_consume_message_batch_max_size(int consumeMessageBatchMaxSize) override {
    if (consumeMessageBatchMaxSize >= 1) {
      consume_message_batch_max_size_ = consumeMessageBatchMaxSize;
    }
  }

  int pull_batch_size() const override { return pull_batch_size_; }
  void set_pull_batch_size(int pull_batch_size) override { pull_batch_size_ = pull_batch_size; }

  int max_reconsume_times() const override { return max_reconsume_times_; }
  void set_max_reconsume_times(int maxReconsumeTimes) override { max_reconsume_times_ = maxReconsumeTimes; }

  long pull_time_delay_millis_when_exception() const override { return pull_time_delay_millis_when_exception_; }
  void set_pull_time_delay_millis_when_exception(long pull_time_delay_millis_when_exception) override {
    pull_time_delay_millis_when_exception_ = pull_time_delay_millis_when_exception;
  }

  AllocateMQStrategy* allocate_mq_strategy() const override { return allocate_mq_strategy_.get(); }
  void set_allocate_mq_strategy(AllocateMQStrategy* strategy) override { allocate_mq_strategy_.reset(strategy); }

 protected:
  MessageModel message_model_;

  ConsumeFromWhere consume_from_where_;
  std::string consume_timestamp_;

  int consume_thread_nums_;

  int pull_threshold_for_queue_;

  int consume_message_batch_max_size_;  // 1
  int pull_batch_size_;                 // 32

  int max_reconsume_times_;

  long pull_time_delay_millis_when_exception_;  // 3000

  std::unique_ptr<AllocateMQStrategy> allocate_mq_strategy_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_CONSUMER_DEFAULTMQPUSHCONSUMERCONFIGIMPL_HPP_
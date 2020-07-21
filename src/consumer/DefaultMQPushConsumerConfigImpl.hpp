
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
        consume_thread_num_(std::min(8, (int)std::thread::hardware_concurrency())),
        consume_message_batch_max_size_(1),
        max_msg_cache_size_(1000),
        async_pull_timeout_(30 * 1000),
        max_reconsume_times_(-1),
        pull_time_delay_mills_when_exception_(3000),
        allocate_mq_strategy_(new AllocateMQAveragely()) {}
  virtual ~DefaultMQPushConsumerConfigImpl() = default;

  MessageModel getMessageModel() const override { return message_model_; }
  void setMessageModel(MessageModel messageModel) override { message_model_ = messageModel; }

  ConsumeFromWhere getConsumeFromWhere() const override { return consume_from_where_; }
  void setConsumeFromWhere(ConsumeFromWhere consumeFromWhere) override { consume_from_where_ = consumeFromWhere; }

  const std::string& getConsumeTimestamp() const override { return consume_timestamp_; }
  void setConsumeTimestamp(const std::string& consumeTimestamp) override { consume_timestamp_ = consumeTimestamp; }

  int getConsumeThreadNum() const override { return consume_thread_num_; }
  void setConsumeThreadNum(int threadNum) override {
    if (threadNum > 0) {
      consume_thread_num_ = threadNum;
    }
  }

  int getConsumeMessageBatchMaxSize() const override { return consume_message_batch_max_size_; }
  void setConsumeMessageBatchMaxSize(int consumeMessageBatchMaxSize) override {
    if (consumeMessageBatchMaxSize >= 1) {
      consume_message_batch_max_size_ = consumeMessageBatchMaxSize;
    }
  }

  int getMaxCacheMsgSizePerQueue() const override { return max_msg_cache_size_; }
  void setMaxCacheMsgSizePerQueue(int maxCacheSize) override {
    if (maxCacheSize > 0 && maxCacheSize < 65535) {
      max_msg_cache_size_ = maxCacheSize;
    }
  }

  int getAsyncPullTimeout() const override { return async_pull_timeout_; }
  void setAsyncPullTimeout(int asyncPullTimeout) override { async_pull_timeout_ = asyncPullTimeout; }

  int getMaxReconsumeTimes() const override { return max_reconsume_times_; }
  void setMaxReconsumeTimes(int maxReconsumeTimes) override { max_reconsume_times_ = maxReconsumeTimes; }

  long getPullTimeDelayMillsWhenException() const override { return pull_time_delay_mills_when_exception_; }
  void setPullTimeDelayMillsWhenException(long pullTimeDelayMillsWhenException) override {
    pull_time_delay_mills_when_exception_ = pullTimeDelayMillsWhenException;
  }

  AllocateMQStrategy* getAllocateMQStrategy() const override { return allocate_mq_strategy_.get(); }
  void setAllocateMQStrategy(AllocateMQStrategy* strategy) override { allocate_mq_strategy_.reset(strategy); }

 protected:
  MessageModel message_model_;

  ConsumeFromWhere consume_from_where_;
  std::string consume_timestamp_;

  int consume_thread_num_;
  int consume_message_batch_max_size_;
  int max_msg_cache_size_;

  int async_pull_timeout_;  // 30s
  int max_reconsume_times_;

  long pull_time_delay_mills_when_exception_;  // 3000

  std::unique_ptr<AllocateMQStrategy> allocate_mq_strategy_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_CONSUMER_DEFAULTMQPUSHCONSUMERCONFIGIMPL_HPP_
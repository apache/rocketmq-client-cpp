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
#ifndef ROCKETMQ_PRODUCER_TOPICPUBLISHINFO_HPP_
#define ROCKETMQ_PRODUCER_TOPICPUBLISHINFO_HPP_

#include <atomic>  // std::atomic
#include <memory>  // std::shared_ptr
#include <mutex>   // std::mutex

#include "MQException.h"
#include "MQMessageQueue.h"
#include "protocol/body/TopicRouteData.hpp"

namespace rocketmq {

class TopicPublishInfo;
typedef std::shared_ptr<const TopicPublishInfo> TopicPublishInfoPtr;

class TopicPublishInfo {
 public:
  typedef std::vector<MQMessageQueue> QueuesVec;

 public:
  TopicPublishInfo() : order_topic_(false), have_topic_router_info_(false), send_which_queue_(0) {}

  virtual ~TopicPublishInfo() = default;

  bool isOrderTopic() const { return order_topic_; }

  void setOrderTopic(bool orderTopic) { order_topic_ = orderTopic; }

  bool ok() const { return !message_queue_list_.empty(); }

  const QueuesVec& getMessageQueueList() const { return message_queue_list_; }

  std::atomic<std::size_t>& getSendWhichQueue() const { return send_which_queue_; }

  bool isHaveTopicRouterInfo() { return have_topic_router_info_; }

  void setHaveTopicRouterInfo(bool haveTopicRouterInfo) { have_topic_router_info_ = haveTopicRouterInfo; }

  const MQMessageQueue& selectOneMessageQueue(const std::string& lastBrokerName) const {
    if (!lastBrokerName.empty()) {
      auto mqSize = message_queue_list_.size();
      if (mqSize <= 1) {
        if (mqSize == 0) {
          LOG_ERROR_NEW("[BUG] messageQueueList is empty");
          THROW_MQEXCEPTION(MQClientException, "messageQueueList is empty", -1);
        }
        return message_queue_list_[0];
      } else {
        // NOTE: If it possible, mq in same broker is nonadjacent.
        auto index = send_which_queue_.fetch_add(1);
        for (size_t i = 0; i < 2; i++) {
          auto pos = index++ % message_queue_list_.size();
          auto& mq = message_queue_list_[pos];
          if (mq.broker_name() != lastBrokerName) {
            return mq;
          }
        }
        return message_queue_list_[(index - 2) % message_queue_list_.size()];
      }
    }
    return selectOneMessageQueue();
  }

  const MQMessageQueue& selectOneMessageQueue() const {
    auto index = send_which_queue_.fetch_add(1);
    auto pos = index % message_queue_list_.size();
    return message_queue_list_[pos];
  }

  int getQueueIdByBroker(const std::string& brokerName) const {
    for (const auto& queueData : topic_route_data_->queue_datas()) {
      if (queueData.broker_name() == brokerName) {
        return queueData.write_queue_nums();
      }
    }

    return -1;
  }

  void setTopicRouteData(TopicRouteDataPtr topicRouteData) { topic_route_data_ = topicRouteData; }

 private:
  bool order_topic_;
  bool have_topic_router_info_;

  QueuesVec message_queue_list_;  // no change after build
  mutable std::atomic<std::size_t> send_which_queue_;

  TopicRouteDataPtr topic_route_data_;  // no change after set
};

}  // namespace rocketmq

#endif  // ROCKETMQ_PRODUCER_TOPICPUBLISHINFO_HPP_

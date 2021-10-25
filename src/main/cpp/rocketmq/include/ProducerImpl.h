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
#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <system_error>

#include "absl/strings/string_view.h"

#include "ClientImpl.h"
#include "ClientManagerImpl.h"
#include "MixAll.h"
#include "SendCallbacks.h"
#include "TopicPublishInfo.h"
#include "TransactionImpl.h"
#include "rocketmq/AsyncCallback.h"
#include "rocketmq/LocalTransactionStateChecker.h"
#include "rocketmq/MQMessage.h"
#include "rocketmq/MQMessageQueue.h"
#include "rocketmq/MQSelector.h"
#include "rocketmq/SendResult.h"
#include "rocketmq/State.h"

ROCKETMQ_NAMESPACE_BEGIN

class ProducerImpl : virtual public ClientImpl, public std::enable_shared_from_this<ProducerImpl> {
public:
  explicit ProducerImpl(absl::string_view group_name);

  ~ProducerImpl() override;

  void prepareHeartbeatData(HeartbeatRequest& request) override;

  void start() override;

  void shutdown() override;

  SendResult send(const MQMessage& message, std::error_code& ec) noexcept;

  void send(const MQMessage& message, SendCallback* callback);

  void sendOneway(const MQMessage& message, std::error_code& ec);

  void setLocalTransactionStateChecker(LocalTransactionStateCheckerPtr checker);

  std::unique_ptr<TransactionImpl> prepare(MQMessage& message, std::error_code& ec);

  bool commit(const MQMessage& message, const std::string& transaction_id, const std::string& target);

  bool rollback(const MQMessage& message, const std::string& transaction_id, const std::string& target);

  /**
   * Check if the RPC client for the target host is isolated or not
   * @param endpoint Address of target host.
   * @return true if client is active; false otherwise.
   */
  bool isEndpointIsolated(const std::string& endpoint) LOCKS_EXCLUDED(isolated_endpoints_mtx_);

  /**
   * Note: This function is purpose-made public such that the whole isolate/add-back mechanism can be properly tested.
   * @param target Endpoint of the target host
   */
  void isolateEndpoint(const std::string& target) LOCKS_EXCLUDED(isolated_endpoints_mtx_);

  int maxAttemptTimes() const {
    return max_attempt_times_;
  }

  void maxAttemptTimes(int times) {
    max_attempt_times_ = times;
  }

  int getFailedTimes() const {
    return failed_times_;
  }

  void setFailedTimes(int times) {
    failed_times_ = times;
  }

  std::vector<MQMessageQueue> listMessageQueue(const std::string& topic, std::error_code& ec);

  uint32_t compressBodyThreshold() const {
    return compress_body_threshold_;
  }

  void compressBodyThreshold(uint32_t threshold) {
    compress_body_threshold_ = threshold;
  }

  /**
   * @brief Send message with tracing.
   *
   * @param message
   * @param callback
   * @param message_queue
   * @param attempt_time current attempt times, which starts from 0.
   */
  void sendImpl(RetrySendCallback* callback);

protected:
  std::shared_ptr<ClientImpl> self() override {
    return shared_from_this();
  }

  void resolveOrphanedTransactionalMessage(const std::string& transaction_id, const MQMessageExt& message) override;

  void notifyClientTermination() override;

private:
  absl::flat_hash_map<std::string, TopicPublishInfoPtr> topic_publish_info_table_ GUARDED_BY(topic_publish_info_mtx_);
  absl::Mutex topic_publish_info_mtx_; // protects topic_publish_info_

  int32_t max_attempt_times_{MixAll::MAX_SEND_MESSAGE_ATTEMPT_TIMES_};
  int32_t failed_times_{0}; // only for test
  uint32_t compress_body_threshold_;

  LocalTransactionStateCheckerPtr transaction_state_checker_;

  void asyncPublishInfo(const std::string& topic,
                        const std::function<void(const std::error_code&, const TopicPublishInfoPtr&)>& cb)
      LOCKS_EXCLUDED(topic_publish_info_mtx_);

  TopicPublishInfoPtr getPublishInfo(const std::string& topic);

  void takeMessageQueuesRoundRobin(const TopicPublishInfoPtr& publish_info, std::vector<MQMessageQueue>& message_queues,
                                   int number);

  void wrapSendMessageRequest(const MQMessage& message, SendMessageRequest& request,
                              const MQMessageQueue& message_queue);

  bool isRunning() const;

  void ensureRunning(std::error_code& ec) const noexcept;

  bool validate(const MQMessage& message);

  void send0(const MQMessage& message, SendCallback* callback, std::vector<MQMessageQueue> list, int max_attempt_times);

  bool endTransaction0(const std::string& target, const MQMessage& message, const std::string& transaction_id,
                       TransactionState resolution);

  void isolatedEndpoints(absl::flat_hash_set<std::string>& endpoints) LOCKS_EXCLUDED(isolated_endpoints_mtx_);

  MQMessageQueue withServiceAddress(const MQMessageQueue& message_queue, std::error_code& ec);
};

ROCKETMQ_NAMESPACE_END
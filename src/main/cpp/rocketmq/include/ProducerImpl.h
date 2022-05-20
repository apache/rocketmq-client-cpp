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

#include "ClientImpl.h"
#include "ClientManagerImpl.h"
#include "MixAll.h"
#include "PublishInfoCallback.h"
#include "SendContext.h"
#include "TopicPublishInfo.h"
#include "TransactionImpl.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/strings/string_view.h"
#include "rocketmq/Message.h"
#include "rocketmq/SendCallback.h"
#include "rocketmq/SendReceipt.h"
#include "rocketmq/State.h"
#include "rocketmq/TransactionChecker.h"

ROCKETMQ_NAMESPACE_BEGIN

class ProducerImpl : virtual public ClientImpl, public std::enable_shared_from_this<ProducerImpl> {
public:
  explicit ProducerImpl();

  ~ProducerImpl() override;

  void prepareHeartbeatData(HeartbeatRequest& request) override;

  void start() override;

  void shutdown() override;

  SendReceipt send(MessageConstPtr message, std::error_code& ec) noexcept;

  void send(MessageConstPtr message, SendCallback callback);

  void setTransactionChecker(TransactionChecker checker);

  std::unique_ptr<TransactionImpl> prepare(MessageConstPtr message, std::error_code& ec);

  bool commit(const Transaction& transaction);

  bool rollback(const Transaction& transaction);

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

  std::size_t maxAttemptTimes() const {
    return max_attempt_times_;
  }

  void maxAttemptTimes(std::size_t times) {
    max_attempt_times_ = times;
  }

  int getFailedTimes() const {
    return failed_times_;
  }

  void setFailedTimes(int times) {
    failed_times_ = times;
  }

  uint32_t compressBodyThreshold() const {
    return compress_body_threshold_;
  }

  void compressBodyThreshold(uint32_t threshold) {
    compress_body_threshold_ = threshold;
  }

  void sendImpl(std::shared_ptr<SendContext> callback);

  void buildClientSettings(rmq::Settings& settings) override;

  void topicsOfInterest(std::vector<std::string> topics) LOCKS_EXCLUDED(topics_mtx_);

protected:
  std::shared_ptr<ClientImpl> self() override {
    return shared_from_this();
  }

  void onOrphanedTransactionalMessage(MessageConstSharedPtr message) override;

  void notifyClientTermination() override;

private:
  absl::flat_hash_map<std::string, TopicPublishInfoPtr> topic_publish_info_table_ GUARDED_BY(topic_publish_info_mtx_);
  absl::Mutex topic_publish_info_mtx_;  // protects topic_publish_info_
  std::size_t max_attempt_times_{MixAll::MAX_SEND_MESSAGE_ATTEMPT_TIMES_};
  int32_t failed_times_{0};  // only for test
  uint32_t compress_body_threshold_;
  TransactionChecker transaction_checker_;
  std::vector<std::string> topics_ GUARDED_BY(topics_mtx_);
  absl::Mutex topics_mtx_;

  /**
   * @brief Acquire PublishInfo for the given topic.
   * Generally speaking, it first checks presence of the desired info in local cache, aka, topic_publish_table_;
   * If not found, query name servers.
   */
  void getPublishInfoAsync(const std::string& topic, const PublishInfoCallback& cb)
      LOCKS_EXCLUDED(topic_publish_info_mtx_);

  void cachePublishInfo(const std::string&, TopicPublishInfoPtr info) LOCKS_EXCLUDED(topic_publish_info_mtx_);

  TopicPublishInfoPtr getPublishInfo(const std::string& topic);

  void wrapSendMessageRequest(const Message& message,
                              SendMessageRequest& request,
                              const rmq::MessageQueue& message_queue);

  bool isRunning() const;

  void ensureRunning(std::error_code& ec) const noexcept;

  bool validate(const Message& message);

  void send0(MessageConstPtr message, SendCallback callback, std::vector<rmq::MessageQueue> list);

  bool endTransaction0(const Transaction& transaction, TransactionState resolution);

  void isolatedEndpoints(absl::flat_hash_set<std::string>& endpoints) LOCKS_EXCLUDED(isolated_endpoints_mtx_);

  friend class ProducerBuilder;
};

ROCKETMQ_NAMESPACE_END
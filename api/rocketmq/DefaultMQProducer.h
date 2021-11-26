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
#include <system_error>
#include <vector>

#include "AsyncCallback.h"
#include "CredentialsProvider.h"
#include "ErrorCode.h"
#include "LocalTransactionStateChecker.h"
#include "Logger.h"
#include "MQMessage.h"
#include "MQSelector.h"
#include "SendResult.h"
#include "rocketmq/ProtocolType.h"

ROCKETMQ_NAMESPACE_BEGIN

/**
 * This class employs pointer-to-implementation paradigm to achieve the goal of stable ABI.
 * Refer https://en.cppreference.com/w/cpp/language/pimpl for an explanation.
 */
class ProducerImpl;

class DefaultMQProducer {
public:
  explicit DefaultMQProducer(const std::string& group_name);

  ~DefaultMQProducer() = default;

  void start();

  void shutdown();

  /**
   * Acquire previous send timeout in milliseconds.
   * @return Send timeout in milliseconds
   */
  std::chrono::milliseconds getSendMsgTimeout() const;

  /**
   * Set default send message timeout in milliseconds.
   * @param timeout_millis Timeout used when sending messages.
   */
  void setSendMsgTimeout(std::chrono::milliseconds timeout);

  SendResult send(MQMessage& message, const std::string& message_group);

  /**
   * Send message in synchronous manner.
   * @param message  Message to send.
   * @param filter_active_broker Do NOT rely on this parameter. it has been deprecated.
   */
  SendResult send(const MQMessage& message, bool filter_active_broker = true);

  SendResult send(const MQMessage& message, std::error_code& ec) noexcept;

  SendResult send(MQMessage& message, const MQMessageQueue& message_queue);
  SendResult send(MQMessage& message, MessageQueueSelector* selector, void* arg);

  /**
   * @brief This function is deprecated.
   *
   * @param message
   * @param selector
   * @param arg
   * @param retry_times retry_times is ignored with respect to member retry_times setting.
   * @param select_active_broker
   * @return SendResult
   */
  SendResult send(MQMessage& message, MessageQueueSelector* selector, void* arg, int retry_times,
                  bool select_active_broker = false);

  /**
   * Send message in asynchronous manner.
   * @param message  Message to send.
   * @param send_callback Callback to execute on completion of message sending.
   * @param select_active_broker Do NOT rely on this parameter. it has been deprecated.
   */
  void send(const MQMessage& message, SendCallback* send_callback, bool select_active_broker = false);
  void send(MQMessage& message, const MQMessageQueue& message_queue, SendCallback* send_callback);
  void send(MQMessage& message, MessageQueueSelector* selector, void* arg, SendCallback* send_callback);

  /**
   * send message in Oneway(The implementation is simply ignore the result of send message in synchronous).
   * @param message  Message to send.
   * @param select_active_broker Do NOT rely on this parameter. it has been deprecated.
   */
  void sendOneway(const MQMessage& message, bool select_active_broker = false);
  void sendOneway(MQMessage& message, const MQMessageQueue& message_queue);
  void sendOneway(MQMessage& message, MessageQueueSelector* selector, void* arg);

  void setLocalTransactionStateChecker(LocalTransactionStateCheckerPtr checker);

  void setNamesrvAddr(const std::string& name_server_address_list);

  void setNameServerListDiscoveryEndpoint(const std::string& discovery_endpoint);

  void setGroupName(const std::string& group_name);

  void setInstanceName(const std::string& instance_name);

  void enableTracing(bool enabled);

  bool isTracingEnabled();

  /**
   * Number of attempts before claiming a send action as failure. By default, 3 attempts will be performed for sync and
   * async send methods.
   *
   * @return
   */
  int getMaxAttemptTimes() const;

  void setMaxAttemptTimes(int max_attempt_times);

  std::vector<MQMessageQueue> getTopicMessageQueueInfo(const std::string& topic);

  void setUnitName(std::string unit_name);

  const std::string& getUnitName();

  uint32_t compressBodyThreshold() const;
  void compressBodyThreshold(uint32_t threshold);

  void setResourceNamespace(const std::string& resource_namespace);

  void setCredentialsProvider(CredentialsProviderPtr credentials_provider);

  void setRegion(const std::string& region);

  void setProtocolType(ProtocolType type);

  TransactionPtr prepare(MQMessage& message);

private:
  std::shared_ptr<ProducerImpl> impl_;
};

ROCKETMQ_NAMESPACE_END
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

#include <memory>
#include <string>

#include "AsyncCallback.h"
#include "ConsumeType.h"
#include "CredentialsProvider.h"
#include "ExpressionType.h"
#include "Logger.h"
#include "MQMessageQueue.h"
#include "MessageListener.h"
#include "rocketmq/Executor.h"
#include "rocketmq/MessageModel.h"

ROCKETMQ_NAMESPACE_BEGIN

class PushConsumerImpl;

class DefaultMQPushConsumer {
public:
  explicit DefaultMQPushConsumer(const std::string& group_name);

  ~DefaultMQPushConsumer() = default;

  void start();

  void shutdown();

  void subscribe(const std::string& topic, const std::string& expression,
                 ExpressionType expression_type = ExpressionType::TAG);

  void setConsumeFromWhere(ConsumeFromWhere policy);

  void registerMessageListener(MessageListener* listener);

  void setNamesrvAddr(const std::string& name_srv);

  void setNameServerListDiscoveryEndpoint(const std::string& discovery_endpoint);

  void setGroupName(const std::string& group_name);

  void setConsumeThreadCount(int thread_count);

  void setInstanceName(const std::string& instance_name);

  int getProcessQueueTableSize();

  void setUnitName(std::string unit_name);

  const std::string& getUnitName() const;

  void enableTracing(bool enabled);

  bool isTracingEnabled();

  /**
   * SDK of this version always uses asynchronous IO operation. As such, this
   * function is no-op to keep backward compatibility.
   */
  void setAsyncPull(bool);

  /**
   * Maximum number of messages passed to each callback.
   * @param batch_size Batch size
   */
  void setConsumeMessageBatchMaxSize(int batch_size);

  /**
   * Lifecycle of executor is managed by external application. Passed-in
   * executor should remain valid after consumer start and before stopping.
   * @param executor Executor pool used to invoke consume callback.
   */
  void setCustomExecutor(const Executor& executor);

  /**
   * This function sets maximum number of message that may be consumed per
   * second.
   * @param topic Topic to control
   * @param threshold Threshold before throttling is enforced.
   */
  void setThrottle(const std::string& topic, uint32_t threshold);

  /**
   * Set abstract-resource-namespace, in which canonical name of topic, group
   * remains unique.
   * @param resource_namespace Abstract resource namespace.
   */
  void setResourceNamespace(const std::string& resource_namespace);

  void setCredentialsProvider(CredentialsProviderPtr credentials_provider);

  void setMessageModel(MessageModel message_model);

  std::string groupName() const;

private:
  std::shared_ptr<PushConsumerImpl> impl_;
};

ROCKETMQ_NAMESPACE_END
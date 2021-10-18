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
#include <future>
#include <memory>
#include <string>
#include <vector>

#include "AsyncCallback.h"
#include "ConsumeType.h"
#include "CredentialsProvider.h"
#include "MQMessageExt.h"
#include "MQMessageQueue.h"

ROCKETMQ_NAMESPACE_BEGIN

class PullConsumerImpl;

class DefaultMQPullConsumer {
public:
  explicit DefaultMQPullConsumer(const std::string& group_name);

  void start();

  void shutdown();

  std::future<std::vector<MQMessageQueue>> queuesFor(const std::string& topic);

  std::future<int64_t> queryOffset(const OffsetQuery& query);

  bool pull(const PullMessageQuery& request, PullResult& pull_result);

  void pull(const PullMessageQuery& request, PullCallback* callback);

  void setResourceNamespace(const std::string& resource_namespace);

  void setNamesrvAddr(const std::string& name_srv);

  void setNameServerListDiscoveryEndpoint(const std::string& discovery_endpoint);

  void setCredentialsProvider(std::shared_ptr<CredentialsProvider> credentials_provider);

private:
  std::shared_ptr<PullConsumerImpl> impl_;
};

ROCKETMQ_NAMESPACE_END

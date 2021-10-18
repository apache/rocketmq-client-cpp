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

#include <future>
#include <memory>

#include "absl/strings/string_view.h"

#include "ClientConfig.h"
#include "ClientImpl.h"
#include "ClientManagerImpl.h"
#include "rocketmq/ConsumeType.h"
#include "rocketmq/MQMessageQueue.h"
#include "rocketmq/MessageModel.h"

ROCKETMQ_NAMESPACE_BEGIN

class PullConsumerImpl : public ClientImpl, public std::enable_shared_from_this<PullConsumerImpl> {
public:
  explicit PullConsumerImpl(absl::string_view group_name) : ClientImpl(group_name) {
  }

  void start() override;

  void shutdown() override;

  std::future<std::vector<MQMessageQueue>> queuesFor(const std::string& topic);

  std::future<int64_t> queryOffset(const OffsetQuery& query);

  void pull(const PullMessageQuery& query, PullCallback* callback);

  void prepareHeartbeatData(HeartbeatRequest& request) override;

protected:
  std::shared_ptr<ClientImpl> self() override {
    return shared_from_this();
  }

  void notifyClientTermination() override;

  MessageModel message_model_{MessageModel::CLUSTERING};
};

ROCKETMQ_NAMESPACE_END
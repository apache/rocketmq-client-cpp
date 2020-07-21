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
#ifndef ROCKETMQ_CONSUMER_PULLMESSAGESERVICE_HPP_
#define ROCKETMQ_CONSUMER_PULLMESSAGESERVICE_HPP_

#include "concurrent/executor.hpp"
#include "DefaultMQPushConsumerImpl.h"
#include "Logging.h"
#include "MQClientInstance.h"
#include "PullRequest.h"

namespace rocketmq {

class PullMessageService {
 public:
  PullMessageService(MQClientInstance* instance)
      : client_instance_(instance), scheduled_executor_service_(getServiceName(), 1, false) {}

  void start() { scheduled_executor_service_.startup(); }

  void shutdown() { scheduled_executor_service_.shutdown(); }

  void executePullRequestLater(PullRequestPtr pullRequest, long timeDelay) {
    if (client_instance_->isRunning()) {
      scheduled_executor_service_.schedule(
          std::bind(&PullMessageService::executePullRequestImmediately, this, pullRequest), timeDelay,
          time_unit::milliseconds);
    } else {
      LOG_WARN_NEW("PullMessageServiceScheduledThread has shutdown");
    }
  }

  void executePullRequestImmediately(PullRequestPtr pullRequest) {
    scheduled_executor_service_.submit(std::bind(&PullMessageService::pullMessage, this, pullRequest));
  }

  void executeTaskLater(const handler_type& task, long timeDelay) {
    scheduled_executor_service_.schedule(task, timeDelay, time_unit::milliseconds);
  }

  std::string getServiceName() { return "PullMessageService"; }

 private:
  void pullMessage(PullRequestPtr pullRequest) {
    MQConsumerInner* consumer = client_instance_->selectConsumer(pullRequest->consumer_group());
    if (consumer != nullptr &&
        std::type_index(typeid(*consumer)) == std::type_index(typeid(DefaultMQPushConsumerImpl))) {
      auto* impl = static_cast<DefaultMQPushConsumerImpl*>(consumer);
      impl->pullMessage(pullRequest);
    } else {
      LOG_WARN_NEW("No matched consumer for the PullRequest {}, drop it", pullRequest->toString());
    }
  }

 private:
  MQClientInstance* client_instance_;
  scheduled_thread_pool_executor scheduled_executor_service_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_CONSUMER_PULLMESSAGESERVICE_HPP_

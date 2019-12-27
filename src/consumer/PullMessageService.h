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
#ifndef __PULL_MESSAGE_SERVICE_H__
#define __PULL_MESSAGE_SERVICE_H__

#include "concurrent/executor.hpp"

#include "DefaultMQPushConsumer.h"
#include "Logging.h"
#include "MQClientInstance.h"
#include "PullRequest.h"

namespace rocketmq {

class PullMessageService {
 public:
  PullMessageService(MQClientInstance* instance)
      : m_clientInstance(instance), m_scheduledExecutorService(getServiceName(), 1, false) {}

  void start() { m_scheduledExecutorService.startup(); }

  void shutdown() { m_scheduledExecutorService.shutdown(); }

  void executePullRequestLater(PullRequestPtr pullRequest, long timeDelay) {
    if (m_clientInstance->isRunning()) {
      m_scheduledExecutorService.schedule(
          std::bind(&PullMessageService::executePullRequestImmediately, this, pullRequest), timeDelay,
          time_unit::milliseconds);
    } else {
      LOG_WARN_NEW("PullMessageServiceScheduledThread has shutdown");
    }
  }

  void executePullRequestImmediately(PullRequestPtr pullRequest) {
    m_scheduledExecutorService.submit(std::bind(&PullMessageService::pullMessage, this, pullRequest));
  }

  void executeTaskLater(const handler_type& task, long timeDelay) {
    m_scheduledExecutorService.schedule(task, timeDelay, time_unit::milliseconds);
  }

  std::string getServiceName() { return "PullMessageService"; }

 private:
  void pullMessage(PullRequestPtr pullRequest) {
    MQConsumer* consumer = m_clientInstance->selectConsumer(pullRequest->getConsumerGroup());
    if (consumer != nullptr && std::type_index(typeid(*consumer)) == std::type_index(typeid(DefaultMQPushConsumer))) {
      auto* impl = static_cast<DefaultMQPushConsumer*>(consumer);
      impl->pullMessage(pullRequest);
    } else {
      LOG_WARN_NEW("No matched consumer for the PullRequest {}, drop it", pullRequest->toString());
    }
  }

 private:
  MQClientInstance* m_clientInstance;
  scheduled_thread_pool_executor m_scheduledExecutorService;
};

}  // namespace rocketmq

#endif  // __PULL_MESSAGE_SERVICE_H__

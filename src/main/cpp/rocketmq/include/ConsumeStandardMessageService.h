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

#include "ConsumeMessageServiceBase.h"
#include "ProcessQueue.h"

ROCKETMQ_NAMESPACE_BEGIN

class ConsumeStandardMessageService : public ConsumeMessageServiceBase,
                                      public std::enable_shared_from_this<ConsumeStandardMessageService> {
public:
  ConsumeStandardMessageService(std::weak_ptr<PushConsumerImpl> consumer, int thread_count,
                                MessageListener message_listener);

  ~ConsumeStandardMessageService() override = default;

  void start() override;

  void shutdown() override;

  void submitConsumeTask(const std::weak_ptr<ProcessQueue>& process_queue) override;

private:
  static void consumeTask(std::weak_ptr<ConsumeStandardMessageService> service,
                          const std::weak_ptr<ProcessQueue>& process_queue,
                          const std::vector<MessageConstSharedPtr>& msgs);

  void consume(const std::shared_ptr<ProcessQueue>& process_queue, const std::vector<MessageConstSharedPtr>& msgs);
};

ROCKETMQ_NAMESPACE_END
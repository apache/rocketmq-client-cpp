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
#include "ConsumeMessageServiceBase.h"

ROCKETMQ_NAMESPACE_BEGIN

class ConsumeFifoMessageService : public ConsumeMessageServiceBase,
                                  public std::enable_shared_from_this<ConsumeFifoMessageService> {
public:
  ConsumeFifoMessageService(std::weak_ptr<PushConsumer> consumer, int thread_count, MessageListener* message_listener);
  void start() override;

  void shutdown() override;

  /**
   * @brief Entry of ConsumeMessageService
   *
   * @param process_queue
   */
  void submitConsumeTask(const ProcessQueueWeakPtr& process_queue) override;

  MessageListenerType messageListenerType() override;

private:
  void consumeTask(const ProcessQueueWeakPtr& process_queue, MQMessageExt& message);

  void submitConsumeTask0(const std::shared_ptr<PushConsumer>& consumer, const ProcessQueueWeakPtr& process_queue,
                          const MQMessageExt& message);

  void scheduleAckTask(const ProcessQueueWeakPtr& process_queue, const MQMessageExt& message);

  void onAck(const ProcessQueueWeakPtr& process_queue, const MQMessageExt& message, const std::error_code& ec);

  void scheduleConsumeTask(const ProcessQueueWeakPtr& process_queue, const MQMessageExt& message);

  void onForwardToDeadLetterQueue(const ProcessQueueWeakPtr& process_queue, const MQMessageExt& message, bool ok);

  void scheduleForwardDeadLetterQueueTask(const ProcessQueueWeakPtr& process_queue, const MQMessageExt& message);
};

ROCKETMQ_NAMESPACE_END
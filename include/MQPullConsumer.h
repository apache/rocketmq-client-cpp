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
#ifndef ROCKETMQ_MQPULLCONSUMER_H_
#define ROCKETMQ_MQPULLCONSUMER_H_

#include "MQConsumer.h"
#include "MQueueListener.h"
#include "PullCallback.h"

namespace rocketmq {

class ROCKETMQCLIENT_API MQPullConsumer : public MQConsumer {
 public:
  virtual void registerMessageQueueListener(const std::string& topic, MQueueListener* pListener) = 0;

  /**
   * pull msg from specified queue, if no msg in queue, return directly
   *
   * @param mq
   *            specify the pulled queue
   * @param subExpression
   *            set filter expression for pulled msg, broker will filter msg actively
   *            Now only OR operation is supported, eg: "tag1 || tag2 || tag3"
   *            if subExpression is setted to "null" or "*" all msg will be subscribed
   * @param offset
   *            specify the started pull offset
   * @param maxNums
   *            specify max msg num by per pull
   * @return
   *            accroding to PullResult
   */
  virtual PullResult pull(const MQMessageQueue& mq, const std::string& subExpression, int64_t offset, int maxNums) = 0;

  virtual void pull(const MQMessageQueue& mq,
                    const std::string& subExpression,
                    int64_t offset,
                    int maxNums,
                    PullCallback* pullCallback) = 0;

  /**
   * pull msg from specified queue, if no msg, broker will suspend the pull request 20s
   *
   * @param mq
   *            specify the pulled queue
   * @param subExpression
   *            set filter expression for pulled msg, broker will filter msg actively
   *            Now only OR operation is supported, eg: "tag1 || tag2 || tag3"
   *            if subExpression is setted to "null" or "*" all msg will be subscribed
   * @param offset
   *            specify the started pull offset
   * @param maxNums
   *            specify max msg num by per pull
   * @return
   *            accroding to PullResult
   */
  virtual PullResult pullBlockIfNotFound(const MQMessageQueue& mq,
                                         const std::string& subExpression,
                                         int64_t offset,
                                         int maxNums) = 0;

  virtual void pullBlockIfNotFound(const MQMessageQueue& mq,
                                   const std::string& subExpression,
                                   int64_t offset,
                                   int maxNums,
                                   PullCallback* pullCallback) = 0;

  virtual void updateConsumeOffset(const MQMessageQueue& mq, int64_t offset) = 0;

  /**
   * Fetch the offset
   *
   * @param mq
   * @param fromStore
   * @return
   */
  virtual int64_t fetchConsumeOffset(const MQMessageQueue& mq, bool fromStore) = 0;

  /**
   * Fetch the message queues according to the topic
   *
   * @param topic Message Topic
   * @return
   */
  virtual void fetchMessageQueuesInBalance(const std::string& topic, std::vector<MQMessageQueue>& mqs) = 0;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_MQPULLCONSUMER_H_

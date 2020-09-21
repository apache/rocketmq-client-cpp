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
#ifndef ROCKETMQ_MQADMIN_H_
#define ROCKETMQ_MQADMIN_H_

#include "MQMessageExt.h"
#include "MQMessageQueue.h"
#include "QueryResult.h"

namespace rocketmq {

/**
 * MQ Admin API
 */
class ROCKETMQCLIENT_API MQAdmin {
 public:
  virtual ~MQAdmin() = default;

  /**
   * Creates an topic
   *
   * @param key accesskey
   * @param newTopic topic name
   * @param queueNum topic's queue number
   */
  virtual void createTopic(const std::string& key, const std::string& newTopic, int queueNum) = 0;

  /**
   * Gets the message queue offset according to some time in milliseconds<br>
   * be cautious to call because of more IO overhead
   *
   * @param mq Instance of MessageQueue
   * @param timestamp from when in milliseconds.
   * @return offset
   */
  virtual int64_t searchOffset(const MQMessageQueue& mq, int64_t timestamp) = 0;

  /**
   * Gets the max offset
   *
   * @param mq Instance of MessageQueue
   * @return the max offset
   */
  virtual int64_t maxOffset(const MQMessageQueue& mq) = 0;

  /**
   * Gets the minimum offset
   *
   * @param mq Instance of MessageQueue
   * @return the minimum offset
   */
  virtual int64_t minOffset(const MQMessageQueue& mq) = 0;

  /**
   * Gets the earliest stored message time
   *
   * @param mq Instance of MessageQueue
   * @return the time in microseconds
   */
  virtual int64_t earliestMsgStoreTime(const MQMessageQueue& mq) = 0;

  /**
   * Query message according to message id
   *
   * @param offsetMsgId message id
   * @return message
   */
  virtual MQMessageExt viewMessage(const std::string& offsetMsgId) = 0;

  /**
   * Query messages
   *
   * @param topic message topic
   * @param key message key index word
   * @param maxNum max message number
   * @param begin from when
   * @param end to when
   * @return Instance of QueryResult
   */
  virtual QueryResult queryMessage(const std::string& topic,
                                   const std::string& key,
                                   int maxNum,
                                   int64_t begin,
                                   int64_t end) = 0;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_MQADMIN_H_

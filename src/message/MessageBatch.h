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
#ifndef __MESSAGE_BATCH_H__
#define __MESSAGE_BATCH_H__

#include <memory>
#include <string>

#include "MQMessage.h"

namespace rocketmq {

class MessageBatch : public MQMessage {
 public:
  static MessageBatch* generateFromList(std::vector<MQMessagePtr>& messages);

 public:
  MessageBatch(std::vector<MQMessagePtr>& messages) : m_messages(messages) {}

  std::string encode();

  const std::vector<MQMessagePtr>& getMessages() const { return m_messages; }

  bool isBatch() override final { return true; }

 protected:
  std::vector<MQMessagePtr> m_messages;
};

}  // namespace rocketmq

#endif  // __MESSAGE_BATCH_H__
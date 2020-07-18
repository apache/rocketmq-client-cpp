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

#include "MQMessage.h"
#include "MessageImpl.h"

namespace rocketmq {

class MessageBatch : public MessageImpl {
 public:
  static std::shared_ptr<MessageBatch> generateFromList(std::vector<MQMessage>& messages);

 public:
  MessageBatch(std::vector<MQMessage>& messages) : MessageImpl(), m_messages(messages) {}

 public:  // Message
  bool isBatch() const override final { return true; }

 public:
  std::string encode();

  const std::vector<MQMessage>& getMessages() const { return m_messages; }

 protected:
  std::vector<MQMessage> m_messages;
};

}  // namespace rocketmq

#endif  // __MESSAGE_BATCH_H__
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
#include "MessageBatch.h"

#include <memory>

#include "MQDecoder.h"
#include "MessageClientIDSetter.h"

namespace rocketmq {

std::string MessageBatch::encode() {
  return MQDecoder::encodeMessages(m_messages);
}

MessageBatch* MessageBatch::generateFromList(std::vector<MQMessagePtr>& messages) {
  bool isFirst = true;
  std::string topic;
  bool waitStoreMsgOK = false;

  for (auto& message : messages) {
    if (message->getDelayTimeLevel() > 0) {
      THROW_MQEXCEPTION(MQClientException, "TimeDelayLevel in not supported for batching", -1);
    }
    if (isFirst) {
      isFirst = false;
      topic = message->getTopic();
      waitStoreMsgOK = message->isWaitStoreMsgOK();

      if (UtilAll::isRetryTopic(topic)) {
        THROW_MQEXCEPTION(MQClientException, "Retry Group is not supported for batching", -1);
      }
    } else {
      if (message->getTopic() != topic) {
        THROW_MQEXCEPTION(MQClientException, "The topic of the messages in one batch should be the same", -1);
      }
      if (message->isWaitStoreMsgOK() != waitStoreMsgOK) {
        THROW_MQEXCEPTION(MQClientException, "The waitStoreMsgOK of the messages in one batch should the same", -2);
      }
    }
  }

  MessageBatch* batchMessage = new MessageBatch(messages);
  batchMessage->setTopic(topic);
  batchMessage->setWaitStoreMsgOK(waitStoreMsgOK);
  return batchMessage;
}

}  // namespace rocketmq

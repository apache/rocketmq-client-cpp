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

#include "MQException.h"
#include "MessageClientIDSetter.h"
#include "MessageDecoder.h"

namespace rocketmq {

std::shared_ptr<MessageBatch> MessageBatch::generateFromList(std::vector<MQMessage>& messages) {
  bool is_first = true;
  std::string topic;
  bool wait_store_msg_ok = true;

  // check messages
  for (auto& message : messages) {
    if (message.delay_time_level() > 0) {
      THROW_MQEXCEPTION(MQClientException, "TimeDelayLevel in not supported for batching", -1);
    }
    if (is_first) {
      is_first = false;
      topic = message.topic();
      wait_store_msg_ok = message.wait_store_msg_ok();

      if (UtilAll::isRetryTopic(topic)) {
        THROW_MQEXCEPTION(MQClientException, "Retry Group is not supported for batching", -1);
      }
    } else {
      if (message.topic() != topic) {
        THROW_MQEXCEPTION(MQClientException, "The topic of the messages in one batch should be the same", -1);
      }
      if (message.wait_store_msg_ok() != wait_store_msg_ok) {
        THROW_MQEXCEPTION(MQClientException, "The waitStoreMsgOK of the messages in one batch should the same", -2);
      }
    }
  }

  auto batchMessage = std::make_shared<MessageBatch>(messages);
  batchMessage->set_topic(topic);
  batchMessage->set_wait_store_msg_ok(wait_store_msg_ok);
  return batchMessage;
}

std::string MessageBatch::encode() {
  return MessageDecoder::encodeMessages(messages_);
}

}  // namespace rocketmq

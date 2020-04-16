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
#include "MessageUtil.h"

#include <memory>

#include "ClientErrorCode.h"
#include "MQMessageConst.h"
#include "MessageAccessor.h"
#include "UtilAll.h"

namespace rocketmq {

MQMessagePtr MessageUtil::createReplyMessage(const MQMessagePtr requestMessage,
                                             const std::string& body) throw(MQClientException) {
  if (requestMessage != nullptr) {
    std::unique_ptr<MQMessage> replyMessage(new MQMessage());
    const auto& cluster = requestMessage->getProperty(MQMessageConst::PROPERTY_CLUSTER);
    const auto& replyTo = requestMessage->getProperty(MQMessageConst::PROPERTY_MESSAGE_REPLY_TO_CLIENT);
    const auto& correlationId = requestMessage->getProperty(MQMessageConst::PROPERTY_CORRELATION_ID);
    const auto& ttl = requestMessage->getProperty(MQMessageConst::PROPERTY_MESSAGE_TTL);
    replyMessage->setBody(body);
    if (!cluster.empty()) {
      auto replyTopic = UtilAll::getReplyTopic(cluster);
      replyMessage->setTopic(replyTopic);
      MessageAccessor::putProperty(*replyMessage, MQMessageConst::PROPERTY_MESSAGE_TYPE, REPLY_MESSAGE_FLAG);
      MessageAccessor::putProperty(*replyMessage, MQMessageConst::PROPERTY_CORRELATION_ID, correlationId);
      MessageAccessor::putProperty(*replyMessage, MQMessageConst::PROPERTY_MESSAGE_REPLY_TO_CLIENT, replyTo);
      MessageAccessor::putProperty(*replyMessage, MQMessageConst::PROPERTY_MESSAGE_TTL, ttl);

      return replyMessage.release();
    } else {
      THROW_MQEXCEPTION(MQClientException,
                        "create reply message fail, requestMessage error, property[" +
                            MQMessageConst::PROPERTY_CLUSTER + "] is null.",
                        ClientErrorCode::CREATE_REPLY_MESSAGE_EXCEPTION);
    }
  }
  THROW_MQEXCEPTION(MQClientException, "create reply message fail, requestMessage cannot be null.",
                    ClientErrorCode::CREATE_REPLY_MESSAGE_EXCEPTION);
}

const std::string& MessageUtil::getReplyToClient(const MQMessagePtr msg) {
  return msg->getProperty(MQMessageConst::PROPERTY_MESSAGE_REPLY_TO_CLIENT);
}

}  // namespace rocketmq

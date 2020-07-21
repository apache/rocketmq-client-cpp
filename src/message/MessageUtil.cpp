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

#include "ByteArray.h"
#include "ClientErrorCode.h"
#include "MessageImpl.h"
#include "MessageAccessor.hpp"
#include "MQMessageConst.h"
#include "UtilAll.h"

namespace rocketmq {

MQMessage MessageUtil::createReplyMessage(const Message& requestMessage, const std::string& body) {
  const auto& cluster = requestMessage.getProperty(MQMessageConst::PROPERTY_CLUSTER);
  if (!cluster.empty()) {
    auto replyMessage = std::make_shared<MessageImpl>(UtilAll::getReplyTopic(cluster), body);
    // set properties
    MessageAccessor::putProperty(*replyMessage, MQMessageConst::PROPERTY_MESSAGE_TYPE, REPLY_MESSAGE_FLAG);
    const auto& correlationId = requestMessage.getProperty(MQMessageConst::PROPERTY_CORRELATION_ID);
    MessageAccessor::putProperty(*replyMessage, MQMessageConst::PROPERTY_CORRELATION_ID, correlationId);
    const auto& replyTo = requestMessage.getProperty(MQMessageConst::PROPERTY_MESSAGE_REPLY_TO_CLIENT);
    MessageAccessor::putProperty(*replyMessage, MQMessageConst::PROPERTY_MESSAGE_REPLY_TO_CLIENT, replyTo);
    const auto& ttl = requestMessage.getProperty(MQMessageConst::PROPERTY_MESSAGE_TTL);
    MessageAccessor::putProperty(*replyMessage, MQMessageConst::PROPERTY_MESSAGE_TTL, ttl);
    return MQMessage(replyMessage);
  } else {
    THROW_MQEXCEPTION(MQClientException, "create reply message fail, requestMessage error, property[" +
                                             MQMessageConst::PROPERTY_CLUSTER + "] is null.",
                      ClientErrorCode::CREATE_REPLY_MESSAGE_EXCEPTION);
  }
}

const std::string& MessageUtil::getReplyToClient(const Message& msg) {
  return msg.getProperty(MQMessageConst::PROPERTY_MESSAGE_REPLY_TO_CLIENT);
}

}  // namespace rocketmq

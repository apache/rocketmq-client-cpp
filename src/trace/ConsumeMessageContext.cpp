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

#include "ConsumeMessageContext.h"
#include <string>
#include <vector>
#include "DefaultMQPushConsumerImpl.h"

namespace rocketmq {
ConsumeMessageContext::ConsumeMessageContext() {
  m_defaultMQPushConsumer = NULL;
  // m_traceContext = NULL;
}
ConsumeMessageContext::~ConsumeMessageContext() {
  m_traceContext.reset();
}
std::string ConsumeMessageContext::getConsumerGroup() {
  return m_consumerGroup;
}

void ConsumeMessageContext::setConsumerGroup(const std::string& mConsumerGroup) {
  m_consumerGroup = mConsumerGroup;
}

bool ConsumeMessageContext::getSuccess() {
  return m_success;
}

void ConsumeMessageContext::setSuccess(bool mSuccess) {
  m_success = mSuccess;
}

std::vector<MQMessageExt> ConsumeMessageContext::getMsgList() {
  return m_msgList;
}

void ConsumeMessageContext::setMsgList(std::vector<MQMessageExt> mMsgList) {
  m_msgList = mMsgList;
}

std::string ConsumeMessageContext::getStatus() {
  return m_status;
}

void ConsumeMessageContext::setStatus(const std::string& mStatus) {
  m_status = mStatus;
}

int ConsumeMessageContext::getMsgIndex() {
  return m_msgIndex;
}

void ConsumeMessageContext::setMsgIndex(int mMsgIndex) {
  m_msgIndex = mMsgIndex;
}

MQMessageQueue ConsumeMessageContext::getMessageQueue() {
  return m_messageQueue;
}

void ConsumeMessageContext::setMessageQueue(const MQMessageQueue& mMessageQueue) {
  m_messageQueue = mMessageQueue;
}

DefaultMQPushConsumerImpl* ConsumeMessageContext::getDefaultMQPushConsumer() {
  return m_defaultMQPushConsumer;
}

void ConsumeMessageContext::setDefaultMQPushConsumer(DefaultMQPushConsumerImpl* mDefaultMqPushConsumer) {
  m_defaultMQPushConsumer = mDefaultMqPushConsumer;
}

std::shared_ptr<TraceContext> ConsumeMessageContext::getTraceContext() {
  return m_traceContext;
}

void ConsumeMessageContext::setTraceContext(TraceContext* mTraceContext) {
  m_traceContext.reset(mTraceContext);
}

std::string ConsumeMessageContext::getNameSpace() {
  return m_nameSpace;
}

void ConsumeMessageContext::setNameSpace(const std::string& mNameSpace) {
  m_nameSpace = mNameSpace;
}
}  // namespace rocketmq

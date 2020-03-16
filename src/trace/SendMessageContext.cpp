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

#include "SendMessageContext.h"
#include <string>

namespace rocketmq {
SendMessageContext::SendMessageContext() {}

SendMessageContext::~SendMessageContext() {
  m_defaultMQProducer = NULL;
}

std::string SendMessageContext::getProducerGroup() {
  return m_producerGroup;
}

void SendMessageContext::setProducerGroup(const std::string& mProducerGroup) {
  m_producerGroup = mProducerGroup;
}

MQMessage* SendMessageContext::getMessage() {
  return &m_message;
}

void SendMessageContext::setMessage(const MQMessage& mMessage) {
  m_message = mMessage;
}

TraceMessageType SendMessageContext::getMsgType() {
  return m_msgType;
}

void SendMessageContext::setMsgType(TraceMessageType mMsgType) {
  m_msgType = mMsgType;
}

MQMessageQueue* SendMessageContext::getMessageQueue() {
  return &m_messageQueue;
}

void SendMessageContext::setMessageQueue(const MQMessageQueue& mMq) {
  m_messageQueue = mMq;
}

std::string SendMessageContext::getBrokerAddr() {
  return m_brokerAddr;
}

void SendMessageContext::setBrokerAddr(const std::string& mBrokerAddr) {
  m_brokerAddr = mBrokerAddr;
}

std::string SendMessageContext::getBornHost() {
  return m_bornHost;
}

void SendMessageContext::setBornHost(const std::string& mBornHost) {
  m_bornHost = mBornHost;
}

CommunicationMode SendMessageContext::getCommunicationMode() {
  return m_communicationMode;
}

void SendMessageContext::setCommunicationMode(CommunicationMode mCommunicationMode) {
  m_communicationMode = mCommunicationMode;
}

DefaultMQProducerImpl* SendMessageContext::getDefaultMqProducer() {
  return m_defaultMQProducer;
}

void SendMessageContext::setDefaultMqProducer(DefaultMQProducerImpl* mDefaultMqProducer) {
  m_defaultMQProducer = mDefaultMqProducer;
}

SendResult* SendMessageContext::getSendResult() {
  return &m_sendResult;
}

void SendMessageContext::setSendResult(const SendResult& mSendResult) {
  m_sendResult = mSendResult;
}

TraceContext* SendMessageContext::getTraceContext() {
  return m_traceContext;
}

void SendMessageContext::setTraceContext(TraceContext* mTraceContext) {
  m_traceContext = mTraceContext;
}

std::string SendMessageContext::getNameSpace() {
  return m_nameSpace;
}

void SendMessageContext::setNameSpace(const std::string& mNameSpace) {
  m_nameSpace = mNameSpace;
}
}  // namespace rocketmq

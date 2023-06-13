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
#ifndef __ROCKETMQ_SEND_MESSAGE_CONTEXT_H__
#define __ROCKETMQ_SEND_MESSAGE_CONTEXT_H__

#include <string>
#include "CommunicationMode.h"
#include "MQMessage.h"
#include "MQMessageQueue.h"
#include "SendResult.h"
#include "TraceBean.h"
#include "TraceConstant.h"
#include "TraceContext.h"

namespace rocketmq {
class DefaultMQProducerImpl;
class SendMessageContext {
 public:
  SendMessageContext();

  virtual ~SendMessageContext();

  std::string getProducerGroup();

  void setProducerGroup(const std::string& mProducerGroup);

  MQMessage* getMessage();

  void setMessage(const MQMessage& mMessage);

  TraceMessageType getMsgType();

  void setMsgType(TraceMessageType mMsgType);

  MQMessageQueue* getMessageQueue();

  void setMessageQueue(const MQMessageQueue& mMq);

  std::string getBrokerAddr();

  void setBrokerAddr(const std::string& mBrokerAddr);

  std::string getBornHost();

  void setBornHost(const std::string& mBornHost);

  CommunicationMode getCommunicationMode();

  void setCommunicationMode(CommunicationMode mCommunicationMode);

  DefaultMQProducerImpl* getDefaultMqProducer();

  void setDefaultMqProducer(DefaultMQProducerImpl* mDefaultMqProducer);

  SendResult* getSendResult();

  void setSendResult(const SendResult& mSendResult);

  TraceContext* getTraceContext();

  void setTraceContext(TraceContext* mTraceContext);

  std::string getNameSpace();

  void setNameSpace(const std::string& mNameSpace);

 private:
  std::string m_producerGroup;
  MQMessage m_message;
  TraceMessageType m_msgType;
  MQMessageQueue m_messageQueue;
  std::string m_brokerAddr;
  std::string m_bornHost;
  CommunicationMode m_communicationMode;
  DefaultMQProducerImpl* m_defaultMQProducer;
  SendResult m_sendResult;
  TraceContext* m_traceContext;
  std::string m_nameSpace;
};

}  // namespace rocketmq
#endif
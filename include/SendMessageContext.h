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
#ifndef __SENDMESSAGECONTEXT_H__
#define __SENDMESSAGECONTEXT_H__


#include "MQClientException.h"
#include "MQMessage.h"
#include "MQMessageQueue.h"
#include "RocketMQClient.h"
#include "TraceHelper.h"
#include "SendResult.h"

namespace rocketmq{
/*
class SendMessageContext {
 public:
  std::string producerGroup;
  MQMessage msg;
  MQMessageQueue mq;
  std::string brokerAddrstring;
  int communicationMode;
  SendResult sendResult;
  MQException* pException;
  void* pArg;
};*/


class SendMessageContext {
 private:
  std::string producerGroup;
  MQMessage message;
  MQMessageQueue mq;
  std::string brokerAddr;
  std::string bornHost; 
  int communicationMode;
  /*
  Exception exception;*/
  //unique_ptr<SendResult> sendResult;
  SendResult* sendResult;
  TraceContext* mqTraceContext;
  std::map<std::string, std::string> props;
  //DefaultMQProducerImpl producer;
  MessageType msgType;

  std::string msgnamespace;

 public:
  
  MessageType getMsgType() { return msgType; };

  void setMsgType( MessageType msgTypev) {    msgType = msgTypev;  };/*

  DefaultMQProducerImpl getProducer() { return producer; };

  void setProducer(final DefaultMQProducerImpl producer) { producer = producer; };*/

  std::string getProducerGroup() { return producerGroup; };

  void setProducerGroup(std::string producerGroupv) { producerGroup = producerGroupv; };

  MQMessage getMessage() { return message; };

  void setMessage(MQMessage messagev) { message = messagev; };

  MQMessageQueue getMq() { return mq; };

  void setMq(MQMessageQueue mqv) { mq = mqv; };

  std::string getBrokerAddr() { return brokerAddr; };

  void setBrokerAddr(std::string brokerAddrv) { brokerAddr = brokerAddrv; }; 

  int getCommunicationMode() { return communicationMode; };

  void setCommunicationMode(int communicationModev) { communicationMode = communicationModev; };

  /*

  Exception getException() { return exception; };

  void setException(Exception exception) { exception = exception; };*/

  const SendResult* getSendResult() { return sendResult; };

  void setSendResult(SendResult* sendResultv) { sendResult = sendResultv; };

  TraceContext* getMqTraceContext() { return mqTraceContext; };

  void setMqTraceContext(TraceContext* mqTraceContextv) { mqTraceContext = mqTraceContextv; };

  std::map<std::string, std::string>& getProps() { return props; };

  void setProps(std::map<std::string, std::string>& propsv) { props = propsv; };

  std::string getBornHost() { return bornHost; };

  void setBornHost(std::string bornHostv) { bornHost = bornHostv; };

  std::string getNamespace() { return msgnamespace; };

  void setNamespace(std::string msgnamespacev) { msgnamespace = msgnamespacev; };
};




}

#endif
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
#ifndef __ROCKETMQ_CONSUME_MESSAGE_CONTEXT_H__
#define __ROCKETMQ_CONSUME_MESSAGE_CONTEXT_H__

#include <memory>
#include <string>
#include <vector>
#include "MQMessageExt.h"
#include "MQMessageQueue.h"
#include "TraceBean.h"
#include "TraceConstant.h"
#include "TraceContext.h"

namespace rocketmq {
class DefaultMQPushConsumerImpl;
class ConsumeMessageContext {
 public:
  ConsumeMessageContext();

  virtual ~ConsumeMessageContext();

  std::string getConsumerGroup();

  void setConsumerGroup(const std::string& mConsumerGroup);

  bool getSuccess();

  void setSuccess(bool mSuccess);

  std::vector<MQMessageExt> getMsgList();

  void setMsgList(std::vector<MQMessageExt> mMsgList);

  std::string getStatus();

  void setStatus(const std::string& mStatus);

  int getMsgIndex();

  void setMsgIndex(int mMsgIndex);

  MQMessageQueue getMessageQueue();

  void setMessageQueue(const MQMessageQueue& mMessageQueue);

  DefaultMQPushConsumerImpl* getDefaultMQPushConsumer();

  void setDefaultMQPushConsumer(DefaultMQPushConsumerImpl* mDefaultMqPushConsumer);

  std::shared_ptr<TraceContext> getTraceContext();

  void setTraceContext(TraceContext* mTraceContext);

  std::string getNameSpace();

  void setNameSpace(const std::string& mNameSpace);

 private:
  std::string m_consumerGroup;
  bool m_success;
  std::vector<MQMessageExt> m_msgList;
  std::string m_status;
  int m_msgIndex;
  MQMessageQueue m_messageQueue;
  DefaultMQPushConsumerImpl* m_defaultMQPushConsumer;
  // TraceContext* m_traceContext;
  std::shared_ptr<TraceContext> m_traceContext;
  std::string m_nameSpace;
};
}  // namespace rocketmq
#endif
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
#ifndef __CONSUMEMESSAGECONTEXT_H__
#define __CONSUMEMESSAGECONTEXT_H__
#include <string>
#include <list>
#include <map>


#include "MQMessageExt.h"
#include "MQMessageQueue.h"
#include "TraceHelper.h"

namespace rocketmq {

class ConsumeMessageContext {
 private:
  std::string consumerGroup;
  std::list<MQMessageExt> msgList;
  MQMessageQueue mq;
  bool success;
  std::string status;
  TraceContext* mqTraceContext;
  std::map<std::string, std::string> props;
  std::string msgnamespace;

 public:
  std::string getConsumerGroup() { return consumerGroup; };

  void setConsumerGroup(std::string consumerGroup) { consumerGroup = consumerGroup; };

  std::list<MQMessageExt> getMsgList() { return msgList; };

  void setMsgList(std::list<MQMessageExt> msgList) { msgList = msgList; };
  void setMsgList(std::vector<MQMessageExt> pmsgList) { msgList.assign(pmsgList.begin(), pmsgList.end());
  };
  MQMessageQueue getMq() { return mq; };

  void setMq(MQMessageQueue mq) { mq = mq; };

  bool isSuccess() { return success; };

  void setSuccess(bool success) { success = success; };

  TraceContext* getMqTraceContext() { return mqTraceContext; };

  void setMqTraceContext(TraceContext* pmqTraceContext) { mqTraceContext = pmqTraceContext; };

  std::map<std::string, std::string> getProps() { return props; };

  void setProps(std::map<std::string, std::string> props) { props = props; };

  std::string getStatus() { return status; };

  void setStatus(std::string status) { status = status; };

  std::string getNamespace() { return msgnamespace; };

  void setNamespace(std::string msgnamespace) { msgnamespace = msgnamespace; };
};

}  // namespace rocketmq

#endif  //__ConsumeMessageContext_H__
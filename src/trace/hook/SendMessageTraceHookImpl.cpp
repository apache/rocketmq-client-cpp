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
#include "SendMessageTraceHookImpl.h"
#include "SendMessageHook.h"

#include <chrono>
#include <mutex>
#include "Logging.h"

namespace rocketmq {

SendMessageTraceHookImpl::SendMessageTraceHookImpl(std::shared_ptr<TraceDispatcher>& localDispatcherv) {
  m_localDispatcher = std::shared_ptr<TraceDispatcher>(localDispatcherv);
}

std::string SendMessageTraceHookImpl::hookName() {
  return "SendMessageTraceHook";
}

void SendMessageTraceHookImpl::sendMessageBefore(SendMessageContext& context) {

  TraceContext* tuxeContext = new TraceContext();
  context.setMqTraceContext(tuxeContext);
  tuxeContext->setTraceType(TraceType::Pub);
  tuxeContext->setGroupName(context.getProducerGroup());
  // build the data bean object of message trace
  TraceBean traceBean;
  traceBean.setTopic(context.getMessage().getTopic());
  traceBean.setTags(context.getMessage().getTags());
  traceBean.setKeys(context.getMessage().getKeys());
  traceBean.setStoreHost(context.getBrokerAddr());
  traceBean.setBodyLength(context.getMessage().getBody().length());
  traceBean.setMsgType(context.getMsgType());
  tuxeContext->getTraceBeans().push_back(traceBean);
}

void SendMessageTraceHookImpl::sendMessageAfter(SendMessageContext& context) {

  if (context.getSendResult() == nullptr) {
    return;
  }

  TraceContext* tuxeContext = (TraceContext*)context.getMqTraceContext();
  TraceBean traceBean = tuxeContext->getTraceBeans().front();

  int costTime = time(0) - tuxeContext->getTimeStamp();
  tuxeContext->setCostTime(costTime);
  if (context.getSendResult()->getSendStatus() == (SendStatus::SEND_OK)) {
    tuxeContext->setSuccess(true);
  } else {
    tuxeContext->setSuccess(false);
  }
  std::string regionId = context.getMessage().getProperty(MQMessage::PROPERTY_MSG_REGION);
  std::string traceOn = context.getMessage().getProperty(MQMessage::PROPERTY_TRACE_SWITCH);

  tuxeContext->setRegionId(regionId);
  traceBean.setMsgId(context.getSendResult()->getMsgId());
  traceBean.setOffsetMsgId(context.getSendResult()->getOffsetMsgId());
  traceBean.setStoreTime(tuxeContext->getTimeStamp() + costTime / 2);

  {
    m_localDispatcher->append(tuxeContext);
    LOG_INFO("SendMessageTraceHookImpl::sendMessageAfter append");
  }
}

}  // namespace rocketmq
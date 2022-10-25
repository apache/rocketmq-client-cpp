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

#include "ConsumeMessageHookImpl.h"
#include <memory>
#include <string>
#include "ConsumeMessageContext.h"
#include "DefaultMQPushConsumerImpl.h"
#include "Logging.h"
#include "MQClientException.h"
#include "NameSpaceUtil.h"
#include "TraceConstant.h"
#include "TraceContext.h"
#include "TraceTransferBean.h"
#include "TraceUtil.h"
#include "UtilAll.h"
namespace rocketmq {

class TraceMessageConsumeCallback : public SendCallback {
  virtual void onSuccess(SendResult& sendResult) {
    LOG_DEBUG("TraceMessageConsumeCallback, MsgId:[%s],OffsetMsgId[%s]", sendResult.getMsgId().c_str(),
              sendResult.getOffsetMsgId().c_str());
  }
  virtual void onException(MQException& e) {}
};
static TraceMessageConsumeCallback* consumeTraceCallback = new TraceMessageConsumeCallback();
std::string ConsumeMessageHookImpl::getHookName() {
  return "RocketMQConsumeMessageHookImpl";
}

void ConsumeMessageHookImpl::executeHookBefore(ConsumeMessageContext* context) {
  if (context == NULL || context->getMsgList().empty()) {
    return;
  }
  TraceContext* traceContext = new TraceContext();
  context->setTraceContext(traceContext);
  traceContext->setTraceType(SubBefore);
  traceContext->setGroupName(NameSpaceUtil::withoutNameSpace(context->getConsumerGroup(), context->getNameSpace()));
  std::vector<TraceBean> beans;

  std::vector<MQMessageExt> msgs = context->getMsgList();
  std::vector<MQMessageExt>::iterator it = msgs.begin();
  for (; it != msgs.end(); ++it) {
    std::string traceOn = it->getProperty(MQMessage::PROPERTY_TRACE_SWITCH);
    if (traceOn != "" && traceOn == "false") {
      continue;
    }
    TraceBean bean;
    bean.setTopic((*it).getTopic());
    bean.setMsgId((*it).getMsgId());
    bean.setTags((*it).getTags());
    bean.setKeys((*it).getKeys());
    bean.setStoreHost((*it).getStoreHostString());
    bean.setStoreTime((*it).getStoreTimestamp());
    bean.setBodyLength((*it).getStoreSize());
    bean.setRetryTimes((*it).getReconsumeTimes());
    std::string regionId = (*it).getProperty(MQMessage::PROPERTY_MSG_REGION);
    if (regionId.empty()) {
      regionId = TraceConstant::DEFAULT_REDION;
    }
    traceContext->setRegionId(regionId);
    traceContext->setTraceBean(bean);
  }
  traceContext->setTimeStamp(UtilAll::currentTimeMillis());

  std::string topic = TraceConstant::TRACE_TOPIC + traceContext->getRegionId();

  TraceTransferBean ben = TraceUtil::CovertTraceContextToTransferBean(traceContext);
  MQMessage message(topic, ben.getTransData());
  message.setKeys(ben.getTransKey());

  // send trace message async.
  context->getDefaultMQPushConsumer()->submitSendTraceRequest(message, consumeTraceCallback);
  return;
}

void ConsumeMessageHookImpl::executeHookAfter(ConsumeMessageContext* context) {
  if (context == NULL || context->getMsgList().empty()) {
    return;
  }

  std::shared_ptr<TraceContext> subBeforeContext = context->getTraceContext();
  TraceContext subAfterContext;
  subAfterContext.setTraceType(SubAfter);
  subAfterContext.setRegionId(subBeforeContext->getRegionId());
  subAfterContext.setGroupName(subBeforeContext->getGroupName());
  subAfterContext.setRequestId(subBeforeContext->getRequestId());
  subAfterContext.setStatus(context->getSuccess());
  int costTime = static_cast<int>(UtilAll::currentTimeMillis() - subBeforeContext->getTimeStamp());
  subAfterContext.setCostTime(costTime);
  subAfterContext.setTraceBeanIndex(context->getMsgIndex());
  TraceBean bean = subBeforeContext->getTraceBeans()[subAfterContext.getTraceBeanIndex()];
  subAfterContext.setTraceBean(bean);

  std::string topic = TraceConstant::TRACE_TOPIC + subAfterContext.getRegionId();
  TraceTransferBean ben = TraceUtil::CovertTraceContextToTransferBean(&subAfterContext);
  MQMessage message(topic, ben.getTransData());
  message.setKeys(ben.getTransKey());

  // send trace message async.
  context->getDefaultMQPushConsumer()->submitSendTraceRequest(message, consumeTraceCallback);
  return;
}
}  // namespace rocketmq

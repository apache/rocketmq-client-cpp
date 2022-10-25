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

#include "SendMessageHookImpl.h"
#include <memory>
#include <string>
#include "DefaultMQProducerImpl.h"
#include "Logging.h"
#include "MQClientException.h"
#include "SendMessageContext.h"
#include "TraceConstant.h"
#include "TraceTransferBean.h"
#include "TraceUtil.h"
#include "UtilAll.h"

using namespace std;
namespace rocketmq {

class TraceMessageSendCallback : public SendCallback {
  virtual void onSuccess(SendResult& sendResult) {
    LOG_DEBUG("TraceMessageSendCallback, MsgId:[%s],OffsetMsgId[%s]", sendResult.getMsgId().c_str(),
              sendResult.getOffsetMsgId().c_str());
  }
  virtual void onException(MQException& e) {}
};
static TraceMessageSendCallback* callback = new TraceMessageSendCallback();
std::string SendMessageHookImpl::getHookName() {
  return "RocketMQSendMessageHookImpl";
}

void SendMessageHookImpl::executeHookBefore(SendMessageContext* context) {
  if (context != NULL) {
    string topic = context->getMessage()->getTopic();
    // Check if contains TraceConstants::TRACE_TOPIC
    if (topic.find(TraceConstant::TRACE_TOPIC) != string::npos) {
      // trace message itself
      return;
    }
    TraceContext* traceContext = new TraceContext();
    context->setTraceContext(traceContext);
  }
  return;
}

void SendMessageHookImpl::executeHookAfter(SendMessageContext* context) {
  if (context == NULL || context->getSendResult() == NULL) {
    return;
  }
  string topic = context->getMessage()->getTopic();
  // Check if contains TraceConstants::TRACE_TOPIC
  if (topic.find(TraceConstant::TRACE_TOPIC) != string::npos
    || context->getSendResult()->getTraceOn() == false) {
    // trace message itself
    return;
  }
  std::shared_ptr<TraceContext> traceContext;
  traceContext.reset(context->getTraceContext());

  // OnsTraceContext* onsContext = context->getMqTraceContext();
  traceContext->setTraceType(Pub);
  traceContext->setGroupName(context->getProducerGroup());
  // boost::scoped_ptr<OnsTraceBean> traceBean(new OnsTraceBean());
  TraceBean traceBean;
  traceBean.setTopic(context->getMessage()->getTopic());
  traceBean.setTags(context->getMessage()->getTags());
  traceBean.setKeys(context->getMessage()->getKeys());
  traceBean.setStoreHost(context->getBrokerAddr());
  traceBean.setBodyLength(context->getMessage()->getBody().size());
  traceBean.setMsgType(context->getMsgType());

  int costTime = static_cast<int>(UtilAll::currentTimeMillis() - traceContext->getTimeStamp());
  traceContext->setCostTime(costTime);
  if (context->getSendResult()->getSendStatus() == SEND_OK) {
    traceContext->setStatus(true);
  } else {
    traceContext->setStatus(false);
  }

  traceContext->setRegionId(context->getSendResult()->getRegionId());
  traceBean.setMsgId(context->getMessage()->getProperty(MQMessage::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX));
  traceBean.setOffsetMsgId(context->getSendResult()->getOffsetMsgId());
  traceBean.setStoreTime(traceContext->getTimeStamp() + (costTime / 2));

  traceContext->setTraceBean(traceBean);

  topic = TraceConstant::TRACE_TOPIC + traceContext->getRegionId();
  TraceTransferBean ben = TraceUtil::CovertTraceContextToTransferBean(traceContext.get());
  // encode data
  MQMessage message(topic, ben.getTransData());
  message.setKeys(ben.getTransKey());
  // send trace message.
  context->getDefaultMqProducer()->submitSendTraceRequest(message, callback);
  return;
}
}  // namespace rocketmq

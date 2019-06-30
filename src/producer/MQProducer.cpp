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

#include "MQProducer.h"
#include "AsyncTraceDispatcher.h"
#include "ClientRPCHook.h"
#include "Logging.h"
#include "trace/hook/SendMessageTraceHookImpl.h"
#include "ClientRPCHook.h"

namespace rocketmq {
/*
MQProducer::MQProducer(const string& groupname)
    : m_sendMsgTimeout(3000),
      m_compressMsgBodyOverHowmuch(4 * 1024),
      m_maxMessageSize(1024 * 128),
      //m_retryAnotherBrokerWhenNotStoreOK(false),
      m_compressLevel(5),
      m_retryTimes(5),
      m_retryTimes4Async(1) {
  //<!set default group name;
  string gname = groupname.empty() ? DEFAULT_PRODUCER_GROUP : groupname;
  setGroupName(gname);

  */
MQProducer::MQProducer(bool b, void* rpcHookv) {
  LOG_INFO("MQProducer::MQProducer(bool WithoutTrace) %d", b);
  std::string customizedTraceTopic;
  // char* rpcHook = nullptr;
  bool enableMsgTrace = true;
  WithoutTrace = b;
  traceDispatcher = nullptr;
  // const SessionCredentials& MQClient::getSessionCredentials() const {
  RPCHook* rpcHook = nullptr;
  if (rpcHookv != nullptr) {
    rpcHook = (RPCHook*)rpcHookv;
  } else {
    rpcHook=new ClientRPCHook(getSessionCredentials());
  }

  if (WithoutTrace == false && enableMsgTrace == true) {
    try {
      std::shared_ptr<TraceDispatcher>  ptraceDispatcher =
          std::shared_ptr<TraceDispatcher>(new AsyncTraceDispatcher(customizedTraceTopic, rpcHook));
      // dispatcher.setHostProducer(this.defaultMQProducerImpl);
      traceDispatcher = std::shared_ptr<TraceDispatcher>(ptraceDispatcher);
      std::shared_ptr<SendMessageHook> pSendMessageTraceHookImpl =
          std::shared_ptr<SendMessageHook>(new SendMessageTraceHookImpl(ptraceDispatcher));
      registerSendMessageHook(pSendMessageTraceHookImpl);

      LOG_INFO("registerSendMessageHook");

    } catch (...) {
      LOG_INFO("system mqtrace hook init failed ,maybe can't send msg trace data");
    }
  }  // if
}



MQProducer::~MQProducer() {
  if (traceDispatcher.use_count()>0) {
    traceDispatcher->shutdown();
    traceDispatcher->setdelydelflag(true);
  }
}
void MQProducer::registerSendMessageHook(
        std::shared_ptr<SendMessageHook>& hook) {
  sendMessageHookList.push_back(hook);
  LOG_INFO("register sendMessage Hook, {}, hook.hookName()");
}

bool MQProducer::hasSendMessageHook() {
  return !sendMessageHookList.empty();
}

void MQProducer::executeSendMessageHookBefore(SendMessageContext& context) {
  if (!sendMessageHookList.empty()) {
    for (auto& hook : sendMessageHookList) {
      try {
        LOG_INFO("hook->sendMessageBefore YES:%d", 1);
        hook->sendMessageBefore(context);
      } catch (...) {
        LOG_WARN("failed to executeSendMessageHookBefore, e");
      }
    }
  }
}

void MQProducer::executeSendMessageHookAfter(SendMessageContext& context) {
  if (!sendMessageHookList.empty()) {
    for (auto& hook : sendMessageHookList) {
      try {
        LOG_INFO("hook->executeSendMessageHookAfter YES:%d", 1);
        hook->sendMessageAfter(context);
      } catch (...) {
        LOG_WARN("failed to executeSendMessageHookBefore, e");
      }
    }
  }
}

//<!***************************************************************************
}  // namespace rocketmq

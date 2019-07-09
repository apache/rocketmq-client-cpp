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

MQProducer::MQProducer(bool withoutTrace, void* rpcHookv) {
  LOG_INFO("MQProducer::MQProducer(bool WithoutTrace) %d", m_withoutTrace);
  std::string customizedTraceTopic;
  // char* rpcHook = nullptr;
  bool enableMsgTrace = true;
  m_withoutTrace = withoutTrace;
  m_traceDispatcher = nullptr;
  RPCHook* rpcHook = nullptr;
  if (rpcHookv != nullptr) {
    rpcHook = (RPCHook*)rpcHookv;
  } else {
    rpcHook=new ClientRPCHook(getSessionCredentials());
  }

  if (m_withoutTrace == false && enableMsgTrace == true) {
    try {
      std::shared_ptr<TraceDispatcher>  ptraceDispatcher =
          std::shared_ptr<TraceDispatcher>(new AsyncTraceDispatcher(customizedTraceTopic, rpcHook));

      m_traceDispatcher = std::shared_ptr<TraceDispatcher>(ptraceDispatcher);
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
  if (m_traceDispatcher.use_count()>0) {
    m_traceDispatcher->shutdown();
    m_traceDispatcher->setdelydelflag(true);
  }
}
void MQProducer::registerSendMessageHook(
        std::shared_ptr<SendMessageHook>& hook) {
  m_sendMessageHookList.push_back(hook);
  LOG_INFO("register sendMessage Hook, {}, hook.hookName()");
}

bool MQProducer::hasSendMessageHook() {
  return !m_sendMessageHookList.empty();
}

void MQProducer::executeSendMessageHookBefore(SendMessageContext& context) {
  if (!m_sendMessageHookList.empty()) {
    for (auto& hook : m_sendMessageHookList) {
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
  if (!m_sendMessageHookList.empty()) {
    for (auto& hook : m_sendMessageHookList) {
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

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

#include "MQConsumer.h"
#include "Logging.h"
#include "ConsumeMessageHook.h"
#include "ClientRPCHook.h"
#include "trace/hook/ConsumeMessageTraceHookImpl.h"
#include "AsyncTraceDispatcher.h"
namespace rocketmq {

      MQConsumer::MQConsumer(PullRequest* request) {
  LOG_INFO("MQConsumer::MQConsumer(bool WithoutTrace) ");
  std::string customizedTraceTopic;
  // char* rpcHook = nullptr;
  bool enableMsgTrace = true;
  traceDispatcher = nullptr;
  // const SessionCredentials& MQClient::getSessionCredentials() const {
  RPCHook* rpcHook = nullptr;
    rpcHook = new ClientRPCHook(getSessionCredentials());

  if ( enableMsgTrace == true) {
    try {
          std::shared_ptr<TraceDispatcher> ptraceDispatcher =
              std::shared_ptr<TraceDispatcher>(new AsyncTraceDispatcher(customizedTraceTopic, rpcHook));
          // dispatcher.setHostProducer(this.defaultMQProducerImpl);
          traceDispatcher = std::shared_ptr<TraceDispatcher>(ptraceDispatcher);
          std::shared_ptr<ConsumeMessageTraceHookImpl> pSendMessageTraceHookImpl =
              std::shared_ptr<ConsumeMessageTraceHookImpl>(new ConsumeMessageTraceHookImpl(ptraceDispatcher));
          registerConsumeMessageHook(std::dynamic_pointer_cast<ConsumeMessageHook>(pSendMessageTraceHookImpl));

				}
				catch (...) {
					LOG_ERROR("system mqtrace hook init failed ,maybe can't send msg trace data");
				}
			}//if

        }


   MQConsumer::~MQConsumer() {
	if (traceDispatcher.use_count()>0) {
		traceDispatcher->shutdown();
		traceDispatcher->setdelydelflag(true);
	  }
   }

    void MQConsumer::registerConsumeMessageHook(std::shared_ptr<ConsumeMessageHook>& hook) {
        consumeMessageHookList.push_back(hook);
		LOG_INFO("register consumeMessageHook Hook, {}hook.hookName()");
    }

    void MQConsumer::executeHookBefore(ConsumeMessageContext& context) {
        if (!consumeMessageHookList.empty()) {
            for (auto& hook : consumeMessageHookList) {
                try {
                    hook->consumeMessageBefore(context);
                } catch (...) {

>>>>>>> message trace
                }
            }
        }
    }


    void MQConsumer::executeHookAfter(ConsumeMessageContext& context) {
        if (!consumeMessageHookList.empty()) {
            for (auto& hook : consumeMessageHookList) {
                try {
                    hook->consumeMessageAfter(context);
                } catch (...) {


                }
            }
        }
    }



//<!************************************************************************
}  //<!end namespace;

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

namespace rocketmq {

//<!************************************************************************
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




          if (enableMsgTrace) {
            try {
                AsyncTraceDispatcher dispatcher = new AsyncTraceDispatcher(customizedTraceTopic, rpcHook);
                dispatcher.setHostProducer(this.defaultMQProducerImpl);
                traceDispatcher = dispatcher;
                this.defaultMQProducerImpl.registerSendMessageHook(
                    new SendMessageTraceHookImpl(traceDispatcher));

                    //N:\OPENSOURCE_JOB\APACHE_ROCKETMQ\DEV_SRC\rocketmq-client-cpp\src\trace\hook\SendMessageTraceHookImpl.cpp

                    
            } catch (Throwable e) {
                log.error("system mqtrace hook init failed ,maybe can't send msg trace data");
            }
        }
}



    public void registerSendMessageHook(final SendMessageHook hook) {
        this.sendMessageHookList.add(hook);
        log.info("register sendMessage Hook, {}", hook.hookName());
    }



public boolean hasSendMessageHook() {
        return !this.sendMessageHookList.isEmpty();
    }

    public void executeSendMessageHookBefore(final SendMessageContext context) {
        if (!this.sendMessageHookList.isEmpty()) {
            for (SendMessageHook hook : this.sendMessageHookList) {
                try {
                    hook.sendMessageBefore(context);
                } catch (Throwable e) {
                    log.warn("failed to executeSendMessageHookBefore", e);
                }
            }
        }
    }

    public void executeSendMessageHookAfter(final SendMessageContext context) {
        if (!this.sendMessageHookList.isEmpty()) {
            for (SendMessageHook hook : this.sendMessageHookList) {
                try {
                    hook.sendMessageAfter(context);
                } catch (Throwable e) {
                    log.warn("failed to executeSendMessageHookAfter", e);
                }
            }
        }
    }


    

    

//<!***************************************************************************
}  //<!end namespace;

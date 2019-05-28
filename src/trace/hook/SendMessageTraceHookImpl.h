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
//package org.apache.rocketmq.client.trace.hook;

/*
import java.util.ArrayList;
import org.apache.rocketmq.client.hook.SendMessageContext;
import org.apache.rocketmq.client.hook.SendMessageHook;
import org.apache.rocketmq.client.producer.SendStatus;
import org.apache.rocketmq.client.trace.AsyncTraceDispatcher;
import org.apache.rocketmq.client.trace.TraceBean;
import org.apache.rocketmq.client.trace.TraceContext;
import org.apache.rocketmq.client.trace.TraceDispatcher;
import org.apache.rocketmq.client.trace.TraceType;
import org.apache.rocketmq.common.protocol.NamespaceUtil;
*/
#ifndef __SendMessageTraceHookImpl_H__
#define __SendMessageTraceHookImpl_H__
#include<SendMessageHook.h>


namespace rocketmq {
class SendMessageTraceHookImpl : public SendMessageHook {
 private:
  std::shared_ptr<TraceDispatcher> localDispatcher;

  public:
  SendMessageTraceHookImpl(std::shared_ptr<TraceDispatcher>& localDispatcher);
  virtual std::string hookName();
  //virtual void sendMessageBefore(SendMessageContext* context);
  virtual void sendMessageBefore(SendMessageContext& context);
  virtual void sendMessageAfter(SendMessageContext& context);
  //virtual void sendMessageAfter(SendMessageContext* context);
};





}






#endif
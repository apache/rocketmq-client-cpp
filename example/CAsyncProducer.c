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

#include <stdio.h>
#include "CCommon.h"
#include "CMessage.h"
#include "CProducer.h"
#include "CSendResult.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <memory.h>
#include <unistd.h>
#endif

void thread_sleep(unsigned milliseconds) {
#ifdef _WIN32
  Sleep(milliseconds);
#else
  usleep(milliseconds * 1000);  // takes microseconds
#endif
}

void SendSuccessCallback(CSendResult result) {
  printf("async send success, msgid:%s\n", result.msgId);
}

void SendExceptionCallback(CMQException e) {
  char msg[1024];
  snprintf(msg, sizeof(msg), "error:%d, msg:%s, file:%s:%d", e.error, e.msg, e.file, e.line);
  printf("async send exception %s\n", msg);
}

void StartSendMessage(CProducer* producer) {
  int i = 0;
  int ret_code = 0;
  char body[128];
  CMessage* msg = CreateMessage("T_TestTopic");
  SetMessageTags(msg, "Test_Tag");
  SetMessageKeys(msg, "Test_Keys");
  for (i = 0; i < 10; i++) {
    memset(body, 0, sizeof(body));
    snprintf(body, sizeof(body), "new message body, index %d", i);
    SetMessageBody(msg, body);
    ret_code = SendMessageAsync(producer, msg, SendSuccessCallback, SendExceptionCallback);
    printf("async send message[%d] return code: %d\n", i, ret_code);
    thread_sleep(1000);
  }
  DestroyMessage(msg);
}

void CreateProducerAndStartSendMessage(int i) {
  printf("Producer Initializing.....\n");
  CProducer* producer = CreateProducer("Group_producer");
  SetProducerNameServerAddress(producer, "127.0.0.1:9876");
  if (i == 1) {
    SetProducerSendMsgTimeout(producer, 3);
  }
  StartProducer(producer);
  printf("Producer start.....\n");
  StartSendMessage(producer);
  ShutdownProducer(producer);
  DestroyProducer(producer);
  printf("Producer Shutdown!\n");
}

int main(int argc, char* argv[]) {
  printf("Send Async successCallback.....\n");
  CreateProducerAndStartSendMessage(0);

  printf("Send Async exceptionCallback.....\n");
  CreateProducerAndStartSendMessage(1);
  return 0;
}

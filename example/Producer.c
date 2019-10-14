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

void thread_sleep(unsigned int milliseconds) {
#ifdef _WIN32
  Sleep(milliseconds);
#else
  usleep(milliseconds * 1000);  // suspend execution for microsecond intervals
#endif
}

void StartSendMessage(CProducer* producer) {
  int i = 0;
  char body[256];
  CMessage* msg = CreateMessage("T_TestTopic");
  SetMessageTags(msg, "Test_Tag");
  SetMessageKeys(msg, "Test_Keys");
  CSendResult result;
  for (i = 0; i < 3; i++) {
    memset(body, 0, sizeof(body));
    snprintf(body, sizeof(body), "new message body, index %d", i);
    SetMessageBody(msg, body);
    int status = SendMessageSync(producer, msg, &result);
    if (status == OK) {
      printf("send message[%d] result status:%d, msgId:%s\n", i, (int)result.sendStatus, result.msgId);
    } else {
      printf("send message[%d] failed !\n", i);
    }
    thread_sleep(1000);
  }
  DestroyMessage(msg);
}

int main(int argc, char* argv[]) {
  CProducer* producer = CreateProducer("Group_producer");
  SetProducerNameServerAddress(producer, "127.0.0.1:9876");
  StartProducer(producer);
  printf("Producer initialized. \n");

  StartSendMessage(producer);

  ShutdownProducer(producer);
  DestroyProducer(producer);
  printf("Producer stopped !\n");

  return 0;
}

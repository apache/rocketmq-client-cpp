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
#include <string.h>
#include "CBatchMessage.h"
#include "CCommon.h"
#include "CMessage.h"
#include "CProducer.h"
#include "CSendResult.h"

void StartSendMessage(CProducer* producer) {
  int i = 0;
  int ret_code = 0;
  char body[128];
  CBatchMessage* batchMessage = CreateBatchMessage("T_TestTopic");

  for (i = 0; i < 10; i++) {
    CMessage* msg = CreateMessage("T_TestTopic");
    SetMessageTags(msg, "Test_Tag");
    SetMessageKeys(msg, "Test_Keys");
    memset(body, 0, sizeof(body));
    snprintf(body, sizeof(body), "new message body, index %d", i);
    SetMessageBody(msg, body);
    AddMessage(batchMessage, msg);
  }
  CSendResult result;
  ret_code = SendBatchMessage(producer, batchMessage, &result);
  printf("SendBatchMessage %s .....\n", ret_code == 0 ? "Success" : ret_code == 11 ? "FAILED" : " It is null value");
  DestroyBatchMessage(batchMessage);
}

void CreateProducerAndStartSendMessage() {
  printf("Producer Initializing.....\n");
  CProducer* producer = CreateProducer("Group_producer");
  SetProducerNameServerAddress(producer, "127.0.0.1:9876");
  StartProducer(producer);
  printf("Producer start.....\n");
  StartSendMessage(producer);
  ShutdownProducer(producer);
  DestroyProducer(producer);
  printf("Producer Shutdown!\n");
}

int main(int argc, char* argv[]) {
  printf("Send Batch.....\n");
  CreateProducerAndStartSendMessage();
  return 0;
}

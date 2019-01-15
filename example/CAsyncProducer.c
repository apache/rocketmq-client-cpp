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

#include "CProducer.h"
#include "CCommon.h"
#include "CMessage.h"
#include "CSendResult.h"

#ifdef _WIN32
#include <windows.h>
#else

#include <unistd.h>
#include <memory.h>

#endif

void thread_sleep(unsigned milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);  // takes microseconds
#endif
}

void sendSuccessCallback(CSendResult result){
	printf("Msg Send ID:%s\n", result.msgId);
}

void sendExceptionCallback(CMQException e){
	printf("asyn send exception error : %d\n" , e.error);
	printf("asyn send exception msg : %s\n" , e.msg);
	printf("asyn send exception file : %s\n" , e.file);
	printf("asyn send exception line : %d\n" , e.line);
}

void startSendMessage(CProducer *producer) {
    int i = 0;
    char DestMsg[256];
    CMessage *msg = CreateMessage("T_TestTopic");
    SetMessageTags(msg, "Test_Tag");
    SetMessageKeys(msg, "Test_Keys");
    CSendResult result;
    for (i = 0; i < 10; i++) {
        printf("send one message : %d\n", i);
        memset(DestMsg, 0, sizeof(DestMsg));
        snprintf(DestMsg, sizeof(DestMsg), "New message body: index %d", i);
        SetMessageBody(msg, DestMsg);
        int code = SendMessageAsync(producer, msg, sendSuccessCallback , sendExceptionCallback);
        printf("Async send return code: %d\n", code);
        thread_sleep(1000);
    }
}

void CreateProducerAndStartSendMessage(int i){
	printf("Producer Initializing.....\n");
	CProducer *producer = CreateProducer("Group_producer");
	SetProducerNameServerAddress(producer, "127.0.0.1:9876");
	if(i == 1){
		SetProducerSendMsgTimeout(producer , 3);
	}
	StartProducer(producer);
	printf("Producer start.....\n");
	startSendMessage(producer);
	ShutdownProducer(producer);
	DestroyProducer(producer);
	printf("Producer Shutdown!\n");
}

int main(int argc, char *argv[]) {
    printf("Send Async successCallback.....\n");
    CreateProducerAndStartSendMessage(0);

    printf("Send Async exceptionCallback.....\n");
    CreateProducerAndStartSendMessage(1);

    return 0;
}


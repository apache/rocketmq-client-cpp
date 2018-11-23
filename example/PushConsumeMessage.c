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
#ifndef WIN32
#include <unistd.h>
#else
#include <windows.h>
void sleep(int interval) {
	Sleep(interval * 10);
}
#endif
#include <stdio.h>



#include "CPushConsumer.h"
#include "CCommon.h"
#include "CMessageExt.h"


int doConsumeMessage(struct CPushConsumer * consumer, CMessageExt * msgExt)
{
    printf("Hello,doConsumeMessage by Application!\n");
    printf("Msg Topic:%s\n",GetMessageTopic(msgExt));
    printf("Msg Tags:%s\n",GetMessageTags(msgExt));
    printf("Msg Keys:%s\n",GetMessageKeys(msgExt));
    printf("Msg Body:%s\n",GetMessageBody(msgExt));
    return E_CONSUME_SUCCESS;
}


int main(int argc,char * argv [])
{
    int i = 0;
    printf("PushConsumer Initializing....\n");
    CPushConsumer* consumer = CreatePushConsumer("Group_Consumer_Test");
    SetPushConsumerNameServerAddress(consumer,"172.17.0.2:9876");
    Subscribe(consumer,"T_TestTopic","*");
    RegisterMessageCallback(consumer,doConsumeMessage);
    StartPushConsumer(consumer);
    printf("Push Consumer Start...\n");
    for( i=0; i<10; i++)
    {
        printf("Now Running : %d S\n",i*10);
        sleep(10);
    }
    ShutdownPushConsumer(consumer);
    DestroyPushConsumer(consumer);
    printf("PushConsumer Shutdown!\n");
    return 0;
}

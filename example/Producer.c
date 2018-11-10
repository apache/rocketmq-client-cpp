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
#endif
#include <stdio.h>

#include "CProducer.h"
#include "CCommon.h"
#include "CMessage.h"
#include "CSendResult.h"

void startSendMessage(CProducer* producer)
{
    int i = 0;
    char DestMsg[256];
    CMessage* msg = CreateMessage("T_TestTopic");
    SetMessageTags(msg,"Test_Tag");
    SetMessageKeys(msg,"Test_Keys");
    CSendResult result;
    for( i=0; i<10; i++)
    {
        printf("send one message : %d",i);
        snprintf(DestMsg,255,"New message body: index %d",i);
        SetMessageBody(msg,DestMsg);
        SendMessageSync(Gproducer, msg, &result);
        printf("Msg Send ID:%s\n",result.msgId);
#ifndef WIN32
        sleep(1);
#endif
    }
}


int main(int argc,char * argv [ ])
{
    int i =0;
    printf("Producer Demo Enter Main.\n");

    CProducer* producer = CreateProducer("Group_producer");
    SetProducerNameServerAddress(producer,"172.17.0.5:9876");
    StartProducer(producer);

    startSendMessage(producer);

    ShutdownProducer(producer);
	DestroyProducer(producer);
    printf("Producer Demo Done !\n");
    return 0;
}


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



#include "CPullConsumer.h"
#include "CCommon.h"
#include "CMessageExt.h"
#include "CPullResult.h"
#include "CMessageQueue.h"


int main(int argc,char * argv [])
{
    int i = 0;
    printf("PullConsumer Initializing....\n");
    CPullConsumer* consumer = CreatePullConsumer("Group_Consumer_Test");
    SetPullConsumerNameServerAddress(consumer,"172.17.0.2:9876");
    StartPullConsumer(consumer);
    printf("Pull Consumer Start...\n");
    for( i=0; i<10; i++)
    {
        printf("Now Running : %d S\n",i*10);
        sleep(10);
    }
    ShutdownPullConsumer(consumer);
    DestroyPullConsumer(consumer);
    printf("PullConsumer Shutdown!\n");
    return 0;
}
